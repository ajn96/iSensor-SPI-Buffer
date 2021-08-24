/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		sd_card.h
  * @date		7/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer SD card interfacing and script execution module
 **/

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

/* Header includes require for prototypes */
#include <stdint.h>
#include "stm32f3xx_hal.h"

/* Public functions */

/* @cond DOXYGEN_IGNORE */
void SD_Card_Init();
void SD_Card_Script_Autorun();
void SD_Card_Script_Step();
void SD_Card_Start_Script();
void SD_Card_Stop_Script();
void SD_Card_Tx_Handler(const uint8_t* buf, uint32_t count);
/* @endcond */

/** Maximum script file size supported */
#define SCRIPT_MAX_ENTRIES	64

/** Invalid loop index constant (used in parsing) */
#define INVALID_LOOP_INDEX 	0xFFFFFFFF

/* Script error bits */

/** No SD card detected */
#define SCR_NO_SD				(1 << 0)

/** FAT drive mount failure */
#define SCR_MOUNT_ERROR			(1 << 1)

/** script.txt does not exist failed to open for read */
#define SCR_SCRIPT_OPEN_ERROR	(1 << 2)

/** result.txt failed to open for write */
#define SCR_RESULT_OPEN_ERROR	(1 << 3)

/** Invalid command provided in script.txt */
#define SCR_PARSE_INVALID_CMD	(1 << 4)

/** Valid command with bad arguments provided in script.txt */
#define SCR_PARSE_INVALID_ARGS	(1 << 5)

/** Invalid loop parameters */
#define SCR_PARSE_INVALID_LOOP	(1 << 6)

/** SD card write failure occurred during script run */
#define SCR_WRITE_FAIL			(1 << 7)

/* Public variables exported from module */
extern SPI_HandleTypeDef g_spi3;

#endif /* INC_SD_CARD_H_ */
