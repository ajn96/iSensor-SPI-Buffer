/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer register interfacing
 **/

#ifndef INC_REGISTERS_H_
#define INC_REGISTERS_H_

#include <spi_passthrough.h>
#include "main.h"
#include "buffer.h"
#include "flash.h"
#include "data_capture.h"

uint16_t ReadReg(uint8_t regAddr);
uint16_t WriteReg(uint8_t regAddr, uint8_t regValue);
uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue);

void GetSN();
void GetBuildDate();

void UpdateUserSpiConfig();
void ProcessCommand();

/* Number of registers per page */
#define REG_PER_PAGE			64

/** Page register (0 for all pages) */
#define PAGE_ID					0

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE			253

#define BUF_CONFIG_REG			0x01
#define BUF_LEN_REG				0x02
#define BUF_MAX_CNT_REG			0x03
#define DIO_CONFIG_REG			0x04
#define IMU_SPI_CONFIG_REG		0x05
#define USER_SPI_CONFIG_REG		0x06
#define USER_COMMAND_REG		0x07
#define USER_SCR_0_REG			0x08
#define USER_SCR_1_REG			0x09
#define USER_SCR_2_REG			0x0A
#define USER_SCR_3_REG			0x0B
#define STATUS_REG				0x36
#define FW_DAY_MONTH_REG		0x37
#define FW_YEAR_REG				0x38
#define FW_REV_REG				0x39
#define DEV_SN_REG				0x3A

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE			254

#define BUF_WRITE_0_REG			0x43
#define FLASH_SIG				0x7F

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE			255

#define BUF_CNT_REG				0x81
#define BUF_RETRIEVE_REG		0x82
#define BUF_DATA_0_REG			0x83

/* Update flags definitions */
#define DIO_CONFIG_FLAG			(1 << 0)
#define IMU_SPI_CONFIG_FLAG		(1 << 1)
#define USER_SPI_CONFIG_FLAG	(1 << 2)
#define USER_COMMAND_FLAG		(1 << 3)

/* Command register bits */
#define CLEAR_BUFFER			(1 << 0)
#define FACTORY_RESET			(1 << 1)
#define FLASH_UPDATE			(1 << 3)
#define SOFTWARE_RESET			(1 << 15)

#endif /* INC_REGISTERS_H_ */
