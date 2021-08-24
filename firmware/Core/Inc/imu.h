/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		imu.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation for iSensor-SPI-Buffer IMU interfacing module
 **/

#ifndef INC_IMU_H_
#define INC_IMU_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void IMU_SPI_Init();
uint16_t IMU_SPI_Transfer(uint32_t MOSI);
uint16_t IMU_Read_Register(uint8_t RegAddr);
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue);
void IMU_Update_SPI_Config();
void EnableImuSpiDMA();
void IMU_Disable_SPI_DMA();
void IMU_Start_Burst(uint8_t * bufEntry);
void IMU_Reset();
/* @endcond */

#endif
