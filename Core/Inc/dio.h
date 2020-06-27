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
	/** Pins to connect directly from host to IMU */
	uint32_t passPins;

	/** Watermark interrupt pins */
	uint32_t watermarkPins;

	/** Overflow interrupt pins */
	uint32_t overflowPins;

	/** Error interrupt pins */
	uint32_t errorPins;
}DIOConfig;

/* Public function prototypes */
void UpdateDIOInputConfig();
void UpdateDIOOutputConfig();
uint32_t GetHardwareID();

#define DATA_READY_INT_MASK 	(1<<5)|(1<<6)|(1<<9)

#endif /* INC_DIO_H_ */
