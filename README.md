# iSensor-SPI-Buffer
Firmware for the STM32F303 (Nucleo-64) to enable full throughput buffered data capture on Analog Devices IMUs

## Hardware Platform

* This firmware is designed to run on the Nucleo-F303RE board (based on STM32F303). Once prototpye phase is complete will move to a more permanent solution.

![STM Nucleo-64 board](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/stm_nucleo.JPG)

* Will be compatible with all modern Analog Devices iSensor IMUs (excluding those which have an automotive SPI interface)

![ADIS1649x IMU](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/adis_imu.JPG)

## Development Environment

The iSensor SPI buffer project can be loaded and built by the free [STM32 Cube IDE](https://my.st.com/content/my_st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-ides/stm32cubeide)

## Design Requirements

### IMU Register Interfacing

* "Invisible" SPI pass through to a connected iSensor IMU register map, for all IMU pages
* SPI buffer configuration registers and buffered data aquisition registers placed on pages not used by IMU (e.g. page 253 - 255)
* Data ready signal must be passed through by iSensor SPI Buffer firmware, with added phase delay for any autonomous data aquisition

### Buffered Data Aquisition

* Autonomous IMU data aquisition driven by data ready signal. Data will be stored into a buffer until retrieved by a user
* Programmable length data aquisition after each IMU data ready signal (number of bytes)
* Programmable list of MOSI traffic following each IMU data ready signal (can choose what registers to read/write)
* Programmable data aquisition SPI word size (16 bits default for standard reg reads, gives options for burst reads)

### Configuration Options

* Buffer mode (LIFO/FIFO)
* Buffer overflow setting (stop sampling vs delete oldest)
* Data ready input GPIO
* Data ready trigger polairty
* Master SPI (interface to IMU) clock freq and stall time
* Slave SPI (interface to higher level controller) settings

### Buffer Design

* Data be be dequeued from buffer via read of buffer output registers
* Option to clear buffer via control register read
* Buffer count must be easily accessible without dequeuing from buffer

## Architecture

### SPI Ports

The iSensor SPI buffer firmware will utilize two SPI ports (one master and one slave)
* Slave SPI port will expose register interface of IMU + SPI buffer config registers to a higher level controller
* Slave SPI will have fully configurable interface, to enable interfacing with a variety of microcontrollers
* Master SPI port will communicate to an Analog Devices IMU using the standard ADIS SPI protocol
* Master SPI port will have limited handles to set - SCLK frequency, stall time. All other SPI options will be fixed

### Program Flow

The system will be interrupt driven for all communications

**User SPI interrupt**

There will be an ISR to handle user SPI (slave interface) transactions. If the currently selected page is not one of the SPI buffer pages (253 - 255) any read/write will be forwarded to the IMU. A single read request to the user SPI will result in *two* SPI transactions sent to the IMU - one to send the register read request and a second to get the result data back from the IMU. The IMU result data will be loaded into the user SPI transmit buffer, so that it is available on the next user SPI transaction.  For write transactions, a single write command will be sent to the IMU, and the IMU reponse data during the write loaded into the user SPI transmit buffer.

**Autonomous Data Capture**

When the currently selected page is 255 (buffer output registers), the iSensor SPI Buffer firmware will enable a GPIO interrupt on the selected data ready pin (with the selected data ready polarity). After each data ready interrupt, the firmware will transmit the data entered on the BUF_WRITE registers to the IMU, and place the data received back into a new buffer entry in SRAM. The firmware will use DMA and a timer peripheral to capture all data with minimal CPU intervention, while giving control over the SPI stall time. 

The GPIO edge will trigger a data capture ISR. The data capture ISR will configure a timer peripheral (which drives SPI word transmission timing) to trigger a DMA between memory and SPI. A DMA done ISR will handle cleaning up the DMA transactions and incrementing buffer pointers following a complete data set aquisition from the IMU. If the selected page is changed off page 255, the IMU data ready interrupt will be disabled (can be re-enabled by writing 255 to the page ID register on any page).

**Interrupt Signalling**

The data ready output from the iSensor-SPI-Buffer will be configurable to serve two purposes
* In "Data Ready" mode, it will pulse each time a new buffer sample is entered into the buffer (same functionality as IMU data ready)
* In interrupt mode, the data ready output will go high once a specified number of samples are available to be dequeued. This allows a master device to simply monitor the data ready signal for a posedge, and then dequeue a large number of IMU data samples

### Register Interface

* Configuration registers for iSensor SPI buffer will be available on page 253
* The data aquisition write data registers (data to transmit per data ready) will be on page 254
* Buffer output data registers will be on page 255

### iSensor SPI buffer register structure

Page 253 - iSensor SPI buffer configuration

| Address | Name | Description |
| --- | --- | --- |
| 0x00 | PAGE_ID | Page register. Used to change the currently selected register page |
| 0x02 | BUF_CONFIG | Buffer configuration settings (FIFO/LIFO, SPI word size, overflow behavior) |
| 0x04 | BUF_LEN | Length (in bytes) of each buffered data capture |
| 0x06 | IMU_DR_CONFIG | IMU data ready configuration (GPIO number, polarity setting) |
| 0x08 | IMU_SPI_SCLK | SCLK frequency to the IMU |
| 0x0A | IMU_SPI_STALL | SPI stall time to the IMU |
| 0x0C | USER_SPI_CONFIG | User SPI configuration (mode, etc) |
| 0x0E | USER_DR_CONFIG | User interrupt (data ready) config. Will have the mode (Data ready vs interrupt) and the count to trigger interrupt at |
| 0x10 | USER_COMMAND | Command register (flash update, factory reset, clear buffer, software reset, others?) |

Page 254 - buffer write data

| Address | Name | Description |
| --- | --- | --- |
| 0x00 | PAGE_ID | Page register. Used to change the currently selected register page |
| 0x06 | BUF_WRITE_0 | First transmit data register (data sent to IMU DIN) |
| ... | ... | ... |
| 0x06 + (2N) | BUF_WRITE_N | Last transmit data register |

Page 255 - buffer output registers

| Address | Name | Description |
| --- | --- | --- |
| 0x00 | PAGE_ID | Page register. Used to change the currently selected register page |
| 0x02 | BUF_CNT | The number of samples in buffer. Write 0 to this register to clear buffer |
| 0x04 | BUF_RETRIEVE | Read this register to dequeue new data from buffer to buffer output registers |
| 0x06 | BUF_DATA_0 | First buffer output register (data received from IMU DOUT) |
| ... | ... | ... |
| 0x06 + (2N) | BUF_DATA_N | Last buffer output register |
