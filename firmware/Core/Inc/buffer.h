/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		buffer.h
  * @date		3/19/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer buffer data structure module
 **/

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void BufReset();
uint8_t* BufTakeElement();
uint8_t* BufAddElement();
uint32_t BufCanAddElement();
/* @endcond */

/** Buffer memory allocation */
#define BUF_SIZE	0xA000

/** Smallest buffer data entry size. Real size is +10 bytes */
#define BUF_MIN_ENTRY	2

/** Largest buffer data entry size. Real size is +10 bytes */
#define BUF_MAX_ENTRY	64

/* Public variables exported from module */
extern uint32_t g_bufLastRegIndex;
extern uint32_t g_bufCount;
extern uint32_t g_bufNumWords32;

#endif /* INC_BUFFER_H_ */
