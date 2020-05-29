using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NUnit.Framework;
using FX3Api;
using System.IO;
using System.Reflection;

namespace iSensor_SPI_Buffer_Test
{
    class StatusTests : TestBase
    {
        [Test]
        public void StatusClearTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void StatusBufFullTest()
        {
            InitializeTestCase();

            uint status, buf_capacity;

            status = ReadUnsigned("STATUS");
            Console.WriteLine("Initial status: 0x" + status.ToString("X4"));

            WriteUnsigned("BUF_LEN", 16, true);
            buf_capacity = ReadUnsigned("BUF_MAX_CNT");
            Console.WriteLine("Max buffer count: " + buf_capacity.ToString());

            Console.WriteLine("Filling buffer...");
            FX3.StartPWM(1000, 0.5, FX3.DIO1);
            ReadUnsigned("BUF_TIMESTAMP_LWR");
            System.Threading.Thread.Sleep((int) buf_capacity * 2);
            FX3.StopPWM(FX3.DIO1);
            System.Threading.Thread.Sleep(10);
            Console.WriteLine("BUF_CNT: " + ReadUnsigned("BUF_CNT").ToString());
            Assert.AreEqual(buf_capacity, ReadUnsigned("BUF_CNT"), "ERROR: Expected buffer to be at max capacity");
            for(int trial = 0; trial < 5; trial++)
            {
                status = ReadUnsigned("STATUS");
                Console.WriteLine("Status: 0x" + status.ToString("X4"));
                Assert.AreEqual(1 << STATUS_BUF_FULL, status & (1 << STATUS_BUF_FULL), "ERROR: Expected BUF_FULL status bit to be set");
                System.Threading.Thread.Sleep(1);
            }
            Console.WriteLine("Removing item from buffer...");
            ReadUnsigned("BUF_RETRIEVE");
            status = ReadUnsigned("STATUS");
            Console.WriteLine("Status: 0x" + status.ToString("X4"));
            Assert.AreEqual(1 << STATUS_BUF_FULL, status & (1 << STATUS_BUF_FULL), "ERROR: Expected BUF_FULL status bit to be set");
            status = ReadUnsigned("STATUS");
            Console.WriteLine("Status: 0x" + status.ToString("X4"));
            Assert.AreEqual(0, status & (1 << STATUS_BUF_FULL), "ERROR: Expected BUF_FULL status bit to be cleared");
        }

        [Test]
        public void StatusTCTest()
        {
            InitializeTestCase();

            uint oldTc, expectedTc;

            oldTc = (ReadUnsigned("STATUS") >> 12);
            for (int trial = 0; trial < 32; trial++)
            {
                expectedTc = (oldTc + 3) & 0xF;
                oldTc = (ReadUnsigned("STATUS") >> 12);
                Console.WriteLine("Status TC: " + oldTc.ToString());
                Assert.AreEqual(expectedTc, oldTc, "ERROR: Invalid TC");
            }
        }


    }
}
