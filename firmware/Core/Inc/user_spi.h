/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_spi.h
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer user SPI (slave SPI) module
 **/

#ifndef INC_USER_SPI_H_
#define INC_USER_SPI_H_

#include "registers.h"
#include "main.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void BurstReadSetup();
void BurstReadDisable();
void UpdateUserSpiConfig(uint32_t CheckUnlock);
void UserSpiReset();
/* @endcond */

#endif /* INC_USER_SPI_H_ */
