# SD Card Data Logging

The iSensor-SPI-Buffer comes equipped with an microSD card slot to allow for autonomous device configuration and data logging via a simple scripting language. The available SD card script commands are functionally the same as the [USB command line interface](https://github.com/ajn96/iSensor-SPI-Buffer/blob/master/USB_CLI.md) commands, with a superset of additional program flow commands added.

This SD card script engine can be used for headless data capture with a USB battery pack. This is useful for systems where connecting the IMU to a PC is not feasible.

![iSensor SPI Buffer Headless Capture](https://raw.githubusercontent.com/ajn96/iSensor-SPI-Buffer/master/img/sd_headless_capture.jpg)

## Script Format

iSensor-SPI-Buffer SD card scripts use the same command format as the USB CLI, with any data which would normally be sent to the USB virtual COM port being written to the SD card result file. In addition to the USB CLI commands, there are three additional commands supported for SD card scripts which are not supported for the USB CLI:

* loop [loopcount]: Start of a fixed count loop
* endloop: End of a fixed count loop. No loop nesting is allowed
* sleep [sleep time, in ms]: Sleep script execution process

This set of commands gives the user significant flexibility, without making the implementation or script writing process overly complicated. Keeping all loops fixed count, and not including conditional operations ensures that all scripts should finish in a deterministic amount of time. Consistent timing behavior is important, since improperly terminating a script (by power loss, reset, etc) can result in data loss on the SD card.

## SD Card Configuration

In order to run an SD card script, the SD card must be formatted FAT32, with a single volume. 

The script file must be saved as "script.txt" in the top level directory of the SD card volume. The output from the script running will be appended to "result.txt" in the top level directory. If "result.txt" does not exist, it will be created by the iSensor-SPI-Buffer firmware.

At the start of each script run, "Script Starting..." will be printed to the result file, to delineate different script runs

## Starting a Script

If the iSensor-SPI-Buffer is connected to a host processor, a script can be started by setting the SCRIPT_START bit of the COMMAND register once a correctly formatted SD card is inserted.

If the iSensor-SPI-Buffer is intended to run headless (powered by a USB battery pack), a user can set the SCRIPT_AUTORUN bit of the USB_CONFIG register, and issue a flash update to store the register value to non-volatile memory. If the script autorun bit is set when the iSensor-SPI-Buffer firmware finishes initialization, a script start command will be executed autonomously.

If there is an error starting a script (no SD card, invalid format, invalid script file, etc) the SCRIPT_ERROR bit of the STATUS register will be set. If the script starts successfully, the SCRIPT_RUNNING bit of the STATUS register will be set. This bit will stay set until the script execution process finishes, or is terminated. Once the script has finished, the SCRIPT_RUNNING bit will clear automatically, without the STATUS register being read. This allows a user to monitor the script execution process without register access by looking at the error LED and ERROR interrupt outputs (set ERROR_INT_CONFIG to mask out other error sources).

To cancel a running script, the user can set the SCRIPT_CANCEL bit of the COMMAND register. This will stop the script execution process, close any open files, and unmount the SD card volume. Unfortunately, there is not any way to cancel a script if the iSensor-SPI-Buffer is running headless.

## Example Scripts

These are some potential scripts a user could utilize for the [ADIS16497](https://www.analog.com/en/products/adis16497.html) high performance IMU. These scripts assume that the iSensor-SPI-Buffer registers are already configured as desired by the user.

### Continuous Data Streaming Through Buffer

`write 0 3 //select page 3 on IMU`

`write c 50 //set DEC_RATE to 0x50 (80) to reduce ODR to a rate slower than the SD card can write`

`write 0 0 //Select page 0 on IMU (output data page)`

`stream 1 //enable data streaming`

`write 0 ff //select page 255 (buf data page). This starts data capture process`

`sleep 2710 //sleep for 0x2710ms (10 seconds). The stream will log data while this sleep is processing`

`stream 0 //disable streaming`

### Asynchronous IMU Register Logging

`write 0 0 //select IMU page 0`

`loop 64 //loop 0x64 (100) times`

`read 4 28 //read IMU output data registers`

`sleep 64 //sleep for 100ms`

`endloop //end of loop`

### Full Bandwidth Buffered Data Capture

`write 0 3 //select page 3 on IMU`

`write c 0 //set DEC_RATE to 0 to get max ODR`

`write 0 0 //Select page 0 on IMU (output data page)`

`loop a //loop 10 times`

`write 0 ff //select page 255 (buf data page). This starts buffered data capture process`

`sleep 64 //sleep for 100ms, to allow the buffer to fill with IMU data`

`write 0 fd //select page 253 (buf config page). This stops buffered data capture process`

`readbuf //write full buffer contents to the SD card`

`endloop //end of loop`



