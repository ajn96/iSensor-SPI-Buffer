/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		data_capture.h
  * @date		4/24/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer autonomous IMU data acquisition header file.
 **/


#ifndef INC_DATA_CAPTURE_H_
#define INC_DATA_CAPTURE_H_

void EnableDataCapture();
void DisableDataCapture();
void UpdateDRConfig();

#endif /* INC_DATA_CAPTURE_H_ */
