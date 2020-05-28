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
    class RobustnessTests : TestBase
    {
        [Test]
        public void InvalidSpiTrafficTest()
        {
            InitializeTestCase();

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
