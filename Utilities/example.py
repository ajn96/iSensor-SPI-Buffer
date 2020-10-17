from spi_buf_cli import ISensorSPIBuffer
import time

#board conneted to COM11
buf = ISensorSPIBuffer("COM11")
print(buf.version())
print(buf.check_connection())

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
print("Decimate: " + str(buf.read_reg(0xC)))
#set decimate for 400Hz data production
buf.write_reg(0xC, 9)
print("Decimate: " + str(buf.read_reg(0xC)))

#set IMU to page 0
buf.select_page(0)
print("IMU Page 0: " + str(buf.read_regs(0x0, 0x26)))

#start a stream (buffer captures data and automatically sends over USB)
print("Start data count: " + str(buf.StreamData.qsize()))
buf.start_stream()
#get 2 sec of data
time.sleep(2)
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

print("Starting buffer timestamp: " + str(startTime))
print("Ending buffer timestamp: " + str(timeStamp))
print("Max timestamp delta: " + str(maxDelta))
