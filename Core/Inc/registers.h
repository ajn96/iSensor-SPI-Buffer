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

/* Includes */
#include "spi_passthrough.h"
#include "main.h"
#include "buffer.h"
#include "flash.h"
#include "data_capture.h"
#include "burst.h"

/* Public function prototypes */
uint16_t ReadReg(uint8_t regAddr);
uint16_t WriteReg(uint8_t regAddr, uint8_t regValue);
void GetSN();
void GetBuildDate();
void ProcessCommand();
void FactoryReset();
void UpdateUserSpiConfig();

/* Number of registers per page */
#define REG_PER_PAGE				64

/** Page register (0 for all pages) */
#define PAGE_ID						0

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE				253

#define BUF_CONFIG_REG				0x01
#define BUF_LEN_REG					0x02
#define BUF_MAX_CNT_REG				0x03
#define DR_CONFIG_REG				0x04
#define DIO_CONFIG_REG				0x05
#define INT_CONFIG_REG				0x06
#define IMU_SPI_CONFIG_REG			0x07
#define USER_SPI_CONFIG_REG			0x08
#define USER_COMMAND_REG			0x09
#define USER_SCR_0_REG				0x0A
#define USER_SCR_1_REG				0x0B
#define USER_SCR_2_REG				0x0C
#define USER_SCR_3_REG				0x0D
#define FW_REV_REG					0x34
#define ENDURANCE_REG				0x35
#define STATUS_0_REG				0x36
#define BUF_CNT_0_REG				0x37
#define FW_DAY_MONTH_REG			0x38
#define FW_YEAR_REG					0x39
#define DEV_SN_REG					0x3A

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE				254

#define BUF_WRITE_0_REG				0x44
#define FLASH_SIG_REG				0x7F

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE				255

#define STATUS_1_REG				0x81
#define BUF_CNT_1_REG				0x82
#define BUF_RETRIEVE_REG			0x83
#define BUF_DATA_0_REG				0x84

/* Register (non-zero) default values */
#define FW_REV_DEFAULT				0x0100
#define BUF_CONFIG_DEFAULT			0x0200
#define BUF_LEN_DEFAULT				0x0014
#define DR_CONFIG_DEFAULT			0x0011
#define DIO_CONFIG_DEFAULT			0x0843
#define INT_CONFIG_DEFAULT			0x0020
#define IMU_SPI_CONFIG_DEFAULT		0x2014
#define USER_SPI_CONFIG_DEFAULT		0x0007
#define FLASH_SIG_DEFAULT			0x2d9e

/* Update flags definitions */
#define DIO_CONFIG_FLAG				(1 << 0)
#define IMU_SPI_CONFIG_FLAG			(1 << 1)
#define USER_SPI_CONFIG_FLAG		(1 << 2)
#define USER_COMMAND_FLAG			(1 << 3)
#define DR_CONFIG_FLAG				(1 << 4)
#define DISABLE_CAPTURE_FLAG		(1 << 5)
#define ENABLE_CAPTURE_FLAG			(1 << 6)

/* Command register bits */
#define CLEAR_BUFFER				(1 << 0)
#define FACTORY_RESET				(1 << 1)
#define FLASH_UPDATE				(1 << 3)
#define SOFTWARE_RESET				(1 << 15)

/* User SPI config register bits */
#define SPI_CONF_CPHA				(1 << 0)
#define SPI_CONF_CPOL				(1 << 1)
#define SPI_CONF_MSB_FIRST			(1 << 2)
#define SPI_CONF_BURST_RD			(1 << 15)
#define SPI_CONF_MASK				(SPI_CONF_CPHA|SPI_CONF_CPOL|SPI_CONF_MSB_FIRST|SPI_CONF_BURST_RD)

/* Status register bits */
#define STATUS_SPI_ERROR			(1 << 0)
#define STATUS_SPI_OVERFLOW			(1 << 1)
#define STATUS_OVERRUN				(1 << 2)
#define STATUS_DMA_ERROR			(1 << 3)
#define STATUS_FLASH_ERROR			(1 << 6)
#define STATUS_FLASH_UPDATE			(1 << 7)
#define STATUS_FAULT				(1 << 8)
#define STATUS_WATCHDOG				(1 << 9)
#define STATUS_BUF_FULL				(1 << 10)
#define STATUS_BUF_INT				(1 << 11)
#define STATUS_CLEAR_MASK			(STATUS_FLASH_ERROR|STATUS_FAULT|STATUS_FLASH_UPDATE|STATUS_WATCHDOG)

#endif /* INC_REGISTERS_H_ */
