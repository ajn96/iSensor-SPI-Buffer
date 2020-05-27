/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		data_capture.h
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer autonomous IMU data acquisition module
 **/

#ifndef INC_DATA_CAPTURE_H_
#define INC_DATA_CAPTURE_H_

/* Includes */
#include "registers.h"
#include "led.h"

/* Public function prototypes */
void EnableDataCapture();
void DisableDataCapture();
void UpdateDRConfig();
void EnableSampleTimer(uint32_t timerfreq);
uint32_t GetCurrentSampleTime();

#endif /* INC_DATA_CAPTURE_H_ */
