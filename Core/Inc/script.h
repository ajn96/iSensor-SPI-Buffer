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
#include "cli.h"

/** Available script commands */
typedef enum
{
	read,
	write,
	delim,
	readbuf,
	stream,
	sleep,
	loop,
	endloop,
	freset,
	help,
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

script ParseScriptElement(uint8_t* buf);
uint32_t RunScriptElement(script* scr, uint8_t * outBuf);

#endif /* INC_SCRIPT_H_ */
