/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		dfu.h
  * @date		10/9/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer runtime firmware upgrade module
 **/

#ifndef INC_DFU_H_
#define INC_DFU_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void CheckDFUFlags();
void PrepareDFUBoot();
/* @endcond */

#define ENABLE_DFU_KEY		0xA5A51234

#define DFU_FLAG_ADDR		(volatile uint32_t *) 0x2000FFFC

#endif /* INC_DFU_H_ */
