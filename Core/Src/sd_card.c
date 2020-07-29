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
static void SPI3_Init(void);

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[3 * REG_PER_PAGE];

/** SPI handle for SD card master port */
static SPI_HandleTypeDef spi3;

/* Command list. Loaded from cmd.txt on the SD card */
static script cmdList[SCRIPT_MAX_ENTRIES];

/** Track index within the command list currently being executed */
static uint32_t cmdIndex;

/* Number of commands within the current script */
static uint32_t numCmds;

/** Track if a script is actively running */
static uint32_t scriptRunning;

/** Handle for command file (cmd.txt in top level directory) */
static FIL cmdFile;

/** Handle for output file (result.txt in top level directory) */
static FIL outFile;

/** File system object */
static FATFS fs;

/**
  * @brief Init SD card hardware interface and FATFs driver
  *
  * @return void
  *
  * This function configures the SPI3 port for use with an
  * SD card and inits all GPIO as needed. This function is called
  * directly by the application on startup. It does not attempt to
  * connect to the SD card in any way.
  */
void SDCardInit()
{
	/* Init SPI3 */
	SPI3_Init();

	/* Init FAT file system */
	MX_FATFS_Init();
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

	/* Set error and return if no SD card attached */
	if(!SDCardAttached())
	{
		g_regs[STATUS_0_REG] |= STATUS_SCR_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
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

	/* If we reach here, script is loaded and good. Can start execution process */

	/* Set script running status bit (sticky so won't clear on read) */
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

	/* Clear script running flag */
	scriptRunning = 0;

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
  * output to the SD card.
  */
void ScriptStep()
{
	uint32_t numBytes;

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
	/* Enable pull up on SD detect line */

	/* Measure input. If high then return false */

	/* Pulled low means there is an SD card */
	return true;
}

static bool OpenScriptFiles()
{
	/* mount SD card */
	if(f_mount(&fs, "", 1) != FR_OK)
	{
		/* Unmount */
		f_mount(0, "", 0);

		/* Return error */
		return false;
	}


	/* Open script.txt in read only mode */
	if(f_open(&cmdFile, "script.txt", FA_READ) != FR_OK)
	{
		/* Attempt file close */
		f_close(&cmdFile);

		/* Unmount */
		f_mount(0, "", 0);

		/* Return error */
		return false;
	}

	/* Open result.txt in append write mode */
	if(f_open(&outFile, "result.txt", FA_OPEN_ALWAYS) != FR_OK)
	{
		/* Attempt file close on script and result */
		f_close(&outFile);
		f_close(&cmdFile);

		/* Unmount */
		f_mount(0, "", 0);

		/* Return error */
		return false;
	}

	/* Seek to end of result.txt */

	return true;
}

/**
  * @brief Parse script.txt into script element array
  *
  * @return true if good script, false otherwise
  *
  * This function loops through script.txt line by line. For
  * each line, ParseScriptElement is called to find the command
  * type and arguments. Any error flags set by ParseScriptElement
  * will cause this function to return false. These flags can be
  * set for invalid commands or arguments
  */
static bool ParseScriptFile()
{
	numCmds = 0;

	return true;
}

/**
  * @brief SPI3 Initialization Function (SD card master SPI port)
  *
  * @return void
  */
static void SPI3_Init(void)
{
  /* SPI3 parameter configuration*/
  spi3.Instance = SPI3;
  spi3.Init.Mode = SPI_MODE_MASTER;
  spi3.Init.Direction = SPI_DIRECTION_2LINES;
  spi3.Init.DataSize = SPI_DATASIZE_4BIT;
  spi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  spi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  spi3.Init.NSS = SPI_NSS_HARD_OUTPUT;
  spi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  spi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  spi3.Init.TIMode = SPI_TIMODE_DISABLE;
  spi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  spi3.Init.CRCPolynomial = 7;
  spi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  spi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&spi3) != HAL_OK)
  {
    Error_Handler();
  }
}
