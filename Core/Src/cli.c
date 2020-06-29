/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		cli.c
  * @date		6/26/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		Implementation file for iSensor-SPI-Buffer command line register interface
 **/

#include "cli.h"

/* Private function prototypes */
static void ParseCommand();
static void Read();
static void Write();
static void ReadBuf();
static void Stream();
static void PrintHelp(uint8_t * outBuf);
static void ReadHandler(uint32_t startAddr, uint32_t endAddr, uint8_t * outBuf);


