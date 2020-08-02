/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		cli.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer USB command line register interface
 **/

#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "main.h"
#include "usbd_cdc_if.h"
#include "registers.h"

/* Public function prototypes */
void USBSerialHandler();
void USBReadBuf();

#define USB_STREAM_BITP				0
#define USB_ECHO_BITP				1
#define SD_AUTORUN_BITP				2
#define USB_DELIM_BITP				8

#define USB_STREAM_BITM				(1 << USB_STREAM_BITP)
#define USB_ECHO_BITM				(1 << USB_ECHO_BITP)
#define SD_AUTORUN_BITM				(1 << SD_AUTORUN_BITP)
#define USB_DELIM_BITM				(0xFF << USB_DELIM_BITP)

/** Buffer output base address (on page 255) */
#define BUF_BASE_ADDR			8

/** Buffer output data base address (on page 255) */
#define BUF_DATA_BASE_ADDR		16

#endif /* INC_CLI_H_ */
