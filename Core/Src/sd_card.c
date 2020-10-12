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

/* Private functions */
static bool OpenScriptFiles();
static bool SDCardAttached();
static bool ParseScriptFile();
static bool CommandPostLoadProcess();
static bool CreateResultFile();
static void SPI3_Init(void);
static void ParseReadBuffer(UINT bytesRead);

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[3 * REG_PER_PAGE];

/** SPI handle for SD card master port (global scope) */
SPI_HandleTypeDef g_spi3;

/** Buffer for SD card read/writes */
static uint8_t sd_buf[STREAM_BUF_SIZE];

/** Command list. Loaded from cmd.txt on the SD card */
static script cmdList[SCRIPT_MAX_ENTRIES];

/** Track index within the command list currently being executed */
static uint32_t cmdIndex;

/** Number of commands within the current script */
static uint32_t numCmds;

/** Track if a script is actively running */
static uint32_t scriptRunning;

/** Handle for command file (cmd.txt in top level directory) */
static FIL cmdFile = {0};

/** Handle for output file (result.txt in top level directory) */
static FIL outFile = {0};

/** File system object */
static FATFS fs = {0};

/** String literal script start message */
static const uint8_t ScriptStart[] = "Script Starting...\r\n";

/**
  * @brief SD card write handler function
  *
  * @param buf Buffer containing data to write
  *
  * @param count Number of bytes to write
  *
  * @return void
  *
  * This function is called by the script execution
  * routines, when a script object is executed
  * from a SD card script context.
  */
void SDTxHandler(const uint8_t* buf, uint32_t count)
{
	if(count == 0)
		return;
	UINT writeCount = 0;
	FRESULT result = f_write(&outFile, buf, count, &writeCount);
	f_sync(&outFile);

	/* Flag error if write was not processed correctly */
	if((result != FR_OK) || (writeCount < count))
	{
		g_regs[SCR_ERROR_REG] |= SCR_WRITE_FAIL;
		g_regs[STATUS_0_REG] |= STATUS_SCR_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
}

/**
  * @brief Init SD card hardware interface and FATFs driver
  *
  * @return void
  *
  * This function configures the SPI3 port for use with an
  * SD card and inits all GPIO as needed. This function is called
  * directly by the application on startup. It does not attempt to
  * connect to the SD card in any way.lked right into tha
  */
void SDCardInit()
{
	/* Init SPI3 */
	SPI3_Init();

	/* Init FAT file system */
	MX_FATFS_Init();
}

/**
  * @brief Run SD card script automatic execution process
  *
  * @return void
  *
  * This function is intended to be called at the end of the initialization
  * portion of main, just before entering the cyclic executive. If the SD
  * card autorun bit is set in USB_CONFIG and the watchdog reset bit is not
  * set in STATUS, the script will be run. The watchdog reset check is in place to
  * remove the potential for a watchdog reset loop.
  */
void ScriptAutorun()
{
	if(((g_regs[CLI_CONFIG_REG] & SD_AUTORUN_BITM) != 0)&&((g_regs[STATUS_0_REG] & STATUS_WATCHDOG) == 0))
	{
		StartScript();
	}
}

/**
  * @brief Start script run
  *
  * @return void
  *
  * This function checks if an SD card is connected, then sets
  * up the FAT file system if there is a card. Once the file system
  * is set up, "script.txt" is read and parsed, if it exists. Once
  * the script has been processed, the script execution state variables
  * are initialized, and the script is kicked off. The actual script
  * execution work occurs in ScriptStep(), which is called periodically
  * from the main cyclic executive loop.
  */
void StartScript()
{
	/* Set script running flag to false */
	scriptRunning = 0;

	/* Init script line and error to 0 */
	g_regs[SCR_LINE_REG] = 0;
	g_regs[SCR_ERROR_REG] = 0;

	/* Set error and return if no SD card attached */
	if(!SDCardAttached())
	{
		g_regs[STATUS_0_REG] |= STATUS_SCR_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		g_regs[SCR_ERROR_REG] |= SCR_NO_SD;
		return;
	}

	/* Mount SD card and open files (script.txt, result.txt). Abort for any error */
	if(!OpenScriptFiles())
	{
		g_regs[STATUS_0_REG] |= STATUS_SCR_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		return;
	}

	/* Parse script.txt */
	if(!ParseScriptFile())
	{
		g_regs[STATUS_0_REG] |= STATUS_SCR_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		return;
	}

	/* Write start message */
	SDTxHandler(ScriptStart, sizeof(ScriptStart) - 1);

	/* If we reach here, script is loaded and good. Can start execution
	 * process. First set script running status bit
	 * (sticky so won't clear on read) */
	g_regs[STATUS_0_REG] |= STATUS_SCR_RUNNING;
	g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

	/* Init script state variables */
	cmdIndex = 0;

	/* Set script running flag for main loop processing */
	scriptRunning = 1;
}

/**
  * @brief Stop a running SD card script.
  *
  * @return void
  *
  * This function can be invoked by the user (via the
  * script cancel command), or will run automatically once
  * the script execution process has finished. It clears the
  * script running flags and the script active STATUS register bit.
  * It then closes the output text file and unmounts from the SD
  * card.
  */
void StopScript()
{
	/* Clear script running status bit */
	g_regs[STATUS_0_REG] &= ~(STATUS_SCR_RUNNING);
	g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

	/* Clear script stream bit */
	g_regs[CLI_CONFIG_REG] &= ~(SD_STREAM_BITM);

	/* Clear script running flag */
	scriptRunning = 0;

	/* Set script line and error to 0 */
	g_regs[SCR_LINE_REG] = 0;
	g_regs[SCR_ERROR_REG] = 0;

	/* Close all files and unmount SD card */
	f_close(&cmdFile);
	f_close(&outFile);

	/* Unmount */
	f_mount(0, "", 0);
}

/**
  * @brief Step the script execution process.
  *
  * @return void
  *
  * This function is called from the main loop. It is responsible
  * for performing the actual script execution, and writing the
  * output to the SD card (via
  */
void ScriptStep()
{
	/* Exit if script not running */
	if(!scriptRunning)
	{
		return;
	}

	/* Check if we've finished the script */
	if(cmdIndex >= numCmds)
	{
		StopScript();
		return;
	}

	/* Execute current command and write to file */
	g_regs[SCR_LINE_REG] = cmdIndex + 1;

	if(cmdList[cmdIndex].scrCommand == endloop)
	{
		/* jump index stored in arg1, numloops stored in arg0 */
		if(cmdList[cmdIndex].args[0] > 0)
		{
			cmdList[cmdIndex].args[0]--;
			cmdIndex = cmdList[cmdIndex].args[1];
		}
		else
		{
			/* No more loops left, move to next instruction */
			cmdIndex++;
		}
	}
	else if(cmdList[cmdIndex].scrCommand == sleep)
	{
		/* arg0 is sleep duration. Set arg1 to 1 on first pass to indicate sleep running, arg2 to the end sleep time */
		if(cmdList[cmdIndex].args[1] == 0)
		{
			/* First run */
			cmdList[cmdIndex].args[1] = 1;

			/* End time is current HAL time + arg0 */
			cmdList[cmdIndex].args[2] = cmdList[cmdIndex].args[0] + HAL_GetTick();
		}
		else
		{
			/* Sleep is running */
			if(HAL_GetTick() > cmdList[cmdIndex].args[2])
			{
				/* Reset running flag */
				cmdList[cmdIndex].args[1] = 0;
				/* Move to next command */
				cmdIndex++;
			}
		}
	}
	else
	{
		/* Generic script entry (no program execution control) */
		for(int i = 0; i < sizeof(sd_buf); i++)
		{
			sd_buf[i] = 0;
		}
		RunScriptElement(&cmdList[cmdIndex], sd_buf, false);
		cmdIndex++;
	}
}

/**
  * @brief Check if SD card is attached
  *
  * @return true if card is attached, false otherwise
  *
  * This function makes use of the SD card detect pin to
  * determine if an SD card is inserted without communicating
  * to the SD card directly (floating if no SD card, pulled to
  * ground if there is an SD card).
  */
static bool SDCardAttached()
{
	GPIO_PinState state;

	/* De-init */
	HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

	/* Enable pull up on SD detect line */
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* Wait 1ms for any changes to stabilize and measure input. If high then return false */
	SleepMicroseconds(1000);
	state = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2);

	/* Disable pull up */
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* Pulled low means there is an SD card. High means pin is floating */
	if(state)
		return false;
	else
		return true;
}

/**
  * @brief Mount SD card an open "script.txt" and "result.txt"
  *
  * @return true if files are opened successfully, false otherwise
  *
  * This function first mounts an attached SD card, then populates the
  * two file handles required for the application. If either file fails
  * to open, both file handles are closed, and the SD card file
  * system is unmounted.
  */
static bool OpenScriptFiles()
{
	/* mount SD card */
	if(f_mount(&fs, "", 1) != FR_OK)
	{
		/* Unmount */
		f_mount(0, "", 0);

		/* Save error */
		g_regs[SCR_ERROR_REG] |= SCR_MOUNT_ERROR;

		/* Retry once */
		if((g_regs[SCR_ERROR_REG] & SCR_MOUNT_ERROR) == 0)
			return OpenScriptFiles();

		/* Return error */
		return false;
	}

	/* Open script.txt in read only mode */
	if(f_open(&cmdFile, "SCRIPT.TXT", FA_READ) != FR_OK)
	{
		/* Attempt file close */
		f_close(&cmdFile);

		/* Unmount */
		f_mount(0, "", 0);

		/* Save error */
		g_regs[SCR_ERROR_REG] |= SCR_SCRIPT_OPEN_ERROR;

		/* Return error */
		return false;
	}

	/* Open result.txt in write mode */
	if(!CreateResultFile())
	{
		/* Attempt file close on script and result */
		f_close(&outFile);
		f_close(&cmdFile);

		/* Unmount */
		f_mount(0, "", 0);

		/* Save error */
		g_regs[SCR_ERROR_REG] |= SCR_RESULT_OPEN_ERROR;

		/* Return error */
		return false;
	}
	return true;
}

/**
  * @brief Create new result file
  *
  * @return true if file created, false otherwise
  *
  * This function creates a unique result file for each
  * script run. The result file number increments from 0 to
  * 999.
  */
static bool CreateResultFile()
{
	bool fileFound;
	FRESULT res;
	/* Result file name */
	char fileName[13];

	fileFound = false;
	for(uint16_t fileNum = 0; fileNum < 10000; fileNum++)
	{
		sprintf(fileName, "RES_%03d.TXT", fileNum);
		res = f_stat(fileName, 0);
		if(res != FR_OK)
		{
			fileFound = true;
			break;
		}
	}
	if(!fileFound)
		return false;

	res = f_open(&outFile, fileName, (FA_CREATE_ALWAYS|FA_WRITE));
	if(res != FR_OK)
		return false;

	return true;
}

/**
  * @brief Parse script.txt into script element array (cmdList)
  *
  * @return true if good script, false otherwise
  *
  * This function loops through script.txt one read buffer at a time.
  * Each read buffer is then parsed to find the command
  * type and arguments. Any error flags set by ParseScriptElement
  * will cause this function to return false. These flags can be
  * set for invalid commands or arguments. In addition, any error
  * found in command load post-process (which links all control
  * flow related commands) will result in the script being cancelled.
  */
static bool ParseScriptFile()
{
	numCmds = 0;
	UINT fileLen;
	UINT bytesRead;
	UINT totalBytesRead;

	fileLen = cmdFile.fsize;
	totalBytesRead = 0;
	while(totalBytesRead < fileLen)
	{
		/* If we got too many lines return false */
		if(numCmds >= SCRIPT_MAX_ENTRIES)
			return false;

		/* Read buffer from file */
		f_read(&cmdFile, (char *)sd_buf, sizeof(sd_buf), &bytesRead);
		if(bytesRead == 0)
			return false;
		totalBytesRead += bytesRead;

		/* Parse */
		ParseReadBuffer(bytesRead);
	}

	/* Close command file */
	f_close(&cmdFile);
	/* Perform post-load processing and return result */
	return CommandPostLoadProcess();
}

/**
  * @brief Parse a script buffer read from the SD card
  *
  * @return void
  *
  * The script read data to be parsed must be stored in
  * sd_buf. This function iterates through the read array
  * and calls the lower level ParseScriptElement() routine
  * whenever a newline character is encountered.
  */
static void ParseReadBuffer(UINT bytesRead)
{
	/* Buffer to read command into. Static allows for state persistence across multiple calls */
	static uint8_t cmd[64] = {0};
	static int cmdIndex = 0;

	/* Go through read data splitting along \n */
	for(int i = 0; i < bytesRead; i++)
	{
		if(sd_buf[i] == '\n')
		{
			/* We got a good line from the file, parse into command array */
			ParseScriptElement(cmd, &cmdList[numCmds]);
			numCmds++;
			if(numCmds >= SCRIPT_MAX_ENTRIES)
				return;
			cmdIndex = 0;
			for(int j = 0; j < 64; j++)
				cmd[j] = 0;
		}
		else if(sd_buf[i] == '\r')
		{
			/* Ignore \r */
		}
		else if(cmdIndex < 64)
		{
			cmd[cmdIndex] = sd_buf[i];
			cmdIndex++;
		}
		else
		{
			/* Extra data is ignored */
		}
	}
	if((bytesRead < sizeof(sd_buf)) && (cmdIndex > 0))
	{
		/* Clean up command at end of file */
		ParseScriptElement(cmd, &cmdList[numCmds]);
		numCmds++;
		cmdIndex = 0;
		for(int j = 0; j < 64; j++)
			cmd[j] = 0;
	}
}

/**
  * @brief Process loaded command array
  *
  * @return true if script is good, false otherwise
  *
  * This function checks all commands for validity (argument and command).
  * It also sets up all loop state variables, and links the end loop commands
  * to the corresponding start loops.
  */
static bool CommandPostLoadProcess()
{
	uint32_t startLoopIndex;

	/* If zero commands parsed return false */
	if(numCmds == 0)
		return false;

	/* Set start loop index to an invalid value */
	startLoopIndex = INVALID_LOOP_INDEX;
	for(int i = 0; i < numCmds; i++)
	{
		if(cmdList[i].invalidArgs != 0)
		{
			/* Invalid arg, return false */
			g_regs[SCR_ERROR_REG] |= SCR_PARSE_INVALID_ARGS;
			return false;
		}
		if(cmdList[i].scrCommand >= invalid)
		{
			/* Invalid command, return false */
			g_regs[SCR_ERROR_REG] |= SCR_PARSE_INVALID_CMD;
			return false;
		}
		if(cmdList[i].scrCommand == loop)
		{
			/* Check that we aren't inside a previous loop */
			if(startLoopIndex != INVALID_LOOP_INDEX)
			{
				g_regs[SCR_ERROR_REG] |= SCR_PARSE_INVALID_LOOP;
				return false;
			}
			else
			{
				startLoopIndex = i;
			}
		}
		if(cmdList[i].scrCommand == endloop)
		{
			/* Check that we already hit a start loop command */
			if(startLoopIndex == INVALID_LOOP_INDEX)
			{
				g_regs[SCR_ERROR_REG] |= SCR_PARSE_INVALID_LOOP;
				return false;
			}
			else
			{
				/* Set jump target to arg 1, count to arg 0 */
				cmdList[i].args[0] = cmdList[startLoopIndex].args[0];
				cmdList[i].args[1] = startLoopIndex;
				startLoopIndex = INVALID_LOOP_INDEX;
			}
		}
	}
	return true;
}

/**
  * @brief SPI3 Initialization Function (SD card master SPI port)
  *
  * @return void
  *
  * Configures SPI 3 as master SPI port for SD card
  * SPI mode: 0
  * SCLK Freq: 4.25MHz
  * SPI word size: 8 bits
  */
static void SPI3_Init(void)
{
	/* SPI3 parameter configuration*/
	g_spi3.Instance = SPI3;
	g_spi3.Init.Mode = SPI_MODE_MASTER;
	g_spi3.Init.Direction = SPI_DIRECTION_2LINES;
	g_spi3.Init.DataSize = SPI_DATASIZE_8BIT;
	g_spi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	g_spi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	g_spi3.Init.NSS = SPI_NSS_HARD_OUTPUT;
	/* 36MHz / 8 -> 4.25MHz SCLK */
	g_spi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	g_spi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	g_spi3.Init.TIMode = SPI_TIMODE_DISABLE;
	g_spi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_spi3.Init.CRCPolynomial = 7;
	g_spi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_spi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	HAL_SPI_Init(&g_spi3);
}
