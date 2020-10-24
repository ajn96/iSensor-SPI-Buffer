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

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[NUM_REG_PAGES * REG_PER_PAGE];

/* Words per buffer (from isr.c) */
extern volatile uint32_t g_wordsPerCapture;

/* Capture in progress (from isr.c) */
extern volatile uint32_t g_captureInProgress;

/** PPS input interrupt mask (from timer.c) */
extern uint32_t g_PPSInterruptMask;

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
	/* Clear pending data ready interrupts */
	EXTI->PR = DATA_READY_INT_MASK;

	/* Update DIO input config (assign DR interrupt) */
	UpdateDIOInputConfig();

	/* Set 16-bit words per capture */
	g_wordsPerCapture = g_regs[BUF_LEN_REG] >> 1;

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
	/* Disable EXTI interrupt if PPS mode is not using interrupt also */
	if(!g_PPSInterruptMask)
		NVIC_DisableIRQ(EXTI9_5_IRQn);

	/* Disable capture timer */
	TIM4->CR1 &= ~0x1;

	/* Disable EXTI data ready interrupt source */
	EXTI->IMR &= ~(DATA_READY_INT_MASK);

	/* Clear any pending interrupts */
	EXTI->PR = DATA_READY_INT_MASK;
	TIM4->SR &= ~TIM_SR_UIF;

	/* Capture in progress set to false */
	g_captureInProgress = 0;

	/* Turn off green LED */
	TurnOffLED(Green);
}
