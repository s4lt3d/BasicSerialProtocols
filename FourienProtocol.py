
import serial
import threading
import time
import numpy as np
import sys

ser = serial.Serial("COM4")
connected = True

PREAMBLE = 85
PREAMBLE_COUNT = 4
VERSION_MESSAGE = 86
ASCII_MESSAGE = 65
MEMORY_READ_MESSAGE = 0x10
MEMORY_WRITE_MESSAGE = 0x20

def service_serial():
    buffer = []
    preamble_count = 0
    while connected:
        if ser.inWaiting() > 0:
        
            b = ord(ser.read())
            buffer.append(b)
            #print(b)
            if b == PREAMBLE:
                preamble_count += 1
            else:
                if preamble_count >= PREAMBLE_COUNT:
                    buffer = []
                    buffer.append(b)
                preamble_count = 0
                
                if len(buffer) > 1:
                    buffer_size = (int(buffer[0]) << 8) + int(buffer[1])
                    if len(buffer) == buffer_size + 1:
                        parse_buffer(buffer.copy())
                        buffer = []
        time.sleep(0.001) # don't run away with the thread
        
        
def parse_buffer(buf):
    packet_len = (buf[0] << 8) + buf[1]
    buf = buf[2:]
    packet_type = buf[0]
    buf = buf[1:]
    address = 0
    data = 0
    data_packet = []
    
    time = buf[0] << 24
    time += buf[1] << 16
    time += buf[2] << 8
    time += buf[3]
    buf = buf[4:]
    
    if packet_type == ASCII_MESSAGE:
        message = ""
        while len(buf) > 0:
            message += chr(buf.pop(0))
        #print(message + " " + str(time))
        
    if packet_type == MEMORY_READ_MESSAGE:
        address = (buf.pop(0) << 8) + buf.pop(0)
        while len(buf) > 3: # full data packets only, stop bytes don't count
            data = (buf.pop(0) << 24) + (buf.pop(0) << 16) + (buf.pop(0) << 8) + buf.pop(0)
            data_packet.append(data)
            
        if address == 255:
            print("Heartbeat Message")
        print(address, data_packet, time)

def get_version():
    packet = []
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(0)
    packet.append(2)
    packet.append(VERSION_MESSAGE)
    packet.append(0)
    send_data(packet)
    
def write_memory(address, data):
    packet = []
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(0)
    packet.append(8)
    packet.append(MEMORY_WRITE_MESSAGE)
    packet.append((address >> 8) & 0xFF)
    packet.append(address & 0xFF)
    packet.append((data >> 24) & 0xFF)
    packet.append((data >> 16) & 0xFF)
    packet.append((data >> 8) & 0xFF)
    packet.append(data & 0xFF)
    packet.append(0)
    send_data(packet)
    
def read_memory(address):
    packet = []
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(PREAMBLE)
    packet.append(0)
    packet.append(4)
    packet.append(MEMORY_READ_MESSAGE)
    packet.append((address >> 8) & 0xFF)
    packet.append(address & 0xFF)
    packet.append(0)
    send_data(packet)

def send_data(data):
    if ser.isOpen():
        ser.write(data)

def main():
    thread = threading.Thread(target=service_serial)
    thread.daemon = True
    thread.start()
    
    while True:
        
        i = input("Q to quit\r\nV for Version\r\nW for write test\r\nR for read test\r\nInput: ")
        i == i.upper()
        if i == 'q':
            break
        if i == 'v':
            get_version()
        if i == 'w':
            r = np.random.randint(0, 2147483647)
            print("Writing random int",  r)
            write_memory(1, 234)
        if i == 'r':
            read_memory(1)
            
    connected = False;
    ser.close()

if __name__ == "__main__":
    main()
