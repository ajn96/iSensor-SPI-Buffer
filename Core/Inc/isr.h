/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		isr.h
  * @date		4/27/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for ISR includes
 **/

#ifndef INC_ISR_H_
#define INC_ISR_H_

/* Includes */
#include "imu_spi.h"
#include "registers.h"
#include "buffer.h"
#include "data_capture.h"
#include "timer.h"

/* User SPI port related interrupts */
void SPI2_IRQHandler(void);
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

#endif /* INC_ISR_H_ */
