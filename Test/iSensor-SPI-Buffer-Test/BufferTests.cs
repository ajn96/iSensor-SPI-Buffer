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
using System.Diagnostics;

namespace iSensor_SPI_Buffer_Test
{
    class BufferTests : TestBase
    {
        [Test]
        public void BurstTest()
        {
            /* Enable user spi burst */
            WriteUnsigned("USER_SPI_CONFIG", 0x8007, true);

        }

        [Test]
        public void BufferRetrieveTimeTest()
        {
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
        }

        [Test]
        public void BufStartTestTest()
        {
        }


        [Test]
        public void BufferTimestampTest()
        {
            const double DR_FREQ = 2000;
            double timeMs;
            List<RegClass> ReadRegs = new List<RegClass>();
            uint[] data;
            uint count;
            double lastPrintTime;
            ReadRegs.Add(RegMap["BUF_RETRIEVE"]);
            ReadRegs.Add(RegMap["STATUS_1"]);
            ReadRegs.Add(RegMap["BUF_UTC_TIME_LWR"]);
            ReadRegs.Add(RegMap["BUF_TIMESTAMP_LWR"]);
            Stopwatch timer = new Stopwatch();
            //Want DR on DIO1, PPS on DIO2
            FX3.StartPWM(1, 0.5, FX3.DIO2);
            FX3.StartPWM(DR_FREQ, 0.5, FX3.DIO1);
            WriteUnsigned("DIO_INPUT_CONFIG", 0x201);
            WriteUnsigned("DIO_OUTPUT_CONFIG", 0x1);
            WriteUnsigned("IMU_SPI_CONFIG", 0x105);
            WriteUnsigned("BUF_LEN", 8);
            WriteUnsigned("BUF_CONFIG", 0x2);
            WriteUnsigned("USER_COMMAND", 1u << COMMAND_PPS_START, false);
            /* Wait 1 second for alignment */
            System.Threading.Thread.Sleep(1000);
            timer.Start();
            timeMs = 0;
            /* Start capture */
            WriteUnsigned("BUF_CNT_1", 0, false);
            /* Run for 1 hour */
            lastPrintTime = 0;
            while (timer.ElapsedMilliseconds < (60 * 60 * 1000))
            {
                System.Threading.Thread.Sleep(10);
                count = ReadUnsigned("BUF_CNT_1");
                data = Dut.ReadUnsigned(ReadRegs, 1, count);
                //Console.WriteLine("Retrieved " + count.ToString() + " samples");
                timeMs = ProcessTimestamps(timeMs, DR_FREQ, data);
                if(timeMs > (lastPrintTime + 1000))
                {
                    Console.WriteLine(timeMs.ToString());
                    lastPrintTime = timeMs;
                }
            }
        }

        private double ProcessTimestamps(double lastTime, double drFreq, uint[] regdata)
        {
            List<double> times = new List<double>();
            double timeMs;
            for(int i = 0; i < regdata.Length; i+= 4)
            {
                /* Grab second value */
                timeMs = regdata[i + 2] * 1000.0;
                /* Add in us */
                timeMs += (regdata[i + 3] / 1000.0);
                times.Add(timeMs);
                //Console.WriteLine(timeMs.ToString());
            }
            /* Handle init condition */
            if(lastTime == 0)
            {
                lastTime = times[0] - (1000 / drFreq);
            }
            double expectedTime = lastTime + (1000 / drFreq);
            for (int i = 0; i < times.Count; i++)
            {
                try
                {
                    Assert.AreEqual(expectedTime, times[i], 0.2, "Invalid timestamp");
                }
                catch(Exception ex)
                {
                    Console.WriteLine(ex.Message);
                    foreach(double time in times)
                    {
                        Console.WriteLine(time.ToString());
                    }
                    Assert.True(false);
                }
                expectedTime = times[i] + (1000 / drFreq);
            }
            return times.Last();
        }

        [Test]
        public void BufferEnqueueDequeueTest()
        {
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

            for(int i = 0; i < 10; i++)
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
                    goodFreq = ValidateBufferData(BufferEntry.ParseBufferData(buf, ReadRegs), freq);
                    buffersRead += count;
                }
                if (goodFreq)
                    freq += 50;
            }
        }

        [Test]
        public void BufferCountTest()
        {
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
            /* Mode, overflow behavior, SPI word size */
        }

        [Test]
        public void BufferMaxDataRateTest()
        {
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
            WriteUnsigned("IMU_SPI_CONFIG", 0x102, true);
            CheckDUTConnection();
            freq = FindMaxDrFreq(2, 200.0, 400.0);
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
            bool goodFreq = true;
            int numSamples = 100;

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
                goodFreq = ValidateBufferData(BufferEntry.ParseBufferData(bufData, ReadDataRegs), freq);

                if (goodFreq)
                    freq += freqStep;
            }
            return freq;
        }



    }
}
