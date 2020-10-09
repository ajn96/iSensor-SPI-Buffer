/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		dfu.c
  * @date		10/9/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer runtime firmware upgrade module
 **/

#include "dfu.h"

/* Private function prototypes */
static void ExecuteDFUBoot();

/* Function pointer to bootloader */
void (*SysMemBootJump)(void);

/**
  * @brief Checks if a reboot into the DFU bootloader is needed
  *
  * @return void
  *
  * This function should be the first called in the main routine. The less
  * which is configured before entering DFU mode, the easier. This function
  * checks for a DFU reboot flag set in a "no-initialize" section of SRAM
  * to determine if booting into DFU mode is needed. This flag must be set
  * by the application code on the prior run, when a bootloader reboot command
  * is received.
  */
void CheckDFUFlags()
{
	uint32_t flag = *DFU_FLAG_ADDR;
	*DFU_FLAG_ADDR = 0;
	if(flag == ENABLE_DFU_KEY)
	{
		ExecuteDFUBoot();
	}
}

/**
  * @brief Set DFU reboot flag in RAM and reset system
  *
  * @return void
  *
  * Resetting the system ensures the processor core is in a good
  * state to load the bootloader on the next initialization. To run
  * the bootloader, it is important that no interrupts are enabled,
  * stack is cleared, processor is running from internal clock,
  * and no hardware peripherals are active.
  */
void PrepareDFUBoot()
{
	*DFU_FLAG_ADDR = ENABLE_DFU_KEY;
	NVIC_SystemReset();
}

/**
  * @brief Executes a DFU reboot
  *
  * @return void
  *
  *
  */
static void ExecuteDFUBoot()
{
	/* Set pointer to bootloader in ROM */
	SysMemBootJump = (void (*)(void)) (*((uint32_t *) 0x1FFFF796));

	/* Reset stack pointer */
	__set_MSP(0);

	/* Jump */
	SysMemBootJump();

	/* Kick to loop to prevent potential prediction issues */
	while (1);
}
