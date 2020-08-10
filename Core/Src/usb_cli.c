/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		usb_cli.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer USB command line register interface
 **/

#include "usb_cli.h"

/* Private function prototypes */
static void BlockingUSBTransmit(const uint8_t * buf, uint32_t Len, uint32_t TimeoutMs);

/** Global register array. (from registers.c) */
extern volatile uint16_t g_regs[];

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
  * @brief Handler for received USB data
  *
  * @return void
  *
  * This function should be called periodically from
  * the main loop to check if new USB data has been received
  */
void USBRxHandler()
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
					BlockingUSBTransmit(EchoBuf, 3, 20);
				}
			}
			/* carriage return char (end of command) */
			else if(UserRxBufferFS[bufIndex] == '\r')
			{
				/* Send newline char if CLI echo is enabled */
				if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
				{
					BlockingUSBTransmit(NewLineStr, sizeof(NewLineStr), 20);
				}
				/* Place a string terminator */
				CurrentCommand[commandIndex] = 0;
				/* Parse command */
				ParseScriptElement(CurrentCommand, &scr);
				/* Execute command */
				RunScriptElement(&scr, UserTxBufferFS, true);
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
					BlockingUSBTransmit(EchoBuf, 1, 20);
				}
			}
		}
	}
}

void USBTxHandler(const uint8_t* buf, uint32_t count)
{
	BlockingUSBTransmit(buf, count, 20);
}

/**
  * @brief Wrapper around CDC_Transmit_FS which retries until the USB transmit is successful
  *
  * @param buf The data buffer to transmit
  *
  * @param Len The number of bytes to transmit
  *
  * @param TimeoutMs Operation timeout (in ms). Must keep under watchdog reset period
  *
  * @return void
  */
static void BlockingUSBTransmit(const uint8_t * buf, uint32_t Len, uint32_t TimeoutMs)
{
	uint8_t status = CDC_Transmit_FS(buf, Len);
	uint32_t endTime = HAL_GetTick() + TimeoutMs;
	while((status != USBD_OK)&&(HAL_GetTick() < endTime))
	{
		status = CDC_Transmit_FS(buf, Len);
	}
}
