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

/** Struct to track config */
extern DIOConfig pinConfig;

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
	/* Interrupt flags */
	uint32_t overflow, interrupt, error;

	/* Get overflow status */
	overflow = (regs[BUF_CNT_0_REG] >= regs[BUF_MAX_CNT_REG]);

	/* Get watermark interrupt status */
	interrupt = (regs[BUF_CNT_0_REG] >= regs[WATERMARK_INT_CONFIG_REG]);

	/* Get error interrupt status, masking out bits which are not error indicators */
	error = regs[STATUS_0_REG] & regs[ERROR_INT_CONFIG_REG];

	/* Apply overflow to status reg. Don't clear because user must read to clear */
	if(overflow)
	{
		regs[STATUS_0_REG] |= STATUS_BUF_FULL;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}

	/* Apply watermark to status reg */
	if(interrupt)
	{
		regs[STATUS_0_REG] |= STATUS_BUF_INT;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}

	/* Apply error to LEDs */
	if(error)
	{
		TurnOnLED(Red);
	}
	else
	{
		TurnOffLED(Red);
	}

	/* Update interrupt output pins */
	UpdateOutputPins(interrupt, overflow, error);
}

/**
  * @brief Updates the output pins based on given interrupt/overflow status
  *
  * @param watermark The buffer water mark interrupt status
  *
  * @param overflow The buffer overflow interrupt status
  *
  * @param error The buffer error interrupt status
  *
  * @return void
 **/
void UpdateOutputPins(uint32_t watermark, uint32_t overflow, uint32_t error)
{
	/* Apply interrupt values to pins */
	if(pinConfig.watermarkPins & 0x1)
	{
		/* PB4 */
		if(watermark)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(pinConfig.watermarkPins & 0x2)
	{
		/* PB8 */
		if(watermark)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(pinConfig.watermarkPins & 0x4)
	{
		/* PC7 */
		if(watermark)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(pinConfig.watermarkPins & 0x8)
	{
		/* PA8 */
		if(watermark)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}

	/* Apply overflow values to pins */
	if(pinConfig.overflowPins & 0x1)
	{
		/* PB4 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(pinConfig.overflowPins & 0x2)
	{
		/* PB8 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(pinConfig.overflowPins & 0x4)
	{
		/* PC7 */
		if(overflow)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(pinConfig.overflowPins & 0x8)
	{
		/* PA8 */
		if(overflow)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}

	/* Apply error values to pins */
	if(pinConfig.errorPins & 0x1)
	{
		/* PB4 */
		if(error)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(pinConfig.errorPins & 0x2)
	{
		/* PB8 */
		if(error)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(pinConfig.errorPins & 0x4)
	{
		/* PC7 */
		if(error)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(pinConfig.errorPins & 0x8)
	{
		/* PA8 */
		if(error)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}
}
