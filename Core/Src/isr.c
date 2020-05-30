/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		isr.c
  * @date		4/27/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer interrupt service routines (user provided)
 **/

#include "isr.h"

/* Global register array */
volatile extern uint16_t regs[3 * REG_PER_PAGE];

/* Buffer internal count variable */
volatile extern uint32_t buf_count;

/* IMU stall time (from pass through module) */
extern uint32_t imu_stalltime_us;

extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;

/** Pointer to buffer element which is being populated */
static uint8_t* BufferElementHandle;

/** Track if there is currently a capture in progress */
volatile uint32_t CaptureInProgress;

/** Track number of words captured within current buffer entry */
static volatile uint32_t WordsCaptured;

/** Current capture size (in 16 bit words) */
volatile uint32_t WordsPerCapture;

/** Sample time stamp */
static uint32_t SampleTimestamp;

/**
  * @brief IMU data ready ISR. Kicks off data capture process.
  *
  * @return void
  *
  * All four DIOx_Master pins map to this interrupt handler. Only one
  * should be enabled as an interrupt source at a time though.
  */
void EXTI9_5_IRQHandler()
{
	/* Clear interrupt first */
	EXTI->PR |= (0x1F << 5);

	/* If capture in progress then set error flag and exit */
	if(CaptureInProgress)
	{
		CaptureInProgress = TIM4->CR1 & 0x1;
		regs[STATUS_0_REG] |= STATUS_OVERRUN;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];
		return;
	}

	/* If buffer element cannot be added then exit */
	if(!BufCanAddElement())
		return;

	/* Get the sample timestamp */
	SampleTimestamp = GetCurrentSampleTime();

	/* Get element handle */
	BufferElementHandle = BufAddElement();

	/* Send first 16 bit word */
	TIM3->CR1 &= ~0x1;
	TIM3->CNT = 0;
	TIM3->CR1 |= 0x1;
	SPI1->DR = regs[BUF_WRITE_0_REG];

	/*Set timer value to 0 */
	TIM4->CNT = 0;
	/* Enable timer */
	TIM4->CR1 |= 0x1;
	/* Clear timer interrupt flag */
	TIM4->SR &= ~TIM_SR_UIF;

	/* Set flag indicating capture is running */
	CaptureInProgress = 1;
	/* Set words captured to 0 */
	WordsCaptured = 0;

	/* Add timestamp to buffer */
	BufferElementHandle[0] = SampleTimestamp & 0xFF;
	BufferElementHandle[1] = (SampleTimestamp >> 8) & 0xFF;
	BufferElementHandle[2] = (SampleTimestamp >> 16) & 0xFF;
	BufferElementHandle[3] = (SampleTimestamp >> 24) & 0xFF;

	/* Offset buffer element handle */
	BufferElementHandle += 4;
}

void TIM4_IRQHandler()
{
	uint32_t miso;

	/* Clear timer interrupt flag */
	TIM4->SR &= ~TIM_SR_UIF;

	/* Disable CS timer */
	TIM3->CR1 = 0;
	TIM3->CNT = 0xFFFF;

	if(!CaptureInProgress)
	{
		/* Disable timers */
		TIM4->CR1 &= ~0x1;
		return;
	}

	/* Grab SPI data from last transaction */
	miso = SPI1->DR;

	BufferElementHandle[0] = (miso & 0xFF);
	BufferElementHandle[1] = (miso >> 8);
	BufferElementHandle += 2;

	/* Increment words captured count */
	WordsCaptured++;

	if(WordsCaptured < WordsPerCapture)
	{
		/* Restart PWM timer for CS */
		TIM3->CR1 |= 0x1;
		/* Load SPI transmit reg */
		SPI1->DR = regs[BUF_WRITE_0_REG + WordsCaptured];
	}
	else
	{
		/* Disable timers */
		TIM4->CR1 &= ~0x1;
		TIM3->CR1 &= ~0x1;
		TIM3->CNT = 0xFFFF;

		/* Update buffer count regs */
		regs[BUF_CNT_0_REG] = buf_count;
		regs[BUF_CNT_1_REG] = regs[BUF_CNT_0_REG];

		/* Mark capture as done */
		CaptureInProgress = 0;
	}
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
	regs[STATUS_0_REG] &= 0x0FFF;
	regs[STATUS_0_REG] |= transaction_counter;
	regs[STATUS_1_REG] = regs[STATUS_0_REG];
	transaction_counter += 0x1000;

	/* Error interrupt source */
	if(itflag & (SPI_FLAG_OVR | SPI_FLAG_MODF))
	{
		/* Set status reg SPI error flag */
		regs[STATUS_0_REG] |= STATUS_SPI_ERROR;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];

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
		regs[STATUS_0_REG] |= STATUS_SPI_OVERFLOW;
		regs[STATUS_1_REG] = regs[STATUS_0_REG];

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

/**
  * @brief This function handles DMA1 channel4 global interrupt (spi2 RX done).
  *
  * @return void
  */
void DMA1_Channel4_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

/**
  * @brief This function handles DMA1 channel5 global interrupt (spi2 TX done).
  *
  * @return void
  */
void DMA1_Channel5_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

/**
  * @brief This function handles Hard fault interrupt.
  *
  * @return void
  */
void HardFault_Handler(void)
{
	/* Store error message for future retrieval and reboot */
	FlashLogError(ERROR_HARDFAULT);
	NVIC_SystemReset();
}

/**
  * @brief This function handles Memory management fault.
  *
  * @return void
  */
void MemManage_Handler(void)
{
	/* Store error message for future retrieval and reboot */
	FlashLogError(ERROR_MEM);
	NVIC_SystemReset();
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  *
  * @return void
  */
void BusFault_Handler(void)
{
	/* Store error message for future retrieval and reboot */
	FlashLogError(ERROR_BUS);
	NVIC_SystemReset();
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  *
  * @return void
  */
void UsageFault_Handler(void)
{
	/* Store error message for future retrieval and reboot */
	FlashLogError(ERROR_USAGE);
	NVIC_SystemReset();
}
