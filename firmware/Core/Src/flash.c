/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		flash.c
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer flash memory interfacing functions.
 **/

#include "reg.h"
#include "flash.h"
#include "stm32f3xx_hal.h"

/* Local function prototypes */
static void PrepareRegsForFlash();
static uint16_t CalcRegSig(uint16_t * regs, uint32_t count);

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
void Flash_Update()
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
		g_regs[STATUS_0_REG] |= STATUS_FLASH_UPDATE;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		return;
	}

	/* Prepare registers for write (clears status on 253) */
	PrepareRegsForFlash();

	/* Erase page */
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = FLASH_REG_ADDR;
	EraseInitStruct.NbPages = 1;
	HAL_FLASHEx_Erase(&EraseInitStruct, &error);

	/* Check error code */
	if(error != 0xFFFFFFFF)
	{
		/* Flag error and try to finish flash write */
		g_regs[STATUS_1_REG] |= STATUS_FLASH_UPDATE;
	}

	/* Write all values */
	flashAddr = FLASH_REG_ADDR;
	for(int regAddr = (1 * REG_PER_PAGE); regAddr <= FLASH_SIG_REG; regAddr++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flashAddr, g_regs[regAddr]);
		flashAddr += 2;
	}

	/* Lock the flash */
	HAL_FLASH_Lock();

	/* Restore SN/Date register values in SRAM */
	Reg_Update_Identifiers();

	/* Copy BUF_CNT and STATUS from last page to config page (last page not stored to flash) */
	g_regs[STATUS_0_REG] = g_regs[STATUS_1_REG];
	g_regs[BUF_CNT_0_REG] = g_regs[BUF_CNT_1_REG];

	/* Restore fault code register */
	Flash_Check_Logged_Fault();
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
void Flash_Load_Registers()
{
	uint16_t expectedSig;
	uint16_t storedSig;

	/* Load regs to SRAM */
	uint16_t* flashAddr = (uint16_t *) FLASH_REG_ADDR;
	for(int regAddr = (1 * REG_PER_PAGE); regAddr <= FLASH_SIG_REG; regAddr++)
	{
		g_regs[regAddr] = (*flashAddr) & 0xFFFF;
		flashAddr++;
	}

	/* Calc expected sig (stop before flash signature register) */
	expectedSig = CalcRegSig((uint16_t *)&g_regs[1 * REG_PER_PAGE], (2 * REG_PER_PAGE) - 2);

	/* Store sig */
	g_regs[FLASH_SIG_DRV_REG] = expectedSig;

	/* Read flash sig value which was stored at last flash update */
	storedSig = g_regs[FLASH_SIG_REG];

	/* Alert user of flash error in case of sig mis-match */
	if(storedSig != expectedSig)
	{
		g_regs[STATUS_0_REG] |= STATUS_FLASH_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}

	/* Make sure endurance loads as 0 */
	if(g_regs[ENDURANCE_REG] == 0xFFFF)
	{
		g_regs[ENDURANCE_REG] = 0;
	}

	Flash_Check_Logged_Fault();
}

/**
  * @brief Checks for error codes which may have been logged to flash, and sets the status register bit
  *
  * @return void
  *
  * Loads the previously stored error code from flash memory to the designated
  * flash error register. If the error code is non-zero, sets the status register fault bit.
  */
void Flash_Check_Logged_Fault()
{
	/* Load error code from flash */
	uint16_t errorCode = (*(uint16_t *) FLASH_ERROR_ADDR) & 0x001F;

	g_regs[FAULT_CODE_REG] = errorCode;
	if(g_regs[FAULT_CODE_REG] != 0)
	{
		g_regs[STATUS_0_REG] |= STATUS_FAULT;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
	else
	{
		g_regs[STATUS_0_REG] &= ~STATUS_FAULT;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
}

/**
  * @brief Stores an error code to flash memory
  *
  * @param faultCode The error code to store. Should be one of the defined fault types
  *
  * @return void
  *
  * This function is called when a fatal error is encountered by the system. Data is logged
  * about the source of the error, and then a system reset is triggered. Once an error has
  * been logged, it will be persistent until a fault code clear command is executed. We also
  * don't care about error handling within this routine - as soon as it returns, the system will
  * be reset.
  */
void Flash_Log_Fault(uint32_t faultCode)
{
	/* struct to erase flash page through HAL */
	FLASH_EraseInitTypeDef EraseInitStruct;

	/* error code from page erase */
	uint32_t error;

	/* Initial error code */
	uint16_t errorCode = (*(uint16_t *) FLASH_ERROR_ADDR) & 0x001F;

	/* Add in fault code (if non-zero) */
	if(faultCode)
	{
		errorCode |= faultCode;
	}
	else
	{
		errorCode = 0;
	}

	/* Unlock flash */
	HAL_FLASH_Unlock();

	/* Erase error page */
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = FLASH_ERROR_ADDR;
	EraseInitStruct.NbPages = 1;
	HAL_FLASHEx_Erase(&EraseInitStruct, &error);

	/* write the error code */
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_ERROR_ADDR, errorCode);

	/* Lock flash */
	HAL_FLASH_Lock();

	/* Goodbye, cruel world... */
}

/**
  * @brief Initialize flash error logging
  *
  * @return void
  *
  * This function is intended to be called as part of the initialization process. It checks
  * the validity of stored error log. If the log is invalid, error logging has not
  * been initialized, and the log is cleared. Updates the FAULT_CODE and STATUS registers.
  */
void Flash_Fault_Log_Init()
{
	/* Initial error code */
	uint16_t errorCode = (*(uint16_t *) FLASH_ERROR_ADDR) & 0xFFFF;

	if(errorCode & 0xFFE0)
	{
		/* Unused bits are set in the error code. Probably hasn't been initialized properly */
		Flash_Log_Fault(ERROR_NONE);
	}
}

/**
  * @brief Calculate a signature of a block of RAM. Used for verifying flash memory contents
  *
  * @return void
  */
static uint16_t CalcRegSig(uint16_t * regs, uint32_t count)
{
	/* Sig is just a sum (should more or less work for verifying flash contents) */
	uint16_t sig = 0;
	for(uint32_t i = 0; i<count; i++)
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
	/* Save endurance and buf max count value */
	uint16_t endur = g_regs[ENDURANCE_REG];
	uint16_t maxCount = g_regs[BUF_MAX_CNT_REG];

	/* Clear all volatile regs (Everything after UTC time) */
	for(int addr = UTC_TIMESTAMP_LWR_REG; addr < (DEV_SN_REG + 6); addr++)
	{
		g_regs[addr] = 0;
	}

	/* Clear volatile bits in CLI_CONFIG */
	g_regs[CLI_CONFIG_REG] &= CLI_CONFIG_CLEAR_MASK;

	/* Store new endurance back to reg array */
	g_regs[ENDURANCE_REG] = endur + 1;

	/* Restore max count */
	g_regs[BUF_MAX_CNT_REG] = maxCount;

	/* Calc sig for pages 253 and 254 and store back to SRAM */
	g_regs[FLASH_SIG_REG] = CalcRegSig((uint16_t *)&g_regs[1 * REG_PER_PAGE], (2 * REG_PER_PAGE) - 2);
}
