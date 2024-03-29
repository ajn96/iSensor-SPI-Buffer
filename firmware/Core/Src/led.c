/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		led.c
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer LED control module
 **/

#include "reg.h"
#include "led.h"
#include "stm32f3xx_hal.h"

/**
  * @brief Turn on a selected LED
  *
  * @param light The LED to turn on. Must be of type LED
  *
  * @return void
  */
void LED_Turn_On(LED light)
{
	switch(light)
	{
	case Blue:
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

/**
  * @brief Turn off a selected LED
  *
  * @param light The LED to turn off. Must be of type LED
  *
  * @return void
  */
void LED_Turn_Off(LED light)
{
	switch(light)
	{
	case Blue:
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
