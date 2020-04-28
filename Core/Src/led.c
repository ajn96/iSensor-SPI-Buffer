/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		led.c
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		LED control module for iSensor-SPI-Buffer
 **/

#include "led.h"

/**
  * @brief Turn on a selected LED
  *
  * @param light The LED to turn on. Must be of type LED
  *
  * @return void
  */
void LEDOn(LED light)
{
	switch(light)
	{
	case Red:
		/*PC0 high */
		GPIOC->ODR |= GPIO_PIN_0;
		break;
	case Green:
		/*PC1 high */
		GPIOC->ODR |= GPIO_PIN_1;
		break;
	default:
		/* Not an LED, shouldn't get here */
		return;
	}
}

/**
  * @brief Turn off a selected LED
  *
  * @param light The LED to turn off. Must be of type LED
  *
  * @return void
  */
void LEDOff(LED light)
{
	switch(light)
	{
	case Red:
		/*PC0 low */
		GPIOC->ODR &= ~GPIO_PIN_0;
		break;
	case Green:
		/*PC1 low */
		GPIOC->ODR &= ~GPIO_PIN_1;
		break;
	default:
		/* Not an LED, shouldn't get here */
		return;
	}
}
