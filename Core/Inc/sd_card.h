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

#include "script.h"
#include "fatfs.h"
#include "registers.h"
#include "timer.h"

/* Public functions */

/* @cond DOXYGEN_IGNORE */
void SDCardInit();
void ScriptAutorun();
void ScriptStep();
void StartScript();
void StopScript();
void SDTxHandler(const uint8_t* buf, uint32_t count);
/* @endcond */

/** Maximum script file size supported */
#define SCRIPT_MAX_ENTRIES	64

/** Invalid loop index constant (used in parsing) */
#define INVALID_LOOP_INDEX 	0xFFFFFFFF

/* Script error bits */
#define SCR_NO_SD				(1 << 0)
#define SCR_MOUNT_ERROR			(1 << 1)
#define SCR_SCRIPT_OPEN_ERROR	(1 << 2)
#define SCR_RESULT_OPEN_ERROR	(1 << 3)
#define SCR_PARSE_INVALID_CMD	(1 << 4)
#define SCR_PARSE_INVALID_ARGS	(1 << 5)
#define SCR_PARSE_INVALID_LOOP	(1 << 6)
#define SCR_WRITE_FAIL			(1 << 7)

#endif /* INC_SD_CARD_H_ */
