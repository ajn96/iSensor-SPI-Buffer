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

/* Global register array */
volatile extern uint16_t regs[3 * REG_PER_PAGE];

/* Words per buffer (from isr) */
volatile extern uint32_t WordsPerCapture;

/* Capture in progress (from isr) */
volatile extern uint32_t CaptureInProgress;

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

	/* Set 16-bit words per capture */
	WordsPerCapture = regs[BUF_LEN_REG] >> 1;

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

	/* Disable capture timer */
	TIM4->CR1 &= ~0x1;

	/* Clear any pending interrupts */
	EXTI->PR |= (0x1F << 5);
	TIM4->SR &= ~TIM_SR_UIF;

	/* Capture in progress set to false */
	CaptureInProgress = 0;

	/* Turn off green LED */
	TurnOffLED(Green);
}
