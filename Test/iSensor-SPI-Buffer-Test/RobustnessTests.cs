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
        public void InvalidSpiTrafficTest()
        {
            InitializeTestCase();

        }

        [Test]
        public void ADIS1649xBufferTest()
        {
            InitializeTestCase();

            uint[] buf;
            uint count;
            bool goodData = true;
            uint buffersRead;
            uint max;
            double freq = 4250;

            /* Want stall time 3us, sclk freq 9MHz */
            WriteUnsigned("IMU_SPI_CONFIG", 0x203, true);

            /* Set writedata regs */
            uint index = 1;
            foreach (RegClass reg in WriteDataRegs)
            {
                Dut.WriteUnsigned(reg, index);
                index++;
            }

            List<RegClass> ReadRegs = new List<RegClass>();
            ReadRegs.Add(RegMap["BUF_RETRIEVE"]);
            ReadRegs.Add(RegMap["BUF_CNT_1"]);
            ReadRegs.Add(RegMap["BUF_TIMESTAMP_LWR"]);
            ReadRegs.Add(RegMap["BUF_TIMESTAMP_UPR"]);
            ReadRegs.Add(RegMap["BUF_DELTA_TIME"]);
            /* Realistic use case: 32-bit data on each axis + temp + data count + status -> 15 register reads -> 16 SPI words */
            for (int i = 0; i < 16; i++)
            {
                ReadRegs.Add(RegMap["BUF_DATA_" + i.ToString()]);
            }
            ReadRegs.Add(RegMap["BUF_SIG"]);

            WriteUnsigned("BUF_LEN", 32, true);
            max = ReadUnsigned("BUF_MAX_CNT");
            Console.WriteLine("Max buffer capacity: " + max.ToString());

            /* Set data production rate 4.25KHz */
            FX3.StartPWM(freq, 0.5, FX3.DIO1);

            /* Clear */
            WriteUnsigned("BUF_CNT_1", 0, false);

            Console.WriteLine("Testing DR freq: " + freq.ToString() + "Hz");
            buffersRead = 0;
            while (goodData)
            {
                System.Threading.Thread.Sleep(10);
                count = ReadUnsigned("BUF_CNT_1");
                count -= 2;
                //Console.WriteLine("Reading " + count.ToString() + " buffer entries...");
                buf = Dut.ReadUnsigned(10, ReadRegs, 1, count);
                goodData = ValidateBufferData(buf, ReadRegs, (int)count, freq);
                buffersRead += count;
                Console.WriteLine("Total buffers read: " + buffersRead.ToString());
            }
        }

        [Test]
        public void ADIS1650xBufferTest()
        {
            InitializeTestCase();

            /* Want stall time 15us, sclk freq 2.25MHz */
            WriteUnsigned("IMU_SPI_CONFIG", 0x810, true);
        }

        [Test]
        public void SclkFreqTest()
        {
            InitializeTestCase();

            int freq = 10000;
            bool goodFreq = true;
            uint readVal, expectedVal;

            while(goodFreq)
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
            InitializeTestCase();

            double readStall, writeStall;

            readStall = FindReadStallTime();
            Console.WriteLine("Min read stall time: " + readStall.ToString() + "us");

            writeStall = FindWriteStallTime();
            Console.WriteLine("Min write stall time: " + writeStall.ToString() + "us");

            Assert.LessOrEqual(readStall, 5, "ERROR: Max stall 5us allowed");
            Assert.LessOrEqual(writeStall, 5, "ERROR: Max stall 5us allowed");
        }

        double FindReadStallTime()
        {
            double stall = 10;
            bool goodStall = true;

            WriteUnsigned("USER_SCR_1", 0x55AA, true);

            List<byte> MOSI = new List<byte>();
            for(int i = 0; i < 128; i++)
            {
                MOSI.Add((byte) RegMap["USER_SCR_1"].Address);
                MOSI.Add(0);
            }

            byte[] MISO;

            FX3.BitBangSpiConfig = new BitBangSpiConfig(true);
            FX3.SetBitBangSpiFreq(900000);
            FX3.BitBangSpiConfig.CSLagTicks = 0;
            FX3.BitBangSpiConfig.CSLeadTicks = 0;

            while (goodStall)
            {
                Console.WriteLine("Testing stall time: " + stall.ToString() + "us");
                FX3.SetBitBangStallTime(stall);
                MISO = FX3.BitBangSpi(16, 128, MOSI.ToArray(), 1000);
                for(int i = 2; i < MISO.Count() - 1; i+= 2)
                {
                    if (MISO[i] != 0x55)
                        goodStall = false;
                    if (MISO[i + 1] != 0xAA)
                        goodStall = false;
                }
                if (goodStall)
                    stall -= 0.2;
                if (stall < 0.5)
                    return stall;
            }
            FX3.RestoreHardwareSpi();
            return stall;
        }

        double FindWriteStallTime()
        {
            double stall = 10;
            bool goodStall = true;

            WriteUnsigned("USER_SCR_1", 0, true);

            List<byte> MOSI = new List<byte>();
            MOSI.Add((byte)(RegMap["USER_SCR_1"].Address | 0x80));
            MOSI.Add(0x55);
            MOSI.Add((byte)(RegMap["USER_SCR_1"].Address + 1 | 0x80));
            MOSI.Add(0xAA);

            List<byte> MOSIRead = new List<byte>();
            MOSIRead.Add((byte)RegMap["USER_SCR_1"].Address);
            MOSIRead.Add(0);
            MOSIRead.Add(0);
            MOSIRead.Add(0);

            List<byte> MOSIClear = new List<byte>();
            MOSIClear.Add((byte)(RegMap["USER_SCR_1"].Address | 0x80));
            MOSIClear.Add(0);
            MOSIClear.Add((byte)(RegMap["USER_SCR_1"].Address + 1 | 0x80));
            MOSIClear.Add(0);

            byte[] MISO;

            FX3.BitBangSpiConfig = new BitBangSpiConfig(true);
            FX3.SetBitBangSpiFreq(900000);
            FX3.BitBangSpiConfig.CSLagTicks = 0;
            FX3.BitBangSpiConfig.CSLeadTicks = 0;

            while (goodStall)
            {
                Console.WriteLine("Testing stall time: " + stall.ToString() + "us");
                FX3.SetBitBangStallTime(stall);

                /* Write to 0 */

                for(int trial = 0; trial < 64; trial++)
                {
                    /* Clear register */
                    FX3.BitBangSpi(16, 2, MOSIClear.ToArray(), 1000);
                    MISO = FX3.BitBangSpi(16, 2, MOSIRead.ToArray(), 1000);
                    if (MISO[2] != 0)
                        goodStall = false;
                    if (MISO[3] != 0)
                        goodStall = false;
                    /* Set register */
                    FX3.BitBangSpi(16, 2, MOSI.ToArray(), 1000);
                    MISO = FX3.BitBangSpi(16, 2, MOSIRead.ToArray(), 1000);
                    if (MISO[2] != 0xAA)
                        goodStall = false;
                    if (MISO[3] != 0x55)
                        goodStall = false;
                }

                if (goodStall)
                    stall -= 0.2;
                if (stall < 0.5)
                    return stall;
            }
            return stall;
        }
    }
}
