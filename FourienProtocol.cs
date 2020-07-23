using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace Fourien
{
    public delegate void NotifyAsciiMessage(string message, long time);
    public delegate void NotifyRegisterReadMessage(int reg, long[] memory, int len, long time);
    public delegate void NotifyAnalogReadMessage(int pin, long val, long time);

    class FourienProtocol
    {
        SerialPort serial;
        Queue<byte> buffer = new Queue<byte>();

        Queue<string> ascii_messages = new Queue<string>();

        const int PREAMBLE = 85;
        const int PREAMBLE_COUNT = 4;
        const int ASCII_MESSAGE = 65; // 'A'
        const int VERSION_MESSAGE = 86; // 'V'
        const int MEMORY_READ_MESSAGE = 0x10;
        const int MEMORY_WRITE_MESSAGE = 0x20;

        public event NotifyAsciiMessage AsciiMessageRecieved;
        public event NotifyRegisterReadMessage RegisterReadMessage;
        public event NotifyAnalogReadMessage RegisterAnalogReadMessage;

        public FourienProtocol(string portName, int baud = 1600)
        {

            serial = new SerialPort(portName, baud);

            serial.DataReceived += Serial_DataReceived;
        }

        public void Open()
        {
            
            serial.Open();
            // flush incoming bytes, yes this is an issue. 
            while (serial.BytesToRead > 0) 
                serial.ReadByte();
        }


        int preamble_count = 0;

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (serial.BytesToRead > 0)
            {
                
                int incoming_byte = serial.ReadByte();
                
                buffer.Enqueue((byte)incoming_byte);

                if (incoming_byte == PREAMBLE)
                {
                    preamble_count++;
                 //   Console.WriteLine();
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

                    if (buffer.Count > 1)
                    {
                        int size = (buffer.ElementAt(0) << 8) + buffer.ElementAt(1);

                        if (buffer.Count == size + 1)
                        {
                            parseBuffer();
                        }
                    }
                }
                //Console.Write(incoming_byte.ToString("X2") + " ");
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

        public void FirmwareVersion() {
            byte[] data = new byte[16];
            int i = 0;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 0;
            data[i++] = 2; // size of packet after this. Max size 250 bytes
            data[i++] = VERSION_MESSAGE;
            data[i++] = 0;
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
            data[i++] = 0; // length leading zero
            data[i++] = 8; // length including this byte
            data[i++] = MEMORY_WRITE_MESSAGE;
            data[i++] = (byte)((reg >> 8) & 0xFF);
            data[i++] = (byte)((reg) & 0xFF);
            data[i++] = (byte)((val >> 24) & 0xFF);
            data[i++] = (byte)((val >> 16) & 0xFF);
            data[i++] = (byte)((val >> 08) & 0xFF);
            data[i++] = (byte)((val) & 0xFF);
            data[i++] = 0;
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
            data[i++] = 0; // length leading zero
            data[i++] = 5; // length including this byte
            data[i++] = MEMORY_READ_MESSAGE;
            data[i++] = (byte)((reg >> 8) & 0xFF);
            data[i++] = (byte)((reg) & 0xFF);
            data[i++] = 0;
            SendData(data, i);
        }

        private void parseBuffer()
        {
            long time = 0;
            if (buffer.Count < 3)
                return;
            int len = buffer.Dequeue() << 8;
            len |= buffer.Dequeue();

            int type = buffer.Dequeue();


            switch (type)
            {

                case ASCII_MESSAGE:
                    string message = "";
                    time |= (long)buffer.Dequeue() << 24;
                    time |= (long)buffer.Dequeue() << 16;
                    time |= (long)buffer.Dequeue() << 8;
                    time |= (long)buffer.Dequeue();

                    while (buffer.Count > 0)
                        message += (char)buffer.Dequeue();
                    
                    OnAsciiMessageRecieved(message, time);
                    break;

                case MEMORY_READ_MESSAGE:

                    time |= (long)buffer.Dequeue() << 24;
                    time |= (long)buffer.Dequeue() << 16;
                    time |= (long)buffer.Dequeue() << 8;
                    time |= (long)buffer.Dequeue();

                    int address = 0;
                    address |= (int)buffer.Dequeue() << 8;
                    address |= (int)buffer.Dequeue();

                    long[] memory = new long[2048];
                    int i = 8;
                    int pos = 0;
                    long data = 0;
                    while (i < len)
                    {
                        data = (long)buffer.Dequeue() << 24;
                        data |= (long)buffer.Dequeue() << 16;
                        data |= (long)buffer.Dequeue() << 8;
                        data |= (long)buffer.Dequeue();
                        memory[pos] = data;
                        i += 4;
                        pos++;
                    }
                    
                    OnRegisterReadMessage(address, memory, pos, time);
                    break;
               
                default:

                    break;
            }

            if(buffer.Count > 0)
                while (buffer.Peek() == PREAMBLE)
                    _ = buffer.Dequeue();
        }

        protected virtual void OnAsciiMessageRecieved(string message, long time) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            AsciiMessageRecieved?.Invoke(message, time);
        }

        protected virtual void OnRegisterReadMessage(int reg, long[] memory, int len, long time) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            RegisterReadMessage?.Invoke(reg, memory, len, time);
        }

        protected virtual void OnAnalogReadMessage(int pin, long val, long time) //protected virtual method
        {
            //if AsciiMessageRecieved is not null then call delegate
            RegisterAnalogReadMessage?.Invoke(pin, val, time);
        }

        public void Close() {
            try
            {
                serial.Close();
            }
            catch { }
        }
    }
}
