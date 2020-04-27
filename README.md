# iSensor-SPI-Buffer
Firmware for the STM32F303 (Nucleo-64) to enable full throughput buffered data capture on Analog Devices IMUs

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

The iSensor-SPIBuffer application will be exclusively interrupt-driven to remove as much influence from the ST processor as possible

**User SPI Interrupt**

There will be an ISR to handle user-initiated SPI transactions (slave interface). If the active register page is not one of the SPI buffer pages (253 - 255) all SPI transactions will be forwarded to the IMU downstream. 

SPI transactions will be classified as "read" or "write" by the SPI buffer board the same as the iSensor IMU SPI protocol. A write operation is signaled by setting the MSB of each 16-bit word. The upper 8 bits of the 16-bit transaction contain the register address to be written to as well as the read/write bit. The lower 8 bits of the 16-bit transaction contain the data to be written. 

A single read request transmitted to the SPI buffer on the slave interface will result in *two* SPI transactions sent to the IMU on the master interface. The first transaction sends the register read request to the IMU and the second reads the requested register contents data back from the IMU. The requested IMU register is then loaded into the SPI buffer user SPI transmit buffer so that it is available to the user on the next user SPI transaction.  For write transactions, a single write command will be sent to the IMU. The data resulting from the write transaction is then loaded into the SPI buffer user SPI transmit buffer. Unlike the read operation, a write operation will not automatically generate an additional SPI transaction. 

**Autonomous Data Capture**

When the currently selected page is 255 (buffer output register page), the iSensor SPI Buffer firmware will begin to monitor a user-specified GPIO interrupt for data ready signals pulses. Once a data ready pulse is detected, the firmware will transmit the data contained in page 254 (buffer transmit data) in the order specified by the user. Data received from the IMU during each 16-bit transaction will be stored in a new buffer entry in the iSensor-SPI-Buffer SRAM. The firmware will use DMA and timer peripherals to perform the SPI transactions with minimal CPU intervention while giving control over the SPI SCLK and stall time. 

The data capture ISR will be triggered by a user-specified GPIO, corresponding to the IMU data ready signal. The data capture ISR will configure a timer peripheral (which drives SPI word transmission timing) to trigger a DMA between memory and SPI. A DMA done ISR will handle cleaning up the DMA transactions and incrementing buffer pointers following a complete data set acquisition from the IMU. If the selected page is changed off page 255, the IMU data ready interrupt data capture functionality will be disabled (can be re-enabled by writing 255 to the page ID register on any page).

**Interrupt Signaling**

The data ready output from the iSensor-SPI-Buffer will be configurable to serve two purposes
* In "Data Ready" mode, it will pulse each time a new buffer sample is entered into the buffer (same functionality as IMU data ready)
* If the iSensor-SPI-Buffer firmware is not capturing data from the IMU, the data ready signal will be simply passed through by the GPIO ISR in "Data Ready" mode
* In interrupt mode, the data ready output will go high once a specified number of samples are available to be dequeued. This allows a master device to simply monitor the data ready signal for a positive edge, and then dequeue a large number of IMU data samples
* When operating in interrupt mode, the data ready output will be updated by the cyclic executive once the buffer count criteria is met. In data ready mode, the data ready output will be toggled by the "DMA Done" ISR

**Command Execution**

* When a write is issued the USER_COMMAND register, a COMMAND flag will be set and processed by the cyclic executive loop
* While a command is being executed, the user SPI port will be disabled, and the data ready line will be brought low

### Register Interface

* Configuration registers for iSensor SPI buffer will be available on page 253
* The data acquisition write data registers (data to transmit per data ready) will be on page 254
* Buffer output data registers will be on page 255

### iSensor-SPI-Buffer register structure

Page 253 - iSensor-SPI-Buffer configuration

| Address | Register Name | Default  | Description |
| --- | --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FD | Page register. Used to change the currently selected register page |
| 0x02 | BUF_CONFIG | 0x0200 | Buffer configuration settings (FIFO/LIFO, SPI word size, overflow behavior) |
| 0x04 | BUF_LEN | 0x0014 | Length (in bytes) of each buffered data capture |
| 0x06 | BUF_MAX_CNT | N/A | Maximum entries which can be stored in the buffer. Determined by BUF_LEN. Read-only register |
| 0x08 | DR_CONFIG | 0x0011 | Data ready input (IMU to iSensor-SPI-Buffer) configuration |
| 0x0A | DIO_CONFIG | 0x0843 | DIO configuration. Sets up pin pass-through and assigns interrupts |
| 0x0C | INT_CONFIG | 0x0020 | Interrupt configuration register |
| 0x0E | IMU_SPI_CONFIG | 0x2014 | SCLK frequency to the IMU (specified in terms of clock divider) + stall time between SPI words |
| 0x10 | USER_SPI_CONFIG | 0x0003 | User SPI configuration (mode, etc.) |
| 0x12 | USER_COMMAND | N/A | Command register (flash update, factory reset, clear buffer, software reset, others?) |
| 0x14 | USER_SCR_0 | 0x0000 | User scratch register |
| 0x16 | USER_SCR_1 | 0x0000 | User scratch register |
| 0x18 | USER_SCR_2 | 0x0000 | User scratch register |
| 0x1A | USER_SCR_3 | 0x0000 | User scratch register |
| 0x6A | FW_REV | N/A | Firmware revision |
| 0x6C | ENDURANCE | N/A | Flash update counter |
| 0x6E | STATUS | N/A | Device status register. Clears on read |
| 0x70 | FW_DAY_MONTH | N/A | Firmware build date |
| 0x72 | FW_YEAR | N/A | Firmware build year |
| 0x74 | DEV_SN_0 | N/A | Processor core serial number register, word 0 |
| 0x76 | DEV_SN_1 | N/A | Processor core serial number register, word 1 |
| 0x78 | DEV_SN_2 | N/A | Processor core serial number register, word 2 |
| 0x7A | DEV_SN_3 | N/A | Processor core serial number register, word 3 |
| 0x7C | DEV_SN_4 | N/A | Processor core serial number register, word 4 |
| 0x7E | DEV_SN_5 | N/A | Processor core serial number register, word 5 |

Page 254 - buffer write data

| Address | Register Name | Description |
| --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FE | Page register. Used to change the currently selected register page |
| 0x06 | BUF_WRITE_0 | 0x0000 | First transmit data register (data sent to IMU DIN) |
| ... | ... | ... |
| 0x44 | BUF_WRITE_31 | 0x0000 | Last transmit data register |

Page 255 - buffer output registers

| Address | Register Name | Description |
| --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FF | Page register. Used to change the currently selected register page |
| 0x02 | BUF_CNT | 0x0000 | The number of samples in buffer. Write 0 to this register to clear buffer |
| 0x04 | BUF_RETRIEVE | 0x0000 | Read this register to dequeue new data from buffer to buffer output registers |
| 0x06 | BUF_DATA_0 | 0x0000 | First buffer output register (data received from IMU DOUT) |
| ... | ... | ... |
| 0x44 | BUF_DATA_31 | 0x0000 | Last buffer output register |

### iSensor-SPI-Buffer register bit fields

**PAGE_ID**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | PAGE | Selected page |

**BUF_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | MODE | The buffer mode (0 is FIFO mode, 1 is LIFO mode) |
| 1 | OVERFLOW | Buffer overflow behavior. 0 stop sampling, 1 replace oldest data |
| 15:8 | SPIWORDSIZE | SPI word size for buffered capture (in bytes). Valid range 2 - 64 |

**BUF_LEN**

| Name | Bits | Description |
| --- | --- | --- |
| 15:0 | LEN | Length (in bytes) of each buffer entry. Valid range 2 - 64 |

**DR_CONFIG**
| Bit | Name | Description |
| --- | --- | --- |
| 3:0 | DR_SELECT | Select which IMU ouput pin is treated as data ready. Can only select one pin |
| 4 | POLARITY | Data ready trigger polarity. 1 triggers on rising edge, 0 triggers on falling edge |

**DIO_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 0:3 | PIN_PASS | Select which pins are directly connected to IMU vs passing through iSensor-SPI-Buffer firmware |
| 7:4 | INT_MAP | Select which pins are driven with the buffer data ready interrupt signal from the iSensor-SPI-Buffer firmware |
| 11:8 | OVERFLOW_MAP | Select which pins are driven with the overflow interrupt signal from the iSensor-SPI-Buffer firmware |

For each field in DIO_CONFIG, the following pin mapping is made:
* Bit0 -> DIO1
* Bit1 -> DIO2
* Bit2 -> DIO3
* Bit3 -> DIO4

The following default values will be used for DIO_CONFIG:
* PIN_PASS: 0x3. DIO1 (typically acts as IMU data ready) and DIO2 (typically acts as SYNC input) will be passed through using an Analog Switch. This allows for direct sync strobing and reading of the data ready signal
* INT_MAP: 0x4. The buffer data ready interrupt is applied to DIO3 by default
* OVERFLOW_MAP: 0x8. The buffer overflow interrupt is applied to DIO4 by default

**INT_CONFIG**

| Name | Bits | Description |
| --- | --- | --- |
| 15:0 | WATERMARK | Number of elements stored in buffer before asserting the iSensor-SPI-Buffer data ready interrupt. Range 0 - BUF_MAX_CNT |

**IMU_SPI_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 7:0 | STALL | Stall time between SPI words (in microseconds). Valid range 2us - 255us |
| 8 | SCLK_SCALE_2 | Sets SCLK prescaler to 2 (18MHz) |
| 9 | SCLK_SCALE_4 | Sets SCLK prescaler to 4 (9MHz). Default option selected |
| 10 | SCLK_SCALE_8 | Sets SCLK prescaler to 8 (4.5MHz) |
| 11 | SCLK_SCALE_16 | Sets SCLK prescaler to 16 (2.25MHz) |
| 12 | SCLK_SCALE_32 | Sets SCLK prescaler to 32 (1.125MHz) |
| 13 | SCLK_SCALE_64 | Sets SCLK prescaler to 64 (562.5KHz) |
| 14 | SCLK_SCALE_128 | Sets SCLK prescaler to 128 (281.25KHz) |
| 15 | SCLK_SCALE_256 | Sets SCLK prescaler to 256 (140.625KHz) |

**USER_SPI_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | CPOL | SPI clock polarity |
| 1 | CPHA | SPI clock phase |

**USER_COMMAND**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | CLEAR_BUF | Clears buffer contents |
| 2 | FACTORY_RESET | Restores firmware to a factory default state |
| 3 | FLASH_UPDATE | Save all non-volatile registers to flash memory |
| 15 | RESET | Software reset |

**USER_SCR_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | USER_SCR | User scratch value |

**STATUS**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | SPI_ERR | User SPI error reported by the SPI peripheral |
| 1 | SPI_OVERFLOW | User SPI data overflow (min stall time violated) |
| 2 | FLASH_ERROR | Set when the flash register signature stored does not match signature calculated from SRAM register contents at initialization. Sticky |
| 3 | BUF_FULL | Set when buffer is full |
| 4 | BUF_INTERRUPT | Set when buffer data ready interrupt condition is met |
| 15:12 | TC | User SPI transaction counter. Increments by one with each SPI transaction |

With the exception of the transaction counter field, this register clears on read.

**FW_DAY_MONTH**

| Bit | Name | Description |
| --- | --- | --- |
| 7:0 | MONTH | Firmware program month, in BCD |
| 15:8 | DAY | Firmware program day, in BCD |

For example, April 24th would be represented by 0x2404.

**FW_YEAR**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | YEAR | Firmware program year, in BCD |

For example, the year 2020 would be represented by 0x2020.

**FW_REV**

| Bit | Name | Description |
| --- | --- | --- |
| 7:0 | MINOR | Minor firmware revision number, in BCD |
| 15:8 | MAJOR | Major firmware revision number, in BCD |

This rev corresponds to the release tag for the firmware. For example, rev 1.15 would be represented by 0x0115 in FW_REV.

**DEV_SN_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | SN | These six registers contain the 96 bit unique serial number in the STM32F303 |

**BUF_WRITE_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | WRITE_N | Write data to transmit on MOSI line while capturing a buffered data entry |

**BUF_CNT**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | CNT | Number of entries currently stored in the buffer. Write 0 to clear buffer |

**BUF_RETRIEVE**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | RETRIEVE | Read to place a new sample from the buffer into the BUF_READ output registers. Will always contain 0 |

**BUF_READ_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | READ_N | Read data received on the MISO line while capturing a buffered data entry |

# iSensor-SPI-Buffer Pin Mapping

**Slave SPI Interface (to host microprocessor)**

| Pin Name | STM32 Pin | Description |
| --- | --- | --- |
| SCLK | PB13 | SPI clock signal. This pin is an input for the iSensor-SPI-Buffer |
| CS | PB12 | Chip select signal. Bring low to select the iSensor-SPI-Buffer for communications. This pin is an input for the iSensor-SPI-Buffer |
| MISO | PB14 | Master in slave out (MISO) signal. This pin is an output from the iSensor-SPI-Buffer |
| MOSI | PB15 | Master out slave in (MOSI) signal. This pin is an input for the iSensor-SPI-Buffer |

**Master SPI Interface (to IMU)**

| Pin Name | STM32 Pin | Description |
| --- | --- | --- |
| SCLK | PA5 | SPI clock signal. This pin is an output from the iSensor-SPI-Buffer |
| CS | PA4 | Chip select signal, used to enable the IMU slave SPI interface. This pin is an output from the iSensor-SPI-Buffer |
| MISO | PA6 | Master in slave out (MISO) signal. This pin is an input for the iSensor-SPI-Buffer |
| MOSI | PA7 | Master out slave in (MOSI) signal. This pin is an output from the iSensor-SPI-Buffer |

**SPI 3 (master to communicate with SD card/etc)**

| Pin Name | STM32 Pin | Description |
| --- | --- | --- |
| SCLK | PC10 | SPI clock signal |
| CS | PA15 | Chip select signal |
| MISO | PC11 | Master in slave out (MISO) signal |
| MOSI | PC12 | Master out slave in (MOSI) signal |

**DIO / ADG1611 Control**

| Pin Name | STM32 Pin | Description |
| --- | --- | --- |
| DIO1_Master | PB5 | DIO1 input signal from IMU to iSensor-SPI-Buffer |
| DIO2_Master | PB9 | DIO2 input signal from IMU to iSensor-SPI-Buffer |
| DIO3_Master | PC6 | DIO3 input signal from IMU to iSensor-SPI-Buffer |
| DIO4_Master | PA9 | DIO4 input signal from IMU to iSensor-SPI-Buffer |
| DIO1_Slave | PB4 | DIO1 output signal from iSensor-SPI-Buffer to master |
| DIO2_Slave | PB8 | DIO2 output signal from iSensor-SPI-Buffer to master |
| DIO3_Slave | PC7 | DIO3 output signal from iSensor-SPI-Buffer to master |
| DIO4_Slave | PA8 | DIO4 output signal from iSensor-SPI-Buffer to master |
| SW_IN1 | PB6 | Switch control from iSensor-SPI-Buffer. If set DIO1 will be directly passed from master to IMU |
| SW_IN2 | PB7 | Switch control from iSensor-SPI-Buffer. If set DIO2 will be directly passed from master to IMU |
| SW_IN3 | PC8 | Switch control from iSensor-SPI-Buffer. If set DIO3 will be directly passed from master to IMU |
| SW_IN4 | PC9 | Switch control from iSensor-SPI-Buffer. If set DIO4 will be directly passed from master to IMU |
