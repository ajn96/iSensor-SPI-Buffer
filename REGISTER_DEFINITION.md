# iSensor-SPI-Buffer register structure

## Page 253 - iSensor-SPI-Buffer configuration

| Address | Register Name | Default | R/W | Flash Backup | Description |
| --- | --- | --- | --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FD | R/W | T | Page register. Used to change the currently selected register page |
| 0x02 | BUF_CONFIG | 0x0200 | R/W | T | Buffer configuration settings (FIFO/LIFO, SPI word size, overflow behavior) |
| 0x04 | BUF_LEN | 0x0014 | R/W | T | Length (in bytes) of each buffered data capture |
| 0x06 | BUF_MAX_CNT | N/A | R | T | Maximum entries which can be stored in the buffer. Determined by BUF_LEN. Read-only register |
| 0x08 | DR_CONFIG | 0x0011 | R/W | T | Data ready input (IMU to iSensor-SPI-Buffer) configuration |
| 0x0A | DIO_CONFIG | 0x0843 | R/W | T | DIO configuration. Sets up pin pass-through and assigns interrupts |
| 0x0C | INT_CONFIG | 0x0020 | R/W | T | Interrupt configuration register |
| 0x0E | IMU_SPI_CONFIG | 0x2014 | R/W | T | SCLK frequency to the IMU (specified in terms of clock divider) + stall time between SPI words |
| 0x10 | USER_SPI_CONFIG | 0x0007 | R/W | T | User SPI configuration (mode, etc.) |
| 0x12 | USER_COMMAND | N/A | W | T | Command register (flash update, factory reset, clear buffer, software reset) |
| 0x14 | USER_SCR_0 | 0x0000 | R/W | T | User scratch 0 register |
| ... | ... | ... | ... | ... | ... |
| 0x22 | USER_SCR_7 | 0x0000 | R/W | T | User scratch 7 register |
| 0x28 | FW_REV | N/A | R | T | Firmware revision |
| 0x2A | ENDURANCE | N/A | R | T | Flash update counter |
| 0x6A | FAULT_CODE | 0x0000 | R | N/A | Fault code, stored in case of a hard fault exception. This register is stored on a separate flash page from the primary register array |
| 0x6C | STATUS | N/A | R | F | Device status register. Clears on read |
| 0x6E | BUF_CNT | 0x0000 | R | F | The number of samples in buffer |
| 0x70 | FW_DAY_MONTH | N/A | R | T | Firmware build date |
| 0x72 | FW_YEAR | N/A | R | T | Firmware build year |
| 0x74 | DEV_SN_0 | N/A | R | T | Processor core serial number register, word 0 |
| ... | ... | ... | ... | ... | ... |
| 0x7E | DEV_SN_5 | N/A | R | T | Processor core serial number register, word 5 |

## Page 254 - Buffer write data

| Address | Register Name | Default | R/W | Flash Backup | Description |
| --- | --- | --- | --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FE | R/W | T | Page register. Used to change the currently selected register page |
| 0x10 | BUF_WRITE_0 | 0x0000 | R/W | T | First transmit data register (data sent to IMU DIN) |
| ... | ... | ... | ... | ... | ... |
| 0x4E | BUF_WRITE_31 | 0x0000 | R/W | T | Last transmit data register |
| 0x7C | FLASH_SIG_DRV | N/A | R | F | Derived flash memory signature register (determined at initialization) |
| 0x7E | FLASH_SIG | N/A | R | T | Stored flash memory signature register |

## Page 255 - Buffer output registers

| Address | Register Name | Default | R/W | Flash Backup | Description |
| --- | --- | --- | --- | --- | --- |
| 0x00 | PAGE_ID | 0x00FF | R/W | T | Page register. Used to change the currently selected register page |
| 0x02 | STATUS_1 | 0x0000 | R | F | Mirror of the STATUS register. Clears on read |
| 0x04 | BUF_CNT_1 | 0x0000 | R/W | F | The number of samples in buffer. Write 0 to this register to clear buffer. Other writes are ignored |
| 0x06 | BUF_RETRIEVE | 0x0000 | R | F | Read this register to dequeue new data from buffer to buffer output registers |
| 0x08 | BUF_TIMESTAMP_LWR | 0x0000 | R | F | Lower 16 bits of buffer entry timestamp |
| 0x0A | BUF_TIMESTAMP_UPR | 0x0000 | R | F | Upper 16 bits of buffer entry timestamp |
| 0x0C | BUF_DELTA_TIME | 0x0000 | R | F | Delta time between last buffer entry and current buffer entry, in microseconds. Truncated to 16 bits |
| 0x0E | BUF_SIG | 0x0000 | R | F | Buffer entry checksum register |
| 0x10 | BUF_DATA_0 | 0x0000 | R | F | First buffer output register (data received from IMU DOUT) |
| ... | ... | ... | ... | ... | ... |
| 0x4E | BUF_DATA_31 | 0x0000 | R | F | Last buffer output register |

# iSensor-SPI-Buffer register bit fields

**PAGE_ID**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | PAGE | Selected page |

**BUF_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | MODE | The buffer mode (0 is FIFO mode, 1 is LIFO mode) |
| 1 | OVERFLOW | Buffer overflow behavior. 0 stop sampling, 1 replace oldest data |
| 7:2 | RESERVED | Currently unused |
| 15:8 | SPIWORDSIZE | SPI word size for buffered capture (in bytes). Valid range 2 - 64 |

**BUF_LEN**

| Name | Bits | Description |
| --- | --- | --- |
| 15:0 | LEN | Length (in bytes) of each buffer entry. Valid range 2 - 64 |

**BUF_MAX_CNT**

| Name | Bits | Description |
| --- | --- | --- |
| 15:0 | MAX | Total number of entries which can be stored in the buffer. Updates automatically when BUF_LEN is changed |

**DR_CONFIG**
| Bit | Name | Description |
| --- | --- | --- |
| 3:0 | DR_SELECT | Select which IMU ouput pin is treated as data ready. Can only select one pin |
| 4 | POLARITY | Data ready trigger polarity. 1 triggers on rising edge, 0 triggers on falling edge |
| 15:5 | RESERVED | Currently unused |

**DIO_CONFIG**

| Bit | Name | Description |
| --- | --- | --- |
| 3:0 | PIN_PASS | Select which pins are directly connected to IMU vs passing through iSensor-SPI-Buffer firmware |
| 7:4 | INT_MAP | Select which pins are driven with the buffer data ready interrupt signal from the iSensor-SPI-Buffer firmware |
| 11:8 | OVERFLOW_MAP | Select which pins are driven with the overflow interrupt signal from the iSensor-SPI-Buffer firmware |
| 15:12 | RESERVED | Currently unused |

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
| 0 | CPHA | SPI clock phase |
| 1 | CPOL | SPI clock polarity |
| 2 | MSB_FIRST | 1 = transmit MSB first, 0 = transmit LSB first |
| 14:3 | RESERVED | Currently unused |
| 15 | BUF_BURST | Enable burst read of buffered data, using SPI DMA |

**USER_COMMAND**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | CLEAR_BUF | Clears buffer contents |
| 1 | CLEAR_FAULT | Clears any fault data logged in flash memory. Until this command is run, status FAULT bit will never clear |
| 2 | FACTORY_RESET | Restores firmware to a factory default state |
| 3 | FLASH_UPDATE | Save all non-volatile registers to flash memory |
| 14:5 | RESERVED | Currently unused |
| 15 | RESET | Software reset |

**USER_SCR_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | USER_SCR | User scratch value |

**STATUS**

| Bit | Name | Description |
| --- | --- | --- |
| 0 | SPI_ERROR | SPI error reported by the user SPI or IMU SPI peripheral |
| 1 | SPI_OVERFLOW | User SPI data overflow (min stall time violated) |
| 2 | OVERRUN | Data capture overrun. Set when processor receives an IMU data ready interrupt and has not finished the previous capture |
| 3 | DMA_ERROR | Set when processor DMA peripheral reports an error (user SPI DMA for burst read or IMU SPI DMA) |
| 5:4 | RESERVED | Currently unused |
| 6 | FLASH_ERROR | Set when the flash register signature stored does not match signature calculated from SRAM register contents at initialization. Sticky |
| 7 | FLASH_UPDATE_ERROR | Set when the flash update routine fails. Sticky |
| 8 | FAULT | Set when the processor core generates a fault exception (bus fault, memory fault, hard fault, initialization error). Fault exceptions will force a system reset. Sticky |
| 9 | WATCHDOG | Set when the processor has reset due to a watchdog timeout. Sticky |
| 10 | BUF_FULL | Set when buffer is full (overflow interrupt) |
| 11 | BUF_INTERRUPT | Set when buffer data ready interrupt condition is met (data ready interrupt) |
| 15:12 | TC | User SPI transaction counter. Increments by one with each SPI transaction |

Excluding the transaction counter field and bits identified as sticky, this register clears on read.

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

**FLASH_SIG_DRV**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | SIGNATURE | Derived signature for all registers stored to flash memory. This value is determined at initilization and compared to "FLASH_SIG" to determine if flash memory contents are valid |

**FLASH_SIG**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | SIGNATURE | Signature for all registers stored to flash memory. This value is stored in flash, and is updated when a flash update command is executed |

**BUF_CNT**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | CNT | Number of entries currently stored in the buffer. Write 0 to clear buffer. All other writes ignored |

**BUF_RETRIEVE**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | RETRIEVE | Read to place a new sample from the buffer into the BUF_READ output registers. Will always contain 0 |

**BUF_TIMESTAMP_LWR**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | TIMESTAMP | Lower 16 bits of a 32-bit buffer entry timestamp value which is stored at the start of each data capture. Resolution: 1LSB = 1us |

**BUF_TIMESTAMP_UPR**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | TIMESTAMP | Upper 16 bits of a 32-bit buffer entry timestamp. |

**BUF_DATA_N**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | READ_N | Read data received on the MISO line while capturing a buffered data entry |

**BUF_SIG**

| Bit | Name | Description |
| --- | --- | --- |
| 15:0 | SIGNATURE | Buffer signature. This is the sum of all 16-bit words stored in the buffer (including timestamp) |
