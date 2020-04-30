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

#include "stm32f3xx_hal_rcc.h"

/* Public function prototypes */
void CheckWatchDogStatus();
void FeedWatchDog();

/** Watchdog base freq */
#define WATCHDOG_BASE_FREQ		40000

#define WATCHDOG_RELOAD_KEY

#endif /* INC_WATCHDOG_H_ */
