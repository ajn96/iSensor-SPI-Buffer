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
	/* Exit if script not running */
	if(!scriptRunning)
	{
		return;
	}

	/* Execute current command and write to file */
}

/**
  * @brief Check if SD card is attached
  *
  * @return true if card is attached, false otherwise
  *
  * This function makes use of the SD card detect pin to
  * determine if an SD card is inserted without communicating
  * to the SD card directly.
  */
static bool SDCardAttached()
{

}

static bool OpenScriptFiles()
{

}

static bool ParseScriptFile()
{

}

/**
  * @brief SPI3 Initialization Function (SD card master SPI port)
  *
  * @param None
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
