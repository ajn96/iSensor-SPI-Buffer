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

/** Buffer to receive burst DMA data (from master) into. Global scope */
uint8_t g_BurstRxData[74] = {0};

/** User SPI handle (from main.c) */
extern SPI_HandleTypeDef g_spi2;

/** Pointer to buffer entry. Will be 0 if no buffer entry "loaded" to output registers (from registers.c) */
volatile extern uint16_t* g_CurrentBufEntry;

/** Global register array (from registers.c) */
volatile extern uint16_t g_regs[3 * REG_PER_PAGE];

/* Data read from SPI2->DR */
static volatile uint32_t SpiData;

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
	/* Disable SPI ISR */
	HAL_NVIC_DisableIRQ(SPI2_IRQn);

	/* Flush SPI FIFO */
	for(int i = 0; i < 4; i++)
	{
		SpiData = SPI2->DR;
	}

	/* Burst SPI data capture */

	/* Reset the DMA threshold bit */
	CLEAR_BIT(SPI2->CR2, SPI_CR2_LDMATX | SPI_CR2_LDMARX);

	/************ Enable the Rx DMA Stream/Channel  *********************/

	/* Enable Rx DMA Request in SPI */
	SET_BIT(SPI2->CR2, SPI_CR2_RXDMAEN);

	/* Disable DMA peripheral */
	g_spi2.hdmarx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear all flags */
	g_spi2.hdmarx->DmaBaseAddress->IFCR  = (DMA_FLAG_GL1 << g_spi2.hdmarx->ChannelIndex);

	/* Configure DMA Channel data length (16 bit SPI words) */
	g_spi2.hdmarx->Instance->CNDTR = (g_regs[BUF_LEN_REG] + 10) >> 1;

    /* Configure DMA Channel peripheral address (SPI data register) */
	g_spi2.hdmarx->Instance->CPAR = (uint32_t)&SPI2->DR;

    /* Configure DMA Channel memory address (burst receive buffer) */
	g_spi2.hdmarx->Instance->CMAR = (uint32_t) g_BurstRxData;

	/* Configure interrupts */
	g_spi2.hdmarx->Instance->CCR |= (DMA_IT_TC | DMA_IT_TE);
	g_spi2.hdmarx->Instance->CCR &= ~DMA_IT_HT;

	/* Enable the Peripheral */
	g_spi2.hdmarx->Instance->CCR |= DMA_CCR_EN;

	/************ Enable the Tx DMA Stream/Channel  *********************/

	/* Disable the peripheral */
	g_spi2.hdmatx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear all flags */
	g_spi2.hdmatx->DmaBaseAddress->IFCR  = (DMA_FLAG_GL1 << g_spi2.hdmatx->ChannelIndex);

	/* Configure DMA Channel data length (total buffer entry size) */
	g_spi2.hdmatx->Instance->CNDTR = (g_regs[BUF_LEN_REG] + 10) >> 1;

	/* Configure DMA Channel peripheral address (SPI data register) */
	g_spi2.hdmatx->Instance->CPAR = (uint32_t) &SPI2->DR;

    /* Configure DMA Channel memory address (buffer entry) */
	g_spi2.hdmatx->Instance->CMAR =  (uint32_t) g_CurrentBufEntry;

	/* Configure interrupts */
	g_spi2.hdmatx->Instance->CCR |= (DMA_IT_TC | DMA_IT_TE);
	g_spi2.hdmatx->Instance->CCR &= ~DMA_IT_HT;

	/* Enable the Peripheral */
	g_spi2.hdmatx->Instance->CCR |= DMA_CCR_EN;

	/* Set TX DMA request enable in SPI */
	SET_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);
}

/**
  * @brief Restore SPI functionality after a burst read
  *
  * @return void
  *
  * This function disables SPI2 DMA requests and re-enables the SPI2
  * interrupt service routine (disabled during burst).
  */
void BurstReadDisable()
{
	/* Clear DMA enable bits in SPI control register */
	CLEAR_BIT(SPI2->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

	/* Flush any data in SPI */
	/* Get SPI back to a good state */
	for(uint32_t i = 0; i < 4; i++)
	{
		SpiData = SPI2->DR;
	}
	/* Read status register */
	SpiData = SPI2->SR;

	/* Clear pending interrupts and re-enable */
	HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
	HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

