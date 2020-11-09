/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		imu_spi.c
  * @date		3/18/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation for iSensor-SPI-Buffer IMU interfacing module
 **/

#include "imu_spi.h"

/* Local function prototypes */
static void ApplySclkDivider(uint32_t preScalerSetting);

/* Get reference to master SPI instance (SPI1) */
extern SPI_HandleTypeDef g_spi1;

/** Global register array (from registers.c) */
extern volatile uint16_t g_regs[NUM_REG_PAGES * REG_PER_PAGE];

/** track stall time (microseconds) */
static uint32_t imuStallTimeUs = 25;

/** TIM3 HAL handle */
static TIM_HandleTypeDef htim3;

/** TIM4 HAL handle */
static TIM_HandleTypeDef htim4;

/**
  * @brief Hardware reset connected IMU
  *
  * @return void
  *
  * Reset pin is pulled low for 1ms, then brought high
  */
void ResetImu()
{
	GPIOA->ODR &= ~GPIO_PIN_3;
	SleepMicroseconds(1000);
	GPIOA->ODR |= GPIO_PIN_3;
}

/**
  * @brief Disable IMU burst data stream
  *
  * @return void
  */
void DisableImuSpiDMA()
{
	/* Disable the DMA peripherals */
	g_spi1.hdmarx->Instance->CCR &= ~DMA_CCR_EN;
	g_spi1.hdmatx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear DMA request enable in SPI */
	CLEAR_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);
	CLEAR_BIT(SPI1->CR2, SPI_CR2_RXDMAEN);
}


/**
  * @brief Start an IMU burst data capture (using DMA)
  *
  * @param bufEntry Pointer to the buffer entry to recieve data into
  *
  * @return void
  *
  * This function configures the IMU SPI port for a bi-directional DMA
  * transfer. CS is manually controlled by leaving TIM3 (CS timer) disabled,
  * and manually setting the count register to 0.
 **/
void StartImuBurst(uint8_t* bufEntry)
{
	/* Flush SPI FIFO */
	for(int i = 0; i < 4; i++)
	{
		(void) SPI1->DR;
	}

	/* Drop CS */
	TIM3->CNT = 0x0;
	TIM3->CR1 = 0x0;

	/* Burst SPI data capture */

	/* Reset the DMA threshold bit */
	CLEAR_BIT(SPI1->CR2, SPI_CR2_LDMATX | SPI_CR2_LDMARX);

	/************ Enable the Rx DMA Stream/Channel  *********************/

	/* Enable Rx DMA Request in SPI */
	SET_BIT(SPI1->CR2, SPI_CR2_RXDMAEN);

	/* Disable DMA peripheral */
	g_spi1.hdmarx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear all flags */
	g_spi1.hdmarx->DmaBaseAddress->IFCR  = (DMA_FLAG_GL1 << 4);

	/* Configure DMA Channel data length (16 bit SPI words) */
	g_spi1.hdmarx->Instance->CNDTR = g_regs[BUF_LEN_REG] >> 1;

    /* Configure DMA Channel peripheral address (SPI data register) */
	g_spi1.hdmarx->Instance->CPAR = (uint32_t)&SPI1->DR;

    /* Configure DMA Channel memory address (buffer entry) */
	g_spi1.hdmarx->Instance->CMAR = (uint32_t) bufEntry;

	/* Configure interrupts */
	g_spi1.hdmarx->Instance->CCR |= (DMA_IT_TC | DMA_IT_TE);
	g_spi1.hdmarx->Instance->CCR &= ~DMA_IT_HT;

	/* Enable the Peripheral */
	g_spi1.hdmarx->Instance->CCR |= DMA_CCR_EN;

	/************ Enable the Tx DMA Stream/Channel  *********************/

	/* Disable the peripheral */
	g_spi1.hdmatx->Instance->CCR &= ~DMA_CCR_EN;

	/* Clear all flags */
	g_spi1.hdmatx->DmaBaseAddress->IFCR  = (DMA_FLAG_GL1 << 8);

	/* Configure DMA Channel data length */
	g_spi1.hdmatx->Instance->CNDTR = g_regs[BUF_LEN_REG] >> 1;

	/* Configure DMA Channel peripheral address (SPI data register) */
	g_spi1.hdmatx->Instance->CPAR = (uint32_t) &SPI1->DR;

    /* Configure DMA Channel memory address (write data) */
	g_spi1.hdmatx->Instance->CMAR =  (uint32_t) &g_regs[BUF_WRITE_0_REG];

	/* Configure interrupts */
	g_spi1.hdmatx->Instance->CCR |= (DMA_IT_TC | DMA_IT_TE);
	g_spi1.hdmatx->Instance->CCR &= ~DMA_IT_HT;

	/* Enable the Peripheral */
	g_spi1.hdmatx->Instance->CCR |= DMA_CCR_EN;

	/* Set TX DMA request enable in SPI */
	SET_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);
}

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
uint16_t ImuSpiTransfer(uint32_t MOSI)
{
	/* Drop CS */
	TIM3->CR1 = 0;
	TIM3->CNT = 0xFFFF;
	TIM3->CR1 = 1;

	/* Load data to SPI1 tx */
	SPI1->DR = MOSI;

	/* Wait for rx done, or timeout (120us) */
	TIM8->CNT = 0;
	while(((SPI1->SR & SPI_SR_RXNE) == 0) && (TIM8->CNT < 120));

	/* Bring CS high */
	TIM3->CR1 = 0;
	TIM3->CNT = 0xFFFF;

	/* Return data reg */
	return SPI1->DR;
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
	uint32_t readRequest;

	/* shift address to correct position */
	readRequest = RegAddr << 8;

	/* Mask out write bit */
	readRequest &= 0x7FFF;

	/* Perform first transfer */
	ImuSpiTransfer(readRequest);

	/* Delay for stall time (1us offset) */
	SleepMicroseconds(imuStallTimeUs - 1);

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
	uint32_t writeRequest;

	/* Build write request */
	writeRequest = (0x8000 | (RegAddr << 8) | RegValue);

	/* Perform write and return result */
	return ImuSpiTransfer(writeRequest);
}

/**
 * @brief Sets the TIM3 period for use in PWM mode to drive CS
 *
 * @return void
 *
 * @param period Timer ticks period for TIM3. This corresponds to the CS low pulse width
 *
 * TIM3 will be disabled (CS high) by this function. It should not be called while a data capture is in progress.
 */
void ConfigureImuCsTimer(uint32_t period)
{
	/* Disable */
    TIM3->CR1 &= 0x1;

	/* Set count to 0xFFFF */
	TIM3->CNT = 0xFFFF;

    /* Set compare channel 2 value */
	TIM3->CCR2 = period;
}

/**
 * @brief Inits TIM3 for use in PWM mode to drive CS
 *
 * @return void
 *
 * TIM4 is used to drive SPI buffered data acquisition from the IMU.
 * One timer interrupt is generated per SPI word clocked out from the
 * DUT. TIM4 runs at a full 72MHz. With a 16-bit resolution and a
 * time base of 72MHz, TIM4 will roll over every 910us The worst case
 * spi period allowed is 255us stall + (17 bits / 140KHz) -> 376us
 */
void InitImuCsTimer()
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 0xFFFFFFFF;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim3);

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);
	HAL_TIM_PWM_Init(&htim3);

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);

	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 100;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);

	/* Clear OC2PE bit to make compare values apply immediately */
	TIM3->CCMR1 &= ~(TIM_CCMR1_OC2PE);

    /* Enable PWM */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	/* Disable timer */
    TIM3->CR1 &= ~0x1;
}

/**
 * @brief Configures the period on TIM4
 *
 * @param period The timer period (in 72MHz ticks)
 *
 * @return void
 */
void ConfigureImuSpiTimer(uint32_t period)
{
	/* Disable timer */
	TIM4->CR1 &= ~0x1;

	/* Set new period. Period is stall + SCLK time */
	TIM4->ARR = period;

	/* Clear timer interrupt flag */
	TIM4->SR &= ~TIM_SR_UIF;
}

/**
 * @brief Inits TIM4 for use as a IMU spi period timer
 *
 * @return void
 *
 * TIM4 is used to drive SPI buffered data acquisition from the IMU.
 * One timer interrupt is generated per SPI word clocked out from the
 * DUT. TIM4 runs at a full 72MHz. With a 16-bit resolution and a
 * time base of 72MHz, TIM4 will roll over every 910us The worst case
 * spi period allowed is 255us stall + (17 bits / 140KHz) -> 376us
 */
void InitImuSpiTimer()
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	htim4.Instance = TIM4;
	/* 72MHz clock */
	htim4.Init.Prescaler = 0;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 0;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim4);

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig);

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig);

	/* Enable interrupt in timer */
	TIM4->DIER |= TIM_IT_UPDATE;

	/* Clear timer interrupt flag */
	TIM4->SR &= ~TIM_SR_UIF;

	/* Set interrupt priority and enable */
	HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

/**
 * @brief Processes any changes to IMU_SPI_CONFIG reg and applies
 *
 * @return void
 *
 * Sets the stall time (in microseconds) based on lower byte of the
 * IMU_SPI_CONFIG register. Sets the SPI clock frequency divider based
 * on the upper 8 bits. See register documentation for details of what
 * target SCLK frequencies are achievable.
 */
void UpdateImuSpiConfig()
{
	/* SCLK divider setting (format to be passed to HAL) */
	uint32_t sclkDividerSetting;

	/* SCLK + stall, in terms of 1/8th microsecond ticks */
	uint32_t spiPeriod;

	/* CS period (72MHz ticks) */
	uint32_t csPeriod;

	/* Get the config register value from reg array */
	uint16_t configReg = g_regs[IMU_SPI_CONFIG_REG];

	/* Stall time is lower 8 bits */
	if((configReg & 0xFF) < 2)
	{
		configReg &= 0xFF00;
		configReg |= 2;
	}
	/* set the stall time used to the stall time setting */
	imuStallTimeUs = (configReg & 0xFF);

	/* Sclk divider setting is upper 8 bits */
	if(configReg & (1 << 8))
	{
		/* 18 MHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_2;
		/* 16 bits * (72MHz / 18MHz) -> 16 * 4 */
		csPeriod = 17 * 4;
	}
	else if(configReg & (1 << 9))
	{
		/* 9 MHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_4;
		/* 16 bits * (72MHz / 9MHz) -> 16 * 8 */
		csPeriod = 17 * 8;
	}
	else if(configReg & (1 << 10))
	{
		/* 4.5 MHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_8;
		/* 16 bits * (72MHz / 4.5MHz) -> 16 * 16 */
		csPeriod = 17 * 16;
	}
	else if(configReg & (1 << 11))
	{
		/* 2.25 MHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_16;
		/* 16 bits * (72MHz / 2.25MHz) -> 16 * 32 */
		csPeriod = 17 * 32;
	}
	else if(configReg & (1 << 12))
	{
		/* 1.125 MHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_32;
		/* 16 bits * (72MHz / 1.125MHz) -> 16 * 64 */
		csPeriod = 17 * 64;
	}
	else if(configReg & (1 << 13))
	{
		/* 560 KHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_64;
		/* 16 bits * (72MHz / 560KHz) -> 16 * 128 */
		csPeriod = 17 * 128;
	}
	else if(configReg & (1 << 14))
	{
		/* 281 KHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_128;
		/* 16 bits * (72MHz / 281KHz) -> 16 * 256 */
		csPeriod = 17 * 256;
	}
	else if(configReg & (1 << 15))
	{
		/* 140 KHz */
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_256;
		/* 16 bits * (72MHz / 140KHz) -> 16 * 512 */
		csPeriod = 17 * 512;
	}
	else
	{
		/* Default to divider of 64 (36MHz / 32 = 1.125MHz SCLK, will work with all iSensors IMU's) */
		configReg |= (1 << 12);
		sclkDividerSetting = SPI_BAUDRATEPRESCALER_32;
		/* 16 bits * (72MHz / 1.125MHz) -> 16 * 64 */
		csPeriod = 17 * 64;
	}

	/* Tack an extra 400ns (72MHz * 0.4us) on to the CS time (account for delay from dropping CS to SPI data clocking out) */
	csPeriod += 28;

	/* Spi period is cs period + stall time */
	spiPeriod = csPeriod + (imuStallTimeUs * 72);

	/* Apply spi period to timer */
	ConfigureImuSpiTimer(spiPeriod);

	/* Apply CS period to timer */
	ConfigureImuCsTimer(csPeriod);

	/* Apply setting */
	ApplySclkDivider(sclkDividerSetting);

	/* Apply value back to IMU SPI config register */
	g_regs[IMU_SPI_CONFIG_REG] = configReg;
}

/**
  * @brief Applies baud rate divider setting to master SPI port (to IMU)
  *
  * @param preScalerSetting The SCLK pre-scaler setting to apply.
  *
  * @return void
  */
static void ApplySclkDivider(uint32_t preScalerSetting)
{
	g_spi1.Init.BaudRatePrescaler = preScalerSetting;

	RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
	HAL_SPI_Init(&g_spi1);
	SPI1->CR2 &= ~(SPI_CR2_FRXTH);
    /* Enable SPI peripheral */
    __HAL_SPI_ENABLE(&g_spi1);
}
