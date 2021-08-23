/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		flash.h
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer flash memory interfacing functions
 **/

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void FlashUpdate();
void LoadRegsFlash();
void FlashCheckLoggedError();
void FlashLogError(uint32_t faultCode);
void FlashInitErrorLog();
uint16_t CalcRegSig(uint16_t * regs, uint32_t count);
/* @endcond */

/** Address for flash register storage (starts 256KB down in flash, program starts at 0) */
#define FLASH_REG_ADDR			0x08040000

/** Address for flash error logging (page prior to FLASH_REG_ADDR) */
#define FLASH_ERROR_ADDR		0x0803F800

/* Error codes stored in flash */

/** No error */
#define ERROR_NONE				0

/** Initialization error (in application firmware) */
#define ERROR_INIT				(1 << 0)

/** Hard fault exception */
#define ERROR_HARDFAULT			(1 << 1)

/** Memory access exception */
#define ERROR_MEM				(1 << 2)

/** Bus fault exception */
#define ERROR_BUS				(1 << 3)

/** Usage fault exception */
#define ERROR_USAGE				(1 << 4)

#endif /* INC_FLASH_H_ */
