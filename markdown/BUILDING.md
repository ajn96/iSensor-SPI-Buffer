# Building the iSensor-SPI-Buffer Firmware

The iSensor-SPI-Buffer firmware can be built using the free eclipse-based [STM32Cube IDE](https://www.st.com/en/development-tools/stm32cubeide.html)

For hardware setup instructions for debugging or loading new firmware, see [PROGRAMMING.md](https://github.com/ajn96/iSensor-SPI-Buffer/blob/master/markdown/PROGRAMMING.md)

## Cloning Repo

First, clone the repo locally using the Git client of your choice. The clone link can be found here on the GitHub repo page:

![Clone Link](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/clone_button.PNG)

Example git clone command through Git bash:

![Clone Process](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/git_clone.PNG)

## Importing Project in STM32Cube IDE

Once the repo is cloned locally and STM32CubeIDE is installed, the firmware project can be added to your IDE workspace. Withing STM32CubeIDE, select "File" -> "Open Projects from File System":

![Add Project](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/open_project_menu.PNG)

Then browse to the /firmware directory within the iSensor-SPI-Buffer repo:

![Load Project](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/project_browse.PNG)

After clicking finished, the iSensor-SPI-Buffer project should be added to the project explorer menu.

## Building Project

To build the firmware, right click on the iSensor-SPI-Buffer project in the project explorer, then select "build". This will build the active configuration (debug or release). The build result will be printed to the console (bottom of the window by default).

![Build](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/build_project.PNG)

To change the selected build configuration (Debug without optimization or Release with optimization) right click and select "Build Configurations" -> "Set Active" -> Debug/Release. Generally, release configuration should be used, unless you are debugging and want to step through the code, in which case Debug should be used. Due to compiler optimizations, debugging with release configuration will produce unexpected/erratic results.

![Set Config](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/set_active_config.PNG)

## Loading Code to Hardware

Once the project has been built and the hardware environment is configured as shown in the [programming guide](https://github.com/ajn96/iSensor-SPI-Buffer/blob/master/markdown/PROGRAMMING.md), you can now load code to the iSensor-SPI-Buffer board. To load a release binary to flash, simply drag the Release/iSensor-SPI-Buffer.bin file to the nucleo board device in "This PC". The programmer will enumerate as a drive labeled "NODE_F303".

To debug the code, select "Debug As" -> "Debug Configurations" within the project menu.

![Debug Config](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/debug_selection.PNG)

In the debug configuration menu, select the SWD option and scan for an ST-Link device. If one is detected, the serial number should be populated automatically. Once a ST-Link SN is listed, click "Apply" then "Debug" to start the debug session and begin stepping though code. The code should break at the first line of main() by default. If the IDE shows a warning about "Symbols not found" or the debugger fails to halt at a breakpoint, ensure the active configuration is debug, not release.

![Debug Config Window](https://github.com/ajn96/iSensor-SPI-Buffer/raw/master/img/debug_config.PNG)
