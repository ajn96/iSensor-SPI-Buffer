/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		buffer.h
  * @date		3/19/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer buffer implementation
 **/

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

#include "main.h"
#include "registers.h"

void BufReset();
volatile uint8_t* BufTakeElement();
volatile uint8_t* BufAddElement();

#define BUF_SIZE	0x8000

#define BUF_MIN_ENTRY	1
#define BUF_MAX_ENTRY	64

#endif /* INC_BUFFER_H_ */
