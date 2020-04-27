/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		data_capture.c
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer autonomous IMU data acquisition functions.
 **/

#include "data_capture.h"

/* register array */
extern volatile uint16_t regs[];

/**
  * @brief Enables autonomous data capture by enabling DR ISR in NVIC.
  *
  * @return void
  *
  * This function does not configure the interrupt hardware at all. The config
  * must be performed by UpdateDRConfig prior to calling this function.
  */
void EnableDataCapture()
{

}

/**
  * @brief disables autonomous data capture by disabling DR ISR.
  *
  * @return void
  *
  * This will stop a new capture from starting. A capture in progress
  * will still run to completion.
  */
void DisableDataCapture()
{

}

/**
  * @brief Updates the data ready input configuration based on DR_CONFIG
  *
  * @return void
  *
  * This function validates the DR_CONFIG contents, then uses the selected
  * DR configuration to enable the selected DIOn_Master pin as an interrupt
  * source with the desired polarity.
  */
void UpdateDRConfig()
{
	/* Get current config value */
	uint32_t config = regs[DR_CONFIG_REG];

	/* Clear unused bits */
	config &= 0x1F;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config & 0x1)
		config = 0x1;
	else if(config & 0x2)
		config = 0x2;
	else if(config & 0x4)
		config = 0x4;
	else if(config & 0x8)
		config = 0x8;
	else
		config = 0x1;

	/* Configure selected pin to trigger interrupt. Disable interrupt initially */

	/* Apply modified settings back to reg */
	regs[DR_CONFIG_REG] = config;
}
