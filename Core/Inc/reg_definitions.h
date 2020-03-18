/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		reg_definitions.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer registers
 **/

#ifndef INC_REG_DEFINITIONS_H_
#define INC_REG_DEFINITIONS_H_

/* Number of registers per page */
#define REG_PER_PAGE			64

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
#define USER_SCR_0_REG			0x12
#define USER_SCR_1_REG			0x14
#define USER_SCR_2_REG			0x16
#define USER_SCR_3_REG			0x18

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE			254

#define BUF_WRITE_0_REG			0x46

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE			255

#define BUF_CNT_REG				0x82
#define BUF_RETRIEVE_REG		0x84
#define BUF_DATA_0_REG			0x86

#endif /* INC_REG_DEFINITIONS_H_ */
