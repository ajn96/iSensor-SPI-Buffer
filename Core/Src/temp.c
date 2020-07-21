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
static uint16_t ScaleTempData(uint32_t rawTemp);

/** HAL ADC handle */
static ADC_HandleTypeDef hadc1;

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

void TempInit()
{
	ADC1Init();
	ADC1Start();
}


void UpdateTemp()
{
	g_regs[TEMP_REG] = ScaleTempData(hadc1.Instance->DR);
}

static uint16_t ScaleTempData(uint32_t rawTemp)
{
	/* Temp (in C, 10LSB per degree) = (800 / (TS_CAL2  - TS_CAL1)) * (val - TS_CAL1) + 300 */
	int32_t divisor, result;

	divisor = (*TS_CAL2) - (*TS_CAL1);
	result = 800 * (rawTemp - (*TS_CAL1));
	result = result / divisor;
	result += 300;
	return result & 0xFFFF;
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
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

static void ADC1Start()
{
	ADC12_COMMON->CCR |= (1 << 23);
	HAL_ADC_Start(&hadc1);
}
