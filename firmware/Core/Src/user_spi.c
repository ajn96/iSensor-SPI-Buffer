/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		user_spi.c
  * @date		4/28/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer user (slave) SPI module
 **/

#include "reg.h"
#include "user_spi.h"
#include "buffer.h"
#include "isr.h"

/** Track if a burst read is enabled. Global scope */
volatile uint32_t g_userburstRunning;

/* SPI control 1 register value for operation */
static uint32_t SPI2_CR1;

/* SPI control 1 register value for operation */
static uint32_t SPI2_CR2;

/**
  * @brief Reset user SPI port
  *
  * @return void
  *
  * Disable and re-enable SPI using RCC. Kind of hacky, not a clean way to do this.
  * This is required because there is no way to clear Tx FIFO in the SPI
  * peripheral otherwise
  */
void UserSpiReset(bool register_mode)
{
	/* Hard reset SPI through RCC */
	RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;
	/* Apply SPI settings */
	SPI2->CR1 = SPI2_CR1;
	SPI2->CR2 = SPI2_CR2;
	/* Are we enabling SPI interrupts? */
	if (register_mode)
	{
		/* Enable user SPI to generate interrupt on two bytes received (RXNE) */
		SPI2->CR2 |= SPI_IT_RXNE;
	}
	else
	{
		/* Burst mode, disable SPI interrupts */
		SPI2->CR2 &= ~(SPI_IT_ERR|SPI_IT_TXE|SPI_IT_RXNE);
	}
	/* Enable */
	SPI2->CR1 |= SPI_CR1_SPE;
}

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
	/* Reset SPI, disabling SPI interrupts */
	UserSpiReset(false);

	/* Load buffer count to output initially */
	SPI2->DR = (g_regs[BUF_CNT_0_REG] << 8u)|(g_regs[BUF_CNT_0_REG] >> 8u);

	/************ Enable the Tx DMA Stream/Channel  *********************/

	/* Disable the peripheral */
	g_spi2.hdmatx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear all flags */
	g_spi2.hdmatx->DmaBaseAddress->IFCR  = (DMA_FLAG_GL1 << g_spi2.hdmatx->ChannelIndex);

	/* Configure DMA Channel data length (total buffer entry size in bytes) */
	g_spi2.hdmatx->Instance->CNDTR = g_regs[BUF_LEN_REG] + 10;

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

	/* Enable CS rising edge interrupt after clearing pending IRQ */
	EXTI->PR = (0x3F << 10);
	NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* Set the user burst running flag */
	g_userburstRunning = 1;
}

/**
  * @brief Restore SPI functionality after a burst read
  *
  * @return void
  *
  * This function disables SPI2 DMA requests via hard reset, and disables
  * SPI2 DMA channel. It then re-configures the SPI for user mode.
  */
void BurstReadDisable()
{
	g_spi2.hdmatx->Instance->CCR &= ~DMA_CCR_EN;
	/* Disable CS interrupt */
	NVIC_DisableIRQ(EXTI15_10_IRQn);
	/* Reset SPI, re-enabling SPI IRQ */
	UserSpiReset(true);
}

/**
  * @brief Updates the slave SPI (SPI2) config based on the USER_SPI_CONFIG register
  *
  * @param CheckUnlock Flag to check if the USER_SPI is unlocked (0xA5 written to config reg upper)
  *
  * @return void
  *
  * This function performs all needed initialization for the slave SPI port, and should
  * be called as start of the firmware start up process. If CheckUnlock is true, then
  * the function will check that the upper 8 bits of USER_SPI_CONFIG is 0xA5. If it is
  * not, the SPI update will not be processed. This prevents accidental writes to the
  * SPI config register.
  *
  * IMPORTANT! From the STM32F303 TRM:
  *
  * When the data frame size fits into one byte (less than or equal to 8 bits), data packing is
  * used automatically when any read or write 16-bit access is performed on the SPIx_DR
  * register.
  *
  * As such, words must be received byte-wise and transmitted word-wise (16-bit).
  */
void UpdateUserSpiConfig(uint32_t CheckUnlock)
{
	uint16_t config = g_regs[USER_SPI_CONFIG_REG];
	static uint16_t lastConfig = USER_SPI_CONFIG_DEFAULT;

	if(CheckUnlock)
	{
		if((config >> 8) != 0xA5)
		{
			/* Block register update */
			g_regs[USER_SPI_CONFIG_REG] = lastConfig;
			return;
		}
	}

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
	g_spi2.Init.DataSize = SPI_DATASIZE_8BIT;
	g_spi2.Init.NSS = SPI_NSS_HARD_INPUT;
	g_spi2.Init.TIMode = SPI_TIMODE_DISABLE;
	g_spi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_spi2.Init.CRCPolynomial = 7;
	g_spi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_spi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

	/* Reset SPI and apply settings */
	HAL_SPI_DeInit(&g_spi2);
	SPI2->CR1 &= ~(SPI_CR1_SPE);
	HAL_SPI_Init(&g_spi2);

	/* Clear FRXTH to enable trigger on 16-bits Rx data */
	SPI2->CR2 &= ~(SPI_CR2_FRXTH);
	/* Enable user SPI to generate interrupt on two bytes received (RXNE) */
	SPI2->CR2 |= SPI_IT_RXNE;

	/* Save control settings based on latest user config */
	SPI2_CR1 = SPI2->CR1;
	SPI2_CR2 = SPI2->CR2;

	/* Enable SPI */
	SPI2->CR1 |= SPI_CR1_SPE;

	/* Load output with initial zeros */
	SPI2->DR = 0u;

	/* Set user SPI interrupt priority (highest) */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_SetPriority(SPI2_IRQn, 1, 0);

	/* Clear pending interrupts for SPI */
	EXTI->PR = (0x3F << 10);
	(void) SPI2->SR;
	NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	NVIC_ClearPendingIRQ(SPI2_IRQn);

	/* Enable user SPI interrupts, disable CS interrupt */
	NVIC_DisableIRQ(EXTI15_10_IRQn);
	NVIC_EnableIRQ(SPI2_IRQn);

	/* Apply config value in use back to register */
	g_regs[USER_SPI_CONFIG_REG] = config;

	/* Save last valid config */
	lastConfig = config;
}

