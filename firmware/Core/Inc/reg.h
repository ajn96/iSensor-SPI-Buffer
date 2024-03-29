/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		reg.h
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer register interfacing module
 **/

#ifndef INC_REG_H_
#define INC_REG_H_

/* Header includes require for prototypes */
#include <stdint.h>
#include "main.h"

/* Number of registers per page */
#define REG_PER_PAGE				64

/** Number of pages */
#define NUM_REG_PAGES				4

/** Page register (0 for all pages) */
#define PAGE_ID						0

/** Output page used for storing volatile data */
#define OUTPUT_PAGE					252

/** iSensor-SPI-Buffer config settings page */
#define BUF_CONFIG_PAGE				253

/* Non-volatile R/W regs */
#define BUF_CONFIG_REG				0x41
#define BUF_LEN_REG					0x42
#define BTN_CONFIG_REG				0x43
#define DIO_INPUT_CONFIG_REG		0x44
#define DIO_OUTPUT_CONFIG_REG		0x45
#define WATERMARK_INT_CONFIG_REG	0x46
#define ERROR_INT_CONFIG_REG		0x47
#define IMU_SPI_CONFIG_REG			0x48
#define USER_SPI_CONFIG_REG			0x49
#define CLI_CONFIG_REG				0x4A
#define USER_COMMAND_REG			0x4B /* Clears automatically */
#define SYNC_FREQ_REG				0x4C
/* Space for 12 more regs here */
#define USER_SCR_0_REG				0x5A
#define USER_SCR_3_REG				0x5D
#define UTC_TIMESTAMP_LWR_REG		0x5E
#define UTC_TIMESTAMP_UPR_REG		0x5F

/* All page 253 regs after this are read only */

/* Volatile general info regs */
#define STATUS_0_REG				0x60
#define FAULT_CODE_REG				0x61
#define BUF_CNT_0_REG				0x62
#define BUF_MAX_CNT_REG				0x63

/* Volatile time stamp and output data regs */
#define TIMESTAMP_LWR_REG			0x65
#define TIMESTAMP_UPR_REG			0x66
#define TEMP_REG					0x67
#define VDD_REG						0x68

/* Volatile script info regs */
#define SCR_LINE_REG				0x72
#define SCR_ERROR_REG				0x73

/* Non-volatile device info regs */
#define ENDURANCE_REG				0x76
#define FW_REV_REG					0x77
#define FW_DAY_MONTH_REG			0x78
#define FW_YEAR_REG					0x79
#define DEV_SN_REG					0x7A

/** iSensor-SPI-Buffer buffer Tx data page */
#define BUF_WRITE_PAGE				254

#define BUF_WRITE_0_REG				0x89
#define FLASH_SIG_DRV_REG			0xBE
#define FLASH_SIG_REG				0xBF

/** iSensor-SPI-Buffer buffer Rx data page */
#define BUF_READ_PAGE				255

#define STATUS_1_REG				0xC1
#define BUF_CNT_1_REG				0xC2
#define BUF_RETRIEVE_REG			0xC3
#define BUF_UTC_TIMESTAMP_REG		0xC4
#define BUF_US_TIMESTAMP_REG		0xC6
#define BUF_SIG_REG					0xC8
#define BUF_DATA_0_REG				0xC9

/* Register (non-zero) default values */
#define FW_REV_DEFAULT				0x0115
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
#define SYNC_FREQ_DEFAULT			2000
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
#define CMD_SYNC_GEN				(1 << 9)
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

/* Public variables exported from module */
extern volatile uint32_t g_update_flags;
extern volatile uint16_t g_regs[NUM_REG_PAGES * REG_PER_PAGE];
extern volatile uint16_t* g_CurrentBufEntry;

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void Reg_Init();
void Reg_Update_Identifiers();
bool Reg_Is_Burst_Read(uint8_t addr);
uint16_t Reg_Read(uint8_t regAddr);
uint16_t Reg_Write(uint8_t regAddr, uint8_t regValue);
void Reg_Process_Command();
void Reg_Factory_Reset();
void Reg_Buf_Dequeue_To_Outputs();
void Reg_Button_Handler();
/* @endcond */

#endif /* INC_REG_H_ */
