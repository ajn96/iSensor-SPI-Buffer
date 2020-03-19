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
#include "SpiPassthrough.h"
#include "reg_definitions.h"
#include "buffer.h"

uint16_t ReadReg(uint8_t regAddr);

uint16_t WriteReg(uint8_t regAddr, uint8_t regValue);

uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue);

/* Update flags definitions */
#define IMU_DR_CONFIG_FLAG		(1 << 0)
#define IMU_SPI_CONFIG_FLAG		(1 << 1)
#define USER_DR_CONFIG_FLAG 	(1 << 2)
#define USER_SPI_CONFIG_FLAG	(1 << 3)
#define USER_COMMAND_FLAG		(1 << 4)

#endif /* INC_REGISTERS_H_ */
