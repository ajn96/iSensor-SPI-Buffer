/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		main.h
  * @date		3/14/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer main
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#include "registers.h"
#include "buffer.h"
#include "user_interrupt.h"
#include "flash.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void EnableUserSPI();

#endif /* __MAIN_H */
