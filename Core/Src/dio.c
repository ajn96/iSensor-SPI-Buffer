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

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[3 * REG_PER_PAGE];

/** Struct storing current DIO output config. Global scope */
volatile DIOConfig g_pinConfig = {};

/* Private function prototypes */
static void ValidateDIOOutputConfig();
static uint16_t BuildDIOOutputConfigReg();
static void ParseDIOOutputConfig();

/**
  * @brief Gets two bit hardware identification code from identifier pins
  *
  * @return ID code read from ID pins (0 - 3)
  */
uint32_t GetHardwareID()
{
	uint32_t id;

	/* Get ID from GPIO port C, pins 2-3 */
	id = GPIOC->IDR;
	id = (id >> 2) & 0x3;
	return id;
}

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
	/* Get current config value */
	uint32_t config = g_regs[DIO_INPUT_CONFIG_REG];

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Clear unused bits */
	config &= 0x3F9F;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config & 0x1)
		config &= 0x3F91;
	else if(config & 0x2)
		config &= 0x3F92;
	else if(config & 0x4)
		config &= 0x3F94;
	else if(config & 0x8)
		config &= 0x3F98;
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
	g_regs[DIO_INPUT_CONFIG_REG] = config;
}

/**
  * @brief Validates DIO_OUTPUT_CONFIG settings and applies to GPIO.
  *
  * General procedure is as follows:
  *
  * passPins
  * Set SW_IN1 - SW_IN4 output values based on passPins. These act as inputs
  * to the ADG1611 analog switch. For each bit set in passPins, configure the
  * corresponding DIOx_Slave signal as an input (tristate). Only perform this
  * configuration if the pin is not currently acting as a PPS input
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
	g_regs[DIO_OUTPUT_CONFIG_REG] = BuildDIOOutputConfigReg();

	/* All SW_INx pins are driven as outputs */

	/* SW_IN1 (PB6) */
	if(g_pinConfig.passPins & 0x1)
		GPIOB->ODR &= ~GPIO_PIN_6;
	else
		GPIOB->ODR |= GPIO_PIN_6;

	/* SW_IN2 (PB7) */
	if(g_pinConfig.passPins & 0x2)
		GPIOB->ODR &= ~GPIO_PIN_7;
	else
		GPIOB->ODR |= GPIO_PIN_7;

	/* SW_IN3 (PC8) */
	if(g_pinConfig.passPins & 0x4)
		GPIOC->ODR &= ~GPIO_PIN_8;
	else
		GPIOC->ODR |= GPIO_PIN_8;

	/* SW_IN4 (PC9) */
	if(g_pinConfig.passPins & 0x8)
		GPIOC->ODR &= ~GPIO_PIN_9;
	else
		GPIOC->ODR |= GPIO_PIN_9;

	/* For each DIOx_Slave pin, set as output if used as interrupt source. Set as input if used as pass pin (and not PPS) */

	/* DIO1_Slave (PB4) */
	if((g_pinConfig.watermarkPins & 0x1) || (g_pinConfig.overflowPins & 0x1) || (g_pinConfig.errorPins & 0x1))
	{
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	else if((g_pinConfig.passPins & 0x1)&(~g_pinConfig.ppsPin))
	{
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}

	/* DIO2_Slave (PB8) */
	if((g_pinConfig.watermarkPins & 0x2) || (g_pinConfig.overflowPins & 0x2) || (g_pinConfig.errorPins & 0x2))
	{
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	else if((g_pinConfig.passPins & 0x2)&(~g_pinConfig.ppsPin))
	{
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}

	/* DIO3_Slave (PC7) */
	if((g_pinConfig.watermarkPins & 0x4) || (g_pinConfig.overflowPins & 0x4) || (g_pinConfig.errorPins & 0x4))
	{
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	}
	else if((g_pinConfig.passPins & 0x4)&(~g_pinConfig.ppsPin))
	{
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	}

	/* DIO4_Slave (PA8) */
	if((g_pinConfig.watermarkPins & 0x8) || (g_pinConfig.overflowPins & 0x8) || (g_pinConfig.errorPins & 0x8))
	{
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
	else if((g_pinConfig.passPins & 0x8)&(~g_pinConfig.ppsPin))
	{
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}

/**
  * @brief Parse DIO_CONFIG reg to local config struct
  *
  * @return void
  */
static void ParseDIOOutputConfig()
{
	/* Get current config value */
	uint32_t configReg = g_regs[DIO_OUTPUT_CONFIG_REG];

	g_pinConfig.passPins = (configReg) & 0xF;
	g_pinConfig.watermarkPins = (configReg >> 4) & 0xF;
	g_pinConfig.overflowPins = (configReg >> 8) & 0xF;
	g_pinConfig.errorPins = (configReg >> 12) & 0xF;
}

/**
  * @brief Parse local config struct to DIO_CONFIG reg value
  *
  * @return void
  */
static uint16_t BuildDIOOutputConfigReg()
{
	return g_pinConfig.passPins | (g_pinConfig.watermarkPins << 4) | (g_pinConfig.overflowPins << 8) | (g_pinConfig.errorPins << 12);
}

/**
  * @brief validates the current DIO config struct settings
  *
  * @return void
  */
static void ValidateDIOOutputConfig()
{
	/* Clear upper bits in each */
	g_pinConfig.passPins &= 0xF;
	g_pinConfig.watermarkPins &= 0xF;
	g_pinConfig.overflowPins &= 0xF;
	g_pinConfig.errorPins &= 0xF;

	/* Any pins set as pass pins cannot also be set as interrupt pins */
	g_pinConfig.overflowPins &= ~g_pinConfig.passPins;
	g_pinConfig.watermarkPins &= ~g_pinConfig.passPins;
	g_pinConfig.errorPins &= ~g_pinConfig.passPins;

	/* Any pins set as active PPS input pins cannot be interrupt pins (PPS gets priority) */
	g_pinConfig.overflowPins &= ~g_pinConfig.ppsPin;
	g_pinConfig.watermarkPins &= ~g_pinConfig.ppsPin;
	g_pinConfig.errorPins &= ~g_pinConfig.ppsPin;

	/* Interrupt priority: Error -> Watermark -> Overflow */
	g_pinConfig.watermarkPins &= ~g_pinConfig.errorPins;
	g_pinConfig.overflowPins &= ~g_pinConfig.watermarkPins;
	g_pinConfig.overflowPins &= ~g_pinConfig.errorPins;
}
