/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		flash.c
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer flash memory interfacing functions.
 **/

#include "flash.h"

/* Register array */
volatile extern uint16_t regs[];

void FlashUpdate()
{

}

void LoadRegsFlash()
{

}

void FactoryReset()
{

}

void PrepareRegsForFlash()
{
	/* Goes through pages 253 and 254 to clear all volatile reg values */
	for(int addr = STATUS_REG; addr < (BUF_WRITE_0_REG - 1); addr++)
	{
		regs[addr] = 0;
	}
}

uint32_t CalcRegSig(uint16_t * regs, uint32_t count)
{
	/* Sig is just a sum (should more or less work for verifying flash contents) */
	uint32_t sig = 0;
	for(int i = 0; i<count; i++)
	{
		sig += regs[i];
	}
	return sig;
}
