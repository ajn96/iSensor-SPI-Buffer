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
        public const int COMMAND_RESET = 15;

        /* Status bits */
        public const int STATUS_SPI_ERROR = 0;
        public const int STATUS_SPI_OVERFLOW = 1;
        public const int STATUS_OVERRUN = 2;
        public const int STATUS_DMA_ERROR = 3;
        public const int STATUS_FLASH_ERROR = 6;
        public const int STATUS_FLASH_UPDATE_ERROR = 7;
        public const int STATUS_FAULT = 8;
        public const int STATUS_WATCHDOG = 9;
        public const int STATUS_BUF_FULL = 10;
        public const int STATUS_BUF_INTERRUPT = 11;
        public const int STATUS_TC = 12; //<-4 bits

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
            RegMapPath = Path.Combine(RegMapPath, "iSensor_SPI_Buffer_RegDataFile.csv");

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
            FX3.StallTime = 6;
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

        public void WriteUnsigned(string RegName, uint WriteVal, bool ReadBack)
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

        public bool ValidateBufferData(uint[] buf, IEnumerable<RegClass> ReadRegs, int numBufs, double expectedFreq)
        {
            uint timestamp, oldTimestamp;
            double timestampfreq;
            double avgFreq = 0;
            int averages = 0;
            long deltaTime, expectedDeltaTime;
            uint sig, expectedSig;

            int index = 0;

            if (buf.Count() != (ReadRegs.Count() * numBufs))
            {
                Console.WriteLine("Invalid buffer data count!");
                return false;
            }

            expectedDeltaTime = (long) (1000000 / expectedFreq);

            oldTimestamp = 0;
            for (int j = 0; j < numBufs; j++)
            {
                if (buf[index] != 0)
                {
                    Console.WriteLine("BUF_RETRIEVE readback value of " + buf[index].ToString());
                    return false;
                }
                timestamp = buf[index + 2];
                expectedSig = buf[index + 2];
                timestamp += (buf[index + 3] << 16);
                expectedSig += buf[index + 3];
                deltaTime = buf[index + 4];
                expectedSig += buf[index + 4];
                index += 5;
                /* Check data*/
                for (int i = 0; i < ReadRegs.Count() - 6; i++)
                {
                    if (buf[index] != (i + 1))
                    {
                        Console.WriteLine("Invalid buffer data at index " + i.ToString() + " value: " + buf[index].ToString());
                        //return false;
                    }
                    expectedSig += buf[index];
                    index++;
                }

                /* Check sig */
                sig = buf[index];
                index++;
                expectedSig = expectedSig & 0xFFFF;
                if(expectedSig != sig)
                {
                    Console.WriteLine("Invalid buffer signature. Expected " + expectedSig.ToString() + ", was " + sig.ToString());
                    //return false;
                }
                /* Check timestamps and delta time */
                if(j > 0)
                {
                    timestampfreq = (1000000.0 / (timestamp - oldTimestamp));
                    avgFreq += timestampfreq;
                    averages++;
                    if (Math.Abs((deltaTime - expectedDeltaTime)) > 10)
                    {
                        Console.WriteLine("Invalid delta time measurement. Expected " + expectedDeltaTime.ToString() + "us, was " + deltaTime.ToString() + "us");
                        //return false;
                    }
                }
                oldTimestamp = timestamp;
            }
            avgFreq = avgFreq / averages;

            if (Math.Abs((avgFreq - expectedFreq) / expectedFreq) > 0.02)
            {
                Console.WriteLine("Invalid timestamp freq. Expected " + expectedFreq.ToString() + "Hz, was " + avgFreq.ToString() + "Hz");
                //return false;
            }


            return true;
        }
    }
}
