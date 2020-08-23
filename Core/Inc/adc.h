/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		temp.h
  * @date		7/10/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer ADC module (for temp sensor)
 **/

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "registers.h"
#include "stm32f3xx_hal_conf.h"

void TempInit();
void UpdateTemp();

/** Temp sensor value at 30C (for two point calibration) */
#define TS_CAL1 		((uint16_t*) ((uint32_t) 0x1FFFF7B8))

/** Temp sensor value at 110 C (for two point calibration) */
#define TS_CAL2 		((uint16_t*) ((uint32_t) 0x1FFFF7C2))

/** VREFINT calibration value */
#define VREFINT_CAL		((uint16_t*) ((uint32_t) 0x1FFFF7BA))

#endif /* INC_ADC_H_ */
