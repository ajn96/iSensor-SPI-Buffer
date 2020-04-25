/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_interrupt.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer user interrupt (data ready) config and generation functions
 **/

#include "user_interrupt.h"

/* Register array */
volatile extern uint16_t regs[];

/* Track config */
DIOConfig config;


/**
  * @brief Updates the user interrupt (data ready) signal status
  *
  * @return void
  *
  * This function is called periodically from the main loop.
 **/
void UpdateUserInterrupt()
{
	/* Get overflow status */

	/* Get interrupt status */
}

/** @brief Validates DIO_CONFIG settings and applies to GPIO.
  *
  * General procedure is as follows:
  *
  * passPins
  * Set SW_IN1 - SW_IN4 output values based on passPins. These act as inputs
  * to the ADG1611 analog switch. For each bit set in passPins, configure the
  * corresponding DIOn_master signal as an input (tristate).
  *
  * drPins
  * Configure single selected DR pin as an input with interrupt triggering
  *
  * intPins
  * Configure selected interrupt pins as output GPIO
  *
  * overflowPins
  * Configure selected overflow interrupt pins as output GPIO
  *
 **/
void UpdateDIOConfig()
{
	/* Parse reg value */
	ParseDIOConfig();
	/* Validate settings */
	ValidateDIOConfig();
	/* Write back */
	regs[DIO_CONFIG_REG] = BuildDIOConfigReg();

	//TODO: Apply parsed settings
}

void ParseDIOConfig()
{
	/* Get current config value */
	uint32_t configReg = regs[DIO_CONFIG_REG];

	config.drPins = configReg & 0xF;
	config.passPins = (configReg >> 4) & 0xF;
	config.intPins = (configReg >> 8) & 0xF;
	config.overflowPins = (configReg >> 12) & 0xF;
}

uint16_t BuildDIOConfigReg()
{
	return (config.drPins) | (config.passPins << 4) | (config.intPins << 8) | (config.overflowPins << 12);
}

void ValidateDIOConfig()
{
	/* Clear upper bits in each */
	config.drPins &= 0xF;
	config.passPins &= 0xF;
	config.intPins &= 0xF;
	config.overflowPins &= 0xF;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config.drPins & 0x1)
		config.drPins = 0x1;
	else if(config.drPins & 0x2)
		config.drPins = 0x2;
	else if(config.drPins & 0x4)
		config.drPins = 0x4;
	else if(config.drPins & 0x8)
		config.drPins = 0x8;
	else
		config.drPins = 0x1;

	/* Any pins set as pass pins cannot also be set as int/overflow pins */
	config.overflowPins = config.overflowPins ^ config.passPins;
	config.intPins = config.intPins ^ config.passPins;

	/* Pin set as DR pin cannot be set as int/overflow pin */
	config.overflowPins = config.overflowPins ^ config.drPins;
	config.intPins = config.intPins ^ config.drPins;

	/* Any pins set as interrupt pins cannot be set as overflow pins */
	config.overflowPins = config.overflowPins ^ config.intPins;
}
