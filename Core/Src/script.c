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
static void RegAliasReadHandler(uint8_t* outBuf, bool isUSB, uint16_t regIndex);
static void WriteHandler(script* scr);
static void StreamCmdHandler(script * scr, bool isUSB);
static void AboutHandler(uint8_t* outBuf, bool isUSB);
static void UptimeHandler(uint8_t* outBuf, bool isUSB);
static void FactoryResetHandler();
static void UShortToHex(uint8_t* outBuf, uint16_t val);
static uint32_t HexToUInt(const uint8_t* commandBuf);
static uint32_t StringEquals(const uint8_t* string0, const uint8_t* string1, uint32_t count);

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

/** Register update flags (from registers.c) */
extern volatile uint16_t g_update_flags;

/** Buffer A for stream data (USB or SD card) ping/pong */
static uint8_t StreamBuf_A[STREAM_BUF_SIZE];

/** Buffer B for stream data (USB or SD card) ping/pong */
static uint8_t StreamBuf_B[STREAM_BUF_SIZE];

/** Track which buffer is in use */
static bool BufA;

/** Current index within command buffer */
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

/** String literal for status command. */
static const uint8_t StatusCmd[] = "status";

/** String literal for about command. */
static const uint8_t AboutCmd[] = "about";

/** String literal for uptime command. */
static const uint8_t UptimeCmd[] = "uptime";

/** String literal for stream command. Must be followed by a space */
static const uint8_t StreamCmd[] = "stream ";

/** String literal for delim set command. Must be followed by a space */
static const uint8_t DelimCmd[] = "delim ";

/** String literal for echo enable/disable command. Must be followed by a space */
static const uint8_t EchoCmd[] = "echo ";

/** String literal for sleep command. Must be followed by a space */
static const uint8_t SleepCmd[] = "sleep ";

/** String literal for loop command. Must be followed by a space */
static const uint8_t LoopCmd[] = "loop ";

/** String literal for command run command. Must be followed by a space */
static const uint8_t CommandCmd[] = "cmd ";

/** Print string for invalid command */
static const uint8_t InvalidCmdStr[] = "Error: Invalid command! Type help for list of valid commands\r\n";

/** Print string for not allowed command */
static const uint8_t NotAllowedStr[] = "Error: Command not allowed! Type help for list of valid commands\r\n";

/** Print string for invalid argument */
static const uint8_t InvalidArgStr[] = "Error: Invalid argument!\r\n";

/** Print string for unexpected processing */
static const uint8_t UnknownErrorStr[] = "An unknown error has occurred!\r\n";

/** Print string for help command */
static const uint8_t HelpStr[] = "\r\n"
		"All numeric argument values must be provided in hex. [] arguments are optional\r\n"
		"\r\n"
		"help\r\n"
		"   Lists all available CLI commands\r\n"
		"about\r\n"
		"   Print iSensor-SPI-Buffer firmware identification info\r\n"
		"status\r\n"
		"   Read iSensor-SPI-Buffer STATUS register value. Does not change selected page\r\n"
		"uptime\r\n"
		"   Print the system uptime, in ms. Value is decimal formatted\r\n"
		"\r\n"
		"read <startAddr> [endAddr = startAddr] [numReads = 1]\r\n"
		"   Read registers starting at <startAddr> and ending at <endAddr>, <numReads> times\r\n"
		"write <addr> <value>\r\n"
		"   Writes the 8-bit <value> to the register at address <addr>\r\n"
		"readbuf\r\n"
		"   Reads all stored buffer entries. Values for each buffer entry are each placed on a new line\r\n"
		"stream <startStop>\r\n"
		"   Stops the buffered read stream if <startStop> is zero, otherwise the stream is enabled\r\n"
		"\r\n"
		"cmd <cmdValue>\r\n"
		"   Writes the 16-bit <cmdValue> to the iSensor-SPI-Buffer COMMAND register. Does not change the selected register page\r\n"
		"delim <delimChar>\r\n"
		"   Set the read output delimiter character (between register values) to <delimChar>\r\n"
		"echo <enableDisable>\r\n"
		"   Disables USB command line echo if <enableDisable> is zero, otherwise echo is enabled\r\n"
		"freset\r\n"
		"   Performs a factory reset, followed by flash update. This restores the firmware to a known good state\r\n";

/**
  * @brief Check the stream status
  *
  * This function checks if a watermark interrupt is
  * asserted, based on a minimum watermark level of
  * 1. If a watermark is asserted, and a stream is running,
  * then ReadBufHandler() is called, with isUSB set to match
  * the stream source. SD card streams have priority over
  * USB streams, and will cancel a running USB stream.
  */
void CheckStream()
{
	uint16_t watermarkLevel = g_regs[WATERMARK_INT_CONFIG_REG] & ~WATERMARK_PULSE_MASK;

	/* Min. water mark for stream is 1. Want to allow general value of zero for timing char */
	if(watermarkLevel == 0)
		watermarkLevel = 1;

	/* Check water mark interrupt status */
	if(g_regs[BUF_CNT_0_REG] >= watermarkLevel)
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

/**
  * @brief Parse a command string into a script element
  *
  * @returns void
  *
  * @param commandBuf Script command string (from CLI or SD card)
  *
  * @param scr Script element to populate
  *
  * This function parses the script command type from
  * the available list of script commands, by comparison
  * to a pre-defined string literal. Then, the arguments for each
  * particular command type are parsed and validated.
  */
void ParseScriptElement(const uint8_t* commandBuf, script * scr)
{
	scr->numArgs = 0;
	scr->invalidArgs = 0;

	if(StringEquals(commandBuf, ReadCmd, sizeof(ReadCmd) - 1))
	{
		scr->scrCommand = read;
		/* Parse read arguments (1 - 3 arguments possible) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Clamp address values to 7-bit, don't care about LSB for read */
		scr->args[0] &= 0x7E;
		scr->args[1] &= 0x7E;
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

	if(StringEquals(commandBuf, ReadBufCmd, sizeof(ReadBufCmd) - 1))
	{
		scr->scrCommand = readbuf;
		/* No args */
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

	if(StringEquals(commandBuf, DelimCmd, sizeof(DelimCmd) - 1))
	{
		scr->scrCommand = delim;
		/* 1 arg (delim char) */
		scr->numArgs = 1;
		scr->args[0] = commandBuf[6];
		return;
	}

	if(StringEquals(commandBuf, EchoCmd, sizeof(EchoCmd) - 1))
	{
		scr->scrCommand = echo;
		/* 1 arg (echo enable/disable) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got at least 1 argument */
		if(scr->numArgs == 0)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, CommandCmd, sizeof(CommandCmd) - 1))
	{
		scr->scrCommand = cmd;
		/* 1 arg (command value) */
		scr->numArgs = ParseCommandArgs(commandBuf, scr->args);
		/* Check that we got at least 1 argument */
		if(scr->numArgs == 0)
			scr->invalidArgs = 1;
		return;
	}

	if(StringEquals(commandBuf, StatusCmd, sizeof(StatusCmd) - 1))
	{
		scr->scrCommand = status;
		/* No args */
		return;
	}

	if(StringEquals(commandBuf, UptimeCmd, sizeof(UptimeCmd) - 1))
	{
		scr->scrCommand = uptime;
		/* No args */
		return;
	}

	if(StringEquals(commandBuf, AboutCmd, sizeof(AboutCmd) - 1))
	{
		scr->scrCommand = about;
		/* No args */
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

	/* No matching command */
	scr->numArgs = 0;
	scr->scrCommand = invalid;
	return;
}

/**
  * @brief Executes a script element
  *
  * @return void
  *
  * @param scr The script element to execute
  *
  * @param outBuf Output buffer to write script result to
  *
  * @param isUSB Flag indicating if output data should be transmitted to USB CLI or SD card
  *
  * This function handles all non-control based script elements. It also
  * performs input validation on the script object, and will print an
  * error message for an invalid command or invalid arguments. If the command
  * and arguments are good, this function calls the lower level handler corresponding
  * to the script item. Currently is just a switch statement, could do some neat stuff
  * with a function pointer table in the future.
  */
void RunScriptElement(script* scr, uint8_t * outBuf, bool isUSB)
{
	/* Check that command is valid */
	if(scr->scrCommand >= invalid)
	{
		/* Transmit error and return */
		if(isUSB)
			USBTxHandler(InvalidCmdStr, sizeof(InvalidCmdStr));
		else
			SDTxHandler(InvalidCmdStr, sizeof(InvalidCmdStr));
		return;
	}

	/* Check that args are valid */
	if(scr->invalidArgs != 0)
	{
		/* Transmit error and return */
		if(isUSB)
			USBTxHandler(InvalidArgStr, sizeof(InvalidArgStr));
		else
			SDTxHandler(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}

	/* Squash script elements not handled here (sleep, looping) */
	if(scr->scrCommand > help)
	{
		/* Transmit error and return */
		if(isUSB)
			USBTxHandler(NotAllowedStr, sizeof(NotAllowedStr));
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
		case echo:
			g_regs[CLI_CONFIG_REG] &= ~USB_ECHO_BITM;
			if(scr->args[0] == 0)
			{
				/* Echo disable (set the bit) */
				g_regs[CLI_CONFIG_REG] |= USB_ECHO_BITM;
			}
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
		case cmd:
			/* Set command value and flag for processing */
			g_regs[USER_COMMAND_REG] = scr->args[0] & 0xFFFF;
			g_update_flags |= USER_COMMAND_FLAG;
			break;
		case status:
			RegAliasReadHandler(outBuf, isUSB, STATUS_0_REG);
			/* Clear status */
			g_regs[STATUS_0_REG] &= STATUS_CLEAR_MASK;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
			break;
		case help:
			/* Transmit about message */
			AboutHandler(outBuf, isUSB);
			/* Transmit help message */
			if(isUSB)
				USBTxHandler(HelpStr, sizeof(HelpStr));
			else
				SDTxHandler(HelpStr, sizeof(HelpStr));
			break;
		case about:
			AboutHandler(outBuf, isUSB);
			break;
		case uptime:
			UptimeHandler(outBuf, isUSB);
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

/**
  * @brief Handler for stream start/stop command
  *
  * @return void
  *
  * @param scr Script element being executed
  *
  * @param isUSB flag indicating if command came from SD script or USB CLI
  *
  * This function manages stream priorities. If a stream is already running
  * for the SD card script, a USB stream will not be started. The system currently
  * only supports streaming data to one destination at a time.
  */
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

/**
  * @brief Executes a factory reset + flash update
  *
  * @return void
  *
  * This function is called when the USB CLI executes a
  * freset command.
  */
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
  *
  * @param scr Script element being executed. Contains read arguments
  *
  * @param outBuf Buffer to write data to. Must be at least STREAM_BUF_SIZE bytes
  *
  * @param isUSB Flag indicating if output data should be sent to USB or SD card
  *
  * This function handles all read commands from SD scripts or the
  * USB CLI. Output data is filled to STREAM_BUF_SIZE, then transmitted.
  * The arguments provided in scr are assumed to be valid prior to this
  * function being called, so no additional input validation is performed.
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
		for(uint32_t addr = scr->args[0]; addr <= scr->args[1]; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[CLI_CONFIG_REG] >> CLI_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
			/* Check if transmit needed (not enough space for next loop) */
			if((STREAM_BUF_SIZE - count) < 6)
			{
				/* Check if at last read. If so, insert newline */
				if((addr == scr->args[1])||(addr == (scr->args[1] - 1)))
				{
					/* Move write pointer back one to last delim */
					writeBufPtr -= 1;
					/* Add newline */
					writeBufPtr[0] = '\r';
					writeBufPtr[1] = '\n';
					writeBufPtr += 2;
					/* Only one new char added */
					count += 1;
				}
				if(isUSB)
				{
					USBTxHandler(outBuf, count);
					USBWaitForTxDone(20);
				}
				else
					SDTxHandler(outBuf, count);
				/* Reset pointers */
				writeBufPtr = outBuf;
				count = 0;
			}
		}
		if(count != 0)
		{
			/* Move write pointer back one to last delim */
			writeBufPtr -= 1;
			/* Add newline */
			writeBufPtr[0] = '\r';
			writeBufPtr[1] = '\n';
			writeBufPtr += 2;
			/* Only one new char added */
			count += 1;
		}
	}
	/* transmit any remainder data */
	if(isUSB)
	{
		USBTxHandler(outBuf, count);
		USBWaitForTxDone(20);
	}
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

/**
  * @brief Handler for ReadBuf command
  *
  * @return void
  *
  * @param isUSB Flag indicating if output data should be sent to USB or SD card
  *
  * This function uses a ping-pong architecture for the buffer transmit
  * data. This avoids potential data corruption issues (USB transmit is non-blocking)
  */
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
		for(uint32_t addr = BUF_BASE_ADDR; addr <= bufLastAddr; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[CLI_CONFIG_REG] >> CLI_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
			if((STREAM_BUF_SIZE - count) < 6)
			{
				/* Check if at last read. If so, insert newline */
				if((addr == bufLastAddr)||(addr == (bufLastAddr - 1)))
				{
					/* Move write pointer back one to last delim */
					writeBufPtr -= 1;
					/* Add newline */
					writeBufPtr[0] = '\r';
					writeBufPtr[1] = '\n';
					writeBufPtr += 2;
					/* Only one new char added */
					count += 1;
				}

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
		if(count != 0)
		{
			/* Move write pointer back one to last delim */
			writeBufPtr -= 1;
			/* Add newline */
			writeBufPtr[0] = '\r';
			writeBufPtr[1] = '\n';
			writeBufPtr += 2;
			/* Only one new char added */
			count += 1;
		}
	}
	/* Transmit any residual data */
	if(isUSB)
		USBTxHandler(activeBuf, count);
	else
		SDTxHandler(activeBuf, count);
}

/**
  * @brief Read a register without changing page, and print to CLI
  *
  * @return void
  *
  * @param outBuf Buffer to write data to. Must be at least 6 bytes
  *
  * @param isUSB Flag indicating if output data should be sent to USB or SD card
  *
  * The function is used to implement register read alias commands.
  */
static void RegAliasReadHandler(uint8_t* outBuf, bool isUSB, uint16_t regIndex)
{
	/* Get reg value and covert to string */
	if(regIndex >= (NUM_REG_PAGES * REG_PER_PAGE))
	{
		outBuf[0] = 'B';
		outBuf[1] = 'a';
		outBuf[2] = 'd';
		outBuf[3] = ' ';
	}
	else
	{
		UShortToHex(outBuf, g_regs[regIndex]);
		outBuf[4] = '\r';
		outBuf[5] = '\n';
	}
	if(isUSB)
		USBTxHandler(outBuf, 6);
	else
		SDTxHandler(outBuf, 6);
}

/**
  * @brief Print about message to CLI
  *
  * @return void
  *
  * @param outBuf Buffer to write data to
  *
  * @param isUSB Flag indicating if output data should be sent to USB or SD card
  *
  * The function prints firmware version and date info, as well as a link to detailed docs on GitHub
  */
static void AboutHandler(uint8_t* outBuf, bool isUSB)
{
	uint32_t len = 0;

	/* Print firmware info */
	len = sprintf((char *) outBuf,
			"iSensor-SPI-Buffer, v%X.%02X (%04X-%02X-%02X). See https://github.com/ajn96/iSensor-SPI-Buffer for more info\r\n",
			(g_regs[FW_REV_REG] >> 8) & 0x7F,
			g_regs[FW_REV_REG] & 0xFF,
			g_regs[FW_YEAR_REG],
			g_regs[FW_DAY_MONTH_REG] & 0xFF,
			g_regs[FW_DAY_MONTH_REG] >> 8);

	if(isUSB)
		USBTxHandler(outBuf, len);
	else
		SDTxHandler(outBuf, len);
}

/**
  * @brief Print system uptime CLI
  *
  * @return void
  *
  * @param outBuf Buffer to write data to
  *
  * @param isUSB Flag indicating if output data should be sent to USB or SD card
  *
  * The system uptime is based on the HAL systick counter
  */
static void UptimeHandler(uint8_t* outBuf, bool isUSB)
{
	uint32_t len = 0;

	/* Print uptime */
	len = sprintf((char *) outBuf,
			"%lums\r\n",
			HAL_GetTick());

	if(isUSB)
		USBTxHandler(outBuf, len);
	else
		SDTxHandler(outBuf, len);
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
	/* Set command index to 0*/
	cmdIndex = 0;

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
  * @brief Convert a 16 bit value to the corresponding hex string (4 chars)
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

/**
  * @brief Check equality between two strings
  *
  * @param string0 First string to compare
  *
  * @param string1 Second string to compare
  *
  * @param count Number of chars to compare (from start)
  *
  * @return 1 for equal strings, 0 for not equal
  */
static uint32_t StringEquals(const uint8_t* string0, const uint8_t* string1, uint32_t count)
{
	for(int i = 0; i < count; i++)
	{
		if(string0[i] != string1[i])
		{
			return 0;
		}
	}
	return 1;
}

