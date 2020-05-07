# iSensor-SPI-Buffer
Firmware for the STM32F303 (Nucleo-64) to enable full throughput buffered data capture on Analog Devices IMUs

[Detailed Register Map](REGISTER_DEFINITION.MD)

[Hardware Pin Map](PIN_MAP.MD)

## Hardware Platform

* This firmware is designed to run on the Nucleo-F303RE board (based on STM32F303). Once prototype phase is complete will move to a more permanent solution based around a custom PCB that will mate directly with the IMU using its mounting holes

![STM Nucleo-64 board](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/stm_nucleo.JPG)

* The firmware and hardware will be compatible with all modern Analog Devices iSensor IMUs (excluding those which have non-standard SPI interfaces)

![ADIS1649x IMU](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/adis_imu.JPG)

## Development Environment

The iSensor-SPI-Buffer project can be loaded and built using the free [STM32 Cube IDE](https://my.st.com/content/my_st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-ides/stm32cubeide).

## Design Requirements

### IMU Register Interfacing

* "Invisible" SPI pass-through to an iSensor IMU, including modules that implement register pages
* iSensor-SPI-Buffer configuration registers and buffered data acquisition registers will be placed on pages not assigned by any IMU (Pages 253 - 255)
* The data ready signal must be passed through by iSensor-SPI-Buffer firmware, with added phase delay to allow for any autonomous data acquisition

### Buffered Data Acquisition

* Autonomous IMU data acquisition will be driven by the IMU-generated data ready signal. Data will be stored in the iSensor-SPI-Buffer SRAM until retrieved by a user
* Programmable data request message (MOSI) to be transmitted after each data ready signal. Allows users to customize the register writes, contents, and order
* Programmable data acquisition length after each IMU data ready signal. Allows the user to configure the length of data read back from the IMU
* Programmable data acquisition SPI word size. The default 16-bit length provides support standard register reads, writes, and burst reads on most IMUs

### Configuration Options

* Buffer operating mode (LIFO/FIFO)
* Buffer overflow setting (stop sampling vs delete oldest)
* Data ready input GPIO
* IMU Data ready trigger polarity
* Master SPI (interface between the iSensor-SPI-Buffer firmware and the IMU) 
  * SCLK Frequency
  * Stall time
* Slave SPI (interface between the iSensor-SPI-Buffer firmware and the host processor) 
  * Standard SPI settings

### Buffer Design

* Reading the buffer output registers will dequeue from the buffer
* The user will also be able to clear the buffer using a control register
* A buffer counter will be added to each buffer for the user to keep track of the state of the buffer. This counter must also be accessible without dequeuing data from buffer
* Using a maximum buffer entry size of 64 bytes, the buffer will provide a minimum of 512 frames of buffering using 32KB SRAM
  * STM32F303 has 80KB of SRAM available, so more than 512 entries may be feasible

## Architecture

### SPI Ports

The iSensor-SPI-Buffer firmware will utilize two hardware SPI ports. One operating as a master (SPI buffer to IMU communication) and the other as a slave (SPI buffer to host)
* The slave SPI port will expose the IMU register interface along with the iSensor-SPI-Buffer configuration registers to the master
* The slave SPI port will support all standard SPI configuration parameters
* The master SPI port will communicate with the IMU using the standard, iSensor SPI protocol
* The master SPI port configuration will be limited to customizing SCLK frequency and stall time

### Program Flow

The iSensor-SPI-Buffer application will be exclusively interrupt-driven to remove as much influence from the ST processor as possible

**User SPI Interrupt**

There will be an ISR to handle user-initiated SPI transactions (slave interface). If the active register page is not one of the SPI buffer pages (253 - 255) all SPI transactions will be forwarded to the IMU downstream. 

SPI transactions will be classified as "read" or "write" by the SPI buffer board the same as the iSensor IMU SPI protocol. A write operation is signaled by setting the MSB of each 16-bit word. The upper 8 bits of the 16-bit transaction contain the register address to be written to as well as the read/write bit. The lower 8 bits of the 16-bit transaction contain the data to be written. 

A single read request transmitted to the SPI buffer on the slave interface will result in *two* SPI transactions sent to the IMU on the master interface. The first transaction sends the register read request to the IMU and the second reads the requested register contents data back from the IMU. The requested IMU register is then loaded into the SPI buffer user SPI transmit buffer so that it is available to the user on the next user SPI transaction.  For write transactions, a single write command will be sent to the IMU. The data resulting from the write transaction is then loaded into the SPI buffer user SPI transmit buffer. Unlike the read operation, a write operation will not automatically generate an additional SPI transaction. 

**Autonomous Data Capture**

When the currently selected page is 255 (buffer output register page), the iSensor SPI Buffer firmware will begin to monitor a user-specified GPIO interrupt for data ready signals pulses. Once a data ready pulse is detected, the firmware will transmit the data contained in page 254 (buffer transmit data) in the order specified by the user. Data received from the IMU during each 16-bit transaction will be stored in a new buffer entry in the iSensor-SPI-Buffer SRAM. The firmware will use DMA and timer peripherals to perform the SPI transactions with minimal CPU intervention while giving control over the SPI SCLK and stall time. 

The data capture ISR will be triggered by a user-specified GPIO, corresponding to the IMU data ready signal. The data capture ISR will configure a timer peripheral (which drives SPI word transmission timing) to trigger a DMA between memory and SPI. A DMA done ISR will handle cleaning up the DMA transactions and incrementing buffer pointers following a complete data set acquisition from the IMU. If the selected page is changed off page 255, the IMU data ready interrupt data capture functionality will be disabled (can be re-enabled by writing 255 to the page ID register on any page).

![Buffered Capture Data Flow](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/Data_Capture_Flow.jpg)

**Interrupt Signaling**

Each of the four DIO lines from the iSensor-SPI-Buffer to the master device can be configured to operate in one of three modes
* Data Ready Interrupt Mode: The selected DIO will go high when a specified number of samples are available to be dequeued. This allows a master device to simply monitor the data ready interrupt signal for a positive edge, and then dequeue a large number of IMU data samples
* Buffer Full Interrupt Mode: The selected DIO will go high when the buffer is full
* Pin Pass-Through Mode: The DIO output from the IMU (e.g. a data ready signal or sync signal) will be directly connected to the master device, using an ADG1611 switch. When a DIO is configured in pin pass-through mode it cannot be used for interrupt signalling

The state of each interrupt signal is checked and updated on each iteration of the main cyclic executive loop. This ensures quick response time to changes in the buffer state while maintaining a simple program flow.

**Command Execution**

* When a write is issued the USER_COMMAND register, a COMMAND flag will be set and processed by the cyclic executive loop
* While a command is being executed, the user SPI port will be disabled, and any interrupt lines (data ready or buffer full) will be brought low

### Register Interface

* Configuration registers for iSensor SPI buffer will be available on page 253
* The data acquisition write data registers (data to transmit per data ready) will be on page 254
* Buffer output data registers will be on page 255
