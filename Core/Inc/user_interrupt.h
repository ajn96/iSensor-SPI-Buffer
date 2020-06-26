/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_interrupt.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer user interrupt (data ready) config and generation functions
 **/

#ifndef INC_USER_INTERRUPT_H_
#define INC_USER_INTERRUPT_H_

/* Includes */
#include "registers.h"

/* Public function prototypes */
void UpdateUserInterrupt();
void UpdateOutputPins(uint32_t interrupt, uint32_t overflow, uint32_t error);

#endif /* INC_USER_INTERRUPT_H_ */
