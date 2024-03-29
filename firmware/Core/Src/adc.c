/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		adc.c
  * @date		7/10/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer ADC module (for temp sensor and Vdd monitoring)
 **/

#include "reg.h"
#include "adc.h"
#include "main.h"
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_conf.h"

/* Private function prototypes */
static void ProcessTempReading();
static int16_t ScaleTempData(uint32_t rawTemp);
static uint16_t GetVdd(uint32_t VrefMeasurement);

/** HAL ADC handle */
static ADC_HandleTypeDef hadc1;

/**
  * @brief ADC1 Initialization Function
  *
  * @return None
  *
  * This function should be called once as part of the firmware initialization process.
  *
  * Initializes ADC1 in single sampling mode, with two input
  * channels converted (temp sensor and VREFINT). The sample time
  * is set to 601 ADC cycles. The ADC is clocked from 72MHz core clock
  * divided by 8. 601 cycles / (72MHz/8) -> 66us. The Temp sensor
  * requires a minimum 10us setting time for accurate measurements.
  * Because these values are measured for diagnostics monitoring pursposes
  * only, using a long sample time is not a problem.
  */
void ADC_Init()
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/* Common config */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ENABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = ENABLE;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 2;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Main_Error_Handler();
	}

	/* Configure the ADC multi-mode */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
	{
		Main_Error_Handler();
	}

	/* Configure ADC vrefint channel */
	sConfig.Channel = ADC_CHANNEL_VREFINT;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	/* Configure the ADC temp sensor channel */
	sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	sConfig.Rank = 2;
	sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	/* Enable temp sensor */
	ADC12_COMMON->CCR |= (1 << 23);

	/* Stop ADC and calibrate */
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
}

/**
  * @brief Read and scale ADC values then load to output registers
  *
  * @return void
  *
  * This function drives the ADC state machine and should be called
  * periodically from the cyclic executive. The ADC is configured in
  * non-continuous scanning mode, with the temp sensor and VREFINT
  * channels enabled. For each channel, the state machine initiates
  * an ADC sample, then goes to a wait state which does not advance until
  * the EOC flag for that sample has been set. The VREFINT channel is
  * sampled prior to the temp sensor channel, because the calculated
  * Vdd value is used to compensate the temp sensor scale factor.
  */
void ADC_Update()
{
	static uint32_t adc_state = ADC_VDD_START;

	switch(adc_state)
	{
	case ADC_TEMP_START:
		/* Start conversion, move to next state */
		HAL_ADC_Start(&hadc1);
		adc_state = ADC_TEMP_READ;
		break;
	case ADC_TEMP_READ:
		if(!HAL_IS_BIT_CLR(hadc1.Instance->ISR, (ADC_FLAG_EOC | ADC_FLAG_EOS)))
		{
			ProcessTempReading();
			adc_state = ADC_VDD_START;
		}
		break;
	case ADC_VDD_START:
		HAL_ADC_Start(&hadc1);
		adc_state = ADC_VDD_READ;
		break;
	case ADC_VDD_READ:
		if(!HAL_IS_BIT_CLR(hadc1.Instance->ISR, (ADC_FLAG_EOC | ADC_FLAG_EOS)))
		{
			/* Get VREFINT ADC value and scale to Vdd */
			g_regs[VDD_REG] = GetVdd(hadc1.Instance->DR);
			/* Back to temp */
			adc_state = ADC_TEMP_START;
		}
		break;
	default:
		adc_state = ADC_VDD_START;
	}
}

/**
  * @brief Handle end of conversion for a temp sensor value
  *
  * This function reads the latest temp sensor output value and applies it
  * to the temp sensor decimation averaging filter. If the end of a decimation
  * period has been reached, it updates the temperature sensor output value
  * and flags any temperature over range events in the STATUS register
  * (outside -40C to 85C).
  */
static void ProcessTempReading()
{
	/* Read value from ADC and scale */
	static uint32_t count = 0;
	static int accum = 0;

	accum += ScaleTempData(hadc1.Instance->DR);
	count += 1;

	if(count >= 8)
	{
		/* scale and load to output reg */
		accum = accum >> 3;
		g_regs[TEMP_REG] = (uint16_t) (accum);

		/* Check for alarm */
		if((accum > 850)||(accum < -400))
		{
			g_regs[STATUS_0_REG] |= STATUS_TEMP_WARNING;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		}

		accum = 0;
		count = 0;
	}
}

/**
  * @brief Calculate Vdd from VREFINT measurement
  *
  * @return Vdd voltage (1V = 100LSBs)
  *
  * VREFINT as a regulated ~1.23V supply generated by the STM32 processor core
  * internally. The value of VREFINT with Vdd = 3.3V is logged by ST during
  * production (stored in VREFINT_CAL). Theoretical value of 1501
  *
  * In general, when an ADC measurement is performed,
  * the ADC output = (2^12 - 1) * voltage / Vdd
  *
  * so Vref (real voltage) = VREFINT_CAL * 3.3 / (2^12 - 1)
  *
  * Therefore a measurement of the current Vref is equivalent to:
  *
  * VrefMeasurement = (2^12 - 1) * (VREFINT_CAL * 3.3 / (2^12 - 1)) / Vdd
  * VrefMeasurement = VREFINT_CAL * 3.3 / Vdd
  * Vdd = VREFINT_CAL * 3.3 / VrefMeasurement
  */
static uint16_t GetVdd(uint32_t VrefMeasurement)
{
	if(VrefMeasurement == 0)
		return 0;
	uint32_t val = (*VREFINT_CAL) * 330;
	val = val / VrefMeasurement;
	return val;
}

/**
  * @brief Scale raw ADC temperature data to temp output
  *
  * @return temp value (10LSB = 1C)
  *
  * Temp (in C, 10LSB per degree) = (800 / (TS_CAL2  - TS_CAL1)) * (val - TS_CAL1) + 300
  *
  * The CAL values are measured with VREF = 3.3V at the factory. To make the measurement
  * accurate, the most recent value read for VDD (using ADC VREFINT measurement) is used
  * to normalize the raw temperature sensor measurement to a 3.3V reference voltage.
  */
static int16_t ScaleTempData(uint32_t rawTemp)
{
	int32_t divisor, result;

	/* Compensate raw temp based on measured Vdd value */
	rawTemp = rawTemp * g_regs[VDD_REG];
	rawTemp = rawTemp / 330;

	divisor = (*TS_CAL2) - (*TS_CAL1);
	result = 800 * (rawTemp - (*TS_CAL1));
	result = result / divisor;
	result += 300;
	return result & 0xFFFF;
}
