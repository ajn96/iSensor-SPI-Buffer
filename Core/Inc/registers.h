/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer register interfacing
 **/

#ifndef INC_REGISTERS_H_
#define INC_REGISTERS_H_

/* Includes */
#include <imu_spi.h>
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
void BufDequeueToOutputRegs();

/* Number of registers per page */
#define REG_PER_PAGE				64

/** Page register (0 for all pages) */
#define PAGE_ID						0

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE				253

#define BUF_CONFIG_REG				0x01
#define BUF_LEN_REG					0x02
#define BUF_MAX_CNT_REG				0x03
#define DIO_INPUT_CONFIG_REG		0x04
#define DIO_OUTPUT_CONFIG_REG		0x05
#define WATERMARK_INT_CONFIG_REG	0x06
#define ERROR_INT_CONFIG_REG		0x07
#define IMU_SPI_CONFIG_REG			0x08
#define USER_SPI_CONFIG_REG			0x09
#define USER_COMMAND_REG			0x0A
#define USER_SCR_0_REG				0x0B
#define USER_SCR_7_REG				0x12
#define FW_REV_REG					0x14
#define ENDURANCE_REG				0x15
#define STATUS_0_REG				0x20
#define BUF_CNT_0_REG				0x21
#define FAULT_CODE_REG				0x22
#define UTC_TIMESTAMP_LWR_REG		0x23
#define UTC_TIMESTAMP_UPR_REG		0x24
#define TIMESTAMP_LWR_REG			0x25
#define TIMESTAMP_UPR_REG			0x26
#define FW_DAY_MONTH_REG			0x38
#define FW_YEAR_REG					0x39
#define DEV_SN_REG					0x3A

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE				254

#define BUF_WRITE_0_REG				0x48
#define FLASH_SIG_DRV_REG			0x7E
#define FLASH_SIG_REG				0x7F

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE				255

#define STATUS_1_REG				0x81
#define BUF_CNT_1_REG				0x82
#define BUF_RETRIEVE_REG			0x83
#define BUF_TIMESTAMP_REG			0x84
#define BUF_DELTA_TIME_REG			0x86
#define BUF_SIG_REG					0x87
#define BUF_DATA_0_REG				0x88

/* Register (non-zero) default values */
#define FW_REV_DEFAULT				0x0100
#define BUF_CONFIG_DEFAULT			0x0200
#define BUF_LEN_DEFAULT				0x0014
#define DIO_INPUT_CONFIG_DEFAULT	0x0011
#define DIO_OUTPUT_CONFIG_DEFAULT	0x8421
#define WATER_INT_CONFIG_DEFAULT	0x0020
#define ERROR_INT_CONFIG_DEFAULT	0xFFFC
#define IMU_SPI_CONFIG_DEFAULT		0x100F
#define USER_SPI_CONFIG_DEFAULT		0x0007
#define FLASH_SIG_DEFAULT			0x9F2A

/* Update flags definitions */
#define DIO_OUTPUT_CONFIG_FLAG		(1 << 0)
#define IMU_SPI_CONFIG_FLAG			(1 << 1)
#define USER_SPI_CONFIG_FLAG		(1 << 2)
#define USER_COMMAND_FLAG			(1 << 3)
#define DIO_INPUT_CONFIG_FLAG		(1 << 4)
#define ENABLE_CAPTURE_FLAG			(1 << 5)
#define DEQUEUE_BUF_FLAG			(1 << 6)

/* Command register bits */
#define CMD_CLEAR_BUFFER			(1 << 0)
#define CMD_CLEAR_FAULT				(1 << 1)
#define CMD_FACTORY_RESET			(1 << 2)
#define CMD_FLASH_UPDATE			(1 << 3)
#define CMD_PPS_ENABLE				(1 << 4)
#define CMD_PPS_DISABLE				(1 << 5)
#define CMD_SOFTWARE_RESET			(1 << 15)

/* User SPI config register bits */
#define SPI_CONF_CPHA				(1 << 0)
#define SPI_CONF_CPOL				(1 << 1)
#define SPI_CONF_MSB_FIRST			(1 << 2)
#define SPI_CONF_BURST_RD			(1 << 15)
#define SPI_CONF_MASK				(SPI_CONF_CPHA|SPI_CONF_CPOL|SPI_CONF_MSB_FIRST|SPI_CONF_BURST_RD)

/* Status register bits */
#define STATUS_BUF_WATERMARK		(1 << 0)
#define STATUS_BUF_FULL				(1 << 1)
#define STATUS_SPI_ERROR			(1 << 2)
#define STATUS_SPI_OVERFLOW			(1 << 3)
#define STATUS_OVERRUN				(1 << 4)
#define STATUS_DMA_ERROR			(1 << 5)
#define STATUS_PPS_UNLOCK			(1 << 6)
#define STATUS_FLASH_ERROR			(1 << 12)
#define STATUS_FLASH_UPDATE			(1 << 13)
#define STATUS_FAULT				(1 << 14)
#define STATUS_WATCHDOG				(1 << 15)

/* Status clear mask (defines status bits which are sticky) */
#define STATUS_CLEAR_MASK			(STATUS_FLASH_ERROR|STATUS_FAULT|STATUS_FLASH_UPDATE|STATUS_WATCHDOG)

#endif /* INC_REGISTERS_H_ */
