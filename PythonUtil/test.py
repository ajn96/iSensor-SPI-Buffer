#Author: Alex Nolan (alex.nolan@analog.com)
#These tests should be run without any connected IMU

import unittest
import time
import random
from spi_buf_cli import ISensorSPIBuffer

#iSensor-SPI-Buffer object under test
buf = ISensorSPIBuffer("COM5")

class SpiBufTestMethods(unittest.TestCase):

    def setUp(self):
        #make sure board is in a good state
        buf.stop_stream()
        buf.select_page(253)
        self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

    def test_set_stream_regs(self):
        for i in range(1, 32):
            regs = []
            buf.select_page(253)
            for j in range(0, i):
                regs.append(random.randint(0, 127))
            buf.set_stream_regs(regs)
            #check that values were placed correctly
            self.assertEqual(253, buf.read_reg(0), "ERROR: page not restored")
            self.assertEqual(2 * (i + 1), buf.read_reg(4), "ERROR: Invalid buffer length")
            buf.select_page(254)
            index = 0
            for addr in range(0x12, 0x12 + (2 * i), 2):
                self.assertEqual(buf.read_reg(addr), regs[index] << 8, "ERROR: Invalid write data")
                index += 1

    def test_buf_pageselect(self):
        for trial in range(10):
            for page in range(252, 255):
                buf.select_page(page)
                self.assertEqual(page, buf.read_reg(0), "ERROR: Invalid page")

    def test_imu_pageselect(self):
        buf.select_page(253)
        for page in range(0, 251):
            buf.select_page(page)
            self.assertEqual(0, buf.read_reg(0), "ERROR: Invalid IMU page")
            
    def test_uptime(self):
        uptime = buf.get_uptime()
        time.sleep(0.5)
        self.assertAlmostEqual(uptime + 500, buf.get_uptime(), delta = 50, msg = "Invalid uptime")

    def test_stream(self):
        #set up buffer to trigger itself
        buf.select_page(253)
        #set DIO2 as data ready
        buf.write_reg(0x8, 0x12)
        #set DIO2 as pass pin
        buf.write_reg(0xA, 0x2)
        #start generating 1000Hz data ready
        buf.write_reg(0x18, 1000)
        buf.run_command(0x200)
        buf.flush_streamdata()
        self.assertEqual(0, buf.StreamData.qsize())
        buf.start_stream()
        lastCount = 0
        dataCount = 0
        for trial in range(5):
            time.sleep(0.5)
            dataCount = buf.StreamData.qsize()
            self.assertGreater(dataCount, lastCount, "ERROR: Count failed to increase " + str(trial))
            lastCount = dataCount
        buf.stop_stream()
        dataCount = buf.StreamData.qsize()
        time.sleep(0.1)
        self.assertEqual(dataCount, buf.StreamData.qsize())
        self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

    def test_streamstop(self):
        for trial in range(5):
            buf.stop_stream()
            self.assertTrue(buf.check_connection(), "ERROR: Connection check failed")

    def test_stream_nodata(self):
        #write to dio_output to stop sync gen
        buf.write_reg(0xA, 0x2)
        

if __name__ == '__main__':
    unittest.main()



