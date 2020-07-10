/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		adc.h
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

#endif /* INC_ADC_H_ */
