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
    public class RegisterInterfaceTests : TestBase
    {
        [Test]
        public void SPIWordLenTest()
        {
            CheckDUTConnection();
            FX3.BitBangSpiConfig = new BitBangSpiConfig(true);
            FX3.SetBitBangSpiFreq(1500000);

            FX3.BitBangSpi(8, 4, new byte[] { 110, 0, 0, 0}, 1000);

            FX3.BitBangSpi(16, 2, new byte[] { 110, 0, 0, 0 }, 1000);
        }

        [Test]
        public void ReadOnlyRegsTest()
        {
        }

        [Test]
        public void ReservedBitsTest()
        {
        }

        [Test]
        public void PageRegisterTest()
        {
        }

        [Test]
        public void ScratchRegsTest()
        {
        }

        [Test]
        public void SerialNumberRegsTest()
        {
        }

    }
}
