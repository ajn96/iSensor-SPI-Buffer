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
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Disable SPI ISR */
	HAL_NVIC_DisableIRQ(SPI2_IRQn);

	/* Reconfigure CS (PB12) as EXTI interrupt source (posedge trigger) */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Clear pending EXTI interrupts */
	EXTI->PR |= (0x3F << 10);

	/* Enable CS interrupt */
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* Flush any data currently in SPI Rx FIFO */
	for(int i = 0; i < 4; i++)
	{
		SpiData = SPI2->DR;
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
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Clear Tx DMA enable bits in SPI control register */
	CLEAR_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);

	/* Re-enable CS (PB12) alternate function */
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Disable CS interrupt */
	NVIC_DisableIRQ(EXTI15_10_IRQn);

	/* Start user SPI interrupt processing (Rx and error) */
	__HAL_SPI_ENABLE_IT(&g_spi2, (SPI_IT_RXNE | SPI_IT_ERR));

	/* Clear pending interrupts and re-enable */
	HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
	HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

