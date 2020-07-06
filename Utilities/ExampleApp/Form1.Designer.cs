namespace ExampleApp
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.comPort = new System.Windows.Forms.TextBox();
            this.open = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.read = new System.Windows.Forms.Button();
            this.write = new System.Windows.Forms.Button();
            this.addr = new System.Windows.Forms.TextBox();
            this.val = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(63, 15);
            this.label1.TabIndex = 0;
            this.label1.Text = "COM Port:";
            // 
            // comPort
            // 
            this.comPort.Location = new System.Drawing.Point(81, 12);
            this.comPort.Name = "comPort";
            this.comPort.Size = new System.Drawing.Size(114, 23);
            this.comPort.TabIndex = 1;
            // 
            // open
            // 
            this.open.Location = new System.Drawing.Point(12, 41);
            this.open.Name = "open";
            this.open.Size = new System.Drawing.Size(72, 23);
            this.open.TabIndex = 2;
            this.open.Text = "Open";
            this.open.UseVisualStyleBackColor = true;
            this.open.Click += new System.EventHandler(this.open_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 87);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(36, 15);
            this.label2.TabIndex = 4;
            this.label2.Text = "Addr:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 116);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(38, 15);
            this.label3.TabIndex = 4;
            this.label3.Text = "Value:";
            // 
            // read
            // 
            this.read.Location = new System.Drawing.Point(12, 142);
            this.read.Name = "read";
            this.read.Size = new System.Drawing.Size(72, 23);
            this.read.TabIndex = 2;
            this.read.Text = "Read";
            this.read.UseVisualStyleBackColor = true;
            this.read.Click += new System.EventHandler(this.read_Click);
            // 
            // write
            // 
            this.write.Location = new System.Drawing.Point(123, 142);
            this.write.Name = "write";
            this.write.Size = new System.Drawing.Size(72, 23);
            this.write.TabIndex = 2;
            this.write.Text = "Write";
            this.write.UseVisualStyleBackColor = true;
            this.write.Click += new System.EventHandler(this.write_Click);
            // 
            // addr
            // 
            this.addr.Location = new System.Drawing.Point(56, 84);
            this.addr.Name = "addr";
            this.addr.Size = new System.Drawing.Size(139, 23);
            this.addr.TabIndex = 1;
            this.addr.Text = "0";
            // 
            // val
            // 
            this.val.Location = new System.Drawing.Point(56, 113);
            this.val.Name = "val";
            this.val.Size = new System.Drawing.Size(139, 23);
            this.val.TabIndex = 1;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(225, 187);
            this.Controls.Add(this.val);
            this.Controls.Add(this.addr);
            this.Controls.Add(this.write);
            this.Controls.Add(this.read);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.open);
            this.Controls.Add(this.comPort);
            this.Controls.Add(this.label1);
            this.Name = "Form1";
            this.Text = "iSensor SPI Buffer";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox comPort;
        private System.Windows.Forms.Button open;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button read;
        private System.Windows.Forms.Button write;
        private System.Windows.Forms.TextBox addr;
        private System.Windows.Forms.TextBox val;
    }
}

