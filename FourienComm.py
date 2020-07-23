# -*- coding: utf-8 -*-
"""
Created on Thu Jul 23 11:24:52 2020

@author: Designer
"""

import serial
import threading
import time

import sys

class FourienComm:
    
    
    def __init__(self, device):
        self.PREAMBLE = 85
        self.PREAMBLE_COUNT = 4
        self.VERSION_MESSAGE = 86
        self.ASCII_MESSAGE = 65
        self.MEMORY_READ_MESSAGE = 0x10
        self.MEMORY_WRITE_MESSAGE = 0x20
        self.ascii_message_callback = self.ascii_message_callback_holder
        self.memory_write_callback = self.memory_write_callback_holder
        self.memory_read_callback = self.memory_read_callback_holder
        
        self.open(device)
            
    def open(self, device):
        self.ser = serial.Serial(device)
        self.connected = True
                
        self.thread = threading.Thread(target=self.service_serial)
        self.thread.daemon = True
        self.thread.start()
        
    def close(self):
        self.connected = False;
        self.ser.close()
        
    # Call back place holders if the user doesn't declare any
    def ascii_message_callback_holder(self, message, time):
        return 
    def memory_write_callback_holder(self, address, data, time):
        return 
    def memory_read_callback_holder(self, address, data, time):
        return 
    
    
    def set_ascii_message_callback(self, callback_function):
        self.ascii_message_callback = callback_function
    
    def set_memory_write_callback(self, callback_function):
        self.memory_write_callback = callback_function
        
    def set_memory_read_callback(self, callback_function):
        self.memory_read_callback = callback_function
    
    def service_serial(self):
        
        buffer = []
        preamble_count = 0
        
        while self.connected:
            if self.ser.inWaiting() > 0:
            
                b = ord(self.ser.read())
                buffer.append(b)
                #print(b)
                if b == self.PREAMBLE:
                    preamble_count += 1
                else:
                    if preamble_count >= self.PREAMBLE_COUNT:
                        buffer = []
                        buffer.append(b)
                    preamble_count = 0
                    
                    if len(buffer) > 1:
                        buffer_size = (int(buffer[0]) << 8) + int(buffer[1])
                        if len(buffer) == buffer_size + 1:
                            self.parse_buffer(buffer.copy())
                            buffer = []
            time.sleep(0.001) # don't run away with the thread
    
    
       
    def parse_buffer(self, buf):
        packet_len = (buf[0] << 8) + buf[1]
        buf = buf[2:]
        packet_type = buf[0]
        buf = buf[1:]
        data_packet = []
        
        time = buf[0] << 24
        time += buf[1] << 16
        time += buf[2] << 8
        time += buf[3]
        buf = buf[4:]
        
        if packet_type == self.ASCII_MESSAGE:
            message = ""
            while len(buf) > 0:
                message += chr(buf.pop(0))
            self.ascii_message_callback(message, time)
            
        if packet_type == self.MEMORY_READ_MESSAGE:
            address = (buf.pop(0) << 8) + buf.pop(0)
            while len(buf) > 3: # full data packets only, stop bytes don't count
                data = (buf.pop(0) << 24) + (buf.pop(0) << 16) + (buf.pop(0) << 8) + buf.pop(0)
                data_packet.append(data)
                
            #if address == 255:
            #    print("Heartbeat Message")
            self.memory_read_callback_holder(address, data_packet, time)
            #print(address, data_packet, time)
    
    def get_version(self):
        packet = []
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(0)
        packet.append(2)
        packet.append(self.VERSION_MESSAGE)
        packet.append(0)
        self.send_data(packet)
        
    def write_memory(self, address, data):
        packet = []
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(0)
        packet.append(8)
        packet.append(self.MEMORY_WRITE_MESSAGE)
        packet.append((address >> 8) & 0xFF)
        packet.append(address & 0xFF)
        packet.append((data >> 24) & 0xFF)
        packet.append((data >> 16) & 0xFF)
        packet.append((data >> 8) & 0xFF)
        packet.append(data & 0xFF)
        packet.append(0)
        self.send_data(packet)
        
    def read_memory(self, address):\
        packet = []
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(self.PREAMBLE)
        packet.append(0)
        packet.append(4)
        packet.append(self.MEMORY_READ_MESSAGE)
        packet.append((address >> 8) & 0xFF)
        packet.append(address & 0xFF)
        packet.append(0)
        self.send_data(packet)
    
    def send_data(self, data):
        if self.ser.isOpen():
            self.ser.write(data)

    
