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
#include "sd_card.h"
#include "usb_cli.h"

/** Available script commands */
typedef enum
{
	read,
	write,
	delim,
	echo,
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

/** Buffer output base address (on page 255) */
#define BUF_BASE_ADDR			8

/** Buffer output data base address (on page 255) */
#define BUF_DATA_BASE_ADDR		16

#define STREAM_BUF_SIZE			512

#endif /* INC_SCRIPT_H_ */
