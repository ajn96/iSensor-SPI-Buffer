/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		data_capture.c
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer autonomous IMU data acquisition functions.
 **/

#include "reg.h"
#include "data_capture.h"
#include "stm32f3xx_hal.h"
#include "isr.h"
#include "timer.h"
#include "dio.h"
#include "led.h"

/**
  * @brief Enables autonomous data capture by enabling DR ISR in NVIC.
  *
  * @return void
  *
  * This function does not configure the interrupt hardware at all. The config
  * must be performed by UpdateDRConfig prior to calling this function.
  */
void Data_Capture_Enable()
{
	/* Clear pending data ready interrupts */
	EXTI->PR = DATA_READY_INT_MASK;

	/* Update DIO input config (assign DR interrupt) */
	DIO_Update_Input_Config();

	/* Set 16-bit words per capture */
	g_wordsPerCapture = g_regs[BUF_LEN_REG] >> 1;

	/* Enable data ready interrupts */
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* Turn on green LED */
	LED_Turn_On(Green);
}

/**
  * @brief disables autonomous data capture by disabling DR ISR.
  *
  * @return void
  *
  * This will stop a new capture from starting. A capture in progress
  * will still run to completion.
  */
void Data_Capture_Disable()
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
	LED_Turn_Off(Green);
}
