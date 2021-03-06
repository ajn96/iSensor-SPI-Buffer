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
#include "math.h"

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

	/** Active PPS source pin. Set/cleared in PPS enable/disable command function */
	uint32_t ppsPin;

}DIOConfig;

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void UpdateDIOInputConfig();
void ValidateDIOInputConfig();
void UpdateDIOOutputConfig();
void StartSyncGen();
uint32_t GetHardwareID();
/* @endcond */

/** Mask for EXTI interrupt sources which come from the IMU data ready signal */
#define DATA_READY_INT_MASK 	GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_9

/** Mask to clear reserved bits in DIO_INPUT_CONFIG */
#define DIO_INPUT_CLEAR_MASK	0x3F9F

#endif /* INC_DIO_H_ */
