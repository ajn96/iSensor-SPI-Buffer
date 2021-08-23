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
void InitMicrosecondTimer();
void ClearMicrosecondTimer();
uint32_t GetMicrosecondTimestamp();
void EnablePPSTimer();
void DisablePPSTimer();
void IncrementPPSTime();
uint32_t GetPPSTimestamp();
void SleepMicroseconds(uint32_t microseconds);
void CheckPPSUnlock();
/* @endcond */

/** EXTI interrupt mask for possible PPS inputs */
#define PPS_INT_MASK	 		GPIO_PIN_8|GPIO_PIN_7|GPIO_PIN_4

/* Public variables exported from module */
extern uint32_t g_PPSInterruptMask;

#endif /* INC_TIMER_H_ */
