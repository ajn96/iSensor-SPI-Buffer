import serial
import time
from threading import Thread
from queue import SimpleQueue

class ISensorSPIBuffer():

#Constructor
    
    def __init__(self, portName):
        #Flag to track if stream is in progress
        self.StreamRunning = False
        #Queue for storing data read during stream
        self.StreamData = SimpleQueue()
        self.Ser = serial.Serial(str(portName), 1000000)
        self.Ser.timeout = 0.5
        #Init buffer board
        self._Connect()
        #create stream worker thread
        self.StreamThread = StreamWork(self)

#API to the buffer board

    def select_page(self, page):
        "Set the active iSensor-SPI-Buffer page. Pages less than 253 will pass to IMU"
        self._SendLine("write 0 " + format(page, "x"))

    def read_regs(self, startAddr, endAddr):
        "Read multiple registers on current page, from startAddr to endAddr"
        self._FlushSerialInput()
        self._SendLine("read " + format(startAddr, "x") + " " + format(endAddr, "x"))
        return self._ParseLine(self._ReadLine())

    def read_reg(self, addr):
        "Read a single register on current page, at address addr"
        return self.read_regs(addr, addr)[0]

    def write_reg(self, addr, value):
        "Write a full 16 bit value to a register, lower then upper"
        lower = value & 0xFF
        upper = (value & 0xFF00) >> 8
        self._SendLine("write " + format(addr, "x") + " " + format(lower, "x"))
        time.sleep(0.1)
        self._SendLine("write " + format(addr + 1, "x") + " " + format(upper, "x"))

    def version(self):
        "Get iSensor-SPI-Buffer firmware version info"
        self._FlushSerialInput()
        self._SendLine("about")
        return self._ReadLine().replace("\r\n", "")

    def status(self):
        "Get the iSensor-SPI-Buffer status register value"
        self._FlushSerialInput()
        self._SendLine("status")
        return self._ParseLine(self._ReadLine())

    def run_command(self, cmdValue):
        "Execute iSensor-SPI-Buffer command"
        self._SendLine("cmd " + format(cmdValue, "x"))

    def uptime(self):
        "Get iSensor-SPI-Buffer uptime (ms)"
        self._FlushSerialInput()
        self._SendLine("uptime")
        return int(self._ReadLine().replace("ms\r\n", ""))

    def check_connection(self):
        "Check CLI connection to the iSensor-SPI-Buffer"
        startPage = self.read_reg(0)
        self.select_page(253)
        origScrVal = self.read_reg(0x26)
        newVal = self.uptime()
        self.write_reg(0x26, newVal)
        readBack = self.read_reg(0x26)
        self.write_reg(0x26, origScrVal)
        if self.read_reg(0x26) != origScrVal:
            return False
        self.select_page(startPage)
        if(self.read_reg(0) != startPage):
            return False
        if readBack != newVal:
            return False

        #all checks pass
        return True

    def start_stream(self):
        "Start stream. When a stream is running, data should be read from the StreamQueue"
        self.StreamThread.ThreadActive = True
        self.StreamThread.start()

    def stop_stream(self):
        "Signal stream thread to stop"
        self.StreamThread.ThreadActive = False

#private helper functions (not actually private, but not recommended for end user use)

    def _SendLine(self, line):
        #transmit line to iSensor SPI buffer CLI
        if self.StreamRunning:
            raise Exception("Please stop stream before interfacing with CLI")
        self.Ser.write((line + "\r\n").encode('utf_8'))

    def _FlushSerialInput(self):
        #flush all data on serial input
        self.Ser.reset_input_buffer()

    def _ReadLine(self):
        #read a line from serial input
        return self.Ser.readline().decode('utf_8')

    def _ParseLine(self, line):
        #parse csv line data to numeric array
        line = line.replace("\r", "")
        line = line.replace("\n", "")
        splitLine = line.split(",")
        retVal = []
        for val in splitLine:
            if val != '':
                retVal.append(int(val, 16))
        return retVal

    def _Connect(self):
        #set echo off
        self._SendLine("echo 0")
        time.sleep(0.1)
        #set , delim
        self._SendLine("delim ,")
        time.sleep(0.1)
        self.select_page(253)
        time.sleep(0.1)
        self._FlushSerialInput()

class StreamWork(Thread):
    
    def __init__(self, buf):
        Thread.__init__(self)
        self.buf = buf
        self.ThreadActive = False
        self._StreamStartPage = 253
        
    def run(self):
        #read starting page
        self._StreamStartPage = self.buf.read_reg(0)
        #flush firmware buffer contents prior to stream
        self.buf.run_command(1)
        time.sleep(0.1)
        #start stream on buffer board firmware
        self.buf._SendLine("stream 1")
        self.buf.select_page(255)
        #set stream running flag
        self.buf.StreamRunning = True

        #receive data until stop stream set
        bufEntry = []
        while self.ThreadActive:
            bufEntry = self.buf._ParseLine(self.buf._ReadLine())
            if len(bufEntry) > 0:
                self.buf.StreamData.put(bufEntry)

        #stream stop has been signaled. Cancel in firmware and flag
        self.buf.StreamRunning = False
        self.buf._SendLine("stream 0")
        time.sleep(0.1)
        #restore page
        self.buf.select_page(self._StreamStartPage)
        #read page
        self.buf.read_reg(0)
        
