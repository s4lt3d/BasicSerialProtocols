using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace Fourien
{
    public delegate void NotifyAsciiMessage(string message);
    public delegate void NotifyRegisterReadMessage(int reg, long val);

    class FourienProtocol
    {
        SerialPort serial;
        Queue<byte> buffer = new Queue<byte>();

        Queue<string> ascii_messages = new Queue<string>();

        const int PREAMBLE = 0x55;
        const int PREAMBLE_COUNT = 4;
        const int DIGITAL_IO_MESSAGE = 0x0E;
        const int ASCII_MESSAGE = 65; // 'A'
        const int VERSION_MESSAGE = 85;
        const int MEMORY_READ_MESSAGE = 0x10;
        const int MEMORY_WRITE_MESSAGE = 0x20;

        public event NotifyAsciiMessage AsciiMessageRecieved;
        public event NotifyRegisterReadMessage RegisterReadMessage;


        public FourienProtocol(string portName, int baud = 2000000)
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
                        //while (buffer.Peek() == PREAMBLE)
                        //    _ = buffer.Dequeue();
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
            byte[] data = new byte[9];
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

        public void FirmwareVersion() {
            byte[] data = new byte[8];
            int i = 0;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 1; // size of packet after this. Max size 250 bytes
            data[i++] = VERSION_MESSAGE;
            SendData(data, i);
        }

        public void RegisterWrite(int reg, long val)
        {
            byte[] data = new byte[16];
            int i = 0;
            reg &= 0xFFFF;

            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 8; // length including this byte
            data[i++] = MEMORY_WRITE_MESSAGE;
            data[i++] = (byte)((reg >> 8) & 0xFF);
            data[i++] = (byte)((reg) & 0xFF);
            data[i++] = (byte)((val >> 24) & 0xFF);
            data[i++] = (byte)((val >> 16) & 0xFF);
            data[i++] = (byte)((val >> 08) & 0xFF);
            data[i++] = (byte)((val) & 0xFF);
            SendData(data, i);
        }

        public void RegisterRead(int reg) {
            byte[] data = new byte[16];
            int i = 0;
            reg &= 0xFFFF;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 4; // length including this byte
            data[i++] = MEMORY_READ_MESSAGE;
            data[i++] = (byte)((reg >> 8) & 0xFF);
            data[i++] = (byte)((reg) & 0xFF);
            SendData(data, i);
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

                case MEMORY_READ_MESSAGE:

                    int address = 0;
                    address |= (int)buffer.Dequeue() << 8;
                    address |= (int)buffer.Dequeue();
                    long data = 0;
                    data |= (long)buffer.Dequeue() << 24;
                    data |= (long)buffer.Dequeue() << 16;
                    data |= (long)buffer.Dequeue() << 8;
                    data |= (long)buffer.Dequeue();
                    OnRegisterReadMessage(address, data);
                    break;

                default:

                    break;
            }

            if(buffer.Count > 0)
                while (buffer.Peek() == PREAMBLE)
                    _ = buffer.Dequeue();
        }

        protected virtual void OnAsciiMessageRecieved(string message) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            AsciiMessageRecieved?.Invoke(message);
        }

        protected virtual void OnRegisterReadMessage(int reg, long val) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            RegisterReadMessage?.Invoke(reg, val);
        }
    }
}
