/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		temp.c
  * @date		7/10/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer ADC module (for temp sensor)
 **/

#include "temp.h"

static void ADC1Init();
static void ADC1Start();
static int16_t ScaleTempData(uint32_t rawTemp);

/** HAL ADC handle */
static ADC_HandleTypeDef hadc1;

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

/**
  * @brief Init temp sensor
  *
  * @return void
  *
  * This function should be called once as part of the firmware initialization process
  */
void TempInit()
{
	ADC1Init();
	ADC1Start();
}

/**
  * @brief Read ADC temperature sensor output and load to output register
  *
  * @return void
  *
  * This function should be called periodically from the cyclic executive. It
  * updates the temperature sensor output value and flags any temperature over range
  * events in the STATUS register (outside -40C to 85C)
  */
void UpdateTemp()
{
	/* Read value from ADC and scale */
	int16_t temp = ScaleTempData(hadc1.Instance->DR);

	/* Load to output reg */
	g_regs[TEMP_REG] = (uint16_t) temp;

	/* Check for alarm */
	if((temp > 850)||(temp < -400))
	{
		g_regs[STATUS_0_REG] |= STATUS_TEMP_WARNING;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
}

/**
  * @brief Scale raw ADC temperature data to temp output
  *
  * @return temp value (10LSB = 1C)
  *
  * Temp (in C, 10LSB per degree) = (800 / (TS_CAL2  - TS_CAL1)) * (val - TS_CAL1) + 300
  */
static int16_t ScaleTempData(uint32_t rawTemp)
{
	int32_t divisor, result;

	divisor = (*TS_CAL2) - (*TS_CAL1);
	result = 800 * (rawTemp - (*TS_CAL1));
	result = result / divisor;
	result += 300;
	return result & 0xFFFF;
}

/**
  * @brief ADC1 Initialization Function
  *
  * @return None
  *
  * Initializes ADC1 in continuous sampling mode. Only a single
  * input is converted (temp sensor input channel). The sample time
  * is set to 601 ADC cycles. The ADC is clocked from 72MHz core clock
  * divided by 8. 601 cycles / (72MHz/8) -> 66us. The Temp sensor
  * requires a minimum 10us setting time for accurate measurements.
  */
static void ADC1Init()
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/* Common config */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = DISABLE;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Configure the ADC multi-mode */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
	{
		Error_Handler();
	}

	/* Configure the ADC channel */
	sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief ADC1 Enable Function
  *
  * @return None
  *
  * Enables on-die temperature sensor and starts ADC1
  */
static void ADC1Start()
{
	ADC12_COMMON->CCR |= (1 << 23);
	HAL_ADC_Start(&hadc1);
}