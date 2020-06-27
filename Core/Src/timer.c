/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		timer.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation for for iSensor-SPI-Buffer timer module. Covers microsecond timer and PPS timer.
 **/

#include "timer.h"

/* Global register array */
volatile extern uint16_t g_regs[3 * REG_PER_PAGE];

/** TIM2 handle */
static TIM_HandleTypeDef htim2;

/* Private function prototypes */
static void InitTIM2(uint32_t timerfreq);

/**
  * @brief Gets the current 32-bit value from the IMU sample timestamp timer
  *
  * @return The timer value
  */
uint32_t GetMicrosecondTimestamp()
{
	return TIM2->CNT;
}

uint32_t GetPPSTimestamp()
{
	return (g_regs[UTC_TIMESTAMP_LWR_REG] | (g_regs[UTC_TIMESTAMP_UPR_REG] << 16));
}

/**
  * @brief Init TIM2 to operate in 32-bit mode with a 1MHz timebase.
  *
  * @return void
  *
  * This timer is used to generate the buffer entry timestamp values
  */
void InitMicrosecondTimer()
{
	InitTIM2(1000000);
}

/**
  * @brief Reset TIM2 counter to 0.
  *
  * @return void
  */
void ClearMicrosecondTimer()
{

}

void EnablePPSTimer()
{

}

void DisablePPSTimer()
{

}

/**
  * @brief Enables IMU sample timestamp timer
  *
  * @param timerfreq The desired timer freq (in Hz)
  *
  * @return void
  *
  * This function should be called as part of the buffered data
  * acquisition startup process. The enabled timer is TIM2 (32 bit)
  */
static void InitTIM2(uint32_t timerfreq)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};

	htim2.Instance = TIM2;
	/* Set prescaler to give desired timer freq */
	htim2.Init.Prescaler = (SystemCoreClock / timerfreq) - 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 0xFFFFFFFF;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim2);

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

	/* Enable timer */
	TIM2->CR1 = 0x1;
}
