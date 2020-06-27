/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		burst.c
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer burst data read (slave SPI) module
 **/

#include "burst.h"

/** User SPI handle (from main.c) */
extern SPI_HandleTypeDef g_spi2;

/** Global register array (from registers.c) */
volatile extern uint16_t g_regs[];

/** Buffer to receive burst DMA data (from master) into */
static uint8_t burstRxData[64] = {0};

/**
  * @brief Configures SPI for a burst buffer read
  *
  * @return void
  *
  * This function must be called when there is buffered data available to be read,
  * which has been copied to the buffer output data registers.
  */
void BurstReadSetup()
{
	HAL_SPI_TransmitReceive_DMA(&g_spi2, (uint8_t*) &g_regs[BUF_DATA_0_REG], burstRxData, g_regs[BUF_LEN_REG]);
}

