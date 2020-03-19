/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		SpiPassthrough.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for the iSensor-SPI-Buffer SPI pass through module
 **/

#ifndef INC_SPIPASSTHROUGH_H_
#define INC_SPIPASSTHROUGH_H_

#include "main.h"

uint16_t ImuSpiTransfer(uint16_t MOSI);

uint16_t ImuReadReg(uint8_t RegAddr);

uint16_t ImuWriteReg(uint8_t RegAddr, uint8_t RegValue);

#endif