/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		usb_cli.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer USB command line register interface
 **/

#ifndef INC_USB_CLI_H_
#define INC_USB_CLI_H_

/* Header includes require for prototypes */
#include <stdint.h>
#include "main.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
bool USBWaitForTxDone(uint32_t TimeoutMs);
void USBRxHandler();
void USBTxHandler(const uint8_t* buf, uint32_t count);
void WatermarkLevelAutoset();
void USBReset();
/* @endcond */

#endif /* INC_USB_CLI_H_ */
