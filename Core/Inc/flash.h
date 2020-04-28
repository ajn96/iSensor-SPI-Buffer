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

/* Includes */
#include "stdint.h"
#include "registers.h"
#include "stm32f3xx_hal.h"

/* Public function prototypes */
void FlashUpdate();
void LoadRegsFlash();
uint16_t CalcRegSig(uint16_t * regs, uint32_t count);

/** Flash reg base address */
#define FLASH_BASE_ADDR 		0x0800F800

/** Flash page */
#define FLASH_PAGE				31

#endif /* INC_FLASH_H_ */
