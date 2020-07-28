/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		sd_card.c
  * @date		7/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer SD card interfacing and script execution module
 **/

#include "sd_card.h"

/* Command list. Loaded from cmd.txt on the SD card */
static script cmdList[SCRIPT_MAX_ENTRIES];

/** Track index within the command list currently being executed */
static uint32_t cmdIndex;

/**
  * @brief Turn on a selected LED
  *
  * @return void
  */
uint32_t IsSDCardAttached()
{

}


/**
  * @brief Init SD card interface
  *
  * @return void
  */
void SDCardInit()
{

}

/**
  * @brief Start script run.
  *
  * @return void
  */
void StartScript()
{

}

void StopScript()
{

}

/**
  * @brief Turn on a selected LED
  *
  * @return void
  */
void ScriptStep()
{

}
