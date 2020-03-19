/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		registers.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer register interfacing
 **/

#include "reg_definitions.h"
#include "registers.h"
#include "SpiPassthrough.h"

/* Selected page. Starts on 253 (config page) */
volatile uint8_t selected_page = 253;

volatile uint16_t regs[3 * REG_PER_PAGE] = {
/* Page 253 */
0x00FD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* Page 254 */
0x00FE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
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
		/* get value from reg array */
		return regs[regIndex];
	}
}

uint16_t WriteReg(uint8_t regAddr, uint8_t regValue)
{
	uint16_t regIndex;

	/* Check page first */
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
		/* Process reg write then return value written */
		regIndex = ProcessRegWrite(regAddr, regValue);
		/* get value from reg array */
		return regs[regIndex];
	}
}

uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue)
{
	uint16_t regIndex;
	/* Find offset from page */
	regIndex = (selected_page - BUF_CONFIG_PAGE) * REG_PER_PAGE;
	/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
	regIndex += (regAddr >> 1);

	/* If page register write then ignore (handled higher up) */
	if(regAddr < 2)
		return regIndex;

	/* Ignore writes to out of bounds registers */
	if(selected_page == BUF_CONFIG_PAGE)
	{
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
		if(regIndex > (BUF_READ_0_REG + 31))
			return regIndex;
	}

	/* Find if writing to upper or lower */
	uint32_t isUpper = regAddr & 0x1;

	/* Get initial register value */
	uint16_t regWriteVal = regs[regIndex];

	switch(regIndex)
	{
	/* Any registers which require filtering or special actions */

	/* General registers */
	default:
		if(isUpper)
		{
			regWriteVal &= 0x00FF;
			regWriteVal |= (regValue << 8);
		}
		else
		{
			regWriteVal &= 0xFF00;
			regWriteVal |= regValue;
		}
		regs[regIndex] = regWriteVal;
		break;
	}
	return regIndex;
}
