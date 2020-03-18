/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		SpiPassthrough.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer SPI pass through (to IMU) module
 **/

#include "SpiPassthrough.h"

/*Get reference to master SPI instance */
extern SPI_HandleTypeDef masterSpi;

/**
  * @brief Basic IMU SPI data transfer function (protocol agnostic).
  *
  * @param MOSI The 16 bit MOSI data to transmit to the IMU
  *
  * @return The 16 bit MISO data received during the transmission
  *
  * This function wraps the SPI master HAL layer into something easily usable. All
  * SPI pass through functionality is built on this call.
 **/
uint16_t ImuSpiTransfer(uint16_t MOSI)
{
	uint8_t txBuf[2];
	uint8_t rxBuf[2];
	HAL_StatusTypeDef status;
	uint16_t retVal;

	txBuf[0] = MOSI & 0xFF;
	txBuf[1] = (MOSI & 0xFF00) >> 8;

	status = HAL_SPI_TransmitReceive(&masterSpi, txBuf, rxBuf, 2, 0xFFFFFFFF);

	if(status != HAL_OK)
	{
		return 0;
		//TODO: Maybe something more clever here
	}
	else
	{
		retVal = rxBuf[0] | (rxBuf[1] << 8);
		return retVal;
	}
}

/**
  * @brief Reads 16 bit value from the IMU
  *
  * @param RegAddr The IMU register address to read
  *
  * @return The 16 bit register data read from the IMU
  *
  * This function produces two SPI transfers to the IMU. One to send the
  * initial read request, and a second to get back the read result data.
 **/
uint16_t ImuReadReg(uint8_t RegAddr)
{
	uint16_t readRequest;

	/* shift address to correct position */
	readRequest = RegAddr << 8;

	/* Mask out write bit */
	readRequest &= 0x7F;

	/* Perform first transfer */
	ImuSpiTransfer(readRequest);

	/* Delay for stall time */

	/* Return result data on second word */
	return ImuSpiTransfer(0);
}

/**
  * @brief Writes an 8 bit value to the IMU
  *
  * @param RegAddr The IMU register address to write
  *
  * @param RegValue The 8 bit value to write
  *
  * @return The MISO data clocked out from the IMU during the transaction
  *
  * This function produces only a single SPI transaction to the IMU
 **/
uint16_t ImuWriteReg(uint8_t RegAddr, uint8_t RegValue)
{
	uint16_t writeRequest;

	/* Build write request */
	writeRequest = (0x8000 | (RegAddr << 8) | RegValue);

	/* Perform write and return result */
	return ImuSpiTransfer(writeRequest);
}
