/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		timer.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer timer module
 **/

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void Timer_Init();
void Timer_Clear_Microsecond_Timer();
uint32_t Timer_Get_Microsecond_Timestamp();
void Timer_Enable_PPS();
void Timer_Disable_PPS();
void Timer_Increment_PPS_Time();
uint32_t Timer_Get_PPS_Timestamp();
void Timer_Sleep_Microseconds(uint32_t microseconds);
void Timer_Check_PPS_Unlock();
/* @endcond */

/** EXTI interrupt mask for possible PPS inputs */
#define PPS_INT_MASK	 		GPIO_PIN_8|GPIO_PIN_7|GPIO_PIN_4

/* Public variables exported from module */
extern uint32_t g_PPSInterruptMask;

#endif /* INC_TIMER_H_ */
