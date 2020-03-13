# iSensor-SPI-Buffer
Firmware for the STM32F303 (Nucleo-64) to enable full throughput buffered data capture on Analog Devices IMUs

## Hardware Platform

* This firmware is designed to run on the Nucleo-F303RE board (based on STM32F303)
* Will be compatible with all modern Analog Devices iSensor IMUs (excluding those which have an automotive SPI interface)

## Development Environment

The iSensor SPI buffer project can be loaded and built by Keil IDE

## Design Goals

* "Invisible" SPI pass through to a connected iSensor IMU (possibly added stall time)
