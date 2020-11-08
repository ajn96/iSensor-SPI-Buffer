#Author: Alex Nolan (alex.nolan@analog.com)

import serial
import time
import random
from threading import Thread
from queue import SimpleQueue

class ISensorSPIBuffer():
    """
    Python interface library to the iSensor-SPI-Buffer CLI

    Member Variables
    ---------------------------------------------------------------------------
    StreamRunning: Track if a buffered stream is actively running
    StreamData: Simple queue (of BufferSample) containing stream data
    Ser: Serial port iSensor-SPI-Buffer board is connected to
    """

#Constructor
    
    def __init__(self, portName):
        """
        Initializer. Sets up serial port then connects to iSensor-SPI-Buffer on provided port

        Parameters
        ---------------------------------------------------------------------------
        portName: Name of serial port iSensor-SPI-Buffer is connected to (e.g. COM1, /dev/ttyACM0, etc)
        """
        #Flag to track if stream is in progress
        self.StreamRunning = False
        #Queue for storing data read during stream
        self.StreamData = SimpleQueue()
        self.Ser = serial.Serial(str(portName), 2000000)
        self.Ser.timeout = 0.5
        #Init buffer board
        self.__Connect()
        #create stream worker thread list
        self.StreamThreads = []

#API to the buffer board

    def select_page(self, page):
        """
        Set the active iSensor-SPI-Buffer page. Pages less than 253 will pass to IMU

        Parameters
        ---------------------------------------------------------------------------
        page: Page to select for read/writes. Valid range 0 - 255
        """
        if page > 255:
            raise ValueError("Maximum allowed page value is 255")
        self._SendLine("write 0 " + format(page, "x"))

    def read_regs(self, startAddr, endAddr):
        """
        Read multiple registers on current page, from startAddr to endAddr

        Parameters
        ---------------------------------------------------------------------------
        startAddr: Register starting address to read
        endAddr: End address for the register read operation

        Returns
        ---------------------------------------------------------------------------
        Register read data (as list of int)
        """
        self.__FlushSerialInput()
        self._SendLine("read " + format(startAddr, "x") + " " + format(endAddr, "x"))
        return self._ParseLine(self.__ReadLine())

    def read_reg(self, addr):
        """
        Read a single register on current page, at address addr

        Parameters
        ---------------------------------------------------------------------------
        addr: Address of register to read

        Returns
        ---------------------------------------------------------------------------
        Register value (as int)
        """
        return self.read_regs(addr, addr)[0]

    def write_reg(self, addr, value):
        """
        Write a full 16 bit value to a register, lower then upper

        Parameters
        ---------------------------------------------------------------------------
        addr: Least significant byte address of register to write to
        value: 16-bit value to write to the selected register
        """
        lower = value & 0xFF
        upper = (value & 0xFF00) >> 8
        self._SendLine("write " + format(addr, "x") + " " + format(lower, "x"))
        self._SendLine("write " + format(addr + 1, "x") + " " + format(upper, "x"))

    def version(self):
        """
        Get iSensor-SPI-Buffer firmware version info

        Returns
        ---------------------------------------------------------------------------
        Firmware version info, as a string
        """
        self.__FlushSerialInput()
        self._SendLine("about")
        return self.__ReadLine().replace("\r\n", "")

    def run_command(self, cmdValue):
        """
        Execute an iSensor-SPI-Buffer command

        Parameters
        ---------------------------------------------------------------------------
        cmdValue: 16-bit value to write to the USER_COMMAND register
        """
        self.select_page(253)
        self.write_reg(0x16, cmdValue)

    def set_imu_sclk(self, sclkFreq):
        """
        Configure the SPI clock frequency used to read data from the IMU

        The STM32 processor utilized by the iSensor-SPI-Buffer has a configurable
        2^n divider to set the SPI clock master frequency. This function sets the
        divider to achieve the maximum SPI clock which is lower than the selected
        target frequency. For example, a target frequency of 15MHz would result in
        a real SPI clock frequency of 9MHz (next highest is 18MHz, which exceeds
        the 15MHz limit)

        Parameters
        ---------------------------------------------------------------------------
        sclkFreq: Target SPI clock frequency (Hz)
        """
        baseFreq = 36000000.0
        realFreq = 0.0
        finalBitPos = 0
        for bitPos in range(1, 8):
            realFreq = baseFreq / (2 ^ bitPos)
            if(realFreq <= baseFreq):
                finalBitPos = bitPos
                break
        
        #apply to reg
        self.select_page(253)
        val = self.read_reg(0x10)
        val &= 0x00FF
        val |= (1 << (finalBitPos + 7))
        self.write_reg(0x10, val)

    def set_imu_stall(self, stallUs):
        """
        Configure the stall time between IMU SPI words

        Parameters
        ---------------------------------------------------------------------------
        stallUs: Stall time in microseconds. Valid range 2us - 255us
        """
        if (stallUs < 2) or (stallUs > 255):
            raise ValueError("Invalid stall time!")

        self.select_page(253)
        val = self.read_reg(0x10) #imu spi config
        val &= 0xFF00
        val |= stallUs
        self.__WriteRegAssert(0x10, val)

    def check_connection(self):
        """
        Check CLI connection to the iSensor-SPI-Buffer

        Returns
        ---------------------------------------------------------------------------
        True if connection is good, false otherwise (as bool)
        """
        startPage = self.read_reg(0)
        self.select_page(253)
        origScrVal = self.read_reg(0x26)
        newVal = random.randint(0, 65535)
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

    def enable_imu_burst(self, burstRegAddr, burstLen):
        """
        Configure the buffer board firmware to read burst data from the connected IMU

        Parameters
        ---------------------------------------------------------------------------
        burstRegAddr: Address of the burst trigger register
        burstLen: Length of the burst read operation (bytes)
        """
        if (burstLen < 2) or (burstLen > 64):
            raise ValueError("Invalid burst length!")
        self.select_page(254)
        self.__WriteRegAssert(0x12, (burstRegAddr << 8) & 0xFFFF)
        for addr in range (0x14, 0x52, 2):
            self.write_reg(addr, 0)
        self.select_page(253)
        self.__WriteRegAssert(0x2, 2) #buf_config
        self.__WriteRegAssert(0x4, burstLen) #buf_len

    def set_stream_regs(self, streamRegs):
        """
        Set the IMU registers to be captured during the buffered data stream process

        The provided stream registers are entered into the page 254 write data in the iSensor-SPI-Buffer
        firmware. The provided register list must be less than 32 elements and more than 0 elements, 
        otherwise a ValueError exception will be raised. The buffered capture length register in
        firmware is then set based on the requested number of registers.

        Parameters
        ---------------------------------------------------------------------------
        streamRegs: List of register addresses to read during stream when IMU data ready is triggered
        """
        if (len(streamRegs) > 31) or (len(streamRegs) == 0):
            raise ValueError("Invalid register list length!")
        txLen = 2 * (len(streamRegs) + 1)
        self.select_page(253)
        self.__WriteRegAssert(4, txLen)
        self.select_page(254)
        addr = 0x12
        for reg in streamRegs:
            self.__WriteRegAssert(addr, (reg << 8))
            addr += 2
        self.select_page(253)

    def start_stream(self):
        """
        Start a buffered data capture stream to read data ready synchronous data from the connected IMU

        While a stream is running, data should be read from the StreamData object. Any attempts to directly
        interface with the buffer board during the stream (other than a stream cancel) will result in an 
        exception being raised. The stream start process consists of the following steps:

        1. A new StreamWork thread object is created
        2. The iSensor-SPI-Buffer starting page is read and stored (for restoration after stream)
        3. The expected buffer length is read and stored (to validate buffer data size received during stream)
        4. The firmware FIFO is flushed to ensure only fresh data is present
        5. The firmware watermark auto-set command is executed to ensure optimal USB throughput
        6. The firmware stream start command is issued
        7. The firmware active page is changed to 255 to start the data capture process
        """
        self.StreamThreads.append(StreamWork(self))
        self.StreamThreads[-1].ThreadActive = True
        self.StreamThreads[-1].start()

    def stop_stream(self):
        """
        Signal stream thread to stop

        This function blocks until the active stream thread (if any) exits. At the end of the capture 
        loop, the stream thread object will perform the following steps:
        1. Cancel the stream operation in the firmware
        2. Restore the page selected prior to the stream operation
        """
        if len(self.StreamThreads) > 0:
            self.StreamThreads[-1].ThreadActive = False
            #block until done
            if self.StreamThreads[-1].is_alive():
                self.StreamThreads[-1].join()
        #clear thread list (none active)
        self.StreamThreads.clear()

    def flush_streamdata(self):
        """
        Flush the StreamData Python queue then issue a FIFO reset command to the firmware

        This function can be used to flush all buffered data within the system. Because of the
        double buffered architecture, this should be called any time you wish to start with all 
        fresh data.
        """
        while self.StreamData.empty() == False:
            self.StreamData.get()
        self.run_command(1)
        time.sleep(0.1)

    def get_status(self):
        """
        Get the iSensor-SPI-Buffer status flags

        Returns
        ---------------------------------------------------------------------------
        Error flags parsed from the STATUS register (as Status object)
        """
        self.__FlushSerialInput()
        self._SendLine("status")
        return Status(self._ParseLine(self.__ReadLine())[0])

    def get_uptime(self):
        """
        Get iSensor-SPI-Buffer uptime (ms)

        Returns
        ---------------------------------------------------------------------------
        Uptime, in ms (as int)
        """
        self.__FlushSerialInput()
        self._SendLine("uptime")
        return int(self.__ReadLine().replace("ms\r\n", ""))

    def get_temp(self):
        """
        Get the iSensor-SPI-Buffer processor temperature (degrees C)

        Returns
        ---------------------------------------------------------------------------
        Processor temperature (as float)
        """
        return self.__ReadRegNoPageChange(0x4E, 253) / 10.0

    def get_vdd(self):
        """
        Get the current VDD measurement for the iSensor-SPI-Buffer (volts)

        Returns
        ---------------------------------------------------------------------------
        Processor supply voltage (as float)
        """
        return self.__ReadRegNoPageChange(0x50, 253) / 100.0

#private helper functions (not all actually private, but not recommended for end user use)

    def _SendLine(self, line):
        #transmit line to iSensor SPI buffer CLI
        if self.StreamRunning:
            raise Exception("Please stop stream before interfacing with CLI")
        #10ms (ish) delay before transmitting another line
        time.sleep(0.01)
        self.Ser.write((line + "\r\n").encode('utf_8'))

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

    def __ReadRegNoPageChange(self, addr, page):
        startPage = self.read_reg(0)
        self.select_page(page)
        retVal = self.read_reg(addr)
        self.select_page(startPage)
        return retVal

    def __WriteRegAssert(self, addr, value):
        self.write_reg(addr, value)
        if self.read_reg(addr) != value:
            raise Exception("Read back after write of " + str(value) + " to register at address " + str(addr) + " failed")

    def __FlushSerialInput(self):
        #flush all data on serial input
        self.Ser.reset_input_buffer()

    def __ReadLine(self):
        #read a line from serial input
        return self.Ser.readline().decode('utf_8')

    def __Connect(self):
        #disable echo via reg write rather than command to kinda allow older versions to be used
        self.select_page(253)
        self.__FlushSerialInput()
        #disable streams, disable echo, set comma delim
        self.write_reg(0x14, 0x2C04)
        time.sleep(0.1)
        self.__FlushSerialInput()
        #flush firmare FIFO
        self.run_command(1)
        time.sleep(0.1)


class BufferSample():
    "One sample from the buffer. Contains a data field, microsecond timestamp, UTC timestamp, and checksum validity flag"

    def __init__(self, rawData):
        self.Data = []
        self.ValidChecksum = True
        self.Timestamp = 0
        self.UTC_Timestamp = 0
        self.ReceivedChecksum = 0
        self.ExpectedChecksum = 0

        if (len(rawData) < 7):
            #bad buffer entry
            self.ValidChecksum = False
            return

        #parse out timestamps + data
        self.UTC_Timestamp = rawData[0] + 63356 * rawData[1]
        self.Timestamp = rawData[2] + 63356 * rawData[3]
        self.Data = rawData[5:]

        #compare expected checksum with received
        self.ReceivedChecksum = rawData[4]
        self.ExpectedChecksum = 0
        for i in range(0,4):
            self.ExpectedChecksum += rawData[i]
        for val in self.Data:
            self.ExpectedChecksum += val
        self.ExpectedChecksum = self.ExpectedChecksum & 0xFFFF
        if self.ExpectedChecksum != self.ReceivedChecksum:
            self.ValidChecksum = False

class Status():
    "iSensor-SPI-Buffer device status flags"

    def __init__(self, statusRegVal):
        self.BUF_WATERMARK = bool(statusRegVal & (1 << 0))
        self.BUF_FULL = bool(statusRegVal & (1 << 1))
        self.SPI_ERROR = bool(statusRegVal & (1 << 2))
        self.SPI_OVERFLOW = bool(statusRegVal & (1 << 3))
        self.OVERRUN = bool(statusRegVal & (1 << 4))
        self.DMA_ERROR = bool(statusRegVal & (1 << 5))
        self.PPS_UNLOCK = bool(statusRegVal & (1 << 6))
        self.TEMP_WARNING = bool(statusRegVal & (1 << 7))
        self.SCRIPT_ERROR = bool(statusRegVal & (1 << 10))
        self.SCRIPT_ACTIVE = bool(statusRegVal & (1 << 11))
        self.FLASH_ERROR = bool(statusRegVal & (1 << 12))
        self.FLASH_UPDATE_ERROR = bool(statusRegVal & (1 << 13))
        self.FAULT = bool(statusRegVal & (1 << 14))
        self.WATCHDOG = bool(statusRegVal & (1 << 15))

    def __str__(self):
        attrs = vars(self)
        return (''.join("%s: %s\n" % item for item in attrs.items()))

#Stream worker class
class StreamWork(Thread):
    
    def __init__(self, buf):
        Thread.__init__(self)
        self.buf = buf
        self.ThreadActive = False
        self._StreamStartPage = 253
        self._lineReader = ReadLine(self)
        
    def run(self):
        #read starting page
        self._StreamStartPage = self.buf.read_reg(0)
        #move to page zero and read the buf len
        self.buf.select_page(253)
        bufLen = self.buf.read_reg(4)
        #buffer len should be bufLen / 2 + 5
        bufLen = int(bufLen/2)
        bufLen = bufLen + 5
        #flush firmware buffer contents prior to stream
        self.buf.run_command(1)
        time.sleep(0.1)
        #run watermark autoset
        self.buf.run_command(0x100)
        #start stream on buffer board firmware
        self.buf._SendLine("stream 1")
        self.buf.select_page(255)
        #set stream running flag
        self.buf.StreamRunning = True

        #receive data until stop stream set
        bufEntry = []
        while self.ThreadActive:
            bufEntry = self.buf._ParseLine((self._lineReader.readline()).decode('utf_8'))
            if len(bufEntry) == bufLen:
                self.buf.StreamData.put(BufferSample(bufEntry))

        #stream stop has been signaled. Cancel in firmware and flag
        self.buf.StreamRunning = False
        self.buf._SendLine("stream 0")
        time.sleep(0.1)
        #restore page
        self.buf.select_page(self._StreamStartPage)
        #read page
        self.buf.read_reg(0)

#readline class from pyserial github
class ReadLine:
    def __init__(self, StreamWorker):
        self.buf = bytearray()
        self.StreamWorker = StreamWorker
        self.s = self.StreamWorker.buf.Ser
    
    def readline(self):
        i = self.buf.find(ord('\n'))
        if i >= 0:
            r = self.buf[:i+1]
            self.buf = self.buf[i+1:]
            return r
        while self.StreamWorker.ThreadActive:
            i = max(1, min(2048, self.s.in_waiting))
            data = self.s.read(i)
            i = data.find(ord('\n'))
            if i >= 0:
                r = self.buf + data[:i+1]
                self.buf[0:] = data[i+1:]
                return r
            else:
                self.buf.extend(data)

        #cancelled with no data
        return self.buf
        
