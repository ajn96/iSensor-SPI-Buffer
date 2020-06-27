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

/* Private function prototypes */
static void InitTIM2(uint32_t timerfreq);

/** Global register array (from registers.c) */
volatile extern uint16_t g_regs[3 * REG_PER_PAGE];

/** PPS interrupt source. Global scope */
uint32_t g_PPSInterruptMask = 0;

/** TIM2 handle */
static TIM_HandleTypeDef htim2;

/**
  * @brief Blocking sleep function call
  *
  * @param microseconds The number of microseconds to sleep
  *
  * @return void
  *
  * This function uses DWT peripheral (data watchpoint and trace) cycle counter to perform delay.
 **/
void SleepMicroseconds(uint32_t microseconds)
{
	/* Get the start time */
	uint32_t clk_cycle_start = DWT->CYCCNT;

	/* Go to number of cycles for system */
	microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

	/* Delay till end */
	while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}

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
	TIM2->CNT = 0;
}

/**
  * @brief Enable PPS input for time stamp synchronization (improved long term stability over 20ppm crystal).
  *
  * @return void
  *
  * The PPS input can be on any of the DIOx_Slave pins (host processor the iSensor-SPI-Buffer). The selected
  * pin is configured by the PPS_SELECT field in the DIO_INPUT_CONFIG register. The trigger polarity for
  * the PPS signal is configured by the PPS_POLARITY field in the DIO_INPUT_CONFIG register.
  *
  * Pins:
  *
  * DIO1: PB4 -> EXTI4 interrupt
  * DIO2: PB8 -> EXTI9_5 interrupt, mask (1 << 8)
  * DIO3: PC7 -> EXTI9_5 interrupt, mask (1 << 7)
  * DIO4: PA8 -> EXTI9_5 interrupt, mask (1 << 8)
  */
void EnablePPSTimer()
{
	/* GPIO config struct */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Get current config value */
	uint32_t config = g_regs[DIO_INPUT_CONFIG_REG];

	/* Shift down so PPS timer setting is in lower 5 bits */
	config = config >> 8;

	/* Clear pending EXTI interrupts */
	EXTI->PR = ((0x1F << 5) | (1 << 4));

	/* Configure selected pin to trigger interrupt. Disable interrupt initially */
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	/* DIO1 slave (PB4) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	if(config & 0x1)
	{
		/* This is PPS pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

		/* Need to enable the EXTI4 interrupt source */
		NVIC_EnableIRQ(EXTI4_IRQn);

		/* Set mask to 0 */
		g_PPSInterruptMask = 0;
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO2 slave (PB8) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	if(config & 0x2)
	{
		/* This is PPS pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

		/* Set mask to pin 8 */
		g_PPSInterruptMask = (1 << 8);

		/* Enable PPS interrupts */
		NVIC_EnableIRQ(EXTI9_5_IRQn);
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO3 slave (PC7) */
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7);
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	if(config & 0x4)
	{
		/* This is PPS pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

		/* Set mask to pin 7 */
		g_PPSInterruptMask = (1 << 7);

		/* Enable PPS interrupts */
		NVIC_EnableIRQ(EXTI9_5_IRQn);
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* DIO4 slave (PA8) */
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	if(config & 0x8)
	{
		/* This is PPS pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

		/* Set mask to pin 8 */
		g_PPSInterruptMask = (1 << 8);

		/* Enable PPS interrupts */
		NVIC_EnableIRQ(EXTI9_5_IRQn);
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

	/* Apply settings */
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief Disable PPS timer functionality
  *
  * @return void
  */
void DisablePPSTimer()
{

}

/**
  * @brief Increment PPS timestamp by 1
  *
  * @return void
  */
void IncrementPPSTime()
{
	/* Get starting PPS timestamp and increment */
	uint32_t startTime = GetPPSTimestamp();
	startTime++;
	g_regs[UTC_TIMESTAMP_LWR_REG] = startTime & 0xFFFF;
	g_regs[UTC_TIMESTAMP_UPR_REG] = (startTime >> 16);

	/* Clear microsecond tick counter */
	ClearMicrosecondTimer();
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
