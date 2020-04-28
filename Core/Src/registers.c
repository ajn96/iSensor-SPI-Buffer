/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer register interfacing
 **/

#include "registers.h"

/* Local function prototypes */
static uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue);

/* User SPI handle (from main) */
extern SPI_HandleTypeDef hspi2;

/** Selected page. Starts on 253 (config page) */
volatile uint8_t selected_page = BUF_CONFIG_PAGE;

/** Register update flags for main loop processing */
volatile uint32_t update_flags = 0;

/** iSensor-SPI-Buffer global register array (read-able via SPI) */
uint16_t regs[3 * REG_PER_PAGE] = {
/* Page 253 */

/*0							1						2					3			4					5						6						7 */
BUF_CONFIG_PAGE, 			BUF_CONFIG_DEFAULT, 	BUF_LEN_DEFAULT, 	0x0000, 	DR_CONFIG_DEFAULT, 	DIO_CONFIG_DEFAULT, 	INT_CONFIG_DEFAULT, 	IMU_SPI_CONFIG_DEFAULT, /* PAGE_ID - IMU_SPI_CONFIG */
USER_SPI_CONFIG_DEFAULT, 	0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000, 				0x0000, 				0x0000, /* USER_SPI_CONFIG_DEFAULT - USER_SCR_3 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	FW_REV_DEFAULT, 	0x0000,	 				0x0000, 				0x0000, /* FW_REV - BUF_CNT_0_REG */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* FW_DAY_MONTH - DEV_SN_5 */

/* Page 254 */
BUF_WRITE_PAGE, 			0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000, 				0x0000, 				0x0000, /* PAGE_ID - BUF_WRITE_3 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_WRITE_4 - BUF_WRITE_11 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_WRITE_12 - BUF_WRITE_19 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_WRITE_20 - BUF_WRITE_27 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_WRITE_28 - BUF_WRITE_31 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000, 				0x0000, 				FLASH_SIG_DEFAULT, /* FLASH_SIG at end */

/* Page 255 */
BUF_READ_PAGE, 				0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000, 				0x0000, 				0x0000, /* PAGE_ID - BUF_DATA_3 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_DATA_4 - BUF_DATA_11 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_DATA_12 - BUF_DATA_19 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_DATA_20 - BUF_DATA_27 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000, /* BUF_DATA_28 - BUF_DATA_31 */
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000,
0x0000, 					0x0000, 				0x0000, 			0x0000, 	0x0000, 			0x0000,					0x0000, 				0x0000};

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
	uint8_t* bufEntry;
	uint8_t* bufOutput;
	uint32_t* outputRegs;

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

		/* Handler buffer retrieve case (todo: something faster/safer here) */
		if(regIndex == BUF_RETRIEVE_REG)
		{
			/* Check if buf count > 0) */
			if(regs[BUF_CNT_0_REG] > 0)
			{
				/* Get element from the buffer */
				bufEntry = BufTakeElement();

				/* Copy buffer data to output registers */
				bufOutput = (uint8_t *) &regs[BUF_DATA_0_REG];
				for(uint32_t i = 0; i < regs[BUF_LEN_REG]; i++)
				{
					bufOutput[i] = bufEntry[i];
				}

				/* Burst read mode is enabled */
				if(regs[USER_SPI_CONFIG_REG] & SPI_CONF_BURST_RD)
				{
					BurstReadSetup();
				}
			}
			else
			{
				/* Clear regs */
				outputRegs = (uint32_t *) &regs[BUF_DATA_0_REG];
				for(uint32_t i = 0; i < 16; i++)
				{
					outputRegs[i] = 0;
				}
			}
		}

		/* Clear status upon read */
		if(regIndex == STATUS_0_REG || regIndex == STATUS_1_REG)
		{
			status = regs[STATUS_0_REG];
			regs[STATUS_0_REG] &= STATUS_CLEAR_MASK;
			regs[STATUS_1_REG] = regs[STATUS_0_REG];
			return status;
		}

		/* get value from reg array */
		return regs[regIndex];
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
		return regs[regIndex];
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
	uint16_t command = regs[USER_COMMAND_REG];

	/* Clear command register */
	regs[USER_COMMAND_REG] = 0;

	/* Disable SPI for duration of command processing */
	SPI2->CR1 &= ~SPI_CR1_SPE;

	/* Set output pins low while running command */
	UpdateOutputPins(0, 0);

	if(command & SOFTWARE_RESET)
	{
		NVIC_SystemReset();
	}
	else if(command & CLEAR_BUFFER)
	{
		BufReset();
	}
	else if(command & FLASH_UPDATE)
	{
		FlashUpdate();
	}
	else if(command & FACTORY_RESET)
	{
		FactoryReset();
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
	uint16_t endurance;

	/* Disable data capture from IMU (shouldn't be running, but better safe than sorry) */
	DisableDataCapture();

	/* Reset selected page */
	selected_page = BUF_CONFIG_PAGE;

	/* Save endurance */
	endurance = regs[ENDURANCE_REG];

	/* Reset all registers to 0 */
	for(int i = 0; i < (3 * REG_PER_PAGE); i++)
	{
		regs[i] = 0;
	}

	/* Restore page number registers (addr 0 on each page) */
	regs[0 * REG_PER_PAGE] = BUF_CONFIG_PAGE;
	regs[1 * REG_PER_PAGE] = BUF_WRITE_PAGE;
	regs[2 * REG_PER_PAGE] = BUF_READ_PAGE;

	/* Restore all non-zero default values */
	regs[BUF_CONFIG_REG] = BUF_CONFIG_DEFAULT;
	regs[BUF_LEN_REG] = BUF_LEN_DEFAULT;
	regs[DR_CONFIG_REG] = DR_CONFIG_DEFAULT;
	regs[DIO_CONFIG_REG] = DIO_CONFIG_DEFAULT;
	regs[INT_CONFIG_REG] = INT_CONFIG_DEFAULT;
	regs[IMU_SPI_CONFIG_REG] = IMU_SPI_CONFIG_DEFAULT;
	regs[USER_SPI_CONFIG_REG] = USER_SPI_CONFIG_DEFAULT;
	regs[FW_REV_REG] = FW_REV_DEFAULT;
	regs[FLASH_SIG_REG] = FLASH_SIG_DEFAULT;

	/* Apply endurance back */
	regs[ENDURANCE_REG] = endurance;

	/* Populate SN and build date */
	GetSN();
	GetBuildDate();

	/* Apply all settings and reset buffer */
	UpdateImuSpiConfig();
	UpdateUserSpiConfig();
	UpdateDIOConfig();
	UpdateDRConfig();
	BufReset();
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
		regs[DEV_SN_REG + (i >> 1)] = id;
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

	regs[FW_DAY_MONTH_REG] = (day << 8) | month;
	regs[FW_YEAR_REG] = year;
}

/**
  * @brief Updates the slave SPI (SPI2) config based on the USER_SPI_CONFIG register
  *
  * @return void
  *
  * This function performs all needed initialization for the slave SPI port, and should
  * be called as start of the firmware start up process.
  */
void UpdateUserSpiConfig()
{
	uint16_t config = regs[USER_SPI_CONFIG_REG];

	/* mask unused bits */
	config &= SPI_CONF_MASK;

	/* CPHA */
	if(config & SPI_CONF_CPHA)
		hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
	else
		hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;

	/* CPOL */
	if(config & SPI_CONF_CPOL)
		hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
	else
		hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;

	/* Bit order */
	if(config & SPI_CONF_MSB_FIRST)
		hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	else
		hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;

	/* Reset all immutable settings */
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_SLAVE;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 7;
	hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

	/* De-init SPI */
	HAL_SPI_DeInit(&hspi2);

	/* Init */
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start user SPI interrupt processing (Rx and error) */
	__HAL_SPI_ENABLE_IT(&hspi2, (SPI_IT_RXNE | SPI_IT_ERR));

	/* Set threshold for 16 bit mode */
	CLEAR_BIT(hspi2.Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((hspi2.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(&hspi2);
	}

	/* Load output with initial zeros */
	hspi2.Instance->DR = 0;

	/* Set user SPI interrupt priority */
	HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);

	/* Enable user SPI interrupts */
	HAL_NVIC_EnableIRQ(SPI2_IRQn);

	/* Apply config value in use back */
	regs[USER_SPI_CONFIG_REG] = config;
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
	uint16_t regIndex;

	/* Find offset from page */
	regIndex = (selected_page - BUF_CONFIG_PAGE) * REG_PER_PAGE;

	/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
	regIndex += (regAddr >> 1);

	/* If page register write then enable/disable capture as needed */
	if(regAddr < 2)
	{
		if(selected_page == BUF_READ_PAGE)
			EnableDataCapture();
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
		if(regIndex > USER_SCR_3_REG)
			return regIndex;
	}
	else if(selected_page == BUF_WRITE_PAGE)
	{
		/* outside writable regs */
		if(regIndex > (BUF_WRITE_0_REG + 31))
			return regIndex;

		/* reg 1, 2, 3 also reserved */
		if(regIndex <= 3)
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
	uint32_t isUpper = regAddr & 0x1;

	/* Any registers which require filtering or special actions in main loop */
	if(regIndex == IMU_SPI_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update IMU spi config */
			update_flags |= IMU_SPI_CONFIG_FLAG;
		}
	}
	else if(regIndex == USER_SPI_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update user spi config */
			update_flags |= USER_SPI_CONFIG_FLAG;
		}
	}
	else if(regIndex == DIO_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update DIO config */
			update_flags |= DIO_CONFIG_FLAG;
		}
	}
	else if(regIndex == DR_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update data ready config */
			update_flags |= DR_CONFIG_FLAG;
		}
	}
	else if(regIndex == USER_COMMAND_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to process command */
			update_flags |= USER_COMMAND_FLAG;
		}
	}

	/* Get initial register value */
	uint16_t regWriteVal = regs[regIndex];

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
	regs[regIndex] = regWriteVal;

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
