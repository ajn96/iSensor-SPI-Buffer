/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		REG_DEFINES.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer registers
 **/

#ifndef INC_REG_DEFINES_H_
#define INC_REG_DEFINES_H_

/** Page register (0 for all pages) */
#define PAGE_ID					0

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE			253

#define BUF_CONFIG_REG			0x02
#define BUF_LEN_REG				0x04
#define IMU_DR_CONFIG_REG		0x06
#define IMU_SPI_STALL_REG		0x08
#define IMU_SPI_SCLK_REG		0x0A
#define USER_SPI_CONFIG_REG		0x0C
#define USER_DR_CONFIG_REG		0x0E
#define USER_COMMAND_REG		0x10

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE			254

#define BUF_WRITE_0_REG			0x06

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE			255

#define BUF_CNT_REG				0x02
#define BUF_RETRIEVE_REG		0x04
#define BUF_DATA_0_REG			0x06

#endif /* INC_REG_DEFINES_H_ */
