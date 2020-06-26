/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		dio.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer DIO interfacing module
 **/

#include "dio.h"

/* Global register array */
volatile extern uint16_t regs[3 * REG_PER_PAGE];

/** Struct storing current DIO output config */
volatile DIOConfig pinConfig = {};

/* Private function prototypes */
static void ValidateDIOOutputConfig();
static uint16_t BuildDIOOutputConfigReg();
static void ParseDIOOutputConfig();

/**
  * @brief Validates and updates the data ready and PPS input configuration based on DIO_INPUT_CONFIG
  *
  * @return void
  *
  * This function validates the DIO_INPUT_CONFIG contents, then uses the selected
  * DR configuration to enable the selected DIOn_Master pin as an interrupt
  * source with the desired polarity. This function does not enable PPS input (just ensures
  * that only one bit is set).
  */
void UpdateDIOInputConfig()
{
	/* Disable ISR */
	DisableDataCapture();

	/* Get current config value */
	uint32_t config = regs[DIO_INPUT_CONFIG_REG];

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Clear unused bits */
	config &= 0xF1F;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config & 0x1)
		config &= 0xF11;
	else if(config & 0x2)
		config &= 0xF12;
	else if(config & 0x4)
		config &= 0xF14;
	else if(config & 0x8)
		config &= 0xF18;
	else
		config |= 0x1;

	/* Configure selected pin to trigger interrupt. Disable interrupt initially */
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	/* DIO1 master (PB5) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	if(config & 0x1)
	{
		/* This is DR pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO2 master (PB9) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	if(config & 0x2)
	{
		/* This is DR pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DIO3 master (PC6) */
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6);
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	if(config & 0x4)
	{
		/* This is DR pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* DIO4 master (PA9) */
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	if(config & 0x8)
	{
		/* This is DR pin */
		if(config & 0x10)
			GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		else
			GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	}
	else
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	/* Apply settings */
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Apply modified settings back to reg */
	regs[DIO_INPUT_CONFIG_REG] = config;
}

/**
  * @brief Validates DIO_OUTPUT_CONFIG settings and applies to GPIO.
  *
  * General procedure is as follows:
  *
  * passPins
  * Set SW_IN1 - SW_IN4 output values based on passPins. These act as inputs
  * to the ADG1611 analog switch. For each bit set in passPins, configure the
  * corresponding DIOx_Slave signal as an input (tristate).
  *
  * watermarkPins
  * Configure selected watermark interrupt pins as output GPIO
  *
  * overflowPins
  * Configure selected overflow interrupt pins as output GPIO
  *
  * errorPins
  * Configure selected error interrupt pins as output GPIO
  *
  * Any pins which are unused will be configured as inputs.
 **/
void UpdateDIOOutputConfig()
{
	/* Struct to configure GPIO pins */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Parse reg value */
	ParseDIOOutputConfig();

	/* Validate settings */
	ValidateDIOOutputConfig();

	/* Write back */
	regs[DIO_OUTPUT_CONFIG_REG] = BuildDIOOutputConfigReg();

	/* All SW_INx pins are driven as outputs */

	/* SW_IN1 (PB6) */
	if(pinConfig.passPins & 0x1)
		GPIOB->ODR &= ~GPIO_PIN_6;
	else
		GPIOB->ODR |= GPIO_PIN_6;

	/* SW_IN2 (PB7) */
	if(pinConfig.passPins & 0x2)
		GPIOB->ODR &= ~GPIO_PIN_7;
	else
		GPIOB->ODR |= GPIO_PIN_7;

	/* SW_IN3 (PC8) */
	if(pinConfig.passPins & 0x4)
		GPIOC->ODR &= ~GPIO_PIN_8;
	else
		GPIOC->ODR |= GPIO_PIN_8;

	/* SW_IN4 (PC9) */
	if(pinConfig.passPins & 0x8)
		GPIOC->ODR &= ~GPIO_PIN_9;
	else
		GPIOC->ODR |= GPIO_PIN_9;

	/* For each DIOx_Slave pin, set as input if not used as overflow/interrupt */

	/* DIO1_Slave (PB4) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	if((pinConfig.watermarkPins & 0x1) || (pinConfig.overflowPins & 0x1) || (pinConfig.errorPins & 0x1))
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
	if((pinConfig.watermarkPins & 0x2) || (pinConfig.overflowPins & 0x2) || (pinConfig.errorPins & 0x2))
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
	if((pinConfig.watermarkPins & 0x4) || (pinConfig.overflowPins & 0x4) || (pinConfig.errorPins & 0x4))
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
	if((pinConfig.watermarkPins & 0x8) || (pinConfig.overflowPins & 0x8) || (pinConfig.errorPins & 0x8))
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
static void ParseDIOOutputConfig()
{
	/* Get current config value */
	uint32_t configReg = regs[DIO_OUTPUT_CONFIG_REG];

	pinConfig.passPins = (configReg) & 0xF;
	pinConfig.watermarkPins = (configReg >> 4) & 0xF;
	pinConfig.overflowPins = (configReg >> 8) & 0xF;
	pinConfig.errorPins = (configReg >> 12) & 0xF;
}

/**
  * @brief Parse local config struct to DIO_CONFIG reg value
  *
  * @return void
  */
static uint16_t BuildDIOOutputConfigReg()
{
	return pinConfig.passPins | (pinConfig.watermarkPins << 4) | (pinConfig.overflowPins << 8) | (pinConfig.errorPins << 12);
}

/**
  * @brief validates the current DIO config struct settings
  *
  * @return void
  */
static void ValidateDIOOutputConfig()
{
	/* Clear upper bits in each */
	pinConfig.passPins &= 0xF;
	pinConfig.watermarkPins &= 0xF;
	pinConfig.overflowPins &= 0xF;
	pinConfig.errorPins &= 0xF;

	/* Any pins set as pass pins cannot also be set as interrupt pins */
	pinConfig.overflowPins &= ~pinConfig.passPins;
	pinConfig.watermarkPins &= ~pinConfig.passPins;
	pinConfig.errorPins &= ~pinConfig.passPins;

	/* Interrupt priority: Error -> Watermark -> Overflow */
	pinConfig.watermarkPins &= ~pinConfig.errorPins;
	pinConfig.overflowPins &= ~pinConfig.watermarkPins;
	pinConfig.overflowPins &= ~pinConfig.errorPins;
}
