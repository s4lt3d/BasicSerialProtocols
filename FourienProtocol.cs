using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace Fourien
{
    class FourienProtocol
    {
        SerialPort serial;

        const int START_BYTE = 0xF0;
        const int PREAMBLE = 0x55;
        const int END_BYTE = 0xF7;
        const int DIGITAL_IO_MESSAGE = 0x0E;



        FourienProtocol(string portName, int baud = 1000000) {
            
            serial = new SerialPort(portName, baud);

            serial.DataReceived += Serial_DataReceived;
        }

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            
        }

        /// <summary>
        /// Handles the actual sending of data to the serial port
        /// </summary>
        /// <param name="data">The data in bytes to send</param>
        private void SendData(byte[] data) {
            if (serial.IsOpen) {
                serial.Write(data, 0, data.Length);
            }
        }

        /// <summary>
        /// Writes the value of 0 or 1 to a digital pin
        /// </summary>
        /// <param name="value">0 or 1</param>
        /// <param name="pin">The pin number to write the value to</param>
        public void DigitalWrite(int value, int pin) {
            byte[] data = new byte[4];
            int i = 0;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = PREAMBLE;
            data[i++] = 3; // size of packet after this. Max size 250 bytes
            data[i++] = DIGITAL_IO_MESSAGE;
            data[i++] = (byte)pin;
            data[i++] = (byte)value;
            SendData(data);
        }


        public void RegisterWrite(long value, int pin) { 
        
        }
    }
}
