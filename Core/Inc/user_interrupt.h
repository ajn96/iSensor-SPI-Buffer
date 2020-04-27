/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_interrupt.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer user interrupt (data ready) config and generation functions header
 **/


#ifndef INC_USER_INTERRUPT_H_
#define INC_USER_INTERRUPT_H_

#include "registers.h"

/* Public function prototypes */
void UpdateUserInterrupt();
void UpdateDIOConfig();
void UpdateOutputPins(uint32_t interrupt, uint32_t overflow);

typedef struct DIOConfig
{
	uint32_t passPins;
	uint32_t intPins;
	uint32_t overflowPins;
}DIOConfig;

#endif /* INC_USER_INTERRUPT_H_ */
