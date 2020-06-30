/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		cli.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer command line register interface
 **/

#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "main.h"
#include "usbd_cdc_if.h"
#include "registers.h"

void USBSerialHandler();
void USBReadBuf();

#define BUF_BASE_ADDR			8

#define BUF_DATA_BASE_ADDR		16


#endif /* INC_CLI_H_ */
