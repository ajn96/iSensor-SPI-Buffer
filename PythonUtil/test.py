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


if __name__ == '__main__':
    unittest.main()



