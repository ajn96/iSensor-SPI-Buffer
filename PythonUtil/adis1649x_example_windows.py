#Author: Alex Nolan (alex.nolan@analog.com)

from spi_buf_cli import ISensorSPIBuffer
import time

#port name for iSensor-SPI-Buffer
spi_buf_port = "COM11"

#set the capture time (in seconds) for example app
capture_time_sec = 2

#set the data rate for the IMU
data_rate_hz = 1000

#create ISensorSPIBuffer object
buf = ISensorSPIBuffer(spi_buf_port)

#print version and check functionality
print("Board connected: " + str(buf.check_connection()))

try:
    print(buf.version())
    print("Buffer board uptime: " + str(buf.get_uptime()) + "ms")
except:
    print("Unsupported command on this firmware revision!")

print("Buffer board temperature: " + str(buf.get_temp()) + "C")
print("Buffer board Vdd: " + str(buf.get_vdd()) + "V")

#select buf config page
buf.select_page(253)

#configure buffer settings for ADIS1649x
buf.write_reg(0x2, 2) #IMU burst
buf.write_reg(0x4, 38) # 38-byte burst read
buf.write_reg(0x10, 0x805) # 2.25MHz spi clock, 5us stall

#read regs back
print("Config Page: " + str(buf.read_regs(0x0, 0x10)))

#set burst trigger word
buf.select_page(254)
buf.write_reg(0x12, 0x7C00)

#move to page 3 (ADIS1649x config page)
buf.select_page(3)
print("Configuring IMU for " + str(data_rate_hz) + "Hz ODR")
#set decimate based on user setting
dec = int(4000 / data_rate_hz) - 1
if dec < 0 :
    dec = 0
buf.write_reg(0xC, dec)
print("Decimate: " + str(buf.read_reg(0xC)))

#set IMU to page 0
buf.select_page(0)
print("IMU Page 0: " + str(buf.read_regs(0x0, 0x26)))

#start a stream (buffer captures data and automatically sends over USB)
print("Start data count: " + str(buf.StreamData.qsize()))
buf.start_stream()
print("Sleeping for " + str(capture_time_sec) + " seconds...")
time.sleep(capture_time_sec)
buf.stop_stream()
print("End data count: " + str(buf.StreamData.qsize()))

bufEntry = buf.StreamData.get()
lastTimestamp = bufEntry.Timestamp
startTime = bufEntry.Timestamp
delta = 0.0
maxDelta = 0.0
while buf.StreamData.empty() == False:
    lastTimestamp = bufEntry.Timestamp
    bufEntry = buf.StreamData.get()
    if bufEntry.ValidChecksum == False:
        print("Invalid checksum!")
    timeStamp = bufEntry.Timestamp
    print(str(bufEntry.Data[-2]) + " " + str(timeStamp))
    delta = timeStamp - lastTimestamp
    if delta > maxDelta:
        maxDelta = delta
    if(delta < 0):
        print("Invalid delta!")

startTime /= 1000
lastTimestamp /= 1000
maxDelta /= 1000
print("Starting buffer timestamp: " + str(startTime) + " ms")
print("Ending buffer timestamp: " + str(lastTimestamp) + " ms")
print("Max timestamp delta: " + str(maxDelta) + " ms")

print("Board connected: " + str(buf.check_connection()))
