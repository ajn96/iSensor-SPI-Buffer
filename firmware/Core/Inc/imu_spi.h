/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		imu_spi.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation for iSensor-SPI-Buffer IMU interfacing module
 **/

#ifndef INC_IMU_SPI_H_
#define INC_IMU_SPI_H_

/* Includes */
#include "main.h"
#include "registers.h"
#include "timer.h"
#include "stm32f3xx_hal.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
uint16_t ImuSpiTransfer(uint32_t MOSI);
uint16_t ImuReadReg(uint8_t RegAddr);
uint16_t ImuWriteReg(uint8_t RegAddr, uint8_t RegValue);
void UpdateImuSpiConfig();
void EnableImuSpiDMA();
void DisableImuSpiDMA();
void StartImuBurst(uint8_t * bufEntry);
void ResetImu();
void InitImuCsTimer();
void ConfigureImuCsTimer(uint32_t period);
void InitImuSpiTimer();
void ConfigureImuSpiTimer(uint32_t period);
/* @endcond */

#endif
