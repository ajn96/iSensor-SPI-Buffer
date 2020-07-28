/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer register interfacing module. Called by user SPI and USB CLI
 **/

#include "registers.h"

/* Local function prototypes */
static uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue);

/* Index after last buffer output register (from buffer.c) */
extern uint32_t g_bufLastRegIndex;

/** Number of 32-bit words per buffer (from buffer.c) */
extern uint32_t g_bufNumWords32;

/** Register update flags for main loop processing. Global scope */
volatile uint32_t g_update_flags = 0;

/** Pointer to buffer entry. Will be 0 if no buffer entry "loaded" to output registers */
volatile uint16_t* g_CurrentBufEntry;

/** iSensor-SPI-Buffer global register array (read-able via SPI). Global scope */
uint16_t g_regs[3 * REG_PER_PAGE] __attribute__((aligned (32))) = {

/* Page 253 */
BUF_CONFIG_PAGE, /* 0x0 */
BUF_CONFIG_DEFAULT, /* 0x1 */
BUF_LEN_DEFAULT, /* 0x2 */
0x0000, /* 0x3 */
DIO_INPUT_CONFIG_DEFAULT, /* 0x4 */
DIO_OUTPUT_CONFIG_DEFAULT, /* 0x5 */
WATER_INT_CONFIG_DEFAULT, /* 0x6 */
ERROR_INT_CONFIG_DEFAULT, /* 0x7 */
IMU_SPI_CONFIG_DEFAULT, /* 0x8 */
USER_SPI_CONFIG_DEFAULT, /* 0x9 */
USB_CONFIG_DEFAULT, /* 0xA */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xB - 0xF */
0x0000, 0x0000, 0x0000, 0x0000, /* 0x10 - 0x13 */
FW_REV_DEFAULT, /* 0x14 */
0x0000, 0x0000, 0x0000, /* 0x15 - 0x17 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x18 - 0x1F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x20 - 0x27 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x28 - 0x2F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x30 - 0x37 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x38 - 0x3F */

/* Page 254 */
BUF_WRITE_PAGE, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, /* 0x40 - 0x47 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x48 - 0x4F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x50 - 0x57 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x58 - 0x5F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x60 - 0x67 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x68 - 0x6F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x70 - 0x77 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, FLASH_SIG_DEFAULT, /* 0x78 - 0x7F */

/* Page 255 */
BUF_READ_PAGE, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, /* 0x80 - 0x87 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x88 - 0x8F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x90 - 0x97 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x98 - 0x9F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xA0 - 0xA7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xA8 - 0xAF */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xB0 - 0xB7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xB8 - 0xBF */
};

/** Selected page. Starts on 253 (config page) */
static volatile uint32_t selected_page = BUF_CONFIG_PAGE;

/**
  * @brief Dequeues an entry from the buffer and loads it to the primary output registers
  *
  * @return void
  *
  * This function is called from the main loop to preserve SPI responsiveness while
  * a buffer entry is being dequeued into the output registers. This allows a user to read
  * the buffer contents while the values are being moved (if they start reading at buffer
  * entry 0). After moving all values to the correct location in the output register array,
  * the function sets up the burst read DMA (if enabled in user SPI config).
  */
void BufDequeueToOutputRegs()
{
	/* Get element from the buffer */
	g_CurrentBufEntry = (uint16_t *) BufTakeElement();

	/* Check if burst read mode is enabled */
	if(g_regs[BUF_CONFIG_REG] & BUF_CFG_BUF_BURST)
	{
		BurstReadSetup();
	}
}

/**
  * @brief Process a register read request (from master)
  *
  * @param regAddr The byte address of the register to read
  *
  * @return Value of register requested
  *
  * For selected pages not addressed by iSensor-SPI-Buffer, the read is
  * passed through to the connected IMU, using the spi_passthrough module.
  * If the selected page is [253 - 255] this read request is processed
  * directly.
  */
uint16_t ReadReg(uint8_t regAddr)
{
	uint16_t regIndex;
	uint16_t status;

	if(selected_page < BUF_CONFIG_PAGE)
	{
		return ImuReadReg(regAddr);
	}
	else
	{
		/* Find offset from page */
		regIndex = (selected_page - BUF_CONFIG_PAGE) * REG_PER_PAGE;
		/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
		regIndex += (regAddr >> 1);

		/* Handler buffer retrieve case by setting deferred processing flag */
		if(regIndex == BUF_RETRIEVE_REG)
		{
			/* Check if buf count > 0) */
			if(g_regs[BUF_CNT_0_REG] > 0)
			{
				/* Set update flag for main loop */
				g_update_flags |= DEQUEUE_BUF_FLAG;
			}
			else
			{
				/* Set current buf entry to 0 */
				g_CurrentBufEntry = 0;
			}
		}

		if(regIndex > BUF_RETRIEVE_REG)
		{
			if(g_CurrentBufEntry && (regIndex < g_bufLastRegIndex))
			{
				return g_CurrentBufEntry[regIndex - BUF_UTC_TIMESTAMP_REG];
			}
			else
			{
				return 0;
			}
		}

		/* Clear status upon read */
		if(regIndex == STATUS_0_REG || regIndex == STATUS_1_REG)
		{
			status = g_regs[STATUS_0_REG];
			g_regs[STATUS_0_REG] &= STATUS_CLEAR_MASK;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
			return status;
		}

		/* Load time stamp on demand upon read */
		if(regIndex == TIMESTAMP_LWR_REG)
			return GetMicrosecondTimestamp() & 0xFFFF;
		if(regIndex == TIMESTAMP_UPR_REG)
			return GetMicrosecondTimestamp() >> 16;

		/* get value from reg array */
		return g_regs[regIndex];
	}
}

/**
  * @brief Process a register write request (from master)
  *
  * @param regAddr The address of the register to write to
  *
  * @param regValue The value to write to the register
  *
  * @return The contents of the register being written, after write is processed
  *
  * For selected pages not addressed by iSensor-SPI-Buffer, the write is
  * passed through to the connected IMU, using the spi_passthrough module.
  * If the selected page is [253 - 255] this write request is processed
  * directly. The firmware echoes back the processed write value so that
  * the master can verify the write contents on the next SPI transaction.
  */
uint16_t WriteReg(uint8_t regAddr, uint8_t regValue)
{
	uint16_t regIndex;

	/* Handle page register writes first */
	if(regAddr == 0)
	{
		selected_page = regValue;
	}

	if(selected_page < BUF_CONFIG_PAGE)
	{
		/* Pass to IMU */
		return ImuWriteReg(regAddr, regValue);
	}
	else
	{
		/* Process reg write then return value from addressed register */
		regIndex = ProcessRegWrite(regAddr, regValue);
		/* get value from reg array */
		return g_regs[regIndex];
	}
}

/**
  * @brief Processes a command register write. This function is called from main loop.
  *
  * @return void
  *
  * Only one command can be executed per write to the USER_COMMAND register.
  * Command execution priority is determined by the order in which the command
  * flags are checked.
  */
void ProcessCommand()
{
	uint16_t command = g_regs[USER_COMMAND_REG];

	/* Clear command register */
	g_regs[USER_COMMAND_REG] = 0;

	/* Disable SPI for duration of command processing */
	SPI2->CR1 &= ~SPI_CR1_SPE;

	/* Set output pins low while running command */
	UpdateOutputPins(0, 0, 0);

	if(command & CMD_SOFTWARE_RESET)
	{
		NVIC_SystemReset();
	}
	else if(command & CMD_CLEAR_BUFFER)
	{
		BufReset();
	}
	else if(command & CMD_FLASH_UPDATE)
	{
		FlashUpdate();
	}
	else if(command & CMD_FACTORY_RESET)
	{
		FactoryReset();
	}
	else if(command & CMD_CLEAR_FAULT)
	{
		FlashLogError(ERROR_NONE);
		FlashCheckLoggedError();
	}
	else if(command & CMD_PPS_ENABLE)
	{
		EnablePPSTimer();
	}
	else if(command & CMD_PPS_DISABLE)
	{
		DisablePPSTimer();
	}

	/* Re-enable SPI */
	SPI2->CR1 |= SPI_CR1_SPE;
}

/**
  * @brief Load factory default values for all registers, and applies any settings changes.
  *
  * @return void
  *
  * This is accomplished in "lazy" manner via #define for each register
  * default value (defaults are stored in program memory, storage is managed
  * by compiler). This function only changes values in SRAM, does not change
  * flash contents (registers will reset on next re-boot).
  */
void FactoryReset()
{
	/* Store endurance count during factory reset */
	uint16_t endurance, flash_sig;

	/* Disable data capture from IMU (shouldn't be running, but better safe than sorry) */
	DisableDataCapture();

	/* Reset selected page */
	selected_page = BUF_CONFIG_PAGE;

	/* Save endurance and flash sig */
	endurance = g_regs[ENDURANCE_REG];
	flash_sig = g_regs[FLASH_SIG_REG];

	/* Reset all registers to 0 */
	for(int i = 0; i < (3 * REG_PER_PAGE); i++)
	{
		g_regs[i] = 0;
	}

	/* Restore page number registers (addr 0 on each page) */
	g_regs[0 * REG_PER_PAGE] = BUF_CONFIG_PAGE;
	g_regs[1 * REG_PER_PAGE] = BUF_WRITE_PAGE;
	g_regs[2 * REG_PER_PAGE] = BUF_READ_PAGE;

	/* Restore all non-zero default values */
	g_regs[BUF_CONFIG_REG] = BUF_CONFIG_DEFAULT;
	g_regs[BUF_LEN_REG] = BUF_LEN_DEFAULT;
	g_regs[DIO_INPUT_CONFIG_REG] = DIO_INPUT_CONFIG_DEFAULT;
	g_regs[DIO_OUTPUT_CONFIG_REG] = DIO_OUTPUT_CONFIG_DEFAULT;
	g_regs[WATERMARK_INT_CONFIG_REG] = WATER_INT_CONFIG_DEFAULT;
	g_regs[ERROR_INT_CONFIG_REG] = ERROR_INT_CONFIG_DEFAULT;
	g_regs[IMU_SPI_CONFIG_REG] = IMU_SPI_CONFIG_DEFAULT;
	g_regs[USER_SPI_CONFIG_REG] = USER_SPI_CONFIG_DEFAULT;
	g_regs[FW_REV_REG] = FW_REV_DEFAULT;
	g_regs[USB_CONFIG_REG] = USB_CONFIG_DEFAULT;

	/* Apply endurance and flash sig back */
	g_regs[ENDURANCE_REG] = endurance;
	g_regs[FLASH_SIG_REG] = flash_sig;

	/* Calculate new flash sig */
	g_regs[FLASH_SIG_DRV_REG] = CalcRegSig((uint16_t*)g_regs, FLASH_SIG_DRV_REG - 1);;

	/* Populate SN and build date */
	GetSN();
	GetBuildDate();

	/* Apply all settings and reset buffer */
	UpdateImuSpiConfig();
	UpdateUserSpiConfig();
	UpdateDIOOutputConfig();
	UpdateDIOInputConfig();
	BufReset();

	/* Load logged error status from flash */
	FlashCheckLoggedError();
}

/**
  * @brief Populates the six SN registers automatically
  *
  * @return void
  *
  * The SN registers are populated from the 96-bit unique ID (UID)
  */
void GetSN()
{
	uint16_t id;
	for(uint8_t i = 0; i < 12; i = i + 2)
	{
		id = *(volatile uint8_t *)(UID_BASE + i);
		id |= (*(volatile uint8_t *)(UID_BASE + i + 1)) << 8;
		g_regs[DEV_SN_REG + (i >> 1)] = id;
	}
}

/**
  * @brief Populates the firmware date registers automatically
  *
  * @return void
  *
  * Registers are populated by parsing the __DATE__ macro result, which
  * is set at compile time
  */
void GetBuildDate()
{
	uint8_t date[11] = __DATE__;

	uint16_t year;
	uint8_t day;
	uint8_t month;

	/* Pre-process date */
	for(uint32_t i = 3; i<11; i++)
	{
		date[i] = date[i] - '0';
	}

	/* Get year */
	year = (date[7] << 12) | (date[8] << 8) | (date[9] << 4) | date[10];

	/* Get day */
	day = (date[4] << 4) | date[5];

	/* Get month */
	switch(date[0])
	{
	case 'J':
		if(date[1] == 'a' && date[2] == 'n')
			month = 0x01;
		else if(date[1] == 'u' && date[2] == 'n')
			month = 0x06;
		else
			month = 0x07;
		break;
	case 'F':
		month = 0x02;
		break;
	case 'M':
		if(date[2] == 'r')
			month = 0x03;
		else
			month = 0x05;
		break;
	case 'A':
		if(date[1] == 'p')
			month = 0x04;
		else
			month = 0x08;
		break;
	case 'S':
		month = 0x09;
		break;
	case 'O':
		month = 0x10;
		break;
	case 'N':
		month = 0x11;
		break;
	case 'D':
		month = 0x12;
		break;
	default:
		/* shouldnt get here */
		month = 0x00;
		break;
	}

	g_regs[FW_DAY_MONTH_REG] = (day << 8) | month;
	g_regs[FW_YEAR_REG] = year;

	/* Also load FW rev here just in case */
	g_regs[FW_REV_REG] = FW_REV_DEFAULT;
}

/**
  * @brief Process a write to the iSensor-SPI-Buffer registers
  *
  * @return The index to the register within the global register array
  *
  * This function handles filtering for read-only registers. It also handles
  * setting the deferred processing flags as needed for any config/command register
  * writes. This are processed on the next pass of the main loop.
  */
static uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue)
{
	/* Index within the register array */
	uint32_t regIndex;

	/* Value to write to the register */
	uint16_t regWriteVal;

	/* Track if write is to the upper word of register */
	uint32_t isUpper;

	/* Find offset from page */
	regIndex = (selected_page - BUF_CONFIG_PAGE) * REG_PER_PAGE;

	/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
	regIndex += (regAddr >> 1);

	/* If page register write then enable/disable capture as needed */
	if(regAddr < 2)
	{
		if(selected_page == BUF_READ_PAGE)
			g_update_flags |= ENABLE_CAPTURE_FLAG;
		else
			DisableDataCapture();

		/* Return reg index (points to page reg instance) */
		return regIndex;
	}

	/* Ignore writes to out of bound or read only registers */
	if(selected_page == BUF_CONFIG_PAGE)
	{
		/* Max count is read-only */
		if(regIndex == BUF_MAX_CNT_REG)
			return regIndex;

		/* Last writable reg on config page */
		if(regIndex > USER_SCR_7_REG)
		{
			/* Allow writes to the UTC timestamp regs */
			if((regIndex != UTC_TIMESTAMP_LWR_REG)&&(regIndex != UTC_TIMESTAMP_UPR_REG))
			{
				return regIndex;
			}
		}
	}
	else if(selected_page == BUF_WRITE_PAGE)
	{
		/* regs under write data are reserved */
		if(regIndex < BUF_WRITE_0_REG)
			return regIndex;

		/* regs over write data are reserved */
		if(regIndex > (BUF_WRITE_0_REG + 31))
			return regIndex;
	}
	else if(selected_page == BUF_READ_PAGE)
	{
		/* Buffer output registers / buffer retrieve are read only */
		if(regIndex == BUF_CNT_1_REG)
		{
			if(regValue == 0)
			{
				/* Clear buffer for writes of 0 to count */
				BufReset();
				return regIndex;
			}
			else
			{
				/* Ignore non-zero writes */
				return regIndex;
			}
		}
		else
		{
			/* Can't write any other registers on page */
			return regIndex;
		}
	}
	else
	{
		/* Block all other pages */
		return regIndex;
	}

	/* Find if writing to upper or lower */
	isUpper = regAddr & 0x1;

	/* Any registers which require filtering or special actions in main loop */
	if(regIndex == IMU_SPI_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update IMU spi config */
			g_update_flags |= IMU_SPI_CONFIG_FLAG;
		}
	}
	else if(regIndex == USER_SPI_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update user spi config */
			g_update_flags |= USER_SPI_CONFIG_FLAG;
		}
	}
	else if(regIndex == DIO_OUTPUT_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update DIO output config */
			g_update_flags |= DIO_OUTPUT_CONFIG_FLAG;
		}
	}
	else if(regIndex == DIO_INPUT_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update DIO input config */
			g_update_flags |= DIO_INPUT_CONFIG_FLAG;
		}
	}
	else if(regIndex == USER_COMMAND_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to process command */
			g_update_flags |= USER_COMMAND_FLAG;
		}
	}

	/* Get initial register value */
	regWriteVal = g_regs[regIndex];

	/* Perform write to reg array */
	if(isUpper)
	{
		/* Write upper reg byte */
		regWriteVal &= 0x00FF;
		regWriteVal |= (regValue << 8);
	}
	else
	{
		/* Write lower reg byte */
		regWriteVal &= 0xFF00;
		regWriteVal |= regValue;
	}
	g_regs[regIndex] = regWriteVal;

	/* Check for buffer reset actions which should be performed in ISR */
	if(regIndex == BUF_CONFIG_REG || regIndex == BUF_LEN_REG)
	{
		if(isUpper)
		{
			/* Reset the buffer after writing upper half of register (applies new settings) */
			BufReset();
		}
	}
	return regIndex;
}
