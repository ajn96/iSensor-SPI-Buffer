using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using iSensorSPIBuffer;

namespace ExampleApp
{
    public partial class Form1 : Form
    {
        private CLI SpiBuf;

        public Form1()
        {
            InitializeComponent();
        }

        private void open_Click(object sender, EventArgs e)
        {
            try
            {
                SpiBuf = new CLI(comPort.Text);
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void read_Click(object sender, EventArgs e)
        {
            try
            {
                val.Text = SpiBuf.Read(Convert.ToByte(addr.Text, 16)).ToString("X2");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void write_Click(object sender, EventArgs e)
        {
            try
            {
                SpiBuf.Write(Convert.ToByte(addr.Text, 16), Convert.ToByte(val.Text, 16));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }
    }
}
