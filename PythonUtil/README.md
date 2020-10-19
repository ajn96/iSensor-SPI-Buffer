# iSensor-SPI-Buffer Python Library

This library allows easy programmatic interfacing to the iSensor-SPI-Buffer USB command line interface through a Python API

### Prerequisites 

* Python 3.7 or newer

* PySerial

### Hardware Setup

To use this library, the iSensor-SPI-Buffer board must be connected to your system via USB. For easiest setup, it is recommended to set the buffer board supply jumper to "USB", allowing the buffer board and connected DUT to be powered via a 2A linear regulator (from USB 5V supply).

Once the buffer board is successfully connected and powered over USB, the serial port "ID" must be determined. This port ID is provided to the ISensorSpiBuffer Python class initializer, and is used to set up internal the serial port object for CLI interactions

On Windows, the board will show up as a "USB Serial Device" under the Ports (COM & LPT) section of the device manager. The COM port name (in this example "COM11") is used as the port name for the ISensorSPIBuffer Python library

![win_dev_manager](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/windows_dev_manager.PNG)

In Linux the port name (dev/ path for the device) can be determined using the included bash script. The following image shows the result from running ListDevPath in a Linux VM (with the buffer board passed in from the VirtualBox host). In this example, the buffer board is "/dev/ttyAMC0", which is provided as the port name for the ISensorSPIBuffer Python library.

![linux_dev_list](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/python_listdev_script.png)

### Python Setup

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