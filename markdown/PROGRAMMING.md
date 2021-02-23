# Programming the iSensor SPI Buffer

## Required Hardware

The iSensor SPI Buffer board was heavily based on the STM32 Nucleo-64 ([link](https://www.st.com/en/evaluation-tools/nucleo-f303re.html)) and, as such, is able to be programmed using the same, onboard programmer included with the development board. 

Here's what you'll need to program the SPI Buffer Board:

- STM32 Nucleo-64 ([link](https://www.st.com/en/evaluation-tools/nucleo-f303re.html)) 
- Tag-Connect TC2030-IDC ([link](https://www.tag-connect.com/product/tc2030-idc-6-pin-tag-connect-plug-of-nails-spring-pin-cable-with-legs))
- 6 x 0.1" Jumper Leads/Cables ([link](https://www.amazon.com/Premium-Breadboard-Jumper-100-Pack-Hellotronics/dp/B07GJJRH3M))

## Wiring the Programmer

Using the jumper leads, connect the programmer connector as shown below. 

![SWD Connector Wiring](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_connector_pinout.jpg)

![SWD STM32 Wiring](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_stm32_board.jpg)

## Changing the SWD Jumpers to "External Programming" Mode

The STM32 Nucleo-64 must be configured for external programming by changing two jumpers as shown below.

![SWD STM32 External Programming Jumpers](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_programmer_jumpers.jpg)

## Connecting the SWD Programmer to the SPI Buffer Board

The Tag-Connect pogo-pin adapter should securely clip into the SWD programming port on the SPI Buffer Board.

![SWD Tag-Connect Connector](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_buffer_board_connector.jpg)

## Powering the Buffer Board

The SPI Buffer Board **must be powered externally during programming!** The SWD port does *not* provide power. Power can be supplied to the SPI Buffer Board using either the USB port or the 24-pin IMU host connector. 

