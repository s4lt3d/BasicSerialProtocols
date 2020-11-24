# -*- coding: utf-8 -*-
"""
Created on Thu Nov 12 12:14:09 2020

@author: Walter
"""

# requires pyserial to run
# conda install -c anaconda pyserial
import FourienComm as fc
import numpy as np
import time
import sys
import matplotlib.pyplot as plt
import statistics


# Basic Commands
# Start sweep
# m 16 1 
# Set dwell millis to 1 ms
# s 13 1 
# Change Amplitude of Piezo
# m 12 2000
# Change freq step size to 1 hz
# s 14 1 
# Set start freq to 1000
# s 10 1000
# Set end freq to 5000
# s 11 5000
# Averages to 1024
# m 9 1024

# Read ADC
# m 31 0

# Guide to ADC Channels
# m 30 Channel
# 0 - GND
# 1 - PIEZO ENVELOPE
# 2 - PSD ENVELOPE
# 3 - PSD Gained Signal
# 4 - DDS
# 5 - 1.8V reference
# 6 - PSD Phase
# 7 - PSD Magnitude

# Guide to Phase Channels
# m 19 Channel
# 0 - X AC
# 1 - X DC
# 2 - Y AC
# 3 - Y DC
# 4 - POS DC
# 5 - POS AC
# 6 - SUM DC
# 7 - SUM AC

gl = 0

adc_mux = 18
phase_mux = 19

def ascii_message(message, time):
    print("ascii message ---")
    print(message, time)
    print("---")

burst_data = []

def memory_read(address, data, time):
    global burst_data
    if address > 3:
        print("memory read message ---")
        print(f"{address}, {data}, {time}")
        print("---")
        sys.stdout.flush()
#        plt.plot(data)
#        plt.show()
    elif address == 3: # burst memory message from frequency scan
        print("memory burst message ---")
        #print(f"{address}, {data}, {time}")
        
        
        burst_data = data
        x, y = burst_data[8::2], burst_data[9::2]
        plt.plot(x, y)
        plt.show()
        print("Count:  ", len(data) / 2)
        print("Mean:   ", statistics.mean(y))
        print("Std Dev:", statistics.stdev(y))
        print("---")
        
        
    
def memory_write(address, data, time):
    print("memory write message ---")
    print(f"{address}, {data}, {time}")
    print("---")


menu = ["q to quit",
        "v for Version",
        "m reg value", 
        "s reg freq"]

def main():
    comm = fc.FourienComm("COM3")
    comm.set_ascii_message_callback(ascii_message)
    comm.set_memory_read_callback(memory_read)
    while True:
        
        [print(m) for m in menu]
        
        i = input("Input: ")
        i = i.lower()
        if len(i) == 1:
            if i == 'q':
                comm.close()
                break
            if i == 'v':
                comm.get_version()
        else:
            if i.startswith('d'):
                i = i.split(" ")
                d = int(i[1])
                comm.write_memory(4, d)
            
            elif i.startswith('m'):
                
                i = i.split(" ")
                if len(i) > 2:
                    a = int(i[1])
                    d = int(i[2])
                    comm.write_memory(a, d)
                    
            elif i.startswith('s'):
                    
                i = i.split(" ")
                if len(i) > 2:
                    a = int(i[1])
                    d = int(i[2]) * 10000
                    comm.write_memory(a, d)                    
            
if __name__ == "__main__":
    main()
   
