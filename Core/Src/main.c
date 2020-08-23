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
#include "usbd_cdc_if.h"
#include "usb_cli.h"
#include "sd_card.h"

/* Private function prototypes */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void DMA_Init();
static void DWT_Init();

/* Update processing required (from registers.c) */
extern volatile uint32_t g_update_flags;

/** IMU SPI Rx DMA handle. Global scope */
DMA_HandleTypeDef g_dma_spi1_rx;

/** IMU SPI Tx DMA handle. Global scope */
DMA_HandleTypeDef g_dma_spi1_tx;

/** User SPI Tx DMA handle. Global scope */
DMA_HandleTypeDef g_dma_spi2_tx;

/** SPI handle for IMU master port. Global scope */
SPI_HandleTypeDef g_spi1;

/** SPI handle for slave port (from master controller). Global scope */
SPI_HandleTypeDef g_spi2;

/** Track the cyclic executive state */
static uint32_t state;

/**
  * @brief The application entry point.
  *
  * @return main status code. Should not return.
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Enable watch dog timer (2 seconds period) */
  EnableWatchDog(2000);

  /* Check if system had previously encountered an unexpected fault */
  FlashInitErrorLog();

  /* Initialize GPIO */
  MX_GPIO_Init();

  /* Initialize DWT timer peripheral */
  DWT_Init();

  /* Initialize IMU SPI port */
  MX_SPI1_Init();

  /* Initialize SD card interface */
  SDCardInit();

  /* Initialize USB hardware */
  MX_USB_DEVICE_Init();

  /* Load registers from flash */
  LoadRegsFlash();

  /* Load build date from .data to register array */
  GetBuildDate();

  /* Load STM32 unique SN to register array */
  GetSN();

  /* Check for logged error codes and update FAULT_CODE */
  FlashCheckLoggedError();

  /* Check if system previously reset due to watch dog */
  CheckWatchDogStatus();

  /* Init buffer */
  BufReset();

  /* Init microsecond sample timer (TIM2)  */
  InitMicrosecondTimer();

  /* Init IMU spi period timer (TIM4) */
  InitImuSpiTimer();

  /* Init IMU CS timer in PWM mode (TIM3) */
  InitImuCsTimer();

  /* Config IMU SPI settings */
  UpdateImuSpiConfig();

  /* Init DIO outputs */
  UpdateDIOOutputConfig();

  /* Init DIO inputs (data ready and PPS) */
  UpdateDIOInputConfig();

  /* Disable data capture initially */
  DisableDataCapture();

  /* Set DR int priority (lower than user SPI - no preemption) */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);

  /* Init ADC for housekeeping */
  ADCInit();

  /* Configure and enable user SPI port (based on loaded register values) */
  UpdateUserSpiConfig();

  /* Init and Enable DMA channels */
  DMA_Init();

  /* Check for script auto-run set immediately before entering cyclic executive */
  ScriptAutorun();

  /* Set state to 0 */
  state = 0;

  /* Clear all update flags before entering loop (shouldn't be set anyways, but it doesn't hurt) */
  g_update_flags = 0;

  /* Infinite cyclic executive loop */
  while(1)
  {
	  /* Process buf dequeue every loop iteration (high priority) */
	  if(g_update_flags & DEQUEUE_BUF_FLAG)
	  {
		  g_update_flags &= ~DEQUEUE_BUF_FLAG;
		  BufDequeueToOutputRegs();
	  }

	  /* Check user interrupt generation status */
	  UpdateUserInterrupt();

	  /* Feed watch dog timer */
	  FeedWatchDog();

	  /* State machine for non-timing critical processing tasks */
	  switch(state)
	  {
	  case 0:
		  /* Handle capture enable */
		  if(g_update_flags & ENABLE_CAPTURE_FLAG)
		  {
			  g_update_flags &= ~ENABLE_CAPTURE_FLAG;
			  EnableDataCapture();
		  }
		  /* Advance to next state */
		  state = 1;
		  break;
	  case 1:
		  /* Handle user commands */
		  if(g_update_flags & USER_COMMAND_FLAG)
		  {
			  g_update_flags &= ~USER_COMMAND_FLAG;
			  ProcessCommand();
		  }
		  /* Advance to next state */
		  state = 2;
		  break;
	  case 2:
		  /* Handle change to DIO input config */
		  if(g_update_flags & DIO_INPUT_CONFIG_FLAG)
		  {
			  g_update_flags &= ~DIO_INPUT_CONFIG_FLAG;
			  UpdateDIOInputConfig();
		  }
		  /* Advance to next state */
		  state = 3;
		  break;
	  case 3:
		  /* Handle capture DIO output config change */
		  if(g_update_flags & DIO_OUTPUT_CONFIG_FLAG)
		  {
			  g_update_flags &= ~DIO_OUTPUT_CONFIG_FLAG;
			  UpdateDIOOutputConfig();
		  }
		  /* Advance to next state */
		  state = 4;
		  break;
	  case 4:
		  /* Handle change to IMU SPI config */
		  if(g_update_flags & IMU_SPI_CONFIG_FLAG)
		  {
			  g_update_flags &= ~IMU_SPI_CONFIG_FLAG;
			  UpdateImuSpiConfig();
		  }
		  /* Advance to next state */
		  state = 5;
		  break;
	  case 5:
		  /* Handle change to user SPI config */
		  if(g_update_flags & USER_SPI_CONFIG_FLAG)
		  {
			  g_update_flags &= ~USER_SPI_CONFIG_FLAG;
			  UpdateUserSpiConfig();
		  }
		  /* Advance to next state */
		  state = 6;
		  break;
	  case 6:
		  /* Check that PPS isn't unlocked */
		  CheckPPSUnlock();
		  /* Advance to next state */
		  state = 7;
		  break;
	  case 7:
		  /* Update ADC state machine */
		  UpdateADC();
		  /* Advance to next state */
		  state = 8;
		  break;
	  case 8:
		  /* Handle any USB command line Rx activity */
		  USBRxHandler();
		  /* Advance to next state */
		  state = 9;
		  break;
	  case 9:
		  /* Check stream status for CLI */
		  CheckStream();
		  /* Go back to first state */
		  state = 0;
		  break;
	  default:
		  /* Should not get here, go back to first state */
		  state = 0;
		  break;
	  }
  }

  /* Should never get here */
  return 0;
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
}

/**
  * @brief Init and DMA channels in use and configure DMA interrupts
  *
  * @return void
  *
  * Currently use three DMA channels, on DMA peripheral 1:
  * DMA2/3 - IMU SPI port, used for burst reads from IMU
  * DMA5 - User SPI port, Used for buffer burst outputs. Rx not used
  */
static void DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* SPI1 DMA Init */

	/* SPI1_RX Init */
	g_dma_spi1_rx.Instance = DMA1_Channel2;
	g_dma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	g_dma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_dma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
	g_dma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	g_dma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	g_dma_spi1_rx.Init.Mode = DMA_NORMAL;
	g_dma_spi1_rx.Init.Priority = DMA_PRIORITY_HIGH;
	if (HAL_DMA_Init(&g_dma_spi1_rx) != HAL_OK)
	{
	  Error_Handler();
	}
	__HAL_LINKDMA(&g_spi1,hdmarx,g_dma_spi1_rx);

	/* SPI1_TX Init */
	g_dma_spi1_tx.Instance = DMA1_Channel3;
	g_dma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	g_dma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_dma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
	g_dma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	g_dma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	g_dma_spi1_tx.Init.Mode = DMA_NORMAL;
	g_dma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
	if (HAL_DMA_Init(&g_dma_spi1_tx) != HAL_OK)
	{
	  Error_Handler();
	}
	__HAL_LINKDMA(&g_spi1,hdmatx,g_dma_spi1_tx);

	/* SPI2_TX Init */
	g_dma_spi2_tx.Instance = DMA1_Channel5;
	g_dma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	g_dma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	g_dma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
	g_dma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	g_dma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	g_dma_spi2_tx.Init.Mode = DMA_NORMAL;
	g_dma_spi2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
	if (HAL_DMA_Init(&g_dma_spi2_tx) != HAL_OK)
	{
	  Error_Handler();
	}
	__HAL_LINKDMA(&g_spi2,hdmatx,g_dma_spi2_tx);

	/* DMA interrupt init */

	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	/* DMA1_Channel3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_TIM2
                              |RCC_PERIPHCLK_TIM34|RCC_PERIPHCLK_ADC12;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  PeriphClkInit.Tim2ClockSelection = RCC_TIM2CLK_HCLK;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function (master SPI port to IMU)
  *
  * @param None
  *
  * @return void
  */
static void MX_SPI1_Init(void)
{
	/* SPI1 parameter configuration*/
	g_spi1.Instance = SPI1;
	g_spi1.Init.Mode = SPI_MODE_MASTER;
	g_spi1.Init.Direction = SPI_DIRECTION_2LINES;
	g_spi1.Init.DataSize = SPI_DATASIZE_16BIT;
	g_spi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
	g_spi1.Init.CLKPhase = SPI_PHASE_2EDGE;
	g_spi1.Init.NSS = SPI_NSS_SOFT;
	g_spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	g_spi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	g_spi1.Init.TIMode = SPI_TIMODE_DISABLE;
	g_spi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	g_spi1.Init.CRCPolynomial = 7;
	g_spi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	g_spi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	if (HAL_SPI_Init(&g_spi1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Set fifo rx threshold according the reception data length: 16bit */
	CLEAR_BIT(SPI1->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((g_spi1.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(&g_spi1);
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
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO port A pin Output Level (high) */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);

	/*Configure GPIO port B pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO port C pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

	/*Configure GPIO port A input pin : PA8 PA9 */
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO port A output pins : PA3 (IMU Reset) */
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO port B input pins : PB4 PB5 PB8 PB9 */
	GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO port B output pins : PB6 PB7 (Switch control) */
	GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO port C input pins : PC2 PC3 PC6 PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO port C output pins : PC0 PC1 PC8 PC9 (LED, Switch control) */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_8|GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/**TIM3 GPIO Configuration PA4     ------> TIM3_CH2 */
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO port D input pins : PD2 (SD card detect) */
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
  * @brief  Init DWT peripheral.
  *
  * @return void
  *
  * This peripheral is used to count microseconds for
  * the timer module.
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
