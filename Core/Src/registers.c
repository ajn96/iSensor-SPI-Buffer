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

/* Selected page. Starts on 253 (config page) */
volatile uint8_t selected_page = 253;

/* Register update flags for main loop processing */
volatile uint32_t update_flags = 0;

volatile uint16_t regs[3 * REG_PER_PAGE] = {
/* Page 253 */
0x00FD, 0x0000, 0x0014, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* Page 254 */
0x00FE, 0x0000, 0x0000, 0x0200, 0x0600, 0x0A00, 0x0E00, 0x1200,
0x1600, 0x1A00, 0x1C00, 0x2200, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* Page 255 */
0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

uint16_t ReadReg(uint8_t regAddr)
{
	uint16_t regIndex;

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

		/* Handler buffer retrieve case */
		if(regIndex == BUF_RETRIEVE_REG)
		{
			//TODO
		}

		/* get value from reg array */
		return regs[regIndex];
	}
}

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

uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue)
{
	/* Index within the register array */
	uint16_t regIndex;

	/* Find offset from page */
	regIndex = (selected_page - BUF_CONFIG_PAGE) * REG_PER_PAGE;

	/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
	regIndex += (regAddr >> 1);

	/* If page register write then ignore (handled higher up) */
	if(regAddr < 2)
		return regIndex;

	/* Ignore writes to out of bound or read only registers */
	if(selected_page == BUF_CONFIG_PAGE)
	{
		/* Max count is read-only */
		if(regIndex == BUF_MAX_CNT_REG)
			return regIndex;

		/* Last reg on config page */
		if(regIndex > USER_SCR_3_REG)
			return regIndex;
	}
	else if(selected_page == BUF_WRITE_PAGE)
	{
		/* outside writable regs */
		if(regIndex > (BUF_WRITE_0_REG + 31))
			return regIndex;

		/* reg 1 and 2 also reserved */
		if(regIndex == 1 || regIndex == 2)
			return regIndex;
	}
	else if(selected_page == BUF_READ_PAGE)
	{
		/* Buffer output registers / buffer retrieve are read only */
		if(regIndex > BUF_CNT_REG)
			return regIndex;
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
	else if(regIndex == IMU_DR_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update IMU data ready config */
			update_flags |= IMU_DR_CONFIG_FLAG;
		}
	}
	else if(regIndex == USER_DR_CONFIG_REG)
	{
		if(isUpper)
		{
			/* Need to set a flag to update user data ready config */
			update_flags |= USER_DR_CONFIG_FLAG;
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
	else if((regIndex == BUF_CNT_REG) && (regValue == 0))
	{
		/* Writing zero to buffer count will also reset the buffer */
		BufReset();
	}

	return regIndex;
}

void UpdateImuDrConfig()
{

}

void UpdateImuSpiConfig()
{

}

void UpdateUserDrConfig()
{

}

void UpdateUserSpiConfig()
{

}

void ProcessCommand()
{
	uint16_t command = regs[USER_COMMAND_REG];

	/* Clear command register */
	regs[USER_COMMAND_REG] = 0;

	//TODO: Disable SPI for duration of command processing

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
		//TODO
	}
	else if(command & FACTORY_RESET)
	{
		//TODO
	}
}

