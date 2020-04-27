/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		spi_passthrough.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for the iSensor-SPI-Buffer SPI pass through (to IMU) module
 **/

#ifndef INC_SPI_PASSTHROUGH_H_
#define INC_SPI_PASSTHROUGH_H_

/* Includes */
#include "main.h"
#include "registers.h"

/* Public function prototypes */
uint16_t ImuSpiTransfer(uint32_t MOSI);
uint16_t ImuReadReg(uint8_t RegAddr);
uint16_t ImuWriteReg(uint8_t RegAddr, uint8_t RegValue);
void UpdateImuSpiConfig();
void SleepMicroseconds(uint32_t microseconds);

#endif
