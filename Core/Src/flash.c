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

/**
  * @brief Flash update command handler
  *
  * @return void
  *
  * Saves all non-volatile registers to flash memory. They will be retrieved
  * on the next system boot. Also saves a signature of the registers, which
  * is used for register contents validation on the next boot.
  */
void FlashUpdate()
{

}

/**
  * @brief Load register values from flash memory to SRAM
  *
  * @return void
  *
  * After loading registers from flash, the signature of the
  * loaded registers is compared to the signature stored in
  * flash. If there is a mis-match, the STATUS FLASH_ERROR bit
  * is set.
  */
void LoadRegsFlash()
{
	uint32_t expectedSig;
	uint32_t storedSig;


}

/**
  * @brief Clears all non-volatile registers in RAM
  *
  * @return void
  */
void PrepareRegsForFlash()
{
	/* Goes through pages 253 and 254 to clear all volatile reg values */
	for(int addr = STATUS_REG; addr < (BUF_WRITE_0_REG - 1); addr++)
	{
		regs[addr] = 0;
	}
}

/**
  * @brief Calculate a signature of all register contents in RAM.
  *
  * @return void
  */
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
