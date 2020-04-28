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

/* Local function prototypes */
static void PrepareRegsForFlash();

/* Register array */
volatile extern uint16_t regs[];

/**
  * @brief Flash update command handler
  *
  * @return void
  *
  * Saves all non-volatile registers to flash memory. They will be retrieved
  * on the next system boot. Also saves a signature of the registers, which
  * is used for register contents validation on the next boot. Registers are
  * backed up to flash starting at an offset of 62KB in flash memory
  */
void FlashUpdate()
{
	/* Error code from flash erase page */
	uint32_t error;

	/* struct to erase flash page through HAL */
	FLASH_EraseInitTypeDef EraseInitStruct;

	/* Addr to write in flash */
	uint32_t flashAddr;

	/* Unlock flash. If fails abort the flash update */
	if(HAL_FLASH_Unlock() != HAL_OK)
	{
		regs[STATUS_0_REG] |= STATUS_FLASH_UPDATE;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
		return;
	}

	/* Prepare registers for write */
	PrepareRegsForFlash();

	/* Erase page */
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = FLASH_BASE_ADDR;
	EraseInitStruct.NbPages = 1;
	HAL_FLASHEx_Erase(&EraseInitStruct, &error);

	/* Check error code */
	if(error != 0xFFFFFFFF)
	{
		/* Flag error and try to finish flash write */
		regs[STATUS_0_REG] |= STATUS_FLASH_UPDATE;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}

	/* Write all values */
	flashAddr = FLASH_BASE_ADDR;
	for(int regAddr = 0; regAddr <= FLASH_SIG_REG; regAddr++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flashAddr, regs[regAddr]);
		flashAddr += 2;
	}

	/* Lock the flash */
	HAL_FLASH_Lock();

	/* Restore SN/Date register values in SRAM */
	GetSN();
	GetBuildDate();

	/* Copy BUF_CNT and STATUS from last page (not stored to flash) */
	regs[STATUS_0_REG] = regs[STATUS_1_REG];
	regs[BUF_CNT_0_REG] = regs[BUF_CNT_1_REG];
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
	uint16_t * flashAddr = (uint16_t *) FLASH_BASE_ADDR;
	for(int regAddr = 0; regAddr <= FLASH_SIG_REG; regAddr++)
	{
		regs[regAddr] = (*flashAddr) & 0xFFFF;
		flashAddr++;
	}

	/* Calc expected sig (stop before flash signature register) */
	expectedSig = CalcRegSig((uint16_t*)regs, FLASH_SIG_REG - 1);

	/* Read flash sig value which was loaded */
	storedSig = regs[FLASH_SIG_REG];

	/* Perform factory reset and alert user of flash error in case of sig mis-match */
	if(storedSig != expectedSig)
	{
		FactoryReset();
		regs[STATUS_0_REG] |= STATUS_FLASH_ERROR;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
	}
}

/**
  * @brief Calculate a signature of a block of RAM. Used for verifying flash memory contents
  *
  * @return void
  */
uint16_t CalcRegSig(uint16_t * regs, uint32_t count)
{
	/* Sig is just a sum (should more or less work for verifying flash contents) */
	uint16_t sig = 0;
	for(int i = 0; i<count; i++)
	{
		sig += regs[i];
	}
	return sig;
}

/**
  * @brief Clears all non-volatile registers in RAM and updates reg signature
  *
  * @return void
  */
static void PrepareRegsForFlash()
{
	/* Clear all volatile regs (STATUS, BUF_CNT, day/year, dev SN regs) */
	for(int addr = STATUS_0_REG; addr <= (DEV_SN_REG + 5); addr++)
	{
		regs[addr] = 0;
	}

	/* Increment endurance counter */
	regs[ENDURANCE_REG]++;

	/* Calc sig and store back to SRAM */
	regs[FLASH_SIG_REG] = CalcRegSig((uint16_t*)regs, FLASH_SIG_REG - 1);
}
