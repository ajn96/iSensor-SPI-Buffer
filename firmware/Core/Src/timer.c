/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		timer.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation for for iSensor-SPI-Buffer timer module. Covers microsecond timer and PPS timer.
 **/

#include "reg.h"
#include "timer.h"
#include "dio.h"
#include "main.h"

/* Private function prototypes */
static void InitTIM2(uint32_t timerfreq);
static void InitTIM8();
static void ConfigurePPSPins(uint32_t enable);
static void DWT_Init();

/** PPS interrupt source. Global scope */
uint32_t g_PPSInterruptMask = 0;

/** TIM2 handle */
static TIM_HandleTypeDef htim2;

/** TIM8 handle for init */
static TIM_HandleTypeDef htim8;

/** Track number of "PPS" ticks which have occurred in last second */
static uint32_t PPS_TickCount;

/** Number of PPS ticks per sec */
static uint32_t PPS_MaxTickCount;

/**
  * @brief Initialize all timers for operation.
  *
  * @return void
  *
  * Timer 2 is used as a 1MHz timer for the timestamp measurements
  * Timer 8 is used as a timeout timer, also at 1MHz
  *
  * The DWT timer is used for a polling based timer
  */
void Timer_Init()
{
	/* Set up TIM8 */
	InitTIM8();
	/* Init TIM2 at 1MHz also */
	InitTIM2(1000000);
	/* Init DWT timer */
	DWT_Init();
}

/**
  * @brief Check if the PPS signal is unlocked (greater than 1100ms since last PPS strobe)
  *
  * @return void
  *
  * This function should be periodically called as part of the firmware housekeeping process
 **/
void Timer_Check_PPS_Unlock()
{
	/* Exit if PPS input is disabled */
	if(g_pinConfig.ppsPin == 0)
	{
		return;
	}

	/* If microsecond timestamp is greater than 1,100,000 (1.1 seconds) then unlock has occurred */
	if(Timer_Get_Microsecond_Timestamp() > 1100000)
	{
		g_regs[STATUS_0_REG] |= STATUS_PPS_UNLOCK;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
}

/**
  * @brief Blocking sleep function call
  *
  * @param microseconds The number of microseconds to sleep
  *
  * @return void
  *
  * This function uses TIM6 to perform delay.
 **/
void Timer_Sleep_Microseconds(uint32_t microseconds)
{
	/* Reset timer */
	TIM8->CNT = 0;

	/* Delay till end */
	while (TIM8->CNT < microseconds);
}

/**
  * @brief Gets the current 32-bit value from the IMU sample timestamp timer
  *
  * @return The timer value
  */
uint32_t Timer_Get_Microsecond_Timestamp()
{
	return TIM2->CNT;
}

/**
  * @brief Gets the current 32-bit value from PPS timestamp registers
  *
  * @return The PPS counter value
  */
uint32_t Timer_Get_PPS_Timestamp()
{
	return (g_regs[UTC_TIMESTAMP_LWR_REG] | (g_regs[UTC_TIMESTAMP_UPR_REG] << 16));
}

/**
  * @brief Reset TIM2 counter to 0.
  *
  * @return void
  */
void Timer_Clear_Microsecond_Timer()
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
void Timer_Enable_PPS()
{
	ConfigurePPSPins(1);
	DIO_Update_Output_Config();
}

/**
  * @brief Disable PPS timer functionality
  *
  * @return void
  */
void Timer_Disable_PPS()
{
	ConfigurePPSPins(0);
	DIO_Update_Output_Config();
}

/**
  * @brief Increment PPS timestamp by 1
  *
  * @return void
  *
  * This function should be called whenever a PPS interrupt is generated. It
  * handles PPS tick count incrementation, and resetting the microsecond timestamp
  * every time one second has elapsed.
  */
void Timer_Increment_PPS_Time()
{
	uint32_t internalTime, startTime;

	/* Increment tick count and check if one second has passed */
	PPS_TickCount++;
	if(PPS_TickCount >= PPS_MaxTickCount)
	{
		/* Get the internal timestamp for period measurement */
		internalTime = Timer_Get_Microsecond_Timestamp();
		/* Clear microsecond tick counter */
		Timer_Clear_Microsecond_Timer();
		/* Reset tick count for next second */
		PPS_TickCount = 0;
		/* Get starting PPS timestamp and increment */
		startTime = Timer_Get_PPS_Timestamp();
		startTime++;
		g_regs[UTC_TIMESTAMP_LWR_REG] = startTime & 0xFFFF;
		g_regs[UTC_TIMESTAMP_UPR_REG] = (startTime >> 16);
		/* Check for unlock: If the PPS period is outside of 1 second +- 10ms (1%) then flag PPS unlock error */
		if((internalTime > 1010000)||(internalTime < 990000))
		{
			g_regs[STATUS_0_REG] |= STATUS_PPS_UNLOCK;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		}
	}
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

/**
  * @brief Enables TIM8 as general 1MHz timer
  *
  */
static void InitTIM8()
{
	/* Init TIM8 as basic 16-bit timeout timer */
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};

	htim8.Instance = TIM8;
	/* Set prescaler to give desired timer freq of 1MHz */
	htim8.Init.Prescaler = 71;
	htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim8.Init.Period = 0xFFFF;
	htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim8);

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig);

	/* Enable timer */
	TIM8->CR1 = 0x1;
}

/**
  * @brief Configure PPS timer based on DIO_INPUT_CONFIG value
  *
  * @param enable Bool flagging if PPS functionality is being enabled or disabled
  *
  * This function sets up the PPS input pin, as well as the expected "PPS" period.
  */
static void ConfigurePPSPins(uint32_t enable)
{
	/* GPIO config struct */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Field values for DIO_INPUT_CONFIG */
	uint32_t ppsConfig, freqConfig;

	/* Get current config value */
	uint32_t config = g_regs[DIO_INPUT_CONFIG_REG];

	/* Get PPS settings from config reg */
	ppsConfig = config & 0xF80;

	/* Get freq setting (0 - 3) */
	freqConfig = (config >> 12) & 0x3;

	/* Set freq values */
	switch(freqConfig)
	{
	case 0:
		/* 1Hz sync */
		PPS_MaxTickCount = 1;
		break;
	case 1:
		/* 1Hz sync */
		PPS_MaxTickCount = 10;
		break;
	case 2:
		/* 1Hz sync */
		PPS_MaxTickCount = 100;
		break;
	case 3:
		/* 1Hz sync */
		PPS_MaxTickCount = 1000;
		break;
	default:
		/* 1Hz sync */
		PPS_MaxTickCount = 1;
		freqConfig = 0;
		break;
	}
	/* Init tick count to 0 */
	PPS_TickCount = 0;

	/* Clear pending PPS EXTI interrupts */
	EXTI->PR = PPS_INT_MASK;

	/* Configure selected pin to trigger interrupt. Disable interrupt initially */
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	/* Set mode */
	if(enable)
	{
		if(ppsConfig & 0x80)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}

	/* DIO1 slave (PB4) */
	if(ppsConfig & 0x100)
	{
		/* PB4 is PPS pin */
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);

		/* Disable IRQ */
		NVIC_DisableIRQ(EXTI4_IRQn);

		/* Apply setting */
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* Need to configure the EXTI4 interrupt source */
		if(enable)
		{
			NVIC_EnableIRQ(EXTI4_IRQn);
		}

		/* Set mask to 0 */
		g_PPSInterruptMask = 0;

		/* Mask other PPS config bits */
		ppsConfig &= 0x180;
	}

	/* DIO2 slave (PB8) */
	if(ppsConfig & 0x200)
	{
		/* PB8 is PPS pin */
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

		/* Disable IRQ */
		NVIC_DisableIRQ(EXTI9_5_IRQn);

		/* Apply settings */
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		if(enable)
		{
			/* Set mask to pin 8 */
			g_PPSInterruptMask = GPIO_PIN_8;
			/* Enable PPS interrupts */
			NVIC_EnableIRQ(EXTI9_5_IRQn);
		}
		else
		{
			g_PPSInterruptMask = 0;
		}

		/* Mask other PPS config bits */
		ppsConfig &= 0x280;
	}

	/* DIO3 slave (PC7) */
	if(ppsConfig & 0x400)
	{
		/* PC7 is PPS pin */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7);

		/* Disable IRQ */
		NVIC_DisableIRQ(EXTI9_5_IRQn);

		/* Apply settings */
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		if(enable)
		{
			/* Set mask to pin 7 */
			g_PPSInterruptMask = GPIO_PIN_7;
			/* Enable PPS interrupts */
			NVIC_EnableIRQ(EXTI9_5_IRQn);
		}
		else
		{
			g_PPSInterruptMask = 0;
		}

		/* Mask other PPS config bits */
		ppsConfig &= 0x480;
	}

	/* DIO4 slave (PA8) */
	if(ppsConfig & 0x800)
	{
		/* PA8 is PPS pin */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);

		/* Disable IRQ */
		NVIC_DisableIRQ(EXTI9_5_IRQn);

		/* Apply settings */
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		if(enable)
		{
			/* Set mask to pin 8 */
			g_PPSInterruptMask = GPIO_PIN_8;
			/* Enable PPS interrupts */
			NVIC_EnableIRQ(EXTI9_5_IRQn);
		}
		else
		{
			g_PPSInterruptMask = 0;
		}

		/* Mask other PPS config bits */
		ppsConfig &= 0x880;
	}

	/* Apply setting back to DIO_INPUT_CONFIG */
	g_regs[DIO_INPUT_CONFIG_REG] = (freqConfig << 12) | (ppsConfig) | (config & 0x1F);

	/* Apply setting to pin struct */
	if(enable)
	{
		g_pinConfig.ppsPin = (ppsConfig >> 8) & 0xF;
	}
	else
	{
		g_pinConfig.ppsPin = 0;
	}
}

/**
  * @brief  Init DWT peripheral.
  *
  * @return void
  *
  * This peripheral is used to count microseconds for
  * the timer module.
  */
static void DWT_Init()
{
  /* Disable TRC */
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
  /* Enable TRC */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;
  /* Disable clock cycle counter */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  /* Enable clock cycle counter */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

  /* Enable watch dog debug freeze */
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}
