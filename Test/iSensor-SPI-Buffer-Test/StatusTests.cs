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
    class StatusTests : TestBase
    {
        [Test]
        public void StatusClearTest()
        {
            InitializeTestCase();
        }

        [Test]
        public void StatusTCTest()
        {
            InitializeTestCase();

            uint oldTc, expectedTc;

            oldTc = (ReadUnsigned("STATUS") >> 12);
            for (int trial = 0; trial < 32; trial++)
            {
                expectedTc = (oldTc + 3) & 0xF;
                oldTc = (ReadUnsigned("STATUS") >> 12);
                Console.WriteLine("Status TC: " + oldTc.ToString());
                Assert.AreEqual(expectedTc, oldTc, "ERROR: Invalid TC");
            }
        }


    }
}
