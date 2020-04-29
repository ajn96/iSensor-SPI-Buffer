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

/* Track if a capture is currently running */
volatile uint32_t capture_running = 0;

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
	/* Clear pending interrupts */
	EXTI->PR |= (0x1F << 5);

	/* Enable data ready interrupts */
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* Turn on green LED */
	TurnOnLED(Green);
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
	/* Disable data ready interrupts */
	NVIC_DisableIRQ(EXTI9_5_IRQn);

	/* Clear pending interrupts */
	EXTI->PR |= (0x1F << 5);

	/* Turn off green LED */
	TurnOffLED(Green);
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
	/* Disable ISR */
	DisableDataCapture();

	/* Get current config value */
	uint32_t config = regs[DR_CONFIG_REG];

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Clear unused bits */
	config &= 0x1F;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config & 0x1)
		config &= 0x11;
	else if(config & 0x2)
		config &= 0x12;
	else if(config & 0x4)
		config &= 0x14;
	else if(config & 0x8)
		config &= 0x18;
	else
		config &= 0x11;

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
	regs[DR_CONFIG_REG] = config;
}
