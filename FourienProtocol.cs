using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace Fourien
{
    public delegate void NotifyAsciiMessage(string message);

    class FourienProtocol
    {
        SerialPort serial;
        Queue<byte> buffer = new Queue<byte>();

        Queue<string> ascii_messages = new Queue<string>();

        const int START_BYTE = 0xF0;
        const int PREAMBLE = 0x55;
        const int PREAMBLE_COUNT = 4;
        const int END_BYTE = 0xF7;
        const int DIGITAL_IO_MESSAGE = 0x0E;
        const int ASCII_MESSAGE = 65;

        public event NotifyAsciiMessage AsciiMessageRecieved;

        public FourienProtocol(string portName, int baud = 8000000)
        {

            serial = new SerialPort(portName, baud);

            serial.DataReceived += Serial_DataReceived;
        }

        public void Open()
        {
            serial.Open();
        }


        int preamble_count = 0;
        int buf_pos = 0;

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (serial.BytesToRead > 0)
            {
                int incoming_byte = serial.ReadByte();
                buffer.Enqueue((byte)incoming_byte);

                if (incoming_byte == PREAMBLE)
                {
                    preamble_count++;
                }
                else
                {
                    if (preamble_count >= PREAMBLE_COUNT)
                    {
                        buffer.Clear();
                        buffer.Enqueue((byte)incoming_byte);
                    }

                    preamble_count = 0;

                    if (buffer.Count > 0)
                    {
                        if (buffer.Count == buffer.Peek() + 1)
                        {
                            parseBuffer();
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Handles the actual sending of data to the serial port
        /// </summary>
        /// <param name="data">The data in bytes to send</param>
        private void SendData(byte[] data, int length)
        {
            if (serial.IsOpen)
            {
                serial.Write(data, 0, length);
            }
        }

        /// <summary>
        /// Writes the value of 0 or 1 to a digital pin
        /// </summary>
        /// <param name="pin">The pin number to write the value to</param>
        /// <param name="value">0 or 1</param>
        public void DigitalWrite(int pin, int val)
        {
            byte[] data = new byte[8];
            int i = 0;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 4; // size of packet after this. Max size 250 bytes
            data[i++] = DIGITAL_IO_MESSAGE;
            data[i++] = (byte)pin;
            data[i++] = (byte)val;
            SendData(data, i);
        }


        public void RegisterWrite(int reg, long val)
        {

        }

        private void parseBuffer()
        {
            int len = buffer.Dequeue();

            int type = buffer.Dequeue();


            switch (type)
            {
                case DIGITAL_IO_MESSAGE:

                    break;

                case ASCII_MESSAGE:
                    string message = "";
                    while(buffer.Count > 0)
                        message += (char)buffer.Dequeue();
                    message += "\r\n";
                    OnAsciiMessageRecieved(message);
                    break;
            }
        }

        protected virtual void OnAsciiMessageRecieved(string message) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            AsciiMessageRecieved?.Invoke(message);
        }


    }
}
