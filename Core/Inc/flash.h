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
void FlashCheckLoggedError();
void FlashLogError(uint32_t faultCode);
void FlashInitErrorLog();

/** Address for flash register storage (2KB pages, put on page 31) */
#define FLASH_REG_ADDR			0x0800F800

/** Address for flash error logging (put on page 32) */
#define FLASH_ERROR_ADDR		0x08010000

/* Error codes stored in flash */
#define ERROR_NONE				0
#define ERROR_INIT				(1 << 0)
#define ERROR_HARDFAULT			(1 << 1)
#define ERROR_MEM				(1 << 2)
#define ERROR_BUS				(1 << 3)
#define ERROR_USAGE				(1 << 4)

#endif /* INC_FLASH_H_ */
