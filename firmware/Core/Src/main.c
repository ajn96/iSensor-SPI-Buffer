/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		main.c
  * @date		3/14/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer main. Contains STM init functions and application cyclic executive.
 **/

#include "usb.h"
#include "main.h"
#include "reg.h"
#include "imu.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include "sd_card.h"
#include "buffer.h"
#include "user_interrupt.h"
#include "flash.h"
#include "led.h"
#include "watchdog.h"
#include "timer.h"
#include "dio.h"
#include "adc.h"
#include "dfu.h"
#include "user_spi.h"
#include "data_capture.h"
#include "script.h"

/* Private function prototypes */
static void SystemClock_Config();
static void MX_GPIO_Init();
static void DMA_Init();

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
int main()
{
  /* Check if application needs to reboot into DFU mode prior to any config */
  DFU_Check_Flags();
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  /* Enable watch dog timer (2 seconds period) */
  Watchdog_Enable(2000);
  /* Check if system had previously encountered an unexpected fault */
  Flash_Fault_Log_Init();
  /* Initialize GPIO */
  MX_GPIO_Init();
  /* Init misc timers  */
  Timer_Init();
  /* Initialize IMU SPI port */
  IMU_SPI_Init();
  /* Initialize SD card interface */
  SD_Card_Init();
  /* Initialize USB hardware */
  MX_USB_DEVICE_Init();
  /* Load register values */
  Reg_Init();
  /* Check for logged error codes and update FAULT_CODE */
  Flash_Check_Logged_Fault();
  /* Check if system previously reset due to watch dog */
  Watchdog_Check_Status();
  /* Init buffer */
  Buffer_Reset();
  /* Config IMU SPI settings */
  IMU_Update_SPI_Config();
  /* Init DIO outputs */
  DIO_Update_Output_Config();
  /* Init DIO inputs (data ready and PPS) */
  DIO_Update_Input_Config();
  /* Disable data capture initially */
  Data_Capture_Disable();
  /* Set DR int priority (lower than user SPI - no preemption) */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  /* Init ADC for housekeeping */
  ADC_Init();
  /* Configure and enable user SPI port (based on loaded register values) */
  UpdateUserSpiConfig(false);
  /* Trigger a USB re-enum on host */
  USB_Reset();
  /* Init and Enable DMA channels */
  DMA_Init();
  /* Check for script auto-run set immediately before entering cyclic executive */
  SD_Card_Script_Autorun();

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
		  Reg_Buf_Dequeue_To_Outputs();
	  }

	  /* Check user interrupt generation status */
	  User_Interrupt_Update();

	  /* Feed watch dog timer */
	  Watchdog_Feed();

	  /* State machine for non-timing critical processing tasks */
	  switch(state)
	  {
	  case STATE_CHECK_FLAGS:
		  /* Handle capture disable */
		  if(g_update_flags & DISABLE_CAPTURE_FLAG)
		  {
			  g_update_flags &= ~DISABLE_CAPTURE_FLAG;
			  /* Make sure both can't be set */
			  g_update_flags &= ~(ENABLE_CAPTURE_FLAG);
			  Data_Capture_Disable();
		  }
		  /* Handle change to DIO input config (validate only, no enable) */
		  else if(g_update_flags & DIO_INPUT_CONFIG_FLAG)
		  {
			  g_update_flags &= ~DIO_INPUT_CONFIG_FLAG;
			  DIO_Validate_Input_Config();
		  }
		  /* Handle capture enable */
		  else if(g_update_flags & ENABLE_CAPTURE_FLAG)
		  {
			  g_update_flags &= ~ENABLE_CAPTURE_FLAG;
			  Data_Capture_Enable();
		  }
		  /* Handle user commands */
		  else if(g_update_flags & USER_COMMAND_FLAG)
		  {
			  g_update_flags &= ~USER_COMMAND_FLAG;
			  Reg_Process_Command();
		  }
		  /* Handle capture DIO output config change */
		  else if(g_update_flags & DIO_OUTPUT_CONFIG_FLAG)
		  {
			  g_update_flags &= ~DIO_OUTPUT_CONFIG_FLAG;
			  DIO_Update_Output_Config();
		  }
		  /* Handle change to IMU SPI config */
		  else if(g_update_flags & IMU_SPI_CONFIG_FLAG)
		  {
			  g_update_flags &= ~IMU_SPI_CONFIG_FLAG;
			  IMU_Update_SPI_Config();
		  }
		  /* Handle change to user SPI config */
		  else if(g_update_flags & USER_SPI_CONFIG_FLAG)
		  {
			  g_update_flags &= ~USER_SPI_CONFIG_FLAG;
			  UpdateUserSpiConfig(true);
		  }
		  /* Advance to next state */
		  state = STATE_CHECK_PPS;
		  break;
	  case STATE_CHECK_PPS:
		  /* Check that PPS isn't unlocked */
		  Timer_Check_PPS_Unlock();
		  /* Advance to next state */
		  state = STATE_READ_ADC;
		  break;
	  case STATE_READ_ADC:
		  /* Update ADC state machine */
		  ADC_Update();
		  /* Advance to next state */
		  state = STATE_CHECK_USB;
		  break;
	  case STATE_CHECK_USB:
		  /* Handle any USB command line Rx activity */
		  USB_Rx_Handler();
		  /* Advance to next state */
		  state = STATE_CHECK_STREAM;
		  break;
	  case STATE_CHECK_STREAM:
		  /* Check stream status for CLI */
		  Script_Check_Stream();
		  /* Advance to next state */
		  state = STATE_STEP_SCRIPT;
		  break;
	  case STATE_STEP_SCRIPT:
		  /* Call SD script step function */
		  SD_Card_Script_Step();
	  default:
		  /* Go back to first state */
		  state = STATE_CHECK_FLAGS;
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
  * During init, errors are logged, then init continues. This is
  * done to reduce potential for a fault locking the device in a reset
  * loop.
  */
void Main_Error_Handler()
{
	/* Log error for future retrieval */
	Flash_Log_Fault(ERROR_INIT);
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
static void DMA_Init()
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
	  Main_Error_Handler();
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
	  Main_Error_Handler();
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
	  Main_Error_Handler();
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
static void SystemClock_Config()
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
    Main_Error_Handler();
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
    Main_Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB
		  	  	  	  	  	  	  	  |RCC_PERIPHCLK_TIM2
									  |RCC_PERIPHCLK_TIM34
									  |RCC_PERIPHCLK_ADC12
									  |RCC_PERIPHCLK_TIM8
  	  	  	  	  	  	  	  	  	  |RCC_PERIPHCLK_TIM16;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  PeriphClkInit.Tim2ClockSelection = RCC_TIM2CLK_HCLK;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV16;
  PeriphClkInit.Tim8ClockSelection = RCC_TIM8CLK_HCLK;
  PeriphClkInit.Tim16ClockSelection = RCC_TIM16CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Main_Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  *
  * @return void
  */
static void MX_GPIO_Init()
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
