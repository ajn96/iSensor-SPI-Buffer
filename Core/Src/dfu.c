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

static void ExecuteDFUBoot();

void CheckDFUFlags()
{
	uint32_t flag = *DFU_FLAG_ADDR;
	*DFU_FLAG_ADDR = 0;
	if(flag == ENABLE_DFU_KEY)
	{
		ExecuteDFUBoot();
	}
}

void PrepareDFUBoot()
{
	*DFU_FLAG_ADDR = ENABLE_DFU_KEY;
	NVIC_SystemReset();
}

static void ExecuteDFUBoot()
{

}
