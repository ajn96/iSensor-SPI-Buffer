﻿using System;
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
    class FunctionalTimingTests : TestBase
    {
        /* Number of timing trials */
        const int numTrials = 5;

        [Test]
        public void GenerateTimingReport()
        {
            InitializeTestCase();

            StreamWriter writer;

            string outPath = Path.GetFullPath(Path.Combine(System.AppDomain.CurrentDomain.BaseDirectory, @"..\..\..\..\"));
            outPath = Path.Combine(outPath, "TIMING.MD");
            writer = new StreamWriter(outPath, false);
            FX3.SetPinResistorSetting(FX3.DIO1, FX3PinResistorSetting.PullDown);

            /* Set watermark config to 0 (pin goes high as soon as firmware boots) */
            WriteUnsigned("WATERMARK_INT_CONFIG", 0x0, true);
            WriteUnsigned("DIO_OUTPUT_CONFIG", 0x10, true);

            /* Flash update */
            WriteUnsigned("USER_COMMAND", 1u << COMMAND_FLASH_UPDATE, false);
            System.Threading.Thread.Sleep(500);

            /* Insert header to out file */
            writer.WriteLine("# iSensor-SPI-Buffer Timing Characterization");
            writer.WriteLine("");
            writer.WriteLine("This document is automatically generated using timing measurements performed by an EVAL-ADIS-FX3, using the NUnit test framework");
            writer.WriteLine("");
            writer.WriteLine("Test Date: " + String.Format("{0:s}", DateTime.Now));
            writer.WriteLine("");
            writer.WriteLine("Trials per timing measurement: " + numTrials.ToString());
            writer.WriteLine("");
            writer.WriteLine("Firmware Revision: " + ReadUnsigned("FW_REV").ToString("X4"));
            writer.WriteLine("");
            writer.WriteLine("Firmware Date (YYYY/MM/DD): " + ReadUnsigned("FW_YEAR").ToString("X4") + "/" + (ReadUnsigned("FW_DAY_MONTH") & 0xFF).ToString("X2") + "/" + (ReadUnsigned("FW_DAY_MONTH") >> 8).ToString("X2"));
            writer.WriteLine("");
            writer.WriteLine("Test source code is available [here](Test/iSensor-SPI-Buffer-Test/FunctionalTimingTests.cs)");

            writer.WriteLine("## Reset Times");
            writer.WriteLine();
            writer.WriteLine("|Parameter|Min|Avg|Max|Units|");
            writer.WriteLine("| --- | --- | --- | --- | --- |");
            writer.WriteLine("|Power Cycle Reset||" + PowerCycleTime().ToString("f2") + "||ms|");
            writer.WriteLine("|Software Reset Time||" + SoftResetTime().ToString("f2") + "||ms|");

            writer.WriteLine("## SPI Timings");
            writer.WriteLine();
            writer.WriteLine("|Parameter|Min|Avg|Max|Units|");
            writer.WriteLine("| --- | --- | --- | --- | --- |");
            writer.WriteLine("|Clock Frequency|||" + MaxSclkFreq().ToString() + "|Hz|");
            writer.WriteLine("|Read Stall|" + FindReadStallTime().ToString("f2") + "|||us|");
            writer.WriteLine("|Write Stall|" + FindWriteStallTime().ToString("f2") + "|||us|");

            FX3.RestoreHardwareSpi();
            FX3.SclkFrequency = 10000000;
            ResetDUT();

            writer.WriteLine("## Functional Command Timings");
            writer.WriteLine();
            writer.WriteLine("|Parameter|Min|Avg|Max|Units|");
            writer.WriteLine("| --- | --- | --- | --- | --- |");
            writer.WriteLine("|Clear Buffer||" + ClearBufTime().ToString("f2") + "||us|");
            writer.WriteLine("|Clear Fault||" + ClearFaultTime().ToString("f2") + "||ms|");
            writer.WriteLine("|Factory Reset||" + FactoryResetTime().ToString("f2") + "||ms|");
            writer.WriteLine("|Flash Update||" + FlashUpdateTime().ToString("f2") + "||ms|");
            writer.WriteLine("|PPS Enable||" + PPSEnableTime().ToString("f2") + "||us|");
            writer.WriteLine("|PPS Disable||" + PPSDisableTime().ToString("f2") + "||us|");
            writer.WriteLine("|IMU Reset||" + IMUResetTime().ToString("f2") + "||us|");
            writer.WriteLine();
            writer.Close();
        }

        private double PowerCycleTime()
        {
            double time = 0;
            double avgTime = 0;

            FX3.DutSupplyMode = DutVoltage.Off;
            System.Threading.Thread.Sleep(10);
            FX3.DutSupplyMode = DutVoltage.On3_3Volts;
            System.Threading.Thread.Sleep(100);

            for (int i = 0; i < numTrials; i++)
            {
                FX3.DutSupplyMode = DutVoltage.Off;
                System.Threading.Thread.Sleep(10);
                FX3.DutSupplyMode = DutVoltage.On3_3Volts;
                time = FX3.PulseWait(FX3.DIO1, 1, 0, 100);
                Console.WriteLine("Reset time: " + time.ToString() + "ms");
                avgTime += time;
                System.Threading.Thread.Sleep(10);
            }
            avgTime /= numTrials;
            return avgTime;
        }

        private double SoftResetTime()
        {
            double time = 0;
            double avgTime = 0;

            for (int i = 0; i < numTrials; i++)
            {
                WriteUnsigned("USER_COMMAND", 1u << COMMAND_RESET, false);
                time = FX3.PulseWait(FX3.DIO1, 1, 0, 100);
                Console.WriteLine("Reset time: " + time.ToString() + "ms");
                avgTime += time;
                System.Threading.Thread.Sleep(10);
            }
            avgTime /= numTrials;
            return avgTime;
        }

        private double ClearBufTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8) | (1u << COMMAND_CLEAR_BUFFER));
                time = 1000 * FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("Clear buf time: " + time.ToString() + "us");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private double ClearFaultTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8) | (1u << COMMAND_CLEAR_FAULT));
                time = FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("Clear fault time: " + time.ToString() + "ms");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private double FactoryResetTime()
        {
            double time = 0;
            double avgTime = 0;
            Stopwatch timer = new Stopwatch();

            for (int i = 0; i < numTrials; i++)
            {
                timer.Start();
                WriteUnsigned("USER_COMMAND", 1u << COMMAND_FACTORY_RESET, false);
                while (ReadUnsigned("WATERMARK_INT_CONFIG") != RegMap["WATERMARK_INT_CONFIG"].DefaultValue) ;
                timer.Stop();
                time = timer.ElapsedTicks / (Stopwatch.Frequency / 1000);
                Console.WriteLine("Factory Reset time: " + time.ToString() + "ms");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            ResetDUT();

            return avgTime;
        }

        private double FlashUpdateTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8) | (1u << COMMAND_FLASH_UPDATE));
                time =  FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("Flash update time: " + time.ToString() + "ms");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private double PPSEnableTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));
            WriteUnsigned("DIO_INPUT_CONFIG", 0x411, true);

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8) | (1u << COMMAND_PPS_START));
                time = 1000 * FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("PPS enable time: " + time.ToString() + "us");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private double PPSDisableTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8) | (1u << COMMAND_PPS_STOP));
                time = 1000 * FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("PPS disable time: " + time.ToString() + "us");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private double IMUResetTime()
        {
            double time = 0;
            double avgTime = 0;
            List<byte> cmdData = new List<byte>();
            cmdData.Add(0x40);
            cmdData.Add((byte)(RegMap["USER_COMMAND"].Address + 1u | 0x80));

            for (int i = 0; i < numTrials; i++)
            {
                FX3.Transfer(0x8000 | (RegMap["USER_COMMAND"].Address << 8));
                time = 1000 * FX3.MeasureBusyPulse(cmdData.ToArray(), FX3.DIO1, 0, 2000);
                Console.WriteLine("IMU Reset time: " + time.ToString() + "us");
                avgTime += time;
                System.Threading.Thread.Sleep(5);
            }
            avgTime /= numTrials;

            return avgTime;
        }

        private int MaxSclkFreq()
        {
            int freq = 10000000;
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
            FX3.SclkFrequency = 10000000;
            ResetDUT();
            return freq;
        }
    }
}
