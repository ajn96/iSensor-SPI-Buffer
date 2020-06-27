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

#include "main.h"

/* Public function prototypes */
void InitMicrosecondTimer();
void ClearMicrosecondTimer();
uint32_t GetMicrosecondTimestamp();

void EnablePPSTimer();
void DisablePPSTimer();
uint32_t GetPPSTimestamp();

void SleepMicroseconds(uint32_t microseconds);

#endif /* INC_TIMER_H_ */
