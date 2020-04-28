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

/* Private function prototypes */
static void RestoreStandardSPI();

/* User SPI handle (from main) */
extern SPI_HandleTypeDef hspi2;

/* Register array */
volatile extern uint16_t regs[];

/** Buffer to receive burst DMA data (from master) into */
uint8_t burstRxData[64] = {0};

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
	HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*) &regs[BUF_DATA_0_REG], burstRxData, regs[BUF_LEN_REG]);
}

/**
  * @brief Restore "standard" operating mode for slave SPI, allowing register read/writes
  *
  * @return void
  */
static void RestoreStandardSPI()
{

}

