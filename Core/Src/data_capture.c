/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		data_capture.c
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer autonomous IMU data acquisition functions.
 **/

#include "data_capture.h"

void EnableDataCapture()
{

}

void DisableDataCapture()
{

}

void UpdateDRConfig()
{
	/* Get current config value */
	uint32_t config = regs[DR_CONFIG_REG];

	/* Clear upper bits */
	config &= 0x1F;

	/* Must be one, and only one bit set in drPins. If none, defaults to DIO1 set */
	if(config & 0x1)
		config = 0x1;
	else if(config & 0x2)
		config = 0x2;
	else if(config & 0x4)
		config = 0x4;
	else if(config & 0x8)
		config = 0x8;
	else
		config = 0x1;

	/* Apply modified settings back to reg */
	regs[DR_CONFIG_REG] = config;

}
