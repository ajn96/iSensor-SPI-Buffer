/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		buffer.h
  * @date		3/19/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer buffer data structure module
 **/

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

/* Includes */
#include "main.h"
#include "registers.h"

/* Public function prototypes */
void BufReset();
uint8_t* BufTakeElement();
uint8_t* BufAddElement();

#define BUF_SIZE	0xA000

#define BUF_MIN_ENTRY	2
#define BUF_MAX_ENTRY	64

#endif /* INC_BUFFER_H_ */
