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

/* Global register array */
volatile extern uint16_t regs[3 * REG_PER_PAGE];

/* Local function prototypes */
static void ValidateDIOConfig();
static void ParseDIOConfig();
static uint16_t BuildDIOConfigReg();

/** Struct to track config */
DIOConfig config;

/**
  * @brief Updates the user interrupt (data ready) signal status
  *
  * @return void
  *
  * This function is called periodically from the main loop. It
  * clears or sets the selected interrupt and overflow notification pins
  * (DIOx_Slave) and also updates the STATUS register flags
 **/
void UpdateUserInterrupt()
{
	uint32_t overflow, interrupt;

	/* Get overflow status */
	overflow = (regs[BUF_CNT_0_REG] >= regs[BUF_MAX_CNT_REG]);

	/* Get interrupt status */
	interrupt = (regs[BUF_CNT_0_REG] >= regs[INT_CONFIG_REG]);

	/* Apply overflow to status reg. Don't clear because user must read to clear */
	if(overflow)
	{
		regs[STATUS_0_REG] |= STATUS_BUF_FULL;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}

	/* Apply interrupt to status reg */
	if(interrupt)
	{
		regs[STATUS_0_REG] |= STATUS_BUF_INT;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}

	/* Update pins */
	UpdateOutputPins(interrupt, overflow);
}

/**
  * @brief Updates the output pins based on given interrupt/overflow status
  *
  * @param interrupt The buffer data ready interrupt status
  *
  * @param overflow The buffer overflow interrupt status
  *
  * @return void
 **/
void UpdateOutputPins(uint32_t interrupt, uint32_t overflow)
{
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
  * corresponding DIOx_Slave signal as an input (tristate).
  *
  * intPins
  * Configure selected interrupt pins as output GPIO
  *
  * overflowPins
  * Configure selected overflow interrupt pins as output GPIO
  *
  * Any pins which are unused will be configured as inputs.
 **/
void UpdateDIOConfig()
{
	/* Struct to configure GPIO pins */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Parse reg value */
	ParseDIOConfig();
	/* Validate settings */
	ValidateDIOConfig();
	/* Write back */
	regs[DIO_CONFIG_REG] = BuildDIOConfigReg();

	/* All SW_INx pins are driven as outputs */

	/* SW_IN1 (PB6) */
	if(config.passPins & 0x1)
		GPIOB->ODR &= ~GPIO_PIN_6;
	else
		GPIOB->ODR |= GPIO_PIN_6;


	/* SW_IN2 (PB7) */
	if(config.passPins & 0x2)
		GPIOB->ODR &= ~GPIO_PIN_7;
	else
		GPIOB->ODR |= GPIO_PIN_7;


	/* SW_IN3 (PC8) */
	if(config.passPins & 0x4)
		GPIOC->ODR &= ~GPIO_PIN_8;
	else
		GPIOC->ODR |= GPIO_PIN_8;

	/* SW_IN4 (PC9) */
	if(config.passPins & 0x8)
		GPIOC->ODR &= ~GPIO_PIN_9;
	else
		GPIOC->ODR |= GPIO_PIN_9;

	/* For each DIOx_Slave pin, set as input if not used as overflow/interrupt */

	/* DIO1_Slave (PB4) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	if((config.intPins & 0x1) || (config.overflowPins & 0x1))
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO2_Slave (PB8) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	if((config.intPins & 0x2) || (config.overflowPins & 0x2))
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO3_Slave (PC7) */
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7);
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	if((config.intPins & 0x4) || (config.overflowPins & 0x4))
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* DIO4_Slave (PA8) */
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	if((config.intPins & 0x8) || (config.overflowPins & 0x8))
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief Parse DIO_CONFIG reg to local config struct
  *
  * @return void
  */
static void ParseDIOConfig()
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
static uint16_t BuildDIOConfigReg()
{
	return config.passPins | (config.intPins << 4) | (config.overflowPins << 8);
}

/**
  * @brief validates the current DIO config struct settings
  *
  * @return void
  */
static void ValidateDIOConfig()
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
