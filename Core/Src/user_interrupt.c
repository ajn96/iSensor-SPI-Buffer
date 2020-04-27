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

/** Track config */
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
	uint32_t overflow, interrupt;

	/* Get overflow status */
	overflow = (regs[BUF_CNT_REG] >= regs[BUF_MAX_CNT_REG]);

	/* Get interrupt status */
	interrupt = (regs[BUF_CNT_REG] >= regs[INT_CONFIG_REG]);

	/* Apply overflow to status reg. Don't clear because user must read to clear */
	if(overflow)
	{
		regs[STATUS_REG] |= STATUS_BUF_FULL;
	}

	/* Apply interrupt values to pins */
	if(config.intPins & 0x1)
	{
		/* PB4 */
		if(interrupt)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(config.intPins & 0x2)
	{
		/* PB8 */
		if(interrupt)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(config.intPins & 0x4)
	{
		/* PC7 */
		if(interrupt)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(config.intPins & 0x8)
	{
		/* PA8 */
		if(interrupt)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}

	/* Apply overflow values to pins */
	if(config.overflowPins & 0x1)
	{
		/* PB4 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(config.overflowPins & 0x2)
	{
		/* PB8 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(config.overflowPins & 0x4)
	{
		/* PC7 */
		if(overflow)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(config.overflowPins & 0x8)
	{
		/* PA8 */
		if(overflow)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}
}

/**
  * @brief Validates DIO_CONFIG settings and applies to GPIO.
  *
  * General procedure is as follows:
  *
  * passPins
  * Set SW_IN1 - SW_IN4 output values based on passPins. These act as inputs
  * to the ADG1611 analog switch. For each bit set in passPins, configure the
  * corresponding DIOn_master signal as an input (tristate).
  *
  * intPins
  * Configure selected interrupt pins as output GPIO
  *
  * overflowPins
  * Configure selected overflow interrupt pins as output GPIO
 **/
void UpdateDIOConfig()
{
	/* Parse reg value */
	ParseDIOConfig();
	/* Validate settings */
	ValidateDIOConfig();
	/* Write back */
	regs[DIO_CONFIG_REG] = BuildDIOConfigReg();

	/* Apply parsed settings. Set any pass pins as inputs on DIO slave. Set
	 * any overflow or interrupt pins as output */
}

/**
  * @brief Parse DIO_CONFIG reg to local config struct
  *
  * @return void
  */
void ParseDIOConfig()
{
	/* Get current config value */
	uint32_t configReg = regs[DIO_CONFIG_REG];

	config.passPins = (configReg) & 0xF;
	config.intPins = (configReg >> 4) & 0xF;
	config.overflowPins = (configReg >> 8) & 0xF;
}

/**
  * @brief Parse local config struct to DIO_CONFIG reg value
  *
  * @return void
  */
uint16_t BuildDIOConfigReg()
{
	return config.passPins | (config.intPins << 4) | (config.overflowPins << 8);
}

/**
  * @brief validates the current DIO config struct settings
  *
  * @return void
  */
void ValidateDIOConfig()
{
	/* Clear upper bits in each */
	config.passPins &= 0xF;
	config.intPins &= 0xF;
	config.overflowPins &= 0xF;

	/* Any pins set as pass pins cannot also be set as int/overflow pins */
	config.overflowPins &= ~config.passPins;
	config.intPins &= ~config.passPins;

	/* Any pins set as interrupt pins cannot be set as overflow pins */
	config.overflowPins &= ~config.intPins;
}
