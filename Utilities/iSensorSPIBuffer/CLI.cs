using System;
using System.Collections.Generic;
using System.IO.Ports;

namespace iSensorSPIBuffer
{
    public class CLI
    {
        static SerialPort m_port;

        public CLI(string PortName)
        {
            m_port = new SerialPort(PortName);
            m_port.BaudRate = 460800;
            m_port.NewLine = "\r\n";
            m_port.ReadTimeout = 1000;
            m_port.WriteTimeout = 1000;
            m_port.RtsEnable = true;
            m_port.DtrEnable = true;
            m_port.Open();
            /* Set delim to , */
            System.Threading.Thread.Sleep(10);
            SendCommand("delim ,\r", false);
        }

        ~CLI()
        {
            m_port.Close();
        }

        public ushort Read(byte RegAddr)
        {
            string line;
            SendCommand("read " + RegAddr.ToString("X2"));
            /* Read data response */
            line = m_port.ReadLine();
            return Convert.ToUInt16(line, 16);
        }

        public ushort[] Read(byte StartAddr, byte EndAddr)
        {
            string line;
            string[] vals;
            List<ushort> readData = new List<ushort>();
            SendCommand("read " + StartAddr.ToString("X2") + " " + EndAddr.ToString("X2"));
            /* Read data */
            line = m_port.ReadLine();
            vals = line.Split(",");
            foreach(string str in vals)
            {
                readData.Add(Convert.ToUInt16(str, 16));
            }
            return readData.ToArray();
        }

        public ushort[][] Read(byte StartAddr, byte EndAddr, uint NumReads)
        {
            string line;
            string[] vals;
            List<ushort[]> readData = new List<ushort[]>();
            List<ushort> lineData = new List<ushort>();
            SendCommand("read " + StartAddr.ToString("X2") + " " + EndAddr.ToString("X2") + " " + NumReads.ToString("X4"));
            /* Read data */
            for (int i = 0; i < NumReads; i++)
            {
                line = m_port.ReadLine();
                vals = line.Split(",");
                lineData.Clear();
                foreach (string str in vals)
                {
                    lineData.Add(Convert.ToUInt16(str, 16));
                }
                readData.Add(lineData.ToArray());
            }
            return readData.ToArray();
        }

        public void Write(byte RegAddr, byte WriteValue)
        {
            SendCommand("write " + RegAddr.ToString("X2") + " " + WriteValue.ToString("X2"), false);
        }

        public void StartStream()
        {
            SendCommand("stream 1", false);
        }

        public void StopStream()
        {
            SendCommand("stream 0", false);
        }

        private void SendCommand(string command, bool commandEcho = true)
        {
            m_port.Write(command + "\r");
            /* Read line to discard command echo */
            if(commandEcho)
            {
                m_port.ReadLine();
            }
        }
    }
}
