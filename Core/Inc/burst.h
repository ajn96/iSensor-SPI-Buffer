/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		burst.h
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer burst data read (slave SPI) module
 **/

#ifndef INC_BURST_H_
#define INC_BURST_H_

#include "registers.h"

/* Public function prototypes */
void BurstReadSetup();
void BurstReadDisable();

#endif /* INC_BURST_H_ */
