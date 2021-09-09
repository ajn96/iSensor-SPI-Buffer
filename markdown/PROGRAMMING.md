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

## Loading Firmware to the Buffer Board                      '

To load firmware to the Buffer Board, simply drag and drop iSensor-SPI-Buffer.bin to the "NODE_F303RE" device in the Windows explorer.
![Loading Code](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_loading_code.PNG)

## Debugging Common Issues

If the Programmer cannot connect to the Buffer Board, it may give a "Insufficient Space" error message when attempting to load a firmware binary.

![Code Loading Error](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/swd_load_error.PNG)

A common cause is if the Buffer Board is not powered properly. You can power the Buffer Board over USB or via the 24-pin connector, by changing J4 to the desired setting. The system also typically works best if you apply power to the buffer board, then plug in the Nucleo pogo pin SWD connection to the buffer board, then finally connect the Nucleo to your PC via USB. Powering up the buffer board after it has already been connected to the Nucleo does not work consistently.

Updating the buffer board firmware does not change any register values in flash. When code is loaded initially, many registers will start with a value of 0xFFFF. To ensure the buffer board comes up in a good state, it is recommeded to run the Factory Reset command after initial programming.
