/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		led.h
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer LED control module
 **/

#ifndef INC_LED_H_
#define INC_LED_H_

#include "stm32f3xx_hal.h"
#include "registers.h"

/** Enum for available LED's */
typedef enum LED
{
	Blue = 0,
	Green = 1
}LED;

/* Public function prototypes */
void TurnOnLED(LED light);
void TurnOffLED(LED light);

#endif /* INC_LED_H_ */
