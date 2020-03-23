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

/* Get reference to master SPI instance */
extern SPI_HandleTypeDef hspi2;

/* User register array */
volatile extern uint16_t regs[];

/** track stall time (microseconds) */
uint32_t imu_stalltime_us = 25;

/** Track sclk setting (1 bit per setting) */
uint32_t imu_sclk_divider = SPI_BAUDRATEPRESCALER_4;

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

	/* Set F0 (CS) low */
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);
	status = HAL_SPI_TransmitReceive(&hspi2, txBuf, rxBuf, 1, 0xFFFFFFFF);
	/* Bring F0 (CS) high */
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);

	if(status != HAL_OK)
	{
		return 0;
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
	readRequest &= 0x7FFF;

	/* Perform first transfer */
	ImuSpiTransfer(readRequest);

	/* Delay for stall time */
	SleepMicroseconds(imu_stalltime_us);

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

void SleepMicroseconds(uint32_t microseconds)
{
	  uint32_t clk_cycle_start = DWT->CYCCNT;

	  /* Go to number of cycles for system */
	  microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

	  /* Delay till end */
	  while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}

void UpdateImuSpiConfig()
{
	uint16_t configReg = regs[IMU_SPI_CONFIG_REG];

	/* Stall time is lower 8 bits */
	if((configReg & 0xFF) < 2)
	{
		configReg &= 0xFF00;
		configReg |= 2;
	}
	imu_stalltime_us = configReg & 0xFF;

	/* Sclk divider setting is upper 8 bits */
	uint32_t sclkDividerSetting;
	if(configReg & (1 << 8))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_2;
	}
	else if(configReg & (1 << 9))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_4;
	}
	else if(configReg & (1 << 10))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_8;
	}
	else if(configReg & (1 << 11))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_16;
	}
	else if(configReg & (1 << 12))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_32;
	}
	else if(configReg & (1 << 13))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_64;
	}
	else if(configReg & (1 << 14))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_128;
	}
	else if(configReg & (1 << 15))
	{
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_256;
	}
	else
	{
		configReg |= (1 << 9);
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_4;
	}

	if(imu_sclk_divider != sclkDividerSetting)
		ApplySclkDivider(sclkDividerSetting);

	/* Apply value back to IMU SPI config register */
	regs[IMU_SPI_CONFIG_REG] = configReg;
}

void ApplySclkDivider(uint32_t preScalerSetting)
{
	imu_sclk_divider = preScalerSetting;
	//TODO: Apply and re-init SPI controller
}
