/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		script.c
  * @date		7/23/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer script module (loaded from SD card)
 **/

#include "script.h"

/* Private function prototypes */
static uint32_t ParseCommandArgs(uint8_t* commandBuf, uint32_t* args);
static void UShortToHex(uint8_t* outBuf, uint16_t val);
static uint32_t HexToUInt(uint8_t* commandBuf);
static uint32_t ReadHandler(script* scr, uint8_t* outBuf);
static void WriteHandler(script* scr);
static uint32_t ReadBufHandler(script* scr, uint8_t* outBuf);
static uint32_t SleepHandler(script* scr, uint8_t* outBuf);
static uint32_t LoopHandler(script* scr, uint8_t* outBuf);
static uint32_t EndLoopHandler(script* scr, uint8_t* outBuf);
static uint32_t StringEquals(uint8_t* string0, const uint8_t* compStr, uint32_t count);

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

/* Current index within command buffer */
static uint32_t cmdIndex;

/** Flag to track if current command arguments are valid */
static uint32_t goodArg;

/** String literal for read command. Must be followed by a space */
static const uint8_t ReadCmd[] = "read ";

/** String literal for write command. Must be followed by a space */
static const uint8_t WriteCmd[] = "write ";

/** String literal for help command */
static const uint8_t HelpCmd[] = "help";

/** String literal for factory reset command */
static const uint8_t FactoryResetCmd[] = "freset";

/** String literal for read buffer command */
static const uint8_t ReadBufCmd[] = "readbuf";

/** String literal for endloop command. */
static const uint8_t EndloopCmd[] = "endloop";

/** String literal for stream command. Must be followed by a space */
static const uint8_t StreamCmd[] = "stream ";

/** String literal for delim set command. Must be followed by a space */
static const uint8_t DelimCmd[] = "delim ";

/** String literal for sleep command. Must be followed by a space */
static const uint8_t SleepCmd[] = "sleep ";

/** String literal for loop command. Must be followed by a space */
static const uint8_t LoopCmd[] = "loop ";

/** Print string for invalid command */
static uint8_t InvalidCmdStr[] = "Error: Invalid command!\r\n";

/** Print string for invalid argument */
static uint8_t InvalidArgStr[] = "Error: Invalid argument!\r\n";

/** Print string for help command */
static uint8_t HelpStr[] = "All numeric values are in hex. [] arguments are optional\r\n"
		"help: Print available commands\r\n"
		"reset: Performs a software reset\r\n"
		"read startAddr [endAddr = addr] [numReads = 1]: Read registers starting at startAddr and ending at endAddr numReads times\r\n"
		"write addr value: Write the 8-bit value in value to register at address addr\r\n"
		"readbuf: Read all buffer entries. Values for each entry are placed on a newline\r\n"
		"stream startStop: Start (startStop != 0) or stop (startStop = 0) a read stream\r\n"
		"delim char: Set the read output delimiter character to char\r\n";

script ParseScriptElement(uint8_t* commandBuf)
{
	script scr = {};
	scr.numArgs = 0;
	scr.invalidArgs = 0;

	if(StringEquals(commandBuf, ReadCmd, sizeof(ReadCmd) - 1))
	{
		scr.scrCommand = read;
		/* Parse read arguments (1 - 3 arguments possible) */
		scr.numArgs = ParseCommandArgs(commandBuf, scr.args);
		if(scr.numArgs == 0)
		{
			/* Zero arguments is invalid */
			scr.invalidArgs = 1;
		}
		else if(scr.numArgs == 2)
		{
			/* Arg0 (start read addr) must be less than arg1 (end read addr) */
			if(scr.args[0] > scr.args[1])
				scr.invalidArgs = 1;
		}
		else if(scr.numArgs == 3)
		{
			if(scr.args[0] > scr.args[1])
				scr.invalidArgs = 1;

			/* number of reads can't be 0 */
			if(scr.args[2] == 0)
				scr.invalidArgs = 1;
		}
		return scr;
	}

	if(StringEquals(commandBuf, WriteCmd, sizeof(WriteCmd) - 1))
	{
		scr.scrCommand = write;
		/* 2 args (write addr, write value) */
		scr.numArgs = ParseCommandArgs(commandBuf, scr.args);
		/* Check that we got two arguments */
		if(scr.numArgs != 2)
			scr.invalidArgs = 1;
		return scr;
	}

	if(StringEquals(commandBuf, StreamCmd, sizeof(StreamCmd) - 1))
	{
		scr.scrCommand = stream;
		/* 1 arg (stream enable/disable) */
		scr.numArgs = ParseCommandArgs(commandBuf, scr.args);
		/* Check that we got at least 1 argument */
		if(scr.numArgs == 0)
			scr.invalidArgs = 1;
		return scr;
	}

	if(StringEquals(commandBuf, DelimCmd, sizeof(DelimCmd) - 1))
	{
		scr.scrCommand = delim;
		/* 1 arg (delim char) */
		scr.numArgs = 1;
		scr.args[0] = commandBuf[6];
		return scr;
	}

	if(StringEquals(commandBuf, SleepCmd, sizeof(SleepCmd) - 1))
	{
		scr.scrCommand = sleep;
		/* 1 arg (sleep time, in ms) */
		scr.numArgs = ParseCommandArgs(commandBuf, scr.args);
		/* Check that we got at least 1 argument */
		if(scr.numArgs == 0)
			scr.invalidArgs = 1;
		return scr;
	}

	if(StringEquals(commandBuf, LoopCmd, sizeof(LoopCmd) - 1))
	{
		scr.scrCommand = loop;
		/* 1 arg (number of loops) */
		scr.numArgs = ParseCommandArgs(commandBuf, scr.args);
		/* Check that we got at least 1 argument */
		if(scr.numArgs == 0)
			scr.invalidArgs = 1;
		return scr;
	}

	if(StringEquals(commandBuf, EndloopCmd, sizeof(EndloopCmd) - 1))
	{
		scr.scrCommand = endloop;
		/* No args */
		return scr;
	}

	if(StringEquals(commandBuf, FactoryResetCmd, sizeof(FactoryResetCmd) - 1))
	{
		scr.scrCommand = freset;
		/* No args */
		return scr;
	}

	if(StringEquals(commandBuf, HelpCmd, sizeof(HelpCmd) - 1))
	{
		scr.scrCommand = help;
		/* No args */
		return scr;
	}

	if(StringEquals(commandBuf, ReadBufCmd, sizeof(ReadBufCmd) - 1))
	{
		scr.scrCommand = readbuf;
		/* No args */
		return scr;
	}

	/* No matching command */
	scr.numArgs = 0;
	scr.scrCommand = invalid;
	return scr;
}

uint32_t RunScriptElement(script* scr, uint8_t * outBuf)
{
	uint32_t count = 0;

	/* Check that command is valid */
	if(scr->scrCommand >= invalid)
	{
		for(int i = 0; i < sizeof(InvalidCmdStr); i++)
		{
			outBuf[i] = InvalidCmdStr[i];
			count++;
		}
		return count;
	}

	/* Check that args are valid */
	if(scr->invalidArgs != 0)
	{
		for(int i = 0; i < sizeof(InvalidArgStr); i++)
		{
			outBuf[i] = InvalidArgStr[i];
			count++;
		}
		return count;
	}

	/* Call the respective handler */
	switch(scr->scrCommand)
	{
		case read:
			count = ReadHandler(scr, outBuf);
			break;
		case write:
			/* Set count to 0 */
			count = 0;
			/* Call write handler */
			WriteHandler(scr);
			break;
		case delim:
			/* Set count to 0 */
			count = 0;
			/* Clear delim char in USB config */
			g_regs[USB_CONFIG_REG] &= ~USB_DELIM_BITM;
			/* Set new value */
			g_regs[USB_CONFIG_REG] |= ((scr->args[0] & 0xFF) << USB_DELIM_BITP);
			break;
		case readbuf:
			count = ReadBufHandler(scr, outBuf);
			break;
		case stream:
			/* Set output count to 0 */
			count = 0;
			/* Set/clear stream interrupt enable flag */
			if(scr->args[0])
			{
				g_regs[USB_CONFIG_REG] |= USB_STREAM_BITM;
			}
			else
			{
				g_regs[USB_CONFIG_REG] &= ~USB_STREAM_BITM;
			}
			break;
		case sleep:
			count = SleepHandler(scr, outBuf);
			break;
		case loop:
			count = LoopHandler(scr, outBuf);
			break;
		case endloop:
			count = EndLoopHandler(scr, outBuf);
			break;
		case freset:
			/* Set output count to 0 */
			count = 0;
			/* Perform factory reset */
			g_regs[USER_COMMAND_REG] = CMD_FACTORY_RESET;
			ProcessCommand();
			/* Perform flash update */
			g_regs[USER_COMMAND_REG] = CMD_FLASH_UPDATE;
			ProcessCommand();
			break;
		case help:
			/* Set output count to 0 */
			count = 0;
			for(int i = 0; i < sizeof(HelpStr); i++)
			{
				outBuf[i] = HelpStr[i];
				count++;
			}
			break;
		default:
			count = 0;
	}
	return count;
}

static uint32_t ReadHandler(script* scr, uint8_t* outBuf)
{
	uint32_t count = 0;
	return count;
}

static void WriteHandler(script* scr)
{
	/* Mask addr to 7 bits, value to 8 bits */
	scr->args[0] &= 0x7F;
	scr->args[1] &= 0xFF;

	/* Perform write */
	WriteReg(scr->args[0], scr->args[1]);
}

static uint32_t ReadBufHandler(script* scr, uint8_t* outBuf)
{
	uint32_t count = 0;
	return count;
}

static uint32_t SleepHandler(script* scr, uint8_t* outBuf)
{
	uint32_t count = 0;
	return count;
}

static uint32_t LoopHandler(script* scr, uint8_t* outBuf)
{
	uint32_t count = 0;
	return count;
}

static uint32_t EndLoopHandler(script* scr, uint8_t* outBuf)
{
	uint32_t count = 0;
	return count;
}

/**
  * @brief Parse space delimited arguments out from a command
  *
  * @param commandBuf Pointer to command string
  *
  * @return void
  *
  * Arguments must be separated by a space. Each argument
  * (up to 3) are placed into the args array. The total number
  * of arguments in the current command is placed in numArgs.
  * All arguments must be hex strings.
  */
static uint32_t ParseCommandArgs(uint8_t* commandBuf, uint32_t* args)
{
	/* Set number of args to 0*/
	uint32_t numArgs = 0;

	/* Run through command until we hit a space */
	while(commandBuf[cmdIndex] != ' ')
	{
		cmdIndex++;
	}
	/* Move to first char */
	cmdIndex++;
	/* Parse */
	args[0] = HexToUInt(commandBuf);
	if(!goodArg)
	{
		return numArgs;
	}
	numArgs++;
	/* Parse second arg */
	args[1] = HexToUInt(commandBuf);
	if(!goodArg)
	{
		return numArgs;
	}
	numArgs++;
	/* Parse third arg */
	args[2] = HexToUInt(commandBuf);
	if(!goodArg)
	{
		return numArgs;
	}
	numArgs++;
	return numArgs;
}

/**
  * @brief Convert a hex string to a 32 bit uint
  *
  * @return void
  *
  * The input string must be stored in CurrentCommand, with
  * cmdIndex set to point at the first value in the string.
  */
static uint32_t HexToUInt(uint8_t* commandBuf)
{
	uint8_t currentByte;
	uint32_t value = 0;
	uint32_t stringDone = 0;

	/* Set goodArg flag to true */
	goodArg = 1;

	/* Get first byte */
	currentByte = commandBuf[cmdIndex];
	stringDone = ((currentByte == ' ')||(currentByte == 0));

	/* Not good arg is 0 length */
	if(stringDone)
	{
		goodArg = 0;
		return 0;
	}

	while(!stringDone)
	{
        if(currentByte >= '0' && currentByte <= '9')
        	currentByte = currentByte - '0';
        else if (currentByte >= 'a' && currentByte <='f')
        	currentByte = currentByte - 'a' + 10;
        else if (currentByte >= 'A' && currentByte <='F')
        	currentByte = currentByte - 'A' + 10;
        else
        {
        	/* Bad char */
        	goodArg = 0;
        	return 0;
        }
        /* Add current 4 bit value */
        value = (value << 4) | (currentByte & 0xF);

        /* Increment index and get new byte */
        cmdIndex++;
		currentByte = commandBuf[cmdIndex];
		stringDone = ((currentByte == ' ')||(currentByte == 0));
	}
	cmdIndex++;
	return value;
}

/**
  * @brief Convert a 16 bit value to the corresponding hex string
  *
  * @param outBuf Buffer to place the string result in
  *
  * @param val The 16 bit value to convert to a string
  *
  * @return void
  */
static void UShortToHex(uint8_t * outBuf, uint16_t val)
{
	outBuf[0] = val >> 12;
	if(outBuf[0] < 0xA)
		outBuf[0] += '0';
	else
		outBuf[0] += ('A' - 10);
	outBuf[1] = (val >> 8) & 0xF;
	if(outBuf[1] < 0xA)
		outBuf[1] += '0';
	else
		outBuf[1] += ('A' - 10);
	outBuf[2] = (val >> 4) & 0xF;
	if(outBuf[2] < 0xA)
		outBuf[2] += '0';
	else
		outBuf[2] += ('A' - 10);
	outBuf[3] = val & 0xF;
	if(outBuf[3] < 0xA)
		outBuf[3] += '0';
	else
		outBuf[3] += ('A' - 10);
}

static uint32_t StringEquals(uint8_t* string0, const uint8_t* compStr, uint32_t count)
{
	for(int i = 0; i < count; i++)
	{
		if(string0[i] != compStr[i])
		{
			return 0;
		}
	}
	return 1;
}

