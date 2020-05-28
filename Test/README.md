# iSensor-SPI-Buffer Test Project

## Software Setup

* These tests use the NUnit 2.6.4 testing framework (bin included in repo) to perform system level testing on the iSensor-SPI-Buffer firmware
* Test cases interface to the iSensor-SPI-Buffer DUT over USB using an [iSensor FX3 Eval Board](https://github.com/juchong/iSensor-FX3-API), via the FX3 API
* Test cases exercise system level functionality of the iSensor-SPI-Buffer firmware and hardware
* The iSensor-SPI-Buffer firmware to be tested must already be loaded and running on the DUT prior to starting the test process - test cases do not cover source code compilation (may be added in the future)

## Hardware Setup

In order for all test cases to function as expected, the iSensor-SPI-Buffer DUT must be connected as follows:
* iSensor-Buffer-SPI Slave SPI port connected to iSensor FX3 master SPI port
* iSensor FX3 DIO1 - DIO4 connected to corresponding iSensor-SPI-Buffer slave DIO1 - DIO4
* iSensor-SPI-Buffer IMU SPI MOSI connected to IMU SPI MISO (for loop back)
* iSensor-SPI-Buffer master DIO1 (would typically be connected to IMU DIO1) connected to FX3_GPIO1 

It is recommended to use an iSensor FX3 eval board with a programable power supply

## Running Tests

After setting up the appropriate hardware/software environment, the test project must be built. This can be done in visual studio by loading the .sln file and compiling, or via msbuild

After the test project is built, test cases can be loaded via the nunit GUI using launcher.bat. Once test cases are loaded into the GUI, they can be run by selecting a test case and clicking "run"

The NUnit test cases can also be executed via command line using the included nunit-console-x86.exe
