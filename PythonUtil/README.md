# iSensor-SPI-Buffer Python Library

This library allows easy programmatic interfacing to the iSensor-SPI-Buffer USB command line interface through a Python API. The API allows for configuration of the iSensor-SPI-Buffer board, as well as double buffered high-throughput data capture from a connected IMU. The image below shows the general flow for IMU data capture using this library.

![python_data_flow](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/SpiBuffDoubleBuffer.jpg)

### Prerequisites 

* Python 3.7 or newer
* PySerial (pip install pyserial)
* iSensor-SPI-Buffer firmware v1.11 or newer

### Hardware Setup

To use this library, the iSensor-SPI-Buffer board must be connected to your system via USB. For easiest setup, it is recommended to set the buffer board supply jumper to "USB", allowing the buffer board and connected DUT to be powered via a 2A linear regulator (from USB 5V supply).

Once the buffer board is successfully connected and powered over USB, the serial port "ID" must be determined. This port ID is provided to the ISensorSpiBuffer Python class initializer, and is used to set up internal the serial port object for CLI interactions

On Windows, the board will show up as a "USB Serial Device" under the Ports (COM & LPT) section of the device manager. The COM port name (in this example "COM11") is used as the port name for the ISensorSPIBuffer Python library

![win_dev_manager](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/windows_dev_manager.PNG)

In Linux the port name (dev/ path for the device) can be determined using the included bash script. The following image shows the result from running ListDevPath in a Linux VM (with the buffer board passed in from the VirtualBox host). In this example, the buffer board is "/dev/ttyAMC0", which is provided as the port name for the ISensorSPIBuffer Python library.

![linux_dev_list](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/python_listdev_script.png)

### Python Setup

The PYTHONPATH variable will likely have to be set to include the spi_buf_cli.py file. To do this on Linux, open a terminal and for example type: `EXPORT PYTHONPATH=/home/usr/pi/Desktop/iSensor-SPI-Buffer/PythonUtil:$PYTHONPATH`. The directory should be modified to point to the location of the PythonUtil folder in the cloned repo.

On Windows, the PYTHONPATH can be updated by setting the Environmental Variable PYTHONPATH to include the PythonUtil directory in the cloned repo.

Once you have identified the port "ID" for the buffer board, connecting to the board is as simple as creating and initializing an instance of the ISensorSPIBuffer Python class. The initializer configures the specified serial port for use with the buffer board and performs basic setup on the buffer board itself.

For the Windows example

`buf = ISensorSPIBuffer("COM11")`

For the Linux example

`buf = ISensorSPIBuffer("/dev/ttyACM0")`

Afterwards, the functionality of the CLI can be checked using the check_connection() function. In addition, the current iSensor-SPI-Buffer firmware version can be checked using the version() call.

The outputs for the included adis1649x example programs are shown below:

**Linux**

![python_example_linux](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/python_example_linux.png)

Note, the IMU output data rate was limited to 400Hz because the program is running in a VM

**Windows**

![python_example_windows](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/python_example_windows.png)

In this example, the IMU output data rate was set to 1KHz, and each buffer was a burst output from the IMU. Using burst IMU outputs, data rates up to 2KHz seem to work consistently

## Debugging Common Setup Issues

There are a few problems which can cause difficulties using this library with the iSensor-SPI-Buffer board.

1. iSensor-SPI-Buffer firmware version not up to date. This Python library was not designed with backwards compatibility in mind, due to the relatively large number of changes in the firmware CLI behavior over the last few versions. It is intended to be used with the latest version of the iSensor-SPI-Buffer firmware for best operation. If this causes problems and you do not have the ability to update your buffer board firmware, you can download the python library version at the time of the release tag for your older version of firmware. There may be missing features / bug fixes in the older revision.
2. A "halted" stream (on PC side) can cause a watchdog reset event. This occurs when the PC does not request data back from the buffer board fast enough during a stream (mostly see this if you are debugging the Python library execution). After the watchdog reset, the board will show up as "Non-responding" in the Python library. This behavior should be resolved in future revisions of firmware.