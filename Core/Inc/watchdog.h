/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		watchdog.h
  * @date		4/30/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer watchdog timer module
 **/

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include "stm32f303xe.h"

/* Public function prototypes */
void CheckWatchDogStatus();
void FeedWatchDog();
void EnableWatchDog(uint32_t timeout_ms);

/** Watchdog base freq */
#define WATCHDOG_BASE_FREQ		40000

#define WATCHDOG_RELOAD_KEY		0x0000AAAA
#define WATCHDOG_START_KEY		0x0000CCCC
#define WATCHDOG_UNLOCK_KEY		0x00005555

#endif /* INC_WATCHDOG_H_ */
