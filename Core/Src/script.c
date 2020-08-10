/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		script.c
  * @date		7/23/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer script module (loaded from SD card or provided via USB CLI)
 **/

#include "script.h"

/* Private function prototypes */
static uint32_t ParseCommandArgs(const uint8_t* commandBuf, uint32_t* args);
static void ReadHandler(script* scr, uint8_t* outBuf, bool isUSB);
static void ReadBufHandler(bool isUSB);
static void WriteHandler(script* scr);
static void StreamCmdHandler(script * scr, bool isUSB);
static void FactoryResetHandler();
static void UShortToHex(uint8_t* outBuf, uint16_t val);
static uint32_t HexToUInt(const uint8_t* commandBuf);
static uint32_t StringEquals(const uint8_t* string0, const uint8_t* compStr, uint32_t count);

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

/* Buffer for stream data (USB or SD card) */
static uint8_t StreamBuf_A[STREAM_BUF_SIZE];

static uint8_t StreamBuf_B[STREAM_BUF_SIZE];

static bool BufA;

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
static const uint8_t InvalidCmdStr[] = "Error: Invalid command!\r\n";

/** Print string for not allowed command */
static const uint8_t NotAllowedStr[] = "Error: Not allowed!\r\n";

/** Print string for invalid argument */
static const uint8_t InvalidArgStr[] = "Error: Invalid argument!\r\n";

/** Print string for unexpected processing */
static const uint8_t UnknownErrorStr[] = "Unknown error has occurred!\r\n";

/** Print string for help command */
static const uint8_t HelpStr[] = "All numeric values are in hex. [] arguments are optional\r\n"
		"help: Print available commands\r\n"
		"freset: Performs a factory reset, followed by flash update\r\n"
		"read startAddr [endAddr = addr] [numReads = 1]: Read registers starting at startAddr and ending at endAddr numReads times\r\n"
		"write addr value: Write the 8-bit value in value to register at address addr\r\n"
		"readbuf: Read all buffer entries. Values for each entry are placed on a newline\r\n"
		"stream startStop: Start (startStop != 0) or stop (startStop = 0) a read stream\r\n"
		"delim char: Set the read output delimiter character to char\r\n";

void CheckStream()
{
	/* Check water mark interrupt status */
	if(g_regs[BUF_CNT_0_REG] >= (g_regs[WATERMARK_INT_CONFIG_REG] & ~WATERMARK_PULSE_MASK))
	{
		/* If SD stream then handle */
		if(g_regs[CLI_CONFIG_REG] & SD_STREAM_BITM)
		{
			/* Ensure USB stream is cleared */
			g_regs[CLI_CONFIG_REG] &= ~(USB_STREAM_BITM);
			/* Call handler */
			ReadBufHandler(false);
		}
		/* If USB stream then handle */
		if(g_regs[CLI_CONFIG_REG] & USB_STREAM_BITM)
		{
			/* Call handler */
			ReadBufHandler(true);
		}
	}
}

void ParseScriptElement(const uint8_t* commandBuf, script * scr)
{
	scr->numArgs = 0;
	scr->invalidArgs = 0;

	if(StringEquals(commandBuf, ReadCmd, sizeof(ReadCmd) - 1))
	{
		scr->scrCommand = read;
		/* Parse read arguments (1 - 3 arguments possible) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		if(scr->numArgs == 0)
		{
			/* Zero arguments is invalid */
			scr->invalidArgs = 1;
		}
		else if(scr->numArgs == 1)
		{
			/* Sanitize */
			scr->args[1] = scr->args[0];
			scr->args[2] = 1;
		}
		else if(scr->numArgs == 2)
		{
			/* Arg0 (start read addr) must be less than arg1 (end read addr) */
			if(scr->args[0] > scr->args[1])
				scr->invalidArgs = 1;
			scr->args[2] = 1;
		}
		else if(scr->numArgs == 3)
		{
			if(scr->args[0] > scr->args[1])
				scr->invalidArgs = 1;

			/* number of reads can't be 0 */
			if(scr->args[2] == 0)
				scr->invalidArgs = 1;
		}
		return;
	}

	if(StringEquals(commandBuf, WriteCmd, sizeof(WriteCmd) - 1))
	{
		scr->scrCommand = write;
		/* 2 args (write addr, write value) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got two arguments */
		if(scr->numArgs != 2)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, StreamCmd, sizeof(StreamCmd) - 1))
	{
		scr->scrCommand = stream;
		/* 1 arg (stream enable/disable) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got at least 1 argument */
		if(scr->numArgs == 0)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, DelimCmd, sizeof(DelimCmd) - 1))
	{
		scr->scrCommand = delim;
		/* 1 arg (delim char) */
		scr->numArgs = 1;
		scr->args[0] = commandBuf[6];
		return;
	}

	if(StringEquals(commandBuf, SleepCmd, sizeof(SleepCmd) - 1))
	{
		scr->scrCommand = sleep;
		/* 1 arg (sleep time, in ms) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got at least 1 argument */
		if(scr->numArgs == 0)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, LoopCmd, sizeof(LoopCmd) - 1))
	{
		scr->scrCommand = loop;
		/* 1 arg (number of loops) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got at least 1 argument */
		if(scr->numArgs == 0)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, EndloopCmd, sizeof(EndloopCmd) - 1))
	{
		scr->scrCommand = endloop;
		/* No args */
		return;
	}

	if(StringEquals(commandBuf, FactoryResetCmd, sizeof(FactoryResetCmd) - 1))
	{
		scr->scrCommand = freset;
		/* No args */
		return;
	}

	if(StringEquals(commandBuf, HelpCmd, sizeof(HelpCmd) - 1))
	{
		scr->scrCommand = help;
		/* No args */
		return;
	}

	if(StringEquals(commandBuf, ReadBufCmd, sizeof(ReadBufCmd) - 1))
	{
		scr->scrCommand = readbuf;
		/* No args */
		return;
	}

	/* No matching command */
	scr->numArgs = 0;
	scr->scrCommand = invalid;
	return;
}

void RunScriptElement(script* scr, uint8_t * outBuf, bool isUSB)
{
	/* Check that command is valid */
	if(scr->scrCommand >= invalid)
	{
		/* Transmit error and return */
		if(isUSB)
			USBRxHandler(InvalidCmdStr, sizeof(InvalidCmdStr));
		else
			SDTxHandler(InvalidCmdStr, sizeof(InvalidCmdStr));
		return;
	}

	/* Check that args are valid */
	if(scr->invalidArgs != 0)
	{
		/* Transmit error and return */
		if(isUSB)
			USBRxHandler(InvalidArgStr, sizeof(InvalidArgStr));
		else
			SDTxHandler(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}

	/* Squash script elements not handled here (sleep, looping) */
	if(scr->scrCommand > help)
	{
		/* Transmit error and return */
		if(isUSB)
			USBRxHandler(NotAllowedStr, sizeof(NotAllowedStr));
		else
			SDTxHandler(NotAllowedStr, sizeof(NotAllowedStr));
		return;
	}

	/* Call the respective handler */
	switch(scr->scrCommand)
	{
		case read:
			ReadHandler(scr, outBuf, isUSB);
			break;
		case write:
			WriteHandler(scr);
			break;
		case delim:
			/* Clear delim char in USB config */
			g_regs[CLI_CONFIG_REG] &= ~CLI_DELIM_BITM;
			/* Set new value */
			g_regs[CLI_CONFIG_REG] |= ((scr->args[0] & 0xFF) << CLI_DELIM_BITP);
			break;
		case readbuf:
			ReadBufHandler(isUSB);
			break;
		case stream:
			StreamCmdHandler(scr, isUSB);
			break;
		case freset:
			FactoryResetHandler();
			break;
		case help:
			/* Transmit help message */
			if(isUSB)
				USBRxHandler(HelpStr, sizeof(HelpStr));
			else
				SDTxHandler(HelpStr, sizeof(HelpStr));
			break;
		default:
			/* Should not get here. Transmit error and return */
			if(isUSB)
				USBTxHandler(UnknownErrorStr, sizeof(UnknownErrorStr));
			else
				SDTxHandler(UnknownErrorStr, sizeof(UnknownErrorStr));
			break;
	}
}

static void StreamCmdHandler(script * scr, bool isUSB)
{
	/* Set/clear stream interrupt enable flag */
	if(scr->args[0])
	{
		if(isUSB && (!(g_regs[CLI_CONFIG_REG] & SD_STREAM_BITM)))
			g_regs[CLI_CONFIG_REG] |= USB_STREAM_BITM;
		else
		{
			g_regs[CLI_CONFIG_REG] |= SD_STREAM_BITM;
			g_regs[CLI_CONFIG_REG] &= ~(USB_STREAM_BITM);
		}
	}
	else
	{
		if(isUSB)
			g_regs[CLI_CONFIG_REG] &= ~USB_STREAM_BITM;
		else
			g_regs[CLI_CONFIG_REG] &= ~SD_STREAM_BITM;
	}
}

static void FactoryResetHandler()
{
	/* Perform factory reset */
	g_regs[USER_COMMAND_REG] = CMD_FACTORY_RESET;
	ProcessCommand();
	/* Perform flash update */
	g_regs[USER_COMMAND_REG] = CMD_FLASH_UPDATE;
	ProcessCommand();
}

/**
  * @brief Read command handler
  *
  * @return void
  */
static void ReadHandler(script* scr, uint8_t* outBuf, bool isUSB)
{
	/* Pointer within write buffer */
	uint8_t* writeBufPtr;

	/* Register read value */
	uint16_t readVal;

	/* Buffer byte count */
	uint32_t count;

	/* Init buffer variables */
	writeBufPtr = outBuf;
	count = 0;

	/* Perform read */
	for(int i = 0; i < scr->args[2]; i++)
	{
		for(uint32_t addr = scr->args[0]; addr < scr->args[1]; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[CLI_CONFIG_REG] >> CLI_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
			/* Check if transmit needed (not enough space for next loop) */
			if((STREAM_BUF_SIZE - count) < 7)
			{
				if(isUSB)
					USBTxHandler(outBuf, count);
				else
					SDTxHandler(outBuf, count);
				/* Reset pointers */
				writeBufPtr = outBuf;
				count = 0;
			}
		}
		/* Last read doesn't need delim char */
		readVal = ReadReg(scr->args[1]);
		UShortToHex(writeBufPtr, readVal);
		writeBufPtr += 4;
		count += 4;
		/* Add newline */
		writeBufPtr[0] = '\r';
		writeBufPtr[1] = '\n';
		writeBufPtr += 2;
		count += 2;
	}
	/* transmit any remaineder data */
	if(isUSB)
		USBTxHandler(outBuf, count);
	else
		SDTxHandler(outBuf, count);
}

/**
  * @brief Write command handler
  *
  * @return void
  *
  * @param scr pointer to script element containing write arguments
  *
  * The write address is passed in args[0]. The address is
  * masked to only 7 bits (address space of a page). The
  * write value is passed in args[1]. The write value is masked to
  * 8 bits (byte-wise writes).
  */
static void WriteHandler(script* scr)
{
	/* Mask addr to 7 bits, value to 8 bits */
	scr->args[0] &= 0x7F;
	scr->args[1] &= 0xFF;

	/* Perform write */
	WriteReg(scr->args[0], scr->args[1]);
}

static void ReadBufHandler(bool isUSB)
{
	uint32_t numBufs = g_regs[BUF_CNT_0_REG];
	uint32_t bufLastAddr = g_regs[BUF_LEN_REG] + BUF_DATA_BASE_ADDR;
	uint8_t* writeBufPtr;
	uint8_t* activeBuf;
	uint16_t readVal;
	uint32_t count;

	/* Set page to 255 (if not already) */
	if(ReadReg(0) != BUF_READ_PAGE)
	{
		WriteReg(0, BUF_READ_PAGE);
	}

	if(BufA)
	{
		writeBufPtr = StreamBuf_B;
		activeBuf = StreamBuf_B;
		BufA = false;
	}
	else
	{
		writeBufPtr = StreamBuf_A;
		activeBuf = StreamBuf_A;
		BufA = true;
	}
	count = 0;

	for(uint32_t buf = 0; buf < numBufs; buf++)
	{
		BufDequeueToOutputRegs();
		for(uint32_t addr = BUF_BASE_ADDR; addr < bufLastAddr; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[CLI_CONFIG_REG] >> CLI_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
			if((STREAM_BUF_SIZE - count) < 7)
			{
				/* Perform transmit */
				if(isUSB)
					USBTxHandler(activeBuf, count);
				else
					SDTxHandler(activeBuf, count);
				/* Reset pointers (ping/pong for stream) */
				if(BufA)
				{
					writeBufPtr = StreamBuf_B;
					activeBuf = StreamBuf_B;
					BufA = false;
				}
				else
				{
					writeBufPtr = StreamBuf_A;
					activeBuf = StreamBuf_A;
					BufA = true;
				}
				count = 0;
			}
		}
		readVal = ReadReg(bufLastAddr);
		UShortToHex(writeBufPtr, readVal);
		writeBufPtr += 4;
		count += 4;
		/* Insert newline at end */
		writeBufPtr[0] = '\r';
		writeBufPtr[1] = '\n';
		writeBufPtr += 2;
		count += 2;
	}
	/* Transmit any residual data */
	if(isUSB)
		USBTxHandler(activeBuf, count);
	else
		SDTxHandler(activeBuf, count);
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
static uint32_t ParseCommandArgs(const uint8_t* commandBuf, uint32_t* args)
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
static uint32_t HexToUInt(const uint8_t* commandBuf)
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

static uint32_t StringEquals(const uint8_t* string0, const uint8_t* compStr, uint32_t count)
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

