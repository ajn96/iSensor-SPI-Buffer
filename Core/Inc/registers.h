/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer register interfacing module
 **/

#ifndef INC_REGISTERS_H_
#define INC_REGISTERS_H_

/* Includes */
#include "imu_spi.h"
#include "main.h"
#include "buffer.h"
#include "flash.h"
#include "data_capture.h"
#include "user_spi.h"
#include "dfu.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
uint16_t ReadReg(uint8_t regAddr);
uint16_t WriteReg(uint8_t regAddr, uint8_t regValue);
void GetSN();
void GetBuildDate();
void ProcessCommand();
void FactoryReset();
void BufDequeueToOutputRegs();
/* @endcond */

/* Number of registers per page */
#define REG_PER_PAGE				64

/** Number of pages */
#define NUM_REG_PAGES				3

/** Page register (0 for all pages) */
#define PAGE_ID						0

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE				253

/* Non-volatile R/W regs */
#define BUF_CONFIG_REG				0x01
#define BUF_LEN_REG					0x02
#define BUF_MAX_CNT_REG				0x03
#define DIO_INPUT_CONFIG_REG		0x04
#define DIO_OUTPUT_CONFIG_REG		0x05
#define WATERMARK_INT_CONFIG_REG	0x06
#define ERROR_INT_CONFIG_REG		0x07
#define IMU_SPI_CONFIG_REG			0x08
#define USER_SPI_CONFIG_REG			0x09
#define CLI_CONFIG_REG				0x0A
#define USER_COMMAND_REG			0x0B /* Clears automatically */
#define BTN_CONFIG_REG				0x0C
#define USER_SCR_0_REG				0x0D
#define USER_SCR_6_REG				0x13

/* Non-volatile read only regs */
#define FW_REV_REG					0x14
#define ENDURANCE_REG				0x15

/* Volatile general info regs */
#define STATUS_0_REG				0x20
#define BUF_CNT_0_REG				0x21
#define FAULT_CODE_REG				0x22

/* Volatile time stamp and output data regs */
#define UTC_TIMESTAMP_LWR_REG		0x23
#define UTC_TIMESTAMP_UPR_REG		0x24
#define TIMESTAMP_LWR_REG			0x25
#define TIMESTAMP_UPR_REG			0x26
#define TEMP_REG					0x27
#define VDD_REG						0x28

/* Volatile script info regs */
#define SCR_LINE_REG				0x32
#define SCR_ERROR_REG				0x33

/* Non-volatile device info regs */
#define FW_DAY_MONTH_REG			0x38
#define FW_YEAR_REG					0x39
#define DEV_SN_REG					0x3A

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE				254

#define BUF_WRITE_0_REG				0x49
#define FLASH_SIG_DRV_REG			0x7E
#define FLASH_SIG_REG				0x7F

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE				255

#define STATUS_1_REG				0x81
#define BUF_CNT_1_REG				0x82
#define BUF_RETRIEVE_REG			0x83
#define BUF_UTC_TIMESTAMP_REG		0x84
#define BUF_US_TIMESTAMP_REG		0x86
#define BUF_SIG_REG					0x88
#define BUF_DATA_0_REG				0x89

/* Register (non-zero) default values */
#define FW_REV_DEFAULT				0x0112
#define BUF_CONFIG_DEFAULT			0x0000
#define BUF_LEN_DEFAULT				0x0014
#define DIO_INPUT_CONFIG_DEFAULT	0x0011
#define DIO_OUTPUT_CONFIG_DEFAULT	0x8421
#define WATER_INT_CONFIG_DEFAULT	0x0020
#define ERROR_INT_CONFIG_DEFAULT	0xFFFC
#define IMU_SPI_CONFIG_DEFAULT		0x100F
#define USER_SPI_CONFIG_DEFAULT		0x0007
#define CLI_CONFIG_DEFAULT			0x2000
#define BTN_CONFIG_DEFAULT			0x8000
#define FLASH_SIG_DEFAULT			0x9D2A

/* Update flags definitions */
#define DIO_OUTPUT_CONFIG_FLAG		(1 << 0)
#define IMU_SPI_CONFIG_FLAG			(1 << 1)
#define USER_SPI_CONFIG_FLAG		(1 << 2)
#define USER_COMMAND_FLAG			(1 << 3)
#define DIO_INPUT_CONFIG_FLAG		(1 << 4)
#define ENABLE_CAPTURE_FLAG			(1 << 5)
#define DEQUEUE_BUF_FLAG			(1 << 6)
#define DISABLE_CAPTURE_FLAG		(1 << 7)

/* Command register bits */
#define CMD_CLEAR_BUFFER			(1 << 0)
#define CMD_CLEAR_FAULT				(1 << 1)
#define CMD_FACTORY_RESET			(1 << 2)
#define CMD_FLASH_UPDATE			(1 << 3)
#define CMD_PPS_ENABLE				(1 << 4)
#define CMD_PPS_DISABLE				(1 << 5)
#define CMD_START_SCRIPT			(1 << 6)
#define CMD_STOP_SCRIPT				(1 << 7)
#define CMD_WATERMARK_SET			(1 << 8)
#define CMD_BOOTLOADER				(1 << 13)
#define CMD_IMU_RESET				(1 << 14)
#define CMD_SOFTWARE_RESET			(1 << 15)

/* User SPI config register bits */
#define SPI_CONF_CPHA				(1 << 0)
#define SPI_CONF_CPOL				(1 << 1)
#define SPI_CONF_MSB_FIRST			(1 << 2)
#define SPI_CONF_MASK				(SPI_CONF_CPHA|SPI_CONF_CPOL|SPI_CONF_MSB_FIRST)

/* Status register bits */
#define STATUS_BUF_WATERMARK		(1 << 0)
#define STATUS_BUF_FULL				(1 << 1)
#define STATUS_SPI_ERROR			(1 << 2)
#define STATUS_SPI_OVERFLOW			(1 << 3)
#define STATUS_OVERRUN				(1 << 4)
#define STATUS_DMA_ERROR			(1 << 5)
#define STATUS_PPS_UNLOCK			(1 << 6)
#define STATUS_TEMP_WARNING			(1 << 7)
#define STATUS_SCR_ERROR			(1 << 10)
#define STATUS_SCR_RUNNING			(1 << 11)
#define STATUS_FLASH_ERROR			(1 << 12)
#define STATUS_FLASH_UPDATE			(1 << 13)
#define STATUS_FAULT				(1 << 14)
#define STATUS_WATCHDOG				(1 << 15)

/* Status clear mask (defines status bits which are sticky) */
#define STATUS_CLEAR_MASK			(STATUS_FLASH_ERROR|STATUS_FAULT|STATUS_FLASH_UPDATE|STATUS_WATCHDOG|STATUS_SCR_RUNNING)

/* BUF_CONFIG bit definitions */
#define BUF_CFG_REPLACE_OLDEST		(1 << 0)
#define BUF_CFG_IMU_BURST			(1 << 1)
#define BUF_CFG_BUF_BURST			(1 << 2)
#define BUF_CFG_MASK				(BUF_CFG_REPLACE_OLDEST|BUF_CFG_IMU_BURST|BUF_CFG_BUF_BURST)

/* Watermark int config pulse mode mask */
#define WATERMARK_PULSE_MASK 		(1 << 15)

/* CLI_CONFIG register bit positions */
#define USB_STREAM_BITP				0
#define SD_STREAM_BITP				1
#define USB_ECHO_BITP				2
#define SD_AUTORUN_BITP				3
#define CLI_DELIM_BITP				8

#define USB_STREAM_BITM				(1 << USB_STREAM_BITP)
#define SD_STREAM_BITM				(1 << SD_STREAM_BITP)
#define USB_ECHO_BITM				(1 << USB_ECHO_BITP)
#define SD_AUTORUN_BITM				(1 << SD_AUTORUN_BITP)
#define CLI_DELIM_BITM				(0xFF << CLI_DELIM_BITP)

#define CLI_CONFIG_CLEAR_MASK		0xFF0C

#endif /* INC_REGISTERS_H_ */
