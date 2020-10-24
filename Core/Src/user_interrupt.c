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

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[NUM_REG_PAGES * REG_PER_PAGE];

/** Struct to track config (from dio.c) */
extern DIOConfig g_pinConfig;

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
	uint32_t overflow, interrupt, error, currentTime;

	/* Static flags for driving watermark strobe */
	static uint32_t watermarkState, startTime;

	/* Get overflow status */
	overflow = (g_regs[BUF_CNT_0_REG] >= g_regs[BUF_MAX_CNT_REG]);

	/* Get watermark interrupt status */
	interrupt = (g_regs[BUF_CNT_0_REG] >= (g_regs[WATERMARK_INT_CONFIG_REG] & ~WATERMARK_PULSE_MASK));

	/* Get error interrupt status, masking out bits which are not error indicators */
	error = g_regs[STATUS_0_REG] & g_regs[ERROR_INT_CONFIG_REG];

	/* Apply overflow to status reg. Don't clear because user must read to clear */
	if(overflow)
	{
		g_regs[STATUS_0_REG] |= STATUS_BUF_FULL;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}

	/* Apply watermark to status reg */
	if(interrupt)
	{
		g_regs[STATUS_0_REG] |= STATUS_BUF_WATERMARK;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

		/* Check for strobe status */
		if(g_regs[WATERMARK_INT_CONFIG_REG] & WATERMARK_PULSE_MASK)
		{
			currentTime = DWT->CYCCNT;
			if((currentTime - startTime) > WATERMARK_HALF_PERIOD_TICKS)
			{
				startTime = currentTime;
				if(watermarkState)
					watermarkState = 0;
				else
					watermarkState = 1;
			}
		}
		else
		{
			/* Watermark directly follows interrupt */
			watermarkState = interrupt;
		}
	}
	else
	{
		/* Reset watermark state when interrupt not triggered */
		watermarkState = 0;
	}

	/* Apply error to LEDs */
	if(error)
	{
		TurnOnLED(Blue);
	}
	else
	{
		TurnOffLED(Blue);
	}

	/* Update interrupt output pins */
	UpdateOutputPins(watermarkState, overflow, error);
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
	if(g_pinConfig.watermarkPins & 0x1)
	{
		/* PB4 */
		if(watermark)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(g_pinConfig.watermarkPins & 0x2)
	{
		/* PB8 */
		if(watermark)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(g_pinConfig.watermarkPins & 0x4)
	{
		/* PC7 */
		if(watermark)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(g_pinConfig.watermarkPins & 0x8)
	{
		/* PA8 */
		if(watermark)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}

	/* Apply overflow values to pins */
	if(g_pinConfig.overflowPins & 0x1)
	{
		/* PB4 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(g_pinConfig.overflowPins & 0x2)
	{
		/* PB8 */
		if(overflow)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(g_pinConfig.overflowPins & 0x4)
	{
		/* PC7 */
		if(overflow)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(g_pinConfig.overflowPins & 0x8)
	{
		/* PA8 */
		if(overflow)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}

	/* Apply error values to pins */
	if(g_pinConfig.errorPins & 0x1)
	{
		/* PB4 */
		if(error)
			GPIOB->ODR |= GPIO_PIN_4;
		else
			GPIOB->ODR &= ~GPIO_PIN_4;
	}
	if(g_pinConfig.errorPins & 0x2)
	{
		/* PB8 */
		if(error)
			GPIOB->ODR |= GPIO_PIN_8;
		else
			GPIOB->ODR &= ~GPIO_PIN_8;
	}
	if(g_pinConfig.errorPins & 0x4)
	{
		/* PC7 */
		if(error)
			GPIOC->ODR |= GPIO_PIN_7;
		else
			GPIOC->ODR &= ~GPIO_PIN_7;
	}
	if(g_pinConfig.errorPins & 0x8)
	{
		/* PA8 */
		if(error)
			GPIOA->ODR |= GPIO_PIN_8;
		else
			GPIOA->ODR &= ~GPIO_PIN_8;
	}
}
