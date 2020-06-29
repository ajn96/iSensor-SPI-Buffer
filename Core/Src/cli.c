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
static void ReadBuf();
static void Stream();
static void ParseCommandArgs();
static uint32_t HexToUInt();

/** USB Rx buffer (from usbd_cdc_if.c) */
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** USB Tx buffer (from usbd_cdc_if.c) */
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/** USB Rx count (from usbd_cdc_if.c) */
extern uint32_t USBRxCount;

/* Stream interrupt enable flag */
uint32_t g_StreamEnable = 0;

/** Current command string */
static uint8_t CurrentCommand[64];

/** Byte buffer for echoing to console */
static uint8_t EchoBuf[4];

/** Print string for invalid command */
static uint8_t InvalidCmdStr[] = "\r\nError: Invalid command!\r\n>";

/** Print string for newline */
static uint8_t NewLineStr[] = "\r\n>";

/** Print string for invalid argument */
static uint8_t InvalidArgStr[] = "\r\nError: Invalid argument!\r\n>";

/** Print string for help command */
static uint8_t HelpStr[] = "\r\nAll numeric values are in hex. [] arguments are optional\r\n"
		"help: Print available commands\r\n"
		"reset: Performs a software reset\r\n"
		"read startAddr [endAddr = addr] [numReads = 1]: Read registers starting at startAddr and ending at endAddr numReads times\r\n"
		"write addr value: Write the 8-bit value in value to register at address addr\r\n"
		"readbuf: Read all buffer entries. Values for each entry are placed on a newline\r\n"
		"stream startStop: Start (startStop != 0) or stop (startStop = 0) a read stream\r\n>";

/** String literal for read command. Must be followed by a space */
static const uint8_t ReadCmd[] = "read ";

/** String literal for write command. Must be followed by a space */
static const uint8_t WriteCmd[] = "write ";

/** String literal for help command */
static const uint8_t HelpCmd[] = "help";

/** String literal for reset command */
static const uint8_t ResetCmd[] = "reset";

/** String literal for read buffer command */
static const uint8_t ReadBufCmd[] = "readbuf";

/** String literal for stream command. Must be followed by a space */
static const uint8_t StreamCmd[] = "stream ";

/** Array for command arguments */
static uint32_t args[3];

/** Number of command arguments (0 - 3) */
static uint32_t numArgs;

static uint32_t goodArg;

static uint32_t cmdIndex;

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
				CDC_Transmit_FS(InvalidCmdStr, sizeof(InvalidCmdStr));
				commandIndex = 0;
			}
			/* Backspace typed in console */
			else if(UserRxBufferFS[bufIndex] == '\b')
			{
				/* Move command index back one space */
				if(commandIndex > 0)
					commandIndex--;
				/* Echo \b, space, \b to console */
				EchoBuf[0] = '\b';
				EchoBuf[1] = ' ';
				EchoBuf[2] = '\b';
				CDC_Transmit_FS(EchoBuf, 3);
			}
			/* carriage return char (end of command) */
			else if(UserRxBufferFS[bufIndex] == '\r')
			{
				/* Place a string terminator */
				CurrentCommand[commandIndex] = 0;
				/* Parse command */
				ParseCommand();
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
				EchoBuf[0] = UserRxBufferFS[bufIndex];
				CDC_Transmit_FS(EchoBuf, 1);
			}
		}
	}
}

static void ParseCommand()
{
	uint32_t validCmd;
	/* Do this kinda lazy. Just check if command matches any allowed commands and call handler */

	/* Read command */
	validCmd = 1;
	for(int i = 0; i < sizeof(ReadCmd) - 1; i++)
	{
		if(CurrentCommand[i] != ReadCmd[i])
			validCmd = 0;
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
			validCmd = 0;
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
			validCmd = 0;
	}
	if(validCmd)
	{
		/* Call read buf handler */
		ReadBuf();
		return;
	}

	/* stream start/stop command */
	validCmd = 1;
	for(int i = 0; i < sizeof(StreamCmd) - 1; i++)
	{
		if(CurrentCommand[i] != StreamCmd[i])
			validCmd = 0;
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
			validCmd = 0;
	}
	if(validCmd)
	{
		/* Print help string and return */
		CDC_Transmit_FS(HelpStr, sizeof(HelpStr));
		return;
	}

	/* Reset command */
	validCmd = 1;
	for(int i = 0; i < sizeof(ResetCmd) - 1; i++)
	{
		if(CurrentCommand[i] != ResetCmd[i])
			validCmd = 0;
	}
	if(validCmd)
	{
		/* Reset and return */
		NVIC_SystemReset();
		return;
	}

	/* Send invalid command string */
	CDC_Transmit_FS(InvalidCmdStr, sizeof(InvalidCmdStr));
}

static void Read()
{
	uint32_t startAddr, endAddr, numReads;
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

	uint8_t* writeBufPtr = UserRxBufferFS;
	uint16_t readVal;
	uint32_t count = 0;

	/* Insert initial new line */
	writeBufPtr[0] = '\r';
	writeBufPtr[1] = '\n';
	writeBufPtr += 2;
	count += 2;

	/* Perform read */
	for(int i = 0; i<numReads; i++)
	{
		for(uint32_t addr = startAddr; addr <= endAddr; addr += 2)
		{
			readVal = ReadReg(addr);
			sprintf(writeBufPtr, "%04X", readVal);
			writeBufPtr += 4;
			count += 4;
		}
		writeBufPtr[0] = '\r';
		writeBufPtr[1] = '\n';
		writeBufPtr[2] = '>';
		writeBufPtr += 3;
		count += 3;
	}
	CDC_Transmit_FS(UserRxBufferFS, count);
}

static void Write()
{
	ParseCommandArgs();
	if(numArgs != 2)
	{
		CDC_Transmit_FS(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}
	/* Mask addr to 7 bits, value to 8 bits */
	args[0] &= 0x7F;
	args[1] &= 0xFF;

	WriteReg(args[0], args[1]);
	CDC_Transmit_FS(NewLineStr, sizeof(NewLineStr));
}

static void ReadBuf()
{

}

static void Stream()
{
	ParseCommandArgs();
	if(numArgs != 1)
	{
		CDC_Transmit_FS(InvalidArgStr, sizeof(InvalidArgStr));
		return;
	}

	/* Set stream interrupt enable flag */
	if(args[0])
	{
		g_StreamEnable = 1;
	}
	else
	{
		g_StreamEnable = 0;
	}
}

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
