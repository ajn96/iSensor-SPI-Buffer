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
    class BufferTests : TestBase
    {
        [Test]
        public void BufferRetrieveTimeTest()
        {
            InitializeTestCase();

            /* Set writedata regs */
            uint index = 1;
            foreach (RegClass reg in WriteDataRegs)
            {
                Dut.WriteUnsigned(reg, index);
                index++;
            }

            uint stall;
            for (uint bufLen = 2; bufLen <= 64; bufLen += 2)
            {
                Console.WriteLine("Testing buffer length: " + bufLen.ToString() + " bytes");
                stall = FindBufRetrieveTime(bufLen);
                Console.WriteLine("Minimum stall time to get valid buffers: " + stall.ToString() + "us");
                Assert.LessOrEqual(stall, 10, "ERROR: Required stall > 10us!");
                CheckDUTConnection();
            }
        }

        public uint FindBufRetrieveTime(uint bufLen)
        {
            bool goodStall = true;
            ushort stall = 20;

            uint timeStamp, oldTimestamp;

            /* Set buffer length */
            WriteUnsigned("BUF_LEN", bufLen, true);

            /* Put DUT onto buffer page */
            ReadUnsigned("BUF_TIMESTAMP_LWR");

            uint[] buf;

            oldTimestamp = 0;

            while(goodStall)
            {
                FX3.StallTime = stall;
                for(int trial = 0; trial < 4; trial++)
                {
                    WriteUnsigned("BUF_CNT_1", 0, false);
                    System.Threading.Thread.Sleep(1);
                    ReadUnsigned("BUF_RETRIEVE");
                    System.Threading.Thread.Sleep(1);
                    FX3.SetPin(FX3.DIO1, 0);
                    FX3.SetPin(FX3.DIO1, 1);
                    FX3.SetPin(FX3.DIO1, 0);
                    System.Threading.Thread.Sleep(10);
                    buf = Dut.ReadUnsigned(ReadDataRegs);

                    /* Check timestamp (must be greater than old timestamp) */
                    timeStamp = buf[1] + (buf[2] << 16);
                    if (timeStamp <= oldTimestamp)
                        goodStall = false;
                    oldTimestamp = timeStamp;
                    if (buf[0] != 0)
                        goodStall = false;
                    for (int i = 3; i < buf.Count(); i++)
                    {
                        if ((i - 3) < (bufLen / 2))
                        {
                            if (buf[i] != (i - 2))
                                goodStall = false;
                        }
                        else
                        {
                            if (buf[i] != 0)
                                goodStall = false;
                        }
                    }
                }
                if (goodStall)
                    stall -= 2;

                if (stall < 5)
                    return stall;
            }
            return stall;
        }

        [Test]
        public void BufferLenTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void BufferTimestampTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void BufferEnqueueDequeueTest()
        {
            InitializeTestCase();

            double freq;
            bool goodFreq = true;
            uint index;
            uint buffersRead;
            uint count;
            uint[] buf;

            List<RegClass> ReadRegs = new List<RegClass>();

            /* Set writedata regs */
            index = 1;
            foreach (RegClass reg in WriteDataRegs)
            {
                Dut.WriteUnsigned(reg, index);
                index++;
            }

            /* Set buffer length */
            WriteUnsigned("BUF_LEN", 20, true);

            for(int i = 0; i < 4; i++)
            {
                ReadRegs.Add(ReadDataRegs[i]);
            }

            WriteUnsigned("IMU_SPI_CONFIG", 0x810, true);

            freq = 50;
            while(goodFreq)
            {
                Console.WriteLine("Testing DR freq: " + freq.ToString() + "Hz");
                buffersRead = 0;
                FX3.StartPWM(freq, 0.5, FX3.DIO1);
                WriteUnsigned("BUF_CNT_1", 0, false);
                /* get 3 seconds of data per sample freq */
                while ((buffersRead < 3 * freq) && goodFreq)
                {
                    /* Want to service every 100 ms (or 50 buffers, whichever is more) */
                    System.Threading.Thread.Sleep((int) Math.Max(100, 50000 / freq));
                    count = ReadUnsigned("BUF_CNT_1");
                    count -= 2;
                    Console.WriteLine("Reading " + count.ToString() + " buffer entries...");
                    buf = Dut.ReadUnsigned(10, ReadRegs, 1, count);
                    goodFreq = ValidateBufferData(buf, ReadRegs, (int) count, freq);
                    buffersRead += count;
                }
                if (goodFreq)
                    freq += 50;
            }
        }

        [Test]
        public void BufferCountTest()
        {
            InitializeTestCase();

            uint maxCount, count;

            /* Flush buffer */
            WriteUnsigned("USER_COMMAND", 1 << COMMAND_CLEAR_BUFFER, false);
            System.Threading.Thread.Sleep(100);
            Assert.AreEqual(0, ReadUnsigned("BUF_CNT"), "ERROR: Expected buf count of 0");
            WriteUnsigned("BUF_LEN", 32, true);
            System.Threading.Thread.Sleep(10);
            maxCount = ReadUnsigned("BUF_MAX_CNT");
            FX3.SetPin(FX3.DIO1, 0);
            /* Put DUT on correct page */
            ReadUnsigned("STATUS_1");
            Console.WriteLine("Count: " + ReadUnsigned("BUF_CNT_1"));

            for(uint i = 0; i < maxCount; i++)
            {
                FX3.SetPin(FX3.DIO1, 1);
                FX3.SetPin(FX3.DIO1, 0);
                System.Threading.Thread.Sleep(5);
                count = ReadUnsigned("BUF_CNT_1");
                Console.WriteLine("Buffer count: " + count.ToString());
                Assert.AreEqual(i + 1, count, "ERROR: Expected BUF_CNT to increment");
            }

            for(int trial = 0; trial < 10; trial++)
            {
                FX3.SetPin(FX3.DIO1, 1);
                FX3.SetPin(FX3.DIO1, 0);
                System.Threading.Thread.Sleep(5);
                count = ReadUnsigned("BUF_CNT_1");
                Console.WriteLine("Buffer count: " + count.ToString());
                Assert.AreEqual(maxCount, count, "ERROR: Expected BUF_CNT not to increment when full");
            }
        }

        [Test]
        public void BufferSettingsTest()
        {
            InitializeTestCase();

            /* Mode, overflow behavior, SPI word size */
        }

        [Test]
        public void BufferMaxDataRateTest()
        {
            InitializeTestCase();

            uint index;

            double freq;

            /* Set writedata regs */
            index = 1;
            foreach(RegClass reg in WriteDataRegs)
            {
                Dut.WriteUnsigned(reg, index);
                index++;
            }

            /* Fast IMU SPI config (9MHz SCLK, 2us stall) */
            WriteUnsigned("IMU_SPI_CONFIG", 0x202, true);

            Console.WriteLine("Testing buffer max data rate with no user SPI traffic and smallest buffer size...");
            WriteUnsigned("BUF_LEN", 2, true);
            CheckDUTConnection();
            freq = FindMaxDrFreq(2, 200.0, 200.0);
            Assert.GreaterOrEqual(freq, 5000, "ERROR: Max supported DR freq must be at least 5KHz...");

            Console.WriteLine("Testing buffer max data rate with no user SPI traffic and largest buffer size...");
            WriteUnsigned("BUF_LEN", 64, true);
            freq = FindMaxDrFreq(64, 50.0, 50.0);
            /* 64 bytes of SPI traffic + 32 2us stall times -> ~128us best case without burst read */
            Assert.GreaterOrEqual(freq, 1000, "ERROR: Max supported DR freq must be at least 1KHz...");
        }

        private double FindMaxDrFreq(int bufSize, double startFreq, double freqStep)
        {
            double freq = startFreq;
            double timestampfreq, avgFreq;
            bool goodFreq = true;
            int numSamples = 100;
            int numAverages;

            uint[] bufData;

            while(goodFreq)
            {
                Console.WriteLine("Testing data ready freq of " + freq.ToString() + "Hz");
                /* Flush buffer */
                WriteUnsigned("USER_COMMAND", 1 << COMMAND_CLEAR_BUFFER, false);
                System.Threading.Thread.Sleep(10);
                /* Put DUT on correct page */
                ReadUnsigned("BUF_TIMESTAMP_LWR");
                FX3.StartPWM(freq, 0.5, FX3.DIO1);
                /* Wait for numSamples samples to be enqueued */
                System.Threading.Thread.Sleep((int) (1500 * numSamples / freq));
                /* Stop clock */
                FX3.StopPWM(FX3.DIO1);
                Console.WriteLine("Status: 0x" + ReadUnsigned("STATUS").ToString("X4"));
                if(ReadUnsigned("BUF_CNT") < numSamples)
                {
                    Console.WriteLine("ERROR: Less samples enqueued than expected: " + ReadUnsigned("BUF_CNT").ToString());
                    goodFreq = false;
                }
                bufData = Dut.ReadUnsigned(10, ReadDataRegs, 1, (uint) numSamples);
                long timeStamp, oldTimestamp;
                oldTimestamp = bufData[1];
                oldTimestamp += (bufData[2] << 16);
                int index = ReadDataRegs.Count;
                avgFreq = 0;
                numAverages = 0;
                for(int j = 1; j < numSamples; j++)
                {
                    if (bufData[index] != 0)
                        goodFreq = false;
                    timeStamp = bufData[index + 1];
                    timeStamp += (bufData[index + 2] << 16);
                    index += 3;
                    for (int i = 3; i < ReadDataRegs.Count; i++)
                    {
                        if((i - 3) < (bufSize / 2))
                        {
                            if (bufData[index] != (i - 2))
                            {
                                goodFreq = false;
                            }
                        }
                        else
                        {
                            if (bufData[index] != 0)
                            {
                                goodFreq = false;
                            }
                        }
                        index++;
                    }
                    //Console.WriteLine("Timestamp: " + timeStamp.ToString());
                    timestampfreq = (1000000.0 / (timeStamp - oldTimestamp));
                    avgFreq += timestampfreq;
                    numAverages++;
                    oldTimestamp = timeStamp;
                }

                avgFreq = avgFreq / numAverages;
                Console.WriteLine("Average sample freq (from timestamp): " + avgFreq.ToString("f2") + "Hz");
                /* If average timestamp error of more than 2% then is bad */
                if (Math.Abs((freq - avgFreq) / freq) > 0.02)
                    goodFreq = false;

                if (goodFreq)
                    freq += freqStep;
            }
            return freq;
        }



    }
}
