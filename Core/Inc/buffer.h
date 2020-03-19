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
#include "reg_definitions.h"

void BufReset();
uint8_t* BufTakeElement();
uint8_t* BufAddElement();

#define BUF_SIZE	0x8000

#endif /* INC_BUFFER_H_ */
