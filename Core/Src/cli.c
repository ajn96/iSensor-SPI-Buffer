/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		cli.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer command line register interface
 **/

#include "cli.h"

/* Private function prototypes */
static void ParseCommand();
static void Read();
static void Write();
static void Stream();
static void ParseCommandArgs();
static uint32_t HexToUInt();
static void UShortToHex(uint8_t * outBuf, uint16_t val);
static void BlockingUSBTransmit(uint8_t * buf, uint32_t Len, uint32_t TimeoutMs);

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

/** USB Rx buffer (from usbd_cdc_if.c) */
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** USB Tx buffer (from usbd_cdc_if.c) */
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/** USB Rx count (from usbd_cdc_if.c) */
extern uint32_t USBRxCount;

/** Current command string */
static uint8_t CurrentCommand[64];

/** Byte buffer for echoing to console */
static uint8_t EchoBuf[4];

/** Print string for invalid command */
static uint8_t InvalidCmdStr[] = "Error: Invalid command!\r\n";

/** Print string for newline */
static uint8_t NewLineStr[] = "\r\n";

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

/** String literal for read command. Must be followed by a space */
static const uint8_t ReadCmd[] = "read ";

/** String literal for write command. Must be followed by a space */
static const uint8_t WriteCmd[] = "write ";

/** String literal for help command */
static const uint8_t HelpCmd[] = "help";

/** String literal for factory reset command */
static const uint8_t ResetCmd[] = "freset";

/** String literal for read buffer command */
static const uint8_t ReadBufCmd[] = "readbuf";

/** String literal for stream command. Must be followed by a space */
static const uint8_t StreamCmd[] = "stream ";

/** String literal for delim set command. Must be followed by a space */
static const uint8_t DelimCmd[] = "delim ";

/** Array for command arguments */
static uint32_t args[3];

/** Number of command arguments (0 - 3) */
static uint32_t numArgs;

/** Flag to track if current command arguments are valid */
static uint32_t goodArg;

/** Track parsing index within current command */
static uint32_t cmdIndex;

/**
  * @brief Handler for received USB data
  *
  * @return void
  *
  * This function should be called periodically from
  * the main loop to check if new USB data has been received
  */
void USBSerialHandler()
{
	/* Track index within current command string */
	static uint32_t commandIndex = 0;

	uint32_t bufIndex;
	uint32_t numBytes;

	/* Check if data has been received */
	if(USBRxCount)
	{
		numBytes = USBRxCount;
		USBRxCount = 0;
		for(bufIndex = 0; bufIndex < numBytes; bufIndex++)
		{
			/* Command is too long */
			if(commandIndex > 64)
			{
				BlockingUSBTransmit(InvalidCmdStr, sizeof(InvalidCmdStr), 20);
				commandIndex = 0;
			}
			/* Backspace typed in console */
			else if(UserRxBufferFS[bufIndex] == '\b')
			{
				/* Move command index back one space */
				if(commandIndex > 0)
					commandIndex--;
				/* Echo \b, space, \b to console */
				if(!(g_regs[USB_CONFIG_REG] & USB_ECHO_BITM))
				{
					EchoBuf[0] = '\b';
					EchoBuf[1] = ' ';
					EchoBuf[2] = '\b';
					BlockingUSBTransmit(EchoBuf, 3, 20);
				}
			}
			/* carriage return char (end of command) */
			else if(UserRxBufferFS[bufIndex] == '\r')
			{
				/* Send newline char if CLI echo is enabled */
				if(!(g_regs[USB_CONFIG_REG] & USB_ECHO_BITM))
				{
					BlockingUSBTransmit(NewLineStr, sizeof(NewLineStr), 20);
				}
				/* Place a string terminator */
				CurrentCommand[commandIndex] = 0;
				/* Parse command */
				ParseCommand();
				/* Clear command buffer */
				for(int i = 0; i < sizeof(CurrentCommand); i++)
					CurrentCommand[i] = 0;
				commandIndex = 0;
				return;
			}
			else
			{
				/* Add char to current command */
				CurrentCommand[commandIndex] = UserRxBufferFS[bufIndex];
				commandIndex++;
				/* Echo to console */
				if(!(g_regs[USB_CONFIG_REG] & USB_ECHO_BITM))
				{
					EchoBuf[0] = UserRxBufferFS[bufIndex];
					BlockingUSBTransmit(EchoBuf, 1, 20);
				}
			}
		}
	}
}

/**
  * @brief Print all contents of the buffer to the USB CLI
  *
  * @return void
  */
void USBReadBuf()
{
	uint32_t numBufs = g_regs[BUF_CNT_0_REG];
	uint32_t bufLastAddr = g_regs[BUF_LEN_REG] + BUF_DATA_BASE_ADDR;
	uint32_t buf;
	uint32_t addr;
	uint8_t* writeBufPtr;
	uint16_t readVal;
	uint32_t count;

	/* Set page to 255 */
	WriteReg(0, 255);

	for(buf = 0; buf < numBufs; buf++)
	{
		writeBufPtr = UserTxBufferFS;
		count = 0;
		BufDequeueToOutputRegs();
		for(addr = BUF_BASE_ADDR; addr < bufLastAddr; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[USB_CONFIG_REG] >> USB_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
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
		BlockingUSBTransmit(UserTxBufferFS, count, 20);
	}
}

/**
  * @brief Parse CLI command from CurrentCommand buffer
  *
  * @return void
  */
static void ParseCommand()
{
	uint32_t validCmd;
	/* Do this kinda lazy. Just check if command matches any allowed commands and call handler */

	/* Read command */
	validCmd = 1;
	for(int i = 0; i < sizeof(ReadCmd) - 1; i++)
	{
		if(CurrentCommand[i] != ReadCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Call read handler */
		Read();
		return;
	}

	/* write command */
	validCmd = 1;
	for(int i = 0; i < sizeof(WriteCmd) - 1; i++)
	{
		if(CurrentCommand[i] != WriteCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Call write handler */
		Write();
		return;
	}

	/* readbuf command */
	validCmd = 1;
	for(int i = 0; i < sizeof(ReadBufCmd) - 1; i++)
	{
		if(CurrentCommand[i] != ReadBufCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Call read buf handler */
		USBReadBuf();
		return;
	}

	/* stream start/stop command */
	validCmd = 1;
	for(int i = 0; i < sizeof(StreamCmd) - 1; i++)
	{
		if(CurrentCommand[i] != StreamCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Call stream handler */
		Stream();
		return;
	}

	/* Help command */
	validCmd = 1;
	for(int i = 0; i < sizeof(HelpCmd) - 1; i++)
	{
		if(CurrentCommand[i] != HelpCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Print help string and return */
		BlockingUSBTransmit(HelpStr, sizeof(HelpStr), 20);
		return;
	}

	/* Set delim command */
	validCmd = 1;
	for(int i = 0; i < sizeof(DelimCmd) - 1; i++)
	{
		if(CurrentCommand[i] != DelimCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Clear delim char in USB config */
		g_regs[USB_CONFIG_REG] &= ~USB_DELIM_BITM;
		/* Set new value */
		g_regs[USB_CONFIG_REG] |= (CurrentCommand[6] << USB_DELIM_BITP);
		return;
	}

	/* Reset command */
	validCmd = 1;
	for(int i = 0; i < sizeof(ResetCmd) - 1; i++)
	{
		if(CurrentCommand[i] != ResetCmd[i])
		{
			validCmd = 0;
			break;
		}
	}
	if(validCmd)
	{
		/* Perform factory reset */
		g_regs[USER_COMMAND_REG] = CMD_FACTORY_RESET;
		ProcessCommand();
		/* Perform flash update */
		g_regs[USER_COMMAND_REG] = CMD_FLASH_UPDATE;
		ProcessCommand();
		return;
	}

	/* Else end invalid command string */
	BlockingUSBTransmit(InvalidCmdStr, sizeof(InvalidCmdStr), 20);
}

/**
  * @brief Read command handler
  *
  * @return void
  */
static void Read()
{
	/* Parameters */
	uint32_t startAddr, endAddr, numReads;

	uint8_t* writeBufPtr;
	uint16_t readVal;
	uint32_t count = 0;

	ParseCommandArgs();
	if(numArgs == 0)
	{
		CDC_Transmit_FS(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}
	else if(numArgs == 1)
	{
		startAddr = args[0];
		endAddr = startAddr;
		numReads = 1;
	}
	else if(numArgs == 2)
	{
		startAddr = args[0];
		endAddr = args[1];
		numReads = 1;
	}
	else
	{
		startAddr = args[0];
		endAddr = args[1];
		numReads = args[2];
	}

	/* Limit address to 7 bits */
	startAddr &= 0x7F;
	endAddr &= 0x7F;

	/* Validate inputs */
	if(startAddr > endAddr)
	{
		CDC_Transmit_FS(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}

	/* Perform read */
	for(int i = 0; i<numReads; i++)
	{
		writeBufPtr = UserTxBufferFS;
		count = 0;
		for(uint32_t addr = startAddr; addr < endAddr; addr += 2)
		{
			readVal = ReadReg(addr);
			UShortToHex(writeBufPtr, readVal);
			writeBufPtr[4] = g_regs[USB_CONFIG_REG] >> USB_DELIM_BITP;
			writeBufPtr += 5;
			count += 5;
		}
		/* Last read doesn't need delim char */
		readVal = ReadReg(endAddr);
		UShortToHex(writeBufPtr, readVal);
		writeBufPtr += 4;
		count += 4;
		/* Add newline */
		writeBufPtr[0] = '\r';
		writeBufPtr[1] = '\n';
		writeBufPtr += 2;
		count += 2;
		/* Transmit */
		BlockingUSBTransmit(UserTxBufferFS, count, 20);
	}
}

/**
  * @brief Write command handler
  *
  * @return void
  *
  * The write address is passed in args[0]. The address is
  * masked to only 7 bits (address space of a page). The
  * write value is passed in args[1]. The write value is masked to
  * 8 bits (byte-wise writes).
  */
static void Write()
{
	ParseCommandArgs();
	if(numArgs != 2)
	{
		BlockingUSBTransmit(InvalidArgStr, sizeof(InvalidArgStr), 20);
		return;
	}
	/* Mask addr to 7 bits, value to 8 bits */
	args[0] &= 0x7F;
	args[1] &= 0xFF;

	WriteReg(args[0], args[1]);
}

/**
  * @brief Stream command handler
  *
  * @return void
  *
  * Sets the stream enable bit of USB_CONFIG based on
  * args[0]
  */
static void Stream()
{
	ParseCommandArgs();
	if(numArgs != 1)
	{
		BlockingUSBTransmit(InvalidArgStr, sizeof(InvalidArgStr), 20);
		return;
	}

	/* Set/clear stream interrupt enable flag */
	if(args[0])
	{
		g_regs[USB_CONFIG_REG] |= USB_STREAM_BITM;
	}
	else
	{
		g_regs[USB_CONFIG_REG] &= ~USB_STREAM_BITM;
	}
}

/**
  * @brief Parse arguments out from CurrentCommand
  *
  * @return void
  *
  * Arguments must be separated by a space. Each argument
  * (up to 3) are placed into the args array. The total number
  * of arguments in the current command is placed in numArgs.
  * All arguments must be hex strings.
  */
static void ParseCommandArgs()
{
	/* Set number of args to 0*/
	numArgs = 0;

	/* Set command index to 0 */

	/* Run through command until we hit a space */
	cmdIndex = 0;
	while(CurrentCommand[cmdIndex] != ' ')
	{
		cmdIndex++;
	}
	/* Move to first char */
	cmdIndex++;
	/* Parse */
	args[0] = HexToUInt();
	if(!goodArg)
	{
		return;
	}
	numArgs++;
	/* Parse second arg */
	args[1] = HexToUInt();
	if(!goodArg)
	{
		return;
	}
	numArgs++;
	/* Parse third arg */
	args[2] = HexToUInt();
	if(!goodArg)
	{
		return;
	}
	numArgs++;
	return;
}

/**
  * @brief Convert a hex string to a 32 bit uint
  *
  * @return void
  *
  * The input string must be stored in CurrentCommand, with
  * cmdIndex set to point at the first value in the string.
  */
static uint32_t HexToUInt()
{
	uint8_t currentByte;
	uint32_t value = 0;
	uint32_t stringDone = 0;

	/* Set goodArg flag to true */
	goodArg = 1;

	/* Get first byte */
	currentByte = CurrentCommand[cmdIndex];
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
		currentByte = CurrentCommand[cmdIndex];
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

/**
  * @brief Wrapper around CDC_Transmit_FS which retries until the USB transmit is successful
  *
  * @param buf The data buffer to transmit
  *
  * @param Len The number of bytes to transmit
  *
  * @param TimeoutMs Operation timeout (in ms). Must keep under watchdog reset period
  *
  * @return void
  */
static void BlockingUSBTransmit(uint8_t * buf, uint32_t Len, uint32_t TimeoutMs)
{
	uint8_t status = CDC_Transmit_FS(buf, Len);
	uint32_t endTime = HAL_GetTick() + TimeoutMs;
	while((status != USBD_OK)&&(HAL_GetTick() < endTime))
	{
		status = CDC_Transmit_FS(buf, Len);
	}
}
