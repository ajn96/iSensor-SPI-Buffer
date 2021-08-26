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

/* Header includes require for prototypes */
#include "stdint.h"
#include "main.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void User_SPI_Burst_Setup();
void User_SPI_Burst_Disable();
void User_SPI_Update_Config(uint32_t CheckUnlock);
void User_SPI_Reset(bool register_mode);
/* @endcond */

/* Public variables exported from module */
extern volatile uint32_t g_userburstRunning;
extern volatile uint32_t g_user_burst_start;

#endif /* INC_USER_SPI_H_ */
