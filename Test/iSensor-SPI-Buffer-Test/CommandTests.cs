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
    class CommandTests : TestBase
    {
        [Test]
        public void SoftwareResetTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void FlashUpdateTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void FactoryResetTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void ClearBufferTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void ClearFaultTest()
        {
            InitializeTestCase();
        }

    }
}
