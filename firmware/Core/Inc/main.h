/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		main.h
  * @date		3/14/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer main
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Header includes require for prototypes */
#include "stm32f3xx_hal.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void Main_Error_Handler();
/* @endcond */

/** Bool data type */
typedef enum
{
	false = 0,
	true = 1
}bool;

/** Check for any update flags set for deferred processing */
#define STATE_CHECK_FLAGS		0

/** Check that PPS isn't unlocked */
#define STATE_CHECK_PPS			1

/** Update ADC state machine */
#define STATE_READ_ADC			2

/** Handle any USB command line Rx activity */
#define STATE_CHECK_USB			3

/** Check stream status for CLI (SD and USB) */
#define STATE_CHECK_STREAM		4

/** Step running SD script (if any) */
#define STATE_STEP_SCRIPT		5

/* Public variables exported from module */
extern DMA_HandleTypeDef g_dma_spi1_rx;
extern DMA_HandleTypeDef g_dma_spi1_tx;
extern DMA_HandleTypeDef g_dma_spi2_tx;
extern SPI_HandleTypeDef g_spi1;
extern SPI_HandleTypeDef g_spi2;

#endif /* __MAIN_H */
