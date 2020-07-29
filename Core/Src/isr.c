/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		isr.c
  * @date		4/27/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer interrupt service routines
 **/

#include "isr.h"

/* Private function prototypes */
static void FinishImuBurst();

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[3 * REG_PER_PAGE];

/** Buffer internal count variable (from buffer.c) */
extern volatile uint32_t g_bufCount;

/** User SPI Tx DMA (from main.c) */
extern DMA_HandleTypeDef g_dma_spi2_tx;

/** IMU SPI Rx DMA (from main.c) */
extern DMA_HandleTypeDef g_dma_spi1_rx;

/** IMU SPI Rx DMA (from main.c) */
extern DMA_HandleTypeDef g_dma_spi1_tx;

/** IMU SPI handle (from main.c) */
extern SPI_HandleTypeDef g_spi1;

/** User SPI handle (from main.c) */
extern SPI_HandleTypeDef g_spi2;

/** Track if a burst read is enabled (from user_spi.c) */
extern volatile uint32_t g_userburstRunning;

/** Current capture size (in 16 bit words). Global scope */
volatile uint32_t g_wordsPerCapture;

/** Track if there is currently a capture in progress */
volatile uint32_t g_captureInProgress;

/** Pointer to buffer element which is being populated */
static uint8_t* BufferElementHandle;

/** Pointer to buffer data signature within buffer element which is being populated */
static uint16_t* BufferSigHandle;

/** Track number of words captured within current buffer entry */
static volatile uint32_t WordsCaptured;

/** Buffer signature */
static uint32_t BufferSignature;

/** Flag to track if IMU burst DMA is done */
static volatile uint32_t ImuDMADone = 0;

/**
  * @brief IMU data ready ISR. Kicks off data capture process.
  *
  * @return void
  *
  * All four DIOx_Master pins map to this interrupt handler. Only one
  * should be enabled as an interrupt source at a time. This interrupt
  * also handles PPS input signals for DIO2-4. The EXTI pending interrupt
  * register (PR) can be used to identify if the interrupt is coming from
  * a PPS strobe or an IMU data ready signal.
  */
void EXTI9_5_IRQHandler()
{
	/* EXTI pending request register */
	static uint32_t EXTI_PR;

	/* Microsecond sample time stamp */
	static uint32_t SampleTimestampUs;

	/* Second sample time stamp */
	static uint32_t SampleTimestampS;

	/* Clear exti PR register (lines 5-9) */
	EXTI_PR = EXTI->PR;
	EXTI->PR |= 0x1F << 5;

	/* Check if is PPS interrupt */
	if(EXTI_PR & (PPS_INT_MASK))
	{
		/* Increment PPS counter and clear microsecond timestamp */
		IncrementPPSTime();
	}

	/* Exit if no data ready interrupt */
	if(!(EXTI_PR & (DATA_READY_INT_MASK)))
	{
		return;
	}

	/* If capture in progress then set error flag and exit */
	if(g_captureInProgress)
	{
		/* If SPI DMA and timer 4 are not running then capture is not actually in progress */
		g_captureInProgress = (TIM4->CR1 | g_dma_spi1_tx.Instance->CCR) & 0x1;
		g_regs[STATUS_0_REG] |= STATUS_OVERRUN;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		return;
	}

	/* If buffer element cannot be added then exit */
	if(!BufCanAddElement())
		return;

	/* Get the sample timestamp */
	SampleTimestampS = GetPPSTimestamp();
	SampleTimestampUs = GetMicrosecondTimestamp();

	/* Get element handle */
	BufferElementHandle = BufAddElement();

	/* Add timestamp to buffer */
	*(uint32_t *) BufferElementHandle = SampleTimestampS;
	*(uint32_t *) (BufferElementHandle + 4) = SampleTimestampUs;

	/* Set signature to timestamp value initially */
	BufferSignature = SampleTimestampS & 0xFFFF;
	BufferSignature += (SampleTimestampS >> 16);
	BufferSignature += SampleTimestampUs & 0xFFFF;
	BufferSignature += (SampleTimestampUs >> 16);

	/* Set buffer signature handle */
	BufferSigHandle = (uint16_t *) (BufferElementHandle + 8);

	/* Offset buffer element handle by 10 bytes (timestamp + sig) */
	BufferElementHandle += 10;

	/* Set flag indicating capture is running */
	g_captureInProgress = 1;

	if(g_regs[BUF_CONFIG_REG] & BUF_CFG_IMU_BURST)
	{
		ImuDMADone = 0;
		StartImuBurst(BufferElementHandle);
	}
	else
	{
		/* Register SPI data capture */

		/* Set words captured to 0 */
		WordsCaptured = 0;

		/*Set timer value to 0 */
		TIM4->CNT = 0;
		/* Enable timer */
		TIM4->CR1 |= 0x1;
		/* Clear timer interrupt flag */
		TIM4->SR &= ~TIM_SR_UIF;

		/* Send first 16 bit word */
		TIM3->CR1 &= ~0x1;
		TIM3->CNT = 0;
		TIM3->CR1 |= 0x1;
		SPI1->DR = g_regs[BUF_WRITE_0_REG];
	}
}

/**
  * @brief EXTI 4 interrupt handler.
  *
  * @return void
  *
  * This interrupt is generated by a PPS signal applied to
  * DIO1 only. All other PPS inputs generate an EXTI9_5 interrupt
  */
void EXTI4_IRQHandler()
{
	/* Clear pending interrupts */
	EXTI->PR |= (1 << 4);

	/* Increment PPS timestamp */
	IncrementPPSTime();
}

/**
  * @brief IMU SPI timer ISR.
  *
  * @return void
  *
  * This interrupt retrieves the SPI data read on the previous SPI transaction. It
  * then starts the next SPI transaction (if needed) and returns.
  */
void TIM4_IRQHandler()
{
	/** SPI MISO data */
	static uint32_t SpiMiso;

	/* Clear timer interrupt flag */
	TIM4->SR &= ~TIM_SR_UIF;

	/* Reset timer count. Want to reset it to give a ~0.7us offset */
	TIM4->CNT = 50;

	/* Disable CS timer */
	TIM3->CR1 = 0;
	TIM3->CNT = 0xFFFF;

	if(!g_captureInProgress)
	{
		/* Disable timers */
		TIM4->CR1 &= ~0x1;
		return;
	}

	/* Wait for SPI rx done (running in master mode, shouldn't be an issue) */
	while(!(SPI1->SR & SPI_SR_RXNE));

	/* Grab SPI data from last transaction */
	SpiMiso = SPI1->DR;

	/* Add to signature */
	BufferSignature += SpiMiso;

	BufferElementHandle[0] = (SpiMiso & 0xFF);
	BufferElementHandle[1] = (SpiMiso >> 8);
	BufferElementHandle += 2;

	/* Increment words captured count */
	WordsCaptured++;

	if(WordsCaptured < g_wordsPerCapture)
	{
		/* Restart PWM timer for CS */
		TIM3->CR1 |= 0x1;
		/* Load SPI transmit reg */
		SPI1->DR = g_regs[BUF_WRITE_0_REG + WordsCaptured];
	}
	else
	{
		/* Disable timers and ensure CS is high */
		TIM4->CR1 &= ~0x1;
		TIM3->CR1 &= ~0x1;
		TIM3->CNT = 0xFFFF;

		/* Save final signature */
		BufferSigHandle[0] = BufferSignature;

		/* Update buffer count regs with new count */
		g_regs[BUF_CNT_0_REG] = g_bufCount;
		g_regs[BUF_CNT_1_REG] = g_regs[BUF_CNT_0_REG];

		/* Mark capture as done */
		g_captureInProgress = 0;
	}
}

/**
  * @brief This function handles DMA1 channel2 global interrupt (spi1 (IMU) Rx).
  *
  * @return void
  *
  * This DMA is used to receive data from an IMU during a burst data capture.
  * In order for an IMU data capture to be complete, this DMA channel and the
  * transmit DMA channel have to both finish their work. In general, this channel
  * will finish last.
  **/
void DMA1_Channel2_IRQHandler(void)
{
	uint32_t flags = g_dma_spi1_rx.DmaBaseAddress->ISR;

	/* Clear interrupt enable */
	g_dma_spi1_rx.Instance->CCR &= ~(DMA_IT_TC | DMA_IT_TE);

	/* Clear interrupt flags */
	g_dma_spi1_rx.DmaBaseAddress->IFCR = (DMA_FLAG_TC1|DMA_FLAG_TE1) << g_dma_spi1_rx.ChannelIndex;

	/* Check for error interrupt */
	if(flags & (DMA_FLAG_TE1 << g_dma_spi1_rx.ChannelIndex))
	{
		g_regs[STATUS_0_REG] |= STATUS_DMA_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		FinishImuBurst();
	}

	/* Check if both interrupts complete */
	if(ImuDMADone == 0)
	{
	  ImuDMADone = 1;
	}
	else
	{
	  /* Both are done */
	  FinishImuBurst();
	}
}

/**
  * @brief This function handles DMA1 channel3 global interrupt (spi1 (IMU) Tx).
  *
  * @return void
  *
  * This DMA is used to transmit the write data array to the IMU during a burst
  * data capture. It runs in parallel with DMA channel 2 to achieve full duplex burst.
  */
void DMA1_Channel3_IRQHandler(void)
{
	uint32_t flags = g_dma_spi1_tx.DmaBaseAddress->ISR;

	/* Clear interrupt enable */
	g_dma_spi1_tx.Instance->CCR &= ~(DMA_IT_TC | DMA_IT_TE);

	/* Clear interrupt flags */
	g_dma_spi1_tx.DmaBaseAddress->IFCR = (DMA_FLAG_TC1|DMA_FLAG_TE1) << g_dma_spi1_tx.ChannelIndex;

	/* Check for error interrupt */
	if(flags & (DMA_FLAG_TE1 << g_dma_spi1_tx.ChannelIndex))
	{
		g_regs[STATUS_0_REG] |= STATUS_DMA_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		FinishImuBurst();
	}

	/* Check if both interrupts complete */
	if(ImuDMADone == 0)
	{
	  ImuDMADone = 1;
	}
	else
	{
	  FinishImuBurst();
	}
}

/**
  * @brief Cleans up an IMU burst data read
  *
  * @return void
  *
  * This function is called once the SPI1 Tx and Rx DMA interrupts
  * have both fired, or a single error interrupt has been generated.
  * SPI1 uses DMA1, channel 2/3. These channels have lower priority
  * than the user SPI DMA channels, which also use DMA peripheral 1.
  *
  * This function brings CS high, calculates the buffer signature,
  * and updates the buffer count / capture state variables. Before
  * this function is called, DMA interrupts should be disabled (by
  * their respective ISR's)
  */
static void FinishImuBurst()
{
	/* Bring CS high */
	TIM3->CR1 &= ~0x1;
	TIM3->CNT = 0xFFFF;

	/* Build buffer signature */
	uint16_t * RxData = (uint16_t *) BufferElementHandle;
	for(int reg = 0; reg < (g_regs[BUF_LEN_REG] / 2); reg++)
	{
		BufferSignature += RxData[reg];
	}
	/* Save signature to buffer entry */
	BufferSigHandle[0] = BufferSignature;

	/* Update buffer count regs with new count */
	g_regs[BUF_CNT_0_REG] = g_bufCount;
	g_regs[BUF_CNT_1_REG] = g_regs[BUF_CNT_0_REG];

	/* Mark capture as done */
	g_captureInProgress = 0;
}

/**
  * @brief This function handles DMA1 channel5 global interrupt (spi2 (user) Tx).
  *
  * @return void
  *
  * This interrupt is only used to flag errors. Ending a buffer burst output is
  * triggered by an EXTI interrupt attached to chip select.
  */
void DMA1_Channel5_IRQHandler(void)
{
	uint32_t flags = g_dma_spi2_tx.DmaBaseAddress->ISR;

	/* Clear interrupt enable */
	g_dma_spi2_tx.Instance->CCR &= ~DMA_IT_TE;

	/* Clear interrupt flags */
	g_dma_spi2_tx.DmaBaseAddress->IFCR = (DMA_FLAG_TE1|DMA_FLAG_TC1|DMA_FLAG_HT1) << g_dma_spi2_tx.ChannelIndex;

	/* Check for error interrupt */
	if(flags & (DMA_FLAG_TE1 << g_dma_spi2_tx.ChannelIndex))
	{
		/* Flag DMA error */
		g_regs[STATUS_0_REG] |= STATUS_DMA_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
	}
}

/**
  * @brief This function handles the user SPI (SPI2) chip select rising edge
  *
  * @return void
  *
  * This interrupt is the only interrupt generated by the user SPI port. All
  * new data and error flags are handled in this one interrupt. This should
  * make the burst behavior more consistent, since there is no longer any
  * dependency on how many bytes were clocked out, or potential for
  * weird data offsets.
  */
void EXTI15_10_IRQHandler(void)
{
	/* SPI status register */
	static uint32_t spiSr;

	/* Rx SPI data */
	static uint32_t rxData;

	/* Tx SPI data */
	static uint32_t txData;

	/* Clear exti PR register (lines 10-15) */
	EXTI->PR |= 0x3F << 10;

	/* Read SPI2 SR */
	spiSr = SPI2->SR;

	/* Read first SPI data word received */
	rxData = SPI2->DR;

	/* If a burst is running disable DMA and re-init SPI */
	if(g_userburstRunning)
	{
		/* Disable DMA */
		g_dma_spi2_tx.Instance->CCR &= ~DMA_CCR_EN;

		/* Disable and re-enable SPI. Kind of hacky, not a clean way to do this.
		 * This is required because there is no way to clear Tx FIFO in the SPI
		 * peripheral otherwise */
		RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;
		HAL_SPI_Init(&g_spi2);
		__HAL_SPI_ENABLE(&g_spi2);

		/* Clear burst running flag */
		g_userburstRunning = 0;

		/* Check if another burst was not requested */
		if((rxData >> 8) != 6)
		{
			BurstReadDisable();
		}

		/* Jump to handler, don't want to check errors/overflow while burst is running */
		goto handle_transaction;
	}

	/* If data is not available then re-init and exit */
	if(!(spiSr & SPI_SR_RXNE))
	{
		g_regs[STATUS_0_REG] |= STATUS_SPI_OVERFLOW;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

		/* Re-init SPI to make sure we end up in a good state */
		RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;
		RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;
		HAL_SPI_Init(&g_spi2);
		__HAL_SPI_ENABLE(&g_spi2);

		return;
	}

	/* Check for SPI overflow (FIFO transmit is half full or greater) */
	if(spiSr & SPI_FTLVL_HALF_FULL)
	{
		/* Clear SPI FIFO */
		for(int i = 0; i < 4; i++)
		{
			(void) SPI2->DR;
		}

		/* Set status reg SPI overflow flag */
		g_regs[STATUS_0_REG] |= STATUS_SPI_OVERFLOW;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

		/* Don't want to load any new data to the output, just return here */
		return;
	}

	/* Error interrupt source */
	if(spiSr & (SPI_FLAG_OVR | SPI_FLAG_MODF))
	{
		/* Set status reg SPI error flag */
		g_regs[STATUS_0_REG] |= STATUS_SPI_ERROR;
		g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];

		/* Overrun error, clear out Rx FIFO by repeatedly reading DR */
		for(uint32_t i = 0; i < 4; i++)
		{
			(void) SPI2->DR;
		}
		/* Read status register */
		(void) SPI2->SR;
	}

	/* Handle transaction */
	handle_transaction:
	if(rxData & 0x8000)
	{
		/* Write */
		txData = WriteReg((rxData & 0x7F00) >> 8, rxData & 0xFF);
	}
	else
	{
		/* Read */
		txData = ReadReg(rxData >> 8);
	}

	/* Transmit data back */
	SPI2->DR = txData;
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
