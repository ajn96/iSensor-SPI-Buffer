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

/** Enum for available LED's */
typedef enum LED
{
	Blue = 0,
	Green = 1
}LED;

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void TurnOnLED(LED light);
void TurnOffLED(LED light);
/* @endcond */

#endif /* INC_LED_H_ */
