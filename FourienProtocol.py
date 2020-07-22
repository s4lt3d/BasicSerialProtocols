
import serial
import threading
import time

ser = serial.Serial("COM4")
connected = True


def service_serial():
    while connected:
        if ser.inWaiting() > 0:
            b = ser.read()
            print(str(b))
        time.sleep(0.001) # don't run away with the thread
        
thread = threading.Thread(target=service_serial)
thread.start()

input("Press enter to quit")
connected = False;
ser.close()
