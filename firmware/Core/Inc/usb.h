/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		usb.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer USB command line register interface
 **/

#ifndef INC_USB_H_
#define INC_USB_H_

/* Header includes require for prototypes */
#include <stdint.h>
#include "main.h"

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void USB_Rx_Handler();
bool USB_Wait_For_Tx_Done(uint32_t TimeoutMs);
void USB_Tx_Handler(const uint8_t* buf, uint32_t count);
void USB_Watermark_Autoset();
void USB_Reset();
/* @endcond */

#endif /* INC_USB_H_ */
