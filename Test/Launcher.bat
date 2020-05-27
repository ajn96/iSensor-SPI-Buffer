@echo off
echo Launching NUnit Tests from %CD%
set "environment_good="
if exist "%CD%\NUnit-2.6.4\bin\nunit-x86.exe" (
	echo NUnit executable found
) else (
	echo Error: Please check that you have NUnit-2.6.4 in the proper location
	set "environment_good = false" 
	pause
)
if exist "%CD%\iSensor-SPI-Buffer-Test\bin\Debug\iSensor-SPI-Buffer-Test.dll" (
	echo Test DLL found
) else (
	echo Error: Test DLL not found
	set "environment_good = false" 
	pause
)
if not defined environment_good (
	NUnit-2.6.4\bin\nunit-x86.exe "%CD%\iSensor-SPI-Buffer-Test\bin\Debug\iSensor-SPI-Buffer-Test.dll"
)
