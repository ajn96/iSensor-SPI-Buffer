/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_interrupt.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer user interrupt (data ready) config and generation functions
 **/

#ifndef INC_USER_INTERRUPT_H_
#define INC_USER_INTERRUPT_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void User_Interrupt_Update();
void User_Interrupt_Update_Output_Pins(uint32_t interrupt, uint32_t overflow, uint32_t error);
/* @endcond */

/** Watermark interrupt toggle freq (Hz) when in toggle mode */
#define WATERMARK_FREQ					10000

/** Number of CPU ticks per watermark toggle */
#define WATERMARK_HALF_PERIOD_TICKS		(uint32_t) (72000000 / (WATERMARK_FREQ * 2))

#endif /* INC_USER_INTERRUPT_H_ */
