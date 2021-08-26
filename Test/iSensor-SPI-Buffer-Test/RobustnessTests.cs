using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NUnit.Framework;
using FX3Api;
using System.IO;
using System.Reflection;
using RegMapClasses;

namespace iSensor_SPI_Buffer_Test
{
    class RobustnessTests : TestBase
    {
        [Test]
        public void PPSEnableTest()
        {
            /* Set DIO input config for data ready on DIO1, PPS on DIO2 */
            WriteUnsigned("DIO_INPUT_CONFIG", 0x201);
            /* Enable DIO1, DIO2 pass */
            WriteUnsigned("DIO_OUTPUT_CONFIG", 3);
            /* Generate sync and DR signals from FX3 */
            FX3.StartPWM(1000, 0.5, FX3.DIO1);
            
            Assert.AreEqual(0, ReadUnsigned("BUF_CNT"), "Expected buffer count of 0");
            Assert.AreEqual(0, ReadUnsigned("BUF_CNT"), "Expected buffer count of 0");
            Assert.AreEqual(0, ReadUnsigned("STATUS"), "Expected STATUS of 0");
            WriteUnsigned("USER_COMMAND", 1u << COMMAND_PPS_START, false);
            System.Threading.Thread.Sleep(1500);
            Assert.AreEqual(1u << STATUS_PPS_UNLOCK, ReadUnsigned("STATUS"), "Expected unlock to be flagged");
            FX3.StartPWM(1, 0.5, FX3.DIO2);
            ReadUnsigned("STATUS");
            System.Threading.Thread.Sleep(1500);
            ReadUnsigned("STATUS");
            System.Threading.Thread.Sleep(1500);
            Assert.AreEqual(0, ReadUnsigned("STATUS"), "Expected STATUS of 0");
            Assert.AreEqual(0, ReadUnsigned("BUF_CNT"), "Expected buffer count of 0");
        }

        [Test]
        public void InvalidSpiTrafficTest()
        {

        }

        [Test]
        public void ADIS1649xBurstTest()
        {
            List<ushort[]> data;
            List<ushort[]> temp;
            uint count;
            uint buffersRead;
            uint max;
            RegClass dec_reg = new RegClass { Address = 12, Page = 3 };
            RegClass fnctio_ctrl_reg = new RegClass { Address = 6, Page = 3 };
            RegClass status_reg = new RegClass { Address = 8, Page = 0 };

            /* Want stall time 5us, sclk freq 2MHz */
            WriteUnsigned("IMU_SPI_CONFIG", 0x805);
            /* Set for double burst */
            WriteUnsigned("BUF_CONFIG", 6);
            /* Set 38 bytes capture */
            WriteUnsigned("BUF_LEN", 38);
            /* Set for passthrough on DIO1, DR on DIO1 */
            WriteUnsigned("DIO_INPUT_CONFIG", 0x11);
            WriteUnsigned("DIO_OUTPUT_CONFIG", 1);

            /* Set writedata regs to zero */
            foreach (RegClass reg in WriteDataRegs)
            {
                Dut.WriteUnsigned(reg, 0);
            }
            /* Set burst trigger */
            WriteUnsigned("BUF_WRITE_0", 0x7C00);

            max = ReadUnsigned("BUF_MAX_CNT");
            Console.WriteLine("Max buffer capacity: " + max.ToString());

            /* Ensure IMU is configured for operation */
            FX3.SetPin(FX3.ResetPin, 0);
            FX3.SetPin(FX3.ResetPin, 1);
            System.Threading.Thread.Sleep(1000);
            FX3.StallTime = 50;
            Dut.WriteUnsigned(dec_reg, 1);
            Assert.AreEqual(1, Dut.ReadUnsigned(dec_reg), "Decimate write failed");
            Dut.WriteUnsigned(fnctio_ctrl_reg, 0xC);
            Assert.AreEqual(0xC, Dut.ReadUnsigned(fnctio_ctrl_reg));

            /* Read IMU status */
            Console.WriteLine("IMU Status: 0x" + Dut.ReadUnsigned(status_reg).ToString("X4"));

            /* Clear buffer */
            WriteUnsigned("BUF_CNT_1", 0, false);

            Console.WriteLine("Testing DR freq: " + FX3.MeasurePinFreq(FX3.DIO1, 1, 100, 5).ToString() + "Hz");
            buffersRead = 0;

            /* Set up for streams */
            FX3.TriggerReg = RegMap["BUF_RETRIEVE"];
            FX3.WordCount = 24;

            data = new List<ushort[]>();
            while (buffersRead < 4000)
            {
                count = ReadUnsigned("BUF_CNT_1");
                count -= 2;
                FX3.SetupBurstMode();
                FX3.StartBurstStream(count, FX3.BurstMOSIData);
                System.Threading.Thread.Sleep(20);
                FX3.WaitForStreamCompletion();
                temp = GetBurstData();
                /* First burst read is invalid */
                temp.RemoveAt(0);
                for (int i = 1; i < temp.Count; i++)
                {
                    Assert.AreEqual(temp[i][temp[i].Count() - 2], (temp[i - 1][temp[i].Count() - 2] + 1) & 0xFFFFu, "Invalid data count increment");
                }
                data.AddRange(temp);
                buffersRead += count;
                Console.WriteLine("Available buffers: " + count.ToString());
            }
            Console.WriteLine("Total buffers read: " + data.Count.ToString());
            CheckDUTConnection();

        }

        [Test]
        public void ADIS1650xBufferTest()
        {

            /* Want stall time 15us, sclk freq 2.25MHz */
            WriteUnsigned("IMU_SPI_CONFIG", 0x810, true);
        }

        [Test]
        public void SclkFreqTest()
        {

            int freq = 10000;
            bool goodFreq = true;
            uint readVal, expectedVal;

            while (goodFreq)
            {
                Console.WriteLine("Testing SCLK freq of " + freq.ToString() + "Hz");
                FX3.SclkFrequency = freq;
                expectedVal = (uint)(freq & 0xFFFF);
                WriteUnsigned("USER_SCR_1", expectedVal, false);
                readVal = ReadUnsigned("USER_SCR_1");
                if (readVal != expectedVal)
                {
                    goodFreq = false;
                    Console.WriteLine("ERROR: Invalid read back data. Expected 0x" + expectedVal.ToString("X4") + " received: 0x" + readVal.ToString("X4"));
                }
                if (goodFreq)
                    freq += 10000;
            }
            Assert.GreaterOrEqual(freq, 10000000, "ERROR: Expected supported SCLK freq of > 10MHz");
        }

        [Test]
        public void StallTimeTest()
        {
            double readStall, writeStall;

            readStall = FindReadStallTime();
            Console.WriteLine("Min read stall time: " + readStall.ToString() + "us");

            writeStall = FindWriteStallTime();
            Console.WriteLine("Min write stall time: " + writeStall.ToString() + "us");

            Assert.LessOrEqual(readStall, 5, "ERROR: Max stall 5us allowed");
            Assert.LessOrEqual(writeStall, 5, "ERROR: Max stall 5us allowed");
        }

        [Test]
        public void PageSwitchTest()
        {
            WriteUnsigned("BUF_CONFIG", 6);
            WriteUnsigned("DIO_INPUT_CONFIG", 1);
            WriteUnsigned("DIO_OUTPUT_CONFIG", 1);

            FX3.StartPWM(1000, 0.5, FX3.DIO1);
            FX3.StallTime = 50;
            FX3.SclkFrequency = 8000000;

            Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS").ToString("X4"));

            for(int trial = 0; trial < 128; trial++)
            {
                try
                {
                    PageSwitchSequence();
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                    System.Threading.Thread.Sleep(2200);
                    Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS_1").ToString("X4"));
                    CheckDUTConnection();
                }
            }

        }

        private void PageSwitchSequence()
        {
            CheckDUTConnection();
            Console.WriteLine("Moving to page 255...");
            ReadUnsigned("BUF_SIG");
            System.Threading.Thread.Sleep(100);
            Console.WriteLine("Buffer count: " + ReadUnsigned("BUF_CNT_1").ToString());
            Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS_1").ToString("X4"));

            Console.WriteLine("Moving to IMU page...");
            for (uint addr = 2; addr < 126; addr += 2)
            {
                Dut.ReadUnsigned(new RegClass { Page = 0, Address = 2, NumBytes = 2 });
                System.Threading.Thread.Sleep(10);
            }

            Console.WriteLine("Moving to page 255...");
            Console.WriteLine("Buffer count: " + ReadUnsigned("BUF_CNT_1").ToString());
            Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS_1").ToString("X4"));

            Console.WriteLine("Moving to page 253...");
            Console.WriteLine("Buffer count: " + ReadUnsigned("BUF_CNT").ToString());
            Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS").ToString("X4"));
            System.Threading.Thread.Sleep(100);
            CheckDUTConnection();
            Console.WriteLine("Buffer count: " + ReadUnsigned("BUF_CNT").ToString());
            Console.WriteLine("Clearing buffer...");
            WriteUnsigned("USER_COMMAND", 1u << COMMAND_CLEAR_BUFFER, false);
            System.Threading.Thread.Sleep(10);
            Console.WriteLine("Buffer count: " + ReadUnsigned("BUF_CNT").ToString());
        }
    }
}
