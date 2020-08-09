/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		script.h
  * @date		7/23/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer script module (loaded from SD card)
 **/

#ifndef INC_SCRIPT_H_
#define INC_SCRIPT_H_

#include "main.h"
#include "registers.h"
#include "usb_cli.h"
#include "sd_card.h"

/** Available script commands */
typedef enum
{
	read,
	write,
	delim,
	readbuf,
	stream,
	freset,
	help,
	sleep,
	loop,
	endloop,
	invalid
}command;

typedef struct
{
	/** The command type associated with the script */
	command scrCommand;

	/** Command arguments */
	uint32_t args[3];

	/** Number of command line arguments */
	uint32_t numArgs;

	/** Track if script entry has valid arguments  */
	uint32_t invalidArgs;
}script;

void CheckStream();
void ParseScriptElement(const uint8_t* commandBuf, script * scr);
void RunScriptElement(script* scr, uint8_t * outBuf, bool isUSB);

#endif /* INC_SCRIPT_H_ */
