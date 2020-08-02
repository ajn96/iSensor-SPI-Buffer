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
void SDCardInit();
void ScriptAutorun();
void ScriptStep();
void StartScript();
void StopScript();

#define SCRIPT_MAX_ENTRIES	64

#endif /* INC_SD_CARD_H_ */
