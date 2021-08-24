/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		isr.h
  * @date		4/27/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for the interrupt service routine module
 **/

#ifndef INC_ISR_H_
#define INC_ISR_H_

/* Header includes require for prototypes */
#include <stdint.h>

/** Default value used to track which user SPI byte is being processed */
#define DEFAULT_DATA		0x80000000u

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */

/* User SPI port related interrupts */
void EXTI15_10_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);

/* IMU SPI port related interrupts */
void DMA1_Channel2_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void);

/* Data ready related interrupts */
void EXTI9_5_IRQHandler();
void EXTI4_IRQHandler();
void TIM4_IRQHandler();

/* Fault exceptions */
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
/* @endcond */

/* Public variables exported from module */
extern volatile uint32_t g_wordsPerCapture;
extern volatile uint32_t g_captureInProgress;

#endif /* INC_ISR_H_ */
