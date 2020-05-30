/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		main.c
  * @date		3/14/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer main. Contains STM init functions and application cyclic executive.
 **/

#include "main.h"

/** SPI handle for IMU master port */
SPI_HandleTypeDef hspi1;

/** SPI handle for slave port (from master controller) */
SPI_HandleTypeDef hspi2;

/** SPI handle for SD card master port */
SPI_HandleTypeDef hspi3;

/** TIM2 handle */
TIM_HandleTypeDef htim2;

/* User SPI DMA handles */
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

/* Update processing required */
volatile extern uint32_t update_flags;

/* Local function prototypes */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI3_Init(void);
static void DWT_Init();
static void MX_DMA_Init(void);
static void ConfigureSampleTimer(uint32_t timerfreq);

/**
  * @brief  The application entry point.
  *
  * @return main status code. Should not return.
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Check if system had previously encountered an unexpected fault */
  FlashInitErrorLog();

  /* Check if system previously reset due to watch dog */
  CheckWatchDogStatus();

  /* Enable watch dog timer (2 seconds period) */
  EnableWatchDog(2000);

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI3_Init();
  DWT_Init();

  /* Load registers from flash */
  LoadRegsFlash();

  /* Generate all identifier registers */
  GetBuildDate();
  GetSN();

  /* Check for logged error codes and update FAULT_CODE */
  FlashCheckLoggedError();

  /* Init buffer */
  BufReset();

  /* Init sample timer (TIM2) */
  ConfigureSampleTimer(1000000);

  /* Init IMU stall timer (TIM3) */
  InitIMUStallTimer();

  /* Config IMU SPI settings */
  UpdateImuSpiConfig();

  /* Init DIOs*/
  UpdateDIOConfig();

  /* Init data ready */
  UpdateDRConfig();

  /* Set DR int priority (lower than user SPI - no preemption) */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);

  /* Configure and enable user SPI port (based on loaded register values) */
  UpdateUserSpiConfig();

  /* Enable DMA channels associated with user SPI */
  MX_DMA_Init();

  /* Clear all update flags before entering loop (shouldn't be set anyways, but it doesn't hurt) */
  update_flags = 0;

  /* Infinite loop */
  while(1)
  {
	  /* Process buf dequeue (high priority) */
	  if(update_flags & DEQUEUE_BUF_FLAG)
	  {
		  update_flags &= ~DEQUEUE_BUF_FLAG;
		  BufDequeueToOutputRegs();
	  }
	  /* Process other register update flags (lower priority, done in priority order) */
	  else if(update_flags)
	  {
		  if(update_flags & ENABLE_CAPTURE_FLAG)
		  {
			  update_flags &= ~ENABLE_CAPTURE_FLAG;
			  EnableDataCapture();
		  }
		  else if(update_flags & USER_COMMAND_FLAG)
		  {
			  update_flags &= ~USER_COMMAND_FLAG;
			  ProcessCommand();
		  }
		  else if(update_flags & DR_CONFIG_FLAG)
		  {
			  update_flags &= ~DR_CONFIG_FLAG;
			  UpdateDRConfig();
		  }
		  else if(update_flags & DIO_CONFIG_FLAG)
		  {
			  update_flags &= ~DIO_CONFIG_FLAG;
			  UpdateDIOConfig();
		  }
		  else if(update_flags & IMU_SPI_CONFIG_FLAG)
		  {
			  update_flags &= ~IMU_SPI_CONFIG_FLAG;
			  UpdateImuSpiConfig();
		  }
		  else if(update_flags & USER_SPI_CONFIG_FLAG)
		  {
			  update_flags &= ~USER_SPI_CONFIG_FLAG;
			  UpdateUserSpiConfig();
		  }
	  }
	  /* Housekeeping and interrupt generation */
	  else
	  {
		  /* Check user interrupt generation status */
		  UpdateUserInterrupt();

		  /* Check if red LED needs to be set (status error) */
		  UpdateLEDStatus();
	  }

	  /* Feed watch dog timer */
	  FeedWatchDog();
  }

  /* Should never get here */
  return 0;
}

/**
  * @brief Gets two bit hardware identification code from identifier pins
  *
  * @return ID code read from ID pins (0 - 3)
  */
uint32_t GetHardwareID()
{
	uint32_t id;

	/* Get ID from GPIO port C, pins 2-3 */
	id = GPIOC->IDR;
	id = (id >> 2) & 0x3;
	return id;
}

/**
  * @brief  This function is executed in case of error occurrence.
  *
  * @return void
  *
  * Errors will force a system reset
  */
void Error_Handler(void)
{
	/* Log error for future retrieval */
	FlashLogError(ERROR_INIT);
	/* Reset system */
	NVIC_SystemReset();
}

/**
  * @brief Enables IMU sample timestamp timer
  *
  * @param timerfreq The desired timer freq (in Hz)
  *
  * @return void
  *
  * This function should be called as part of the buffered data
  * acquisition startup process. The enabled timer is TIM2 (32 bit)
  */
static void ConfigureSampleTimer(uint32_t timerfreq)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};

	htim2.Instance = TIM2;
	/* Set prescaler to give desired timer freq */
	htim2.Init.Prescaler = (SystemCoreClock / timerfreq) - 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 0xFFFFFFFF;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim2);

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

	/* Enable timer */
	TIM2->CR1 = 0x1;
}

/**
  * @brief Enable DMA channels in use
  *
  * @return void
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* SPI2 DMA Init */
  /* SPI2_RX Init */
  hdma_spi2_rx.Instance = DMA1_Channel4;
  hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi2_rx.Init.Mode = DMA_NORMAL;
  hdma_spi2_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_LINKDMA(&hspi2,hdmarx,hdma_spi2_rx);

  /* SPI2_TX Init */
  hdma_spi2_tx.Instance = DMA1_Channel5;
  hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi2_tx.Init.Mode = DMA_NORMAL;
  hdma_spi2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_LINKDMA(&hspi2,hdmatx,hdma_spi2_tx);

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

/**
  * @brief System Clock Configuration
  *
  * @return void
  */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM2|RCC_PERIPHCLK_TIM34;
  PeriphClkInit.Tim2ClockSelection = RCC_TIM2CLK_HCLK;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function (master SPI port)
  *
  * @param None
  *
  * @return void
  */
static void MX_SPI1_Init(void)
{
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

  SPI1->CR2 &= ~(SPI_CR2_FRXTH);

  /* Check if the SPI is already enabled */
  if ((hspi1.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
  {
    /* Enable SPI peripheral */
    __HAL_SPI_ENABLE(&hspi1);
  }

}

/**
  * @brief SPI3 Initialization Function (SD card master SPI port)
  *
  * @param None
  *
  * @return void
  */
static void MX_SPI3_Init(void)
{
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  *
  * @param None
  *
  * @return void
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO port A pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO port B pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO port C pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO port A input pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO port A output pins : PA4 PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO port B input pins : PB5 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO port B output pins : PB4 PB6 PB7 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO port C input pins : PC2 PC3 PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO port C output pins : PC0 PC1 PC7 PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
  * @brief  Init DWT peripheral.
  *
  * @return void
  */
static void DWT_Init()
{
  /* Disable TRC */
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
  /* Enable TRC */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;
  /* Disable clock cycle counter */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  /* Enable clock cycle counter */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

  /* Enable watch dog debug freeze */
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}
