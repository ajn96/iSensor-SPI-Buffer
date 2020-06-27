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
volatile extern uint16_t g_regs[3 * REG_PER_PAGE];

/* Words per buffer (from isr.c) */
volatile extern uint32_t g_wordsPerCapture;

/* Capture in progress (from isr.c) */
volatile extern uint32_t g_captureInProgress;

/** PPS input interrupt mask (from timer.c) */
extern uint32_t g_PPSInterruptMask;

/** Data ready input interrupt mask (from dio.c) */
extern uint32_t g_DrInterruptMask;

/** Track EXTI interrupt mask for active data ready signal. Global scope */
uint32_t g_ActiveDrInterruptMask;

/* Track if a capture is currently running */
static volatile uint32_t capture_running = 0;

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
	EXTI->PR |= DATA_READY_INT_MASK;

	/* Set active mask based on latest DR settings */
	g_ActiveDrInterruptMask = g_DrInterruptMask;

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

	/* Clear any pending interrupts */
	EXTI->PR |= DATA_READY_INT_MASK;
	TIM4->SR &= ~TIM_SR_UIF;

	/* Set active interrupt mask to 0 */
	g_ActiveDrInterruptMask = 0;

	/* Capture in progress set to false */
	g_captureInProgress = 0;

	/* Turn off green LED */
	TurnOffLED(Green);
}
