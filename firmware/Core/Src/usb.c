/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		usb.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer USB command line register interface
 **/

#include "usb.h"
#include "reg.h"
#include "main.h"
#include "usbd_cdc_if.h"
#include "script.h"
#include "timer.h"

/** USB handle (from usb_device.c) */
extern USBD_HandleTypeDef hUsbDeviceFS;

/** USB Rx buffer (from usbd_cdc_if.c) */
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** USB Tx buffer (from usbd_cdc_if.c) */
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/** USB Rx count (from usbd_cdc_if.c) */
extern uint32_t USBRxCount;

/** Current command string */
static uint8_t CurrentCommand[64];

/** Script object (parsed from current command) */
static script scr = {};

/** Byte buffer for echoing to console */
static uint8_t EchoBuf[4];

/** newline string */
static const uint8_t NewLineStr[] = "\r\n";

/**
  * @brief Toggle USB PU to force a re-enumeration on the host side
  *
  * @return void
  */
void USB_Reset()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Toggle USB PU high (PB11) */
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIOB->BSRR = GPIO_PIN_11;

	/* Wait 10ms for host to detect */
	Timer_Sleep_Microseconds(10000);

	/* Bring USB PU low again */
	GPIOB->BRR = GPIO_PIN_11;

	/* Set as input */
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief Handler for received USB data
  *
  * @return void
  *
  * This function should be called periodically from
  * the main loop to check if new USB data has been received
  */
void USB_Rx_Handler()
{
	/* Track index within current command string */
	static uint32_t commandIndex = 0;

	uint32_t bufIndex;
	uint32_t numBytes;

	/* Check if data has been received */
	if(USBRxCount)
	{
		numBytes = USBRxCount;
		USBRxCount = 0;
		for(bufIndex = 0; bufIndex < numBytes; bufIndex++)
		{
			/* Backspace typed in console */
			if(UserRxBufferFS[bufIndex] == '\b')
			{
				/* Move command index back one space */
				if(commandIndex > 0)
					commandIndex--;
				/* Echo \b, space, \b to console */
				if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
				{
					EchoBuf[0] = '\b';
					EchoBuf[1] = ' ';
					EchoBuf[2] = '\b';
					USB_Tx_Handler(EchoBuf, 3);
				}
			}
			/* carriage return char (end of command) */
			else if(UserRxBufferFS[bufIndex] == '\r')
			{
				/* Send newline char if CLI echo is enabled */
				if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
				{
					USB_Tx_Handler(NewLineStr, sizeof(NewLineStr));
				}
				/* Place a string terminator */
				CurrentCommand[commandIndex] = 0;
				/* Parse command */
				Script_Parse_Element(CurrentCommand, &scr);
				/* Execute command */
				Script_Run_Element(&scr, UserTxBufferFS, true);
				/* Clear command buffer */
				for(int i = 0; i < sizeof(CurrentCommand); i++)
					CurrentCommand[i] = 0;
				commandIndex = 0;
				return;
			}
			else
			{
				/* Add char to current command */
				if(commandIndex < 64)
				{
					CurrentCommand[commandIndex] = UserRxBufferFS[bufIndex];
					commandIndex++;
				}
				/* Echo to console */
				if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
				{
					EchoBuf[0] = UserRxBufferFS[bufIndex];
					USB_Tx_Handler(EchoBuf, 1);
				}
			}
		}
	}
}

/**
  * @brief USB write handler
  *
  * @param buf Buffer containing data to write
  *
  * @param count Number of bytes to write
  *
  * @return void
  *
  * This function is called by the script execution
  * routines, when a script object is executed
  * from a USB CLI context.
  */
void USB_Tx_Handler(const uint8_t* buf, uint32_t count)
{
	if(count == 0)
		return;
	if(USB_Wait_For_Tx_Done(50))
	{
		USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t *) buf, count);
		USBD_CDC_TransmitPacket(&hUsbDeviceFS);
	}
}

/**
  * @brief Helper function to wait for the USB Tx to be free
  *
  * @param TimeoutMs Operation timeout (in ms). Must keep under watchdog reset period
  *
  * @return true if Tx is free, false is timeout elapses
  */
bool USB_Wait_For_Tx_Done(uint32_t TimeoutMs)
{
	uint32_t endTime = HAL_GetTick() + TimeoutMs;
	/* Check for tx busy */
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
	while((hcdc->TxState != 0) && (HAL_GetTick() < endTime));
	/* Return true if Tx is free, false otherwise */
	return (hcdc->TxState == 0);
}

/**
  * @brief Helper function to set the watermark interrupt level for optimal USB streams
  *
  * @return void
  *
  * This function can be called to configure the watermark interrupt level
  * for ideal operation using a USB CLI stream. To get maximum throughput
  * over the USB CLI stream, while maintaining minimal latency, the watermark
  * interrupt level should be set such that each USB transmission occupies a
  * single full buffer (512 bytes).
  *
  * After calling this function, WATERMARK_INT_CONFIG will be set based on the
  * current buffer length setting.
  */
void USB_Watermark_Autoset()
{
	uint16_t bufNumRegs, bufCliLen, waterMarkLevel;

	/* Get the number of regs in a buffer */
	bufNumRegs = g_regs[BUF_LEN_REG];
	bufNumRegs = bufNumRegs >> 1;

	/* Each CLI buffer entry has five extra regs, and \r\n at end of line
	 * Each reg is 5 bytes (4 hex chars + delim), except last reg, which
	 * does not have delim */
	bufCliLen = ((bufNumRegs + 5) * 5) + 1;

	/* Save to reg (preserving pulse bit) */
	waterMarkLevel = STREAM_BUF_SIZE / bufCliLen;
	g_regs[WATERMARK_INT_CONFIG_REG] &= WATERMARK_PULSE_MASK;
	g_regs[WATERMARK_INT_CONFIG_REG] |= waterMarkLevel;
}
