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
	IWDG->KR = WATCHDOG_RELOAD_KEY;
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
  * @param timeout_ms The watch dog reset time (in ms)
  *
  * This functions calculates the watch dog time base based on
  * desired timeout, to give the best granularity. The STM32
  * independent watch dog peripheral runs on a 40KHz RC oscillator.
  * This 40KHz clock can be divided down in range 4 - 256. The
  * watchdog itself resets the system when the watchdog timer reaches
  * a value of 0 (12 bit counter).
  *
  * Min timeout (highest resolution): 4095 / (40KHz / 4) -> 0.410s
  *
  * Max timeout: 4095 / (40KHz / 256) -> 26.2s
  */
void EnableWatchDog(uint32_t timeout_ms)
{
	/* Watchdog scaled freq */
	uint32_t scaledFreq;

	/* Reg values to write */
	uint32_t scale, reloadVal;

	/* cap at 26.2 s*/
	if(timeout_ms > 26200)
	{
		timeout_ms = 26200;
	}

	/* Set scaler value based on timeout */
	if(timeout_ms < 490)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 4;
		scale = 0;
	}
	else if(timeout_ms < 819)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 8;
		scale = 1;
	}
	else if(timeout_ms < 1638)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 16;
		scale = 2;
	}
	else if(timeout_ms < 3276)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 32;
		scale = 3;
	}
	else if(timeout_ms < 6552)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 64;
		scale = 4;
	}
	else if(timeout_ms < 13104)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 128;
		scale = 5;
	}
	else if(timeout_ms < 26208)
	{
		scaledFreq = WATCHDOG_BASE_FREQ / 256;
		scale = 6;
	}

	/* Set reload value based on freq times timeout */
	reloadVal = ((scaledFreq * timeout_ms) / 1000) + 1;

	/* Enable watchdog */
	IWDG->KR = WATCHDOG_START_KEY;

	/* Unlock watchdog peripheral writes */
	IWDG->KR = WATCHDOG_UNLOCK_KEY;

	/* Set period scaler register */
	IWDG->PR = scale;

	/* Set reload register */
	IWDG->RLR = reloadVal;

	/* Wait for done */
	while(IWDG->SR);

	/* Initial clear */
	FeedWatchDog();
}
