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

/* register array */
extern volatile uint16_t regs[];

/**
  * @brief Turn on a selected LED
  *
  * @param light The LED to turn on. Must be of type LED
  *
  * @return void
  */
void TurnOnLED(LED light)
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
void TurnOffLED(LED light)
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

/**
  * @brief Updates the red LED based on STATUS error bits
  *
  * @return void
  *
  * This function is called periodically from the main loop
  */
void UpdateLEDStatus()
{
	uint16_t status = regs[STATUS_0_REG];
	/* Mask out upper 6 bits (not error indicators) */
	status &= 0x03FF;
	if(status)
	{
		TurnOnLED(Red);
	}
	else
	{
		TurnOffLED(Red);
	}
}
