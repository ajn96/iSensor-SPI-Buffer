#Author: Alex Nolan (alex.nolan@analog.com)

from spi_buf_cli import *
import time

#port name for iSensor-SPI-Buffer
spi_buf_port = "COM11"

#stream time per step
stream_time = 3

#step size (Hz)
step_size_hz = 100

#starting freq
start_freq = 1000

#create ISensorSPIBuffer object
buf = ISensorSPIBuffer(spi_buf_port)

#print version and check functionality
print("Board connected: " + str(buf.check_connection()))

buf.write_reg(0x8, 0x12)
buf.write_reg(0xA, 0x2)
buf.write_reg(0x4, 38)
buf.write_reg(0x2, 0x2)
buf.write_reg(0x10, 0x105)

goodFreq = True
freq = start_freq

while goodFreq:
    print("Testing data ready of " + str(freq) + "Hz")

    buf.write_reg(0x18, freq)
    buf.run_command(0x200)
    time.sleep(0.2)
    buf.flush_streamdata()
    buf.start_stream()
    time.sleep(stream_time)
    buf.stop_stream()
    
    print("Data count: " + str(buf.StreamData.qsize()))
    measuredFreq = 0
    timeStamp = 0
    bufEntry = buf.StreamData.get()
    while buf.StreamData.empty() == False:
        lastTimestamp = bufEntry.Timestamp
        bufEntry = buf.StreamData.get()
        if bufEntry.ValidChecksum == False:
            print("Invalid checksum!")
            goodFreq = False
        timeStamp = bufEntry.Timestamp
        measuredFreq = 1000000.0 / (timeStamp - lastTimestamp)
        #print(measuredFreq)
        if abs(measuredFreq - freq) > (0.1 * freq):
            print("Invalid timestamp freq " + str(measuredFreq))
            #goodFreq = False;
            
    if goodFreq:
        goodFreq = buf.check_connection()

    freq += step_size_hz
