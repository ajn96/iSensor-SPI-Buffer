using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NUnit.Framework;
using FX3Api;
using adisInterface;
using RegMapClasses;
using System.IO;
using System.Reflection;

namespace iSensor_SPI_Buffer_Test
{
    public abstract class TestBase
    {
        /* FX3 object */
        public FX3Connection FX3;

        public adbfInterface Dut;

        public RegMapCollection RegMap;

        public List<RegClass> WriteDataRegs;
        public List<RegClass> ReadDataRegs;

        /* Command bits */
        public const int COMMAND_CLEAR_BUFFER = 0;
        public const int COMMAND_CLEAR_FAULT = 1;
        public const int COMMAND_FACTORY_RESET = 2;
        public const int COMMAND_FLASH_UPDATE = 3;
        public const int COMMAND_PPS_START = 4;
        public const int COMMAND_PPS_STOP = 5;
        public const int COMMAND_RESET = 15;

        /* Status bits */
        public const int STATUS_BUF_WATERMARK = 0;
        public const int STATUS_BUF_FULL = 1;
        public const int STATUS_SPI_ERROR = 2;
        public const int STATUS_SPI_OVERFLOW = 3;
        public const int STATUS_OVERRUN = 4;
        public const int STATUS_DMA_ERROR = 5;
        public const int STATUS_PPS_UNLOCK = 6;
        public const int STATUS_FLASH_ERROR = 12;
        public const int STATUS_FLASH_UPDATE_ERROR = 13;
        public const int STATUS_FAULT = 14;
        public const int STATUS_WATCHDOG = 15;

        [TestFixtureSetUp(), Timeout(5000)]
        public void TestFixtureSetup()
        {
            string ResourcePath, RegMapPath, exePath;

            Console.WriteLine("Starting test fixture setup...");

            exePath = System.AppDomain.CurrentDomain.BaseDirectory;
            Console.WriteLine("Tests executing from " + exePath);

            ResourcePath = Path.GetFullPath(Path.Combine(exePath, @"..\..\..\"));
            ResourcePath = Path.Combine(ResourcePath, "Resources");
            Assert.True(Directory.Exists(ResourcePath), "ERROR: Resource path not found. Build process may have failed!");
            FX3 = new FX3Connection(ResourcePath, ResourcePath, ResourcePath, DeviceType.IMU);
            Connect();
            FX3.DutSupplyMode = DutVoltage.On3_3Volts;
            Console.WriteLine("FX3 Connected! FX3 API Info: " + Environment.NewLine + FX3.GetFX3ApiInfo.ToString());
            Console.WriteLine("FX3 Board Info: " + Environment.NewLine + FX3.ActiveFX3.ToString());

            /* Load regmap and set up DUT interface */
            RegMapPath = Path.GetFullPath(Path.Combine(exePath, @"..\..\..\..\"));
            RegMapPath = Path.Combine(RegMapPath, @"Regmaps\iSensor_SPI_Buffer_RegDataFile.csv");

            RegMap = new RegMapCollection();
            RegMap.ReadFromCSV(RegMapPath);
            Dut = new adbfInterface(FX3, null);

            Console.WriteLine("Test fixture setup complete");

            ReadDataRegs = new List<RegClass>();
            WriteDataRegs = new List<RegClass>();
            foreach (RegClass reg in RegMap)
            {
                if ((reg.Page == 255) && (reg.Address >= 6))
                    ReadDataRegs.Add(reg);
                if ((reg.Page == 254) && (reg.Address != 0) && (reg.Address < 124))
                    WriteDataRegs.Add(reg);
            }
        }

        [TestFixtureTearDown()]
        public void Teardown()
        {
            FX3.Disconnect();
            Console.WriteLine("Test fixture tear down complete");
        }

        public void Connect()
        {
            /* Return if board already connected */
            if (FX3.ActiveFX3 != null)
                return;
            FX3.WaitForBoard(5);
            if (FX3.AvailableFX3s.Count > 0)
            {
                FX3.Connect(FX3.AvailableFX3s[0]);
            }
            else if (FX3.BusyFX3s.Count > 0)
            {
                FX3.ResetAllFX3s();
                FX3.WaitForBoard(5);
                Connect();
            }
            else
            {
                Assert.True(false, "ERROR: No FX3 board connected!");
            }
        }

        private void RestoreFX3State()
        {
            FX3.RestoreHardwareSpi();
            foreach (PropertyInfo prop in FX3.GetType().GetProperties())
            {
                if (prop.PropertyType == typeof(AdisApi.IPinObject))
                {
                    /* Disable PWM if running */
                    if (FX3.isPWMPin((AdisApi.IPinObject)prop.GetValue(FX3)))
                        FX3.StopPWM((AdisApi.IPinObject)prop.GetValue(FX3));
                }
            }
            FX3.SclkFrequency = 10000000;
            FX3.StallTime = 10;
            FX3.DrActive = false;
        }

        public void InitializeTestCase()
        {
            /* Restore FX3 state */
            RestoreFX3State();

            /* Reset DUT */
            ResetDUT();

            /* Read STATUS */
            uint status = ReadUnsigned("STATUS");
            Console.WriteLine("Status: 0x" + status.ToString("X4"));

            /* Check connection */
            CheckDUTConnection();

            RestoreDefaultValues();
            Console.WriteLine("Default values restored!");

            Console.WriteLine("Test initialization complete...");
        }

        public void CheckDUTConnection()
        {
            uint initialVal =  Dut.ReadUnsigned(RegMap["USER_SCR_1"]);
            WriteUnsigned("USER_SCR_1", initialVal ^ 0xFFFFU, true);
            WriteUnsigned("USER_SCR_1", initialVal, true);
        }

        public void RestoreDefaultValues()
        {
            foreach(RegClass Reg in RegMap)
            {
                if(Reg.DefaultValue != null)
                {
                    WriteUnsigned(Reg.Label, (uint) Reg.DefaultValue, true);
                }
            }
        }

        public void ResetDUT()
        {
            if(FX3.ActiveFX3.BoardType != FX3BoardType.CypressFX3Board)
            {
                Console.WriteLine("Starting power cycle reset...");
                FX3.DutSupplyMode = DutVoltage.Off;
                System.Threading.Thread.Sleep(100);
                FX3.DutSupplyMode = DutVoltage.On3_3Volts;
            }
            else
            {
                Console.WriteLine("Starting software reset...");
                WriteUnsigned("USER_COMMAND", 1U << COMMAND_RESET, false);
            }
            System.Threading.Thread.Sleep(500);
            Console.WriteLine("Reset finished!");
        }

        public uint ReadUnsigned(string RegName)
        {
            RegClass strippedReg = new RegClass();
            RegClass baseReg = RegMap[RegName];
            strippedReg.Address = baseReg.Address;
            strippedReg.Page = baseReg.Page;
            /* Limit to 2 byte regs */
            strippedReg.NumBytes = 2;
            strippedReg.ReadLen = baseReg.ReadLen;
            return Dut.ReadUnsigned(strippedReg);
        }

        public void WriteUnsigned(string RegName, uint WriteVal, bool ReadBack = true)
        {
            RegClass strippedReg = new RegClass();
            RegClass baseReg = RegMap[RegName];
            strippedReg.Address = baseReg.Address;
            strippedReg.Page = baseReg.Page;
            /* Limit to 2 byte regs */
            strippedReg.NumBytes = 2;
            strippedReg.ReadLen = baseReg.ReadLen;
            Dut.WriteUnsigned(strippedReg, WriteVal);
            if(ReadBack)
                Assert.AreEqual(WriteVal, Dut.ReadUnsigned(strippedReg), "ERROR: " + RegName + " read back after write failed!");
        }

        public bool ValidateBufferData(List<BufferEntry> Data, double expectedFreq)
        {
            double timestampfreq;
            double avgFreq = 0;
            int averages, index;
            long expectedDeltaTime;
            uint expectedSig;

            if(Data.Count == 1)
            {
                return (Data[0].Signature == Data[0].GetExpectedSignature());
            }

            averages = 0;
            for(index = 1; index < Data.Count; index++)
            {
                if (Data[index - 1].Timestamp > Data[index].Timestamp)
                    expectedDeltaTime = Data[index].DeltaTime;
                else
                    expectedDeltaTime = Data[index].Timestamp - Data[index - 1].Timestamp;
                if(expectedDeltaTime != Data[index].DeltaTime)
                {
                    Console.WriteLine("Invalid delta time measurement in buffer " + index.ToString() + ". Expected " + expectedDeltaTime.ToString() + ", was " + Data[index].DeltaTime.ToString());
                    return false;
                }
                expectedSig = Data[index].GetExpectedSignature();
                if(Data[index].Signature != expectedSig)
                {
                    Console.WriteLine("Invalid buffer signature in buffer " + index.ToString() + ". Expected " + expectedSig.ToString() + ", was " + Data[index].Signature.ToString());
                    return false;
                }
                for(int i = 0; i < Data[index].Data.Count(); i++)
                {
                    if(Data[index].Data[i] != (i + 1))
                    {
                        Console.WriteLine("Invalid buffer contents! Expected " + (i + 1).ToString() + ", was " + Data[index].Data[i].ToString());
                        return false;
                    }
                }
                timestampfreq = (1000000.0 / (Data[index].Timestamp - Data[index - 1].Timestamp));
                avgFreq += timestampfreq;
                averages++;
            }

            avgFreq = avgFreq / averages;

            if (Math.Abs((avgFreq - expectedFreq) / expectedFreq) > 0.02)
            {
                Console.WriteLine("Invalid average timestamp freq. Expected " + expectedFreq.ToString() + "Hz, was " + avgFreq.ToString() + "Hz");
                return false;
            }

            return true;
        }

        public double FindReadStallTime()
        {
            double stall = 10;
            bool goodStall = true;

            WriteUnsigned("USER_SCR_1", 0x55AA, true);

            List<byte> MOSI = new List<byte>();
            for (int i = 0; i < 128; i++)
            {
                MOSI.Add((byte)RegMap["USER_SCR_1"].Address);
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
                for (int i = 2; i < MISO.Count() - 1; i += 2)
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

        public double FindWriteStallTime()
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

                for (int trial = 0; trial < 64; trial++)
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
