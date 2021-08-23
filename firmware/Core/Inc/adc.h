/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		adc.h
  * @date		7/10/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Header file for iSensor-SPI-Buffer ADC module (for temp sensor and Vdd monitoring)
 **/

#ifndef INC_ADC_H_
#define INC_ADC_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

/* @cond DOXYGEN_IGNORE */
void ADCInit();
void UpdateADC();
/* @endcond */

/** Temp sensor value at 30C (for two point calibration) */
#define TS_CAL1 		((uint16_t*) ((uint32_t) 0x1FFFF7B8))

/** Temp sensor value at 110 C (for two point calibration) */
#define TS_CAL2 		((uint16_t*) ((uint32_t) 0x1FFFF7C2))

/** VREFINT calibration value */
#define VREFINT_CAL		((uint16_t*) ((uint32_t) 0x1FFFF7BA))

/** VDD start measurement state */
#define ADC_VDD_START		0

/** Wait for VDD measurement EoC state */
#define ADC_VDD_READ		1

/** Temp sensor start measurement state */
#define ADC_TEMP_START		2

/** Wait for temp measurement EoC state */
#define ADC_TEMP_READ		3

#endif /* INC_ADC_H_ */
