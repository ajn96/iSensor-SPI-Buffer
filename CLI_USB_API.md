# iSensor-SPI-Buffer USB Virtual COM Port CLI

## Driver

The USB port on the iSensor-SPI-Buffer is configured to act as a CDC type device, presenting as a virtual COM (serial) port.

The (windows) drivers to interface with the virtual COM port are provided by ST, and can be downloaded here:
https://www.st.com/en/development-tools/stsw-stm32102.html

## Functionality

The iSensor-SPI-Buffer virtual COM port exposes the same iSensor-SPI-Buffer register interface which can be accessed over SPI. In general, the functionality of each register should be the same, regardless of if the register is accessed over USB or SPI. The registers are accessed via a simple command line interface (CLI). This CLI allows a PC to interface with an IMU without the need for a SPI <-> USB board (like EVAL-ADIS-FX3).

## CLI Definition

Commands are sent to the CLI over the virtual COM port. Each command starts with a command word, followed by a number of arguments. Each command ends with a new line character. All values are in hex.

| Command | arg0 | arg1 | arg2 | Description |
| --- | --- | --- | --- | --- |
| help | N/A | N/A | N/A | List available CLI options |
| reset | N/A | N/A | N/A | Trigger a system reset  |
| read | Register address | N/A | N/A | Read a single register, at the address provided |
| read | Start address | End address | N/A | Read multiple registers, accross the address range provided |
| read | Start address | End address | Number of reads | Read multiple registers, accross the address range provided, number of reads times. Each set of reads is terminated by a new line |
| write | Register address | Write Value | N/A | N/A | Write a byte to the specified register |