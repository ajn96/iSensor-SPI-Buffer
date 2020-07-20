# iSensor-SPI-Buffer USB Virtual COM Port CLI

## Driver

The USB port on the iSensor-SPI-Buffer is configured to act as a CDC type device, presenting as a virtual COM (serial) port.

The default windows virtual COM port driver should allow for communications. 

## Functionality

The iSensor-SPI-Buffer virtual COM port exposes the same iSensor-SPI-Buffer register interface which can be accessed over SPI. In general, the functionality of each register should be the same, regardless of if the register is accessed over USB or SPI. The registers are accessed via a simple command line interface (CLI). This CLI allows a PC to interface with an IMU without the need for a SPI <-> USB board (like EVAL-ADIS-FX3).

The firmware can interface with a terminal emulator at a baud rate up to 230400 bits/sec. Putty or TeraTerm work well on Windows.

## CLI Definition

Commands are sent to the CLI over the virtual COM port. Each command starts with a command word, followed by a number of arguments. Each command ends with a new line character. All values are in hex.

| Command | arg0 | arg1 | arg2 | Description |
| --- | --- | --- | --- | --- |
| help | N/A | N/A | N/A | List available CLI options |
| freset | N/A | N/A | N/A | Trigger a factory reset and flash update. This command restores all registers to their default values and then issues a flash update command to save registers to non-volatile memory  |
| read | Register address | N/A | N/A | Read a single register, at the address provided |
| read | Start address | End address | N/A | Read multiple registers, accross the address range provided. Each register value is seperated by the USB delimiter character |
| read | Start address | End address | Number of reads | Read multiple registers, accross the address range provided, number of reads times. Each set of reads is terminated by a new line |
| readbuf | N/A | N/A | N/A | Read all buffer entries. This call will set the active page to the buffer data page |
| stream | start/stop | N/A | N/A | Start (1) / stop (0) a buffer read stream. When running, a stream is equivalent to calling readbuf every time a watermark interrupt is generated. WATERMARK_INT_CONFIG can be modified to set the data latency. Stream status is stored in USB_CONFIG |
| write | Register address | Write Value | N/A | Write a byte to the specified register |
| delim | Delimiter character | N/A | N/A | Set the delimiter character which is placed between register values for read operations (comma, space, etc) This value is stored in USB_CONFIG |

Help Command
![Help](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_help.JPG)

Factory Reset Command
![Factory Reset](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_reset.JPG)

Register Read Command
![Read](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_read.JPG)

Register Write Command
![Write](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_write.JPG)

Delim Set Command
![Delim](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_delim.JPG)

Read Buffer Command
![ReadBuf](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_readbuf.JPG)

Stream Command (WATERMARK_INT_CONFIG set to 64)

![Stream](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/cli_stream.JPG)