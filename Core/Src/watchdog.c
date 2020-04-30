/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		watchdog.c
  * @date		4/30/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer watchdog timer module
 **/


#include "watchdog.h"
#include "registers.h"

/* Local function prototypes */
static uint32_t EnableWatchDog(uint32_t timeout);

/** Watchdog timer re-load value */
uint32_t watchdog_reset = 0;

/* Global register array */
volatile extern uint16_t regs[3 * REG_PER_PAGE];

/**
  * @brief Feeds the watchdog timer. Should be called periodically from main loop
  *
  * @return void
  */
void FeedWatchDog()
{
	IWDG_KR
}

/**
  * @brief Check if system reset from independent watch dog timeout and sets the appropriate STATUS flag
  *
  * @return void
  */
void CheckWatchDogStatus()
{
	if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
	{
		regs[STATUS_0_REG] |= STATUS_WATCHDOG;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}
}

/**
  * @brief Enables watch dog reset functionality
  *
  * @param timeout The watch dog reset time (in ms)
  *
  * This functions calculates the watch dog time base based on
  * desired timeout, to give the best granularity. PCLK1 runs at 36MHz,
  * watchdog base freq is PCLK1 / 4096 = 8.789KHz. The max reset value
  * for the watchdog counter is 2^7 - 1 (127). The watchdog forces a reset
  * when the timer reaches 1/2 the max value. Therefore, for each divider
  * setting, the max watchdog reset period is:
  *
  * Div 1: 64 ticks / 8789Hz -> 7.2ms
  *
  * Div 2: 64 ticks / (8789Hz / 2) -> 14.45ms
  *
  * Div 4: 64 ticks / (8789Hz / 4) -> 28.9ms
  *
  * Div 4: 64 ticks / (8789Hz / 8) -> 57.8ms
  */
static void EnableWatchDog(uint32_t timeout)
{
	/* Watchdog scaled freq */
	uint32_t scaledFreq;

	/* Reg values to write */
	uint32_t controlReg, configReg;

	/* Starting timer bits all F */
	configReg = 0x7F;

	/* Set scaler value based on timeout */
	if(timeout < 8)
	{
		scaledFreq = WATCHDOG_BASE_FREQ;
	}
	else if(timeout < 15)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 2;
		configReg |= 0x80;
	}
	else if(timeout < 29)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 4;
		configReg |= 0x100;
	}
	else if(timeout < 58)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 8;
		configReg |= 0x180;
	}
	else
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 8;
		configReg |= 0x180;
		timeout = 58;
	}

	/* Initially set bit 6 */
	watchdog_reset = 0x20;


}
