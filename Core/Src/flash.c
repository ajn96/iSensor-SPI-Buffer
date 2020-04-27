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
	/* Prepare for flash */
	PrepareRegsForFlash();

	/* Write register values to flash */
}

/**
  * @brief Load register values from flash memory to SRAM
  *
  * @return void
  *
  * After loading registers from flash, the signature of the
  * loaded registers is compared to the signature stored in
  * flash. If there is a mis-match, the STATUS FLASH_ERROR bit
  * is set. Register values on page 253 - 254 are stored.
  */
void LoadRegsFlash()
{
	uint32_t expectedSig;
	uint32_t storedSig;

	/* Load regs to SRAM */

	/* Calc expected sig (stop before flash signature register) */
	expectedSig = CalcRegSig((uint16_t*)regs, FLASH_SIG_REG - 1);

	/* Read flash sig value which was loaded */
	storedSig = regs[FLASH_SIG_REG];

	/* Alert user of potential flash error */
	if(storedSig != expectedSig)
		regs[STATUS_REG] |= STATUS_FLASH_ERROR;
}

/**
  * @brief Clears all non-volatile registers in RAM and updates reg signature
  *
  * @return void
  */
void PrepareRegsForFlash()
{
	/* Goes through pages 253 to clear all volatile reg values, starting at STATUS */
	for(int addr = STATUS_REG; addr < REG_PER_PAGE; addr++)
	{
		regs[addr] = 0;
	}

	/* Increment endurance counter */
	regs[ENDURANCE_REG] = regs[ENDURANCE_REG] + 1;

	/* Calc sig and store back to SRAM */
	regs[FLASH_SIG_REG] = CalcRegSig((uint16_t*)regs, FLASH_SIG_REG - 1);
}

/**
  * @brief Calculate a signature of a block of RAM. Used for verifying flash memory contents
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
