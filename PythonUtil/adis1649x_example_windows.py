from spi_buf_cli import ISensorSPIBuffer
import time

#port name for iSensor-SPI-Buffer
spi_buf_port = "COM11"

#set the capture time (in seconds) for example app
capture_time_sec = 10

#set the data rate for the IMU
data_rate_hz = 1000

#create ISensorSPIBuffer object
buf = ISensorSPIBuffer(spi_buf_port)

#print version and check functionality
print(buf.version())
print("Board connected: " + str(buf.check_connection()))

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
timeStamp = (bufEntry[2] + (65536 * bufEntry[3])) / 1000000.0
lastTimestamp = timeStamp
startTime = timeStamp
delta = 0.0
maxDelta = 0.0
while buf.StreamData.empty() == False:
    bufEntry = buf.StreamData.get()
    lastTimestamp = timeStamp
    timeStamp = (bufEntry[2] + (65536 * bufEntry[3])) / 1000000.0
    delta = timeStamp - lastTimestamp
    if delta > maxDelta:
        maxDelta = delta

maxDelta *= 1000
print("Starting buffer timestamp: " + str(startTime) + " sec")
print("Ending buffer timestamp: " + str(timeStamp) + " sec")
print("Max timestamp delta: " + str(maxDelta) + " ms")

print("Board connected: " + str(buf.check_connection()))
