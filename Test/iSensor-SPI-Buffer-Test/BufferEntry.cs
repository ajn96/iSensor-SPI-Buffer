using RegMapClasses;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NUnit.Framework;

namespace iSensor_SPI_Buffer_Test
{
    /// <summary>
    /// Class representing a single buffer data entry
    /// </summary>
    public class BufferEntry
    {
        private uint m_timestampLwr, m_timestampUpr;

        public uint Timestamp;
        public uint DeltaTime;
        public uint Signature;
        public List<uint> Data;

        public BufferEntry()
        {
            Data = new List<uint>();
        }

        public BufferEntry(uint[] BufferData, IEnumerable<RegClass> Reglist)
        {
            Assert.AreEqual(BufferData.Count(), Reglist.Count(), "ERROR: Buffer data and register list must be the same size");
            Data = new List<uint>(32);
            int index, dataIndex, maxIndex;
            maxIndex = 0;
            index = 0;
            foreach(RegClass reg in Reglist)
            {
                if (reg.Label == "BUF_TIMESTAMP_LWR")
                    m_timestampLwr = BufferData[index];
                if (reg.Label == "BUF_TIMESTAMP_UPR")
                    m_timestampUpr = BufferData[index];
                if (reg.Label == "BUF_DELTA_TIME")
                    DeltaTime = BufferData[index];
                if (reg.Label == "BUF_SIG")
                    Signature = BufferData[index];
                if (reg.Label.Contains("BUF_DATA_"))
                {
                    dataIndex = Convert.ToInt32(reg.Label.Last());
                    maxIndex = Math.Max(maxIndex, dataIndex);
                    Data[dataIndex] = BufferData[index];
                }
                index++;
            }

            /* Rebuild timestamp */
            Timestamp = m_timestampUpr << 16;
            Timestamp |= m_timestampLwr;

            /* Trim data */
            Data.RemoveRange(maxIndex + 1, Data.Count - (maxIndex + 1));
        }

        public uint GetExpectedSignature()
        {
            uint sig;

            sig = DeltaTime;
            sig += (Timestamp & 0xFFFF);
            sig += (Timestamp >> 16);
            foreach (uint item in Data)
            {
                sig += item;
            }

            return sig & 0xFFFF;
        }

        public static List<BufferEntry> ParseBufferData(uint[] BufferData, IEnumerable<RegClass> Reglist)
        {
            List<BufferEntry> result = new List<BufferEntry>();
            uint[] entryData = new uint[Reglist.Count()];
            int index = 0;
            while(index <= BufferData.Count())
            {
                Array.Copy(BufferData, entryData, Reglist.Count());
                result.Add(new BufferEntry(entryData, Reglist));
                index += Reglist.Count();
            }
            return result;
        }
    }
}
