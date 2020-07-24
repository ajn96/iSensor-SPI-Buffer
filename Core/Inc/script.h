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
	endloop
}command;

typedef struct
{
	/** The command type associated with the script */
	command scrCommand;

	/** Command arguments */
	uint32_t args[3];

	/** Number of command line arguments */
	uint32_t numArgs;
}script;

script ParseScriptElement(uint8_t* buf);
void RunScriptElement(script* scriptElement);

#endif /* INC_SCRIPT_H_ */
