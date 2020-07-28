/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_spi.c
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer user (slave) SPI module
 **/

#include "user_spi.h"

/** Track if a burst read is enabled. Global scope */
volatile uint32_t g_userburstRunning;

/** User SPI handle (from main.c) */
extern SPI_HandleTypeDef g_spi2;

/** Pointer to buffer entry. Will be 0 if no buffer entry "loaded" to output registers (from registers.c) */
extern volatile uint16_t* g_CurrentBufEntry;

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[3 * REG_PER_PAGE];

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
	/* Clear pending EXTI interrupts */
	EXTI->PR |= (0x3F << 10);

	/* Flush any data currently in SPI Rx FIFO */
	for(int i = 0; i < 4; i++)
	{
		 (void) SPI2->DR;
	}

	/* Reset the Tx DMA threshold bit */
	CLEAR_BIT(SPI2->CR2, SPI_CR2_LDMATX);

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

	/* Configure interrupts (error only) */
	g_spi2.hdmatx->Instance->CCR |=  DMA_IT_TE;
	g_spi2.hdmatx->Instance->CCR &= ~(DMA_IT_HT|DMA_IT_TC);

	/* Enable the Peripheral */
	g_spi2.hdmatx->Instance->CCR |= DMA_CCR_EN;

	/* Set TX DMA request enable in SPI */
	SET_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);

	/* Set the user burst running flag */
	g_userburstRunning = 1;
}

/**
  * @brief Restore SPI functionality after a burst read
  *
  * @return void
  *
  * This function disables SPI2 DMA requests
  */
void BurstReadDisable()
{
	/* Clear Tx DMA enable bits in SPI control register */
	CLEAR_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);
}

/**
  * @brief Updates the slave SPI (SPI2) config based on the USER_SPI_CONFIG register
  *
  * @return void
  *
  * This function performs all needed initialization for the slave SPI port, and should
  * be called as start of the firmware start up process.
  */
void UpdateUserSpiConfig()
{
	uint16_t config = g_regs[USER_SPI_CONFIG_REG];

	/* mask unused bits */
	config &= SPI_CONF_MASK;

	/* CPHA */
	if(config & SPI_CONF_CPHA)
		g_spi2.Init.CLKPhase = SPI_PHASE_2EDGE;
	else
		g_spi2.Init.CLKPhase = SPI_PHASE_1EDGE;

	/* CPOL */
	if(config & SPI_CONF_CPOL)
		g_spi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
	else
		g_spi2.Init.CLKPolarity = SPI_POLARITY_LOW;

	/* Bit order */
	if(config & SPI_CONF_MSB_FIRST)
		g_spi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	else
		g_spi2.Init.FirstBit = SPI_FIRSTBIT_LSB;

	/* Reset all immutable settings */
	g_spi2.Instance = SPI2;
	g_spi2.Init.Mode = SPI_MODE_SLAVE;
	g_spi2.Init.Direction = SPI_DIRECTION_2LINES;
	g_spi2.Init.DataSize = SPI_DATASIZE_16BIT;
	g_spi2.Init.NSS = SPI_NSS_HARD_INPUT;
	g_spi2.Init.TIMode = SPI_TIMODE_DISABLE;
	g_spi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_spi2.Init.CRCPolynomial = 7;
	g_spi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_spi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

	/* De-init SPI */
	HAL_SPI_DeInit(&g_spi2);

	/* Init */
	if (HAL_SPI_Init(&g_spi2) != HAL_OK)
	{
		Error_Handler();
	}

	/* Disable SPI interrupt processing - handled by CS */
	__HAL_SPI_DISABLE_IT(&g_spi2, (SPI_IT_ERR|SPI_IT_TXE|SPI_IT_RXNE));

	/* Set threshold for 16 bit mode */
	CLEAR_BIT(g_spi2.Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((g_spi2.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(&g_spi2);
	}

	/* Load output with initial zeros */
	g_spi2.Instance->DR = 0;

	/* Set user SPI interrupt priority (highest) */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);

	/* Enable user SPI interrupts, via CS */
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* Apply config value in use back to register */
	g_regs[USER_SPI_CONFIG_REG] = config;
}

