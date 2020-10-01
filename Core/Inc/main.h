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

/* Includes */
#include "stm32f3xx_hal.h"
#include "registers.h"
#include "buffer.h"
#include "user_interrupt.h"
#include "flash.h"
#include "led.h"
#include "watchdog.h"
#include "timer.h"
#include "dio.h"
#include "usb_device.h"
#include "adc.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void Error_Handler(void);
/* @endcond */

/** Bool data type */
typedef enum
{
	false = 0,
	true = 1
}bool;

#define STATE_CHECK_FLAGS		0
#define STATE_CHECK_PPS			1
#define STATE_READ_ADC			2
#define STATE_CHECK_USB			3
#define STATE_CHECK_STREAM		4
#define STATE_STEP_SCRIPT		5

#endif /* __MAIN_H */
