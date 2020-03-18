/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer register interfacing
 **/

#ifndef INC_REGISTERS_H_
#define INC_REGISTERS_H_

#include "main.h"

uint16_t readReg(uint8_t regAddr);

uint16_t writeReg(uint8_t regAddr, uint8_t regValue);

uint16_t processRegWrite(uint8_t regAddr, uint8_t regValue);

#endif /* INC_REGISTERS_H_ */
