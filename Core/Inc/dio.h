/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		dio.h
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer DIO interfacing module
 **/

#ifndef INC_DIO_H_
#define INC_DIO_H_

#include "registers.h"
#include "data_capture.h"

/** Struct representing DIO configuration settings */
typedef struct DIOConfig
{
	uint32_t passPins;
	uint32_t watermarkPins;
	uint32_t overflowPins;
	uint32_t errorPins;
}DIOConfig;

/* Public function prototypes */
void UpdateDIOInputConfig();
void UpdateDIOOutputConfig();


#endif /* INC_DIO_H_ */
