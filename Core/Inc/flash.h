/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		flash.h
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer flash memory interfacing functions.
 **/

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include "stdint.h"

void FlashUpdate();
void LoadRegsFlash();
void FactoryReset();

void PrepareRegsForFlash();
uint32_t CalcRegSig(uint16_t * regs, uint32_t count);


#endif /* INC_FLASH_H_ */
