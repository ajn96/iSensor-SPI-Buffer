/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		isr.c
  * @date		4/27/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer interrupt service routines (user provided)
 **/

#include "main.h"

/* Register array */
volatile extern uint16_t regs[];

/**
  * @brief Data ready ISR
  *
  * @return void
  *
  * All four DIOn_Master pins map to this interrupt handler.
  */
void EXTI9_5_IRQHandler()
{

}

/**
  * @brief Slave SPI ISR
  *
  * @return void
  *
  * This function handles SPI traffic from the master device
  */
void SPI2_IRQHandler(void)
{
	/* Transaction counter */
	static uint32_t transaction_counter = 0;

	uint32_t itflag = SPI2->SR;
	uint32_t transmitData;
	uint32_t rxData;

	/* Apply transaction counter to STATUS upper 4 bits and increment */
	regs[STATUS_REG] &= 0x0FFF;
	regs[STATUS_REG] |= transaction_counter;
	transaction_counter += 0x1000;

	/* Error interrupt source */
	if(itflag & (SPI_FLAG_OVR | SPI_FLAG_MODF))
	{
		/* Set status reg SPI error flag */
		regs[STATUS_REG] |= STATUS_SPI_ERROR;

		/* Overrun error, can be cleared by repeatedly reading DR */
		for(uint32_t i = 0; i < 4; i++)
		{
			transmitData = SPI2->DR;
		}
		/* Load zero to output */
		SPI2->DR = 0;
		/* Read status register */
		itflag = SPI2->SR;
		/* Exit ISR */
		return;
	}

	/* Spi overflow (received transaction while transmit pending) */
	if(itflag & 0x1000)
	{
		/* Get data from FIFO */
		rxData = SPI2->DR;

		/* Set status reg SPI overflow flag */
		regs[STATUS_REG] |= STATUS_SPI_OVERFLOW;

		/* Exit ISR */
		return;
	}

	/* Rx done interrupt source */
	if(itflag & SPI_FLAG_RXNE)
	{
		/* Get data from FIFO */
		rxData = SPI2->DR;

		/* Handle transaction */
		if(rxData & 0x8000)
		{
			/* Write */
			transmitData = WriteReg((rxData & 0x7F00) >> 8, rxData & 0xFF);
		}
		else
		{
			/* Read */
			transmitData = ReadReg(rxData >> 8);
		}

		/* Transmit data back */
		SPI2->DR = transmitData;
	}
}
