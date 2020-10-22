#Author: Alex Nolan (alex.nolan@analog.com)
#These tests should be run without any connected IMU

import unittest
import time
from spi_buf_cli import ISensorSPIBuffer

#iSensor-SPI-Buffer object under test
buf = ISensorSPIBuffer("COM11")

class SpiBufTestMethods(unittest.TestCase):

    def test_buf_pageselect(self):
        for trial in range(10):
            for page in range(253, 255):
                buf.select_page(page)
                self.assertEqual(page, buf.read_reg(0), "ERROR: Invalid page")

    def test_imu_pageselect(self):
        buf.select_page(253)
        for page in range(0, 252):
            buf.select_page(page)
            self.assertEqual(0, buf.read_reg(0), "ERROR: Invalid IMU page")
            
    def test_uptime(self):
        uptime = buf.get_uptime()
        time.sleep(0.5)
        self.assertAlmostEqual(uptime + 500, buf.get_uptime(), delta = 50, msg = "Invalid uptime")

    def test_stream(self):
        #set up buffer to trigger itself
        buf.select_page(253)
        #set DIO1 as data ready
        buf.write_reg(0x8, 0x11)
        #watermark toggle mode, 0 samples
        buf.write_reg(0xC, 0x8000)
        #output watermark interrupt on DIO1 and pass DIO1 back to buffer
        buf.write_reg(0xA, 0x11)
        buf.flush_streamdata()
        self.assertEqual(0, buf.StreamData.qsize())
        buf.start_stream()
        lastCount = 0
        dataCount = 0
        for trial in range(10):
            time.sleep(0.2)
            dataCount = buf.StreamData.qsize()
            self.assertGreater(dataCount, lastCount, "ERROR: Count failed to increase")
            lastCount = dataCount
        buf.stop_stream()
        dataCount = buf.StreamData.qsize()
        time.sleep(0.1)
        self.assertEqual(dataCount, buf.StreamData.qsize())
        self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

    def test_streamstop(self):
        self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

    def test_stream_nodata(self):
        self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

if __name__ == '__main__':
    unittest.main()



