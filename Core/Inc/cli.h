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
void USBRxHandler();
void USBReadBuf();
void USBTxHandler(uint8_t* buf, uint32_t count);

/** Buffer output base address (on page 255) */
#define BUF_BASE_ADDR			8

/** Buffer output data base address (on page 255) */
#define BUF_DATA_BASE_ADDR		16

#endif /* INC_CLI_H_ */
