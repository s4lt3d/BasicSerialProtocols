
import FourienComm as fc
import numpy as np
import matplotlib.pyplot as plt

gl = 0

def ascii_message(message, time):
    print("ascii message ---")
    print(message, time)
    print("---")

def memory_read(address, data, time):
    if address > 0:
        print("memory read message ---")
        print(f"{address}, {data}, {time}")
        print("---")
        plt.plot(data)
        plt.show()
    else:
        print(time)
    

def memory_write(address, data, time):
    print("memory write message ---")
    print(f"{address}, {data}, {time}")
    print("---")

def main():
    comm = fc.FourienComm("COM4")
    comm.set_ascii_message_callback(ascii_message)
    comm.set_memory_read_callback(memory_read)
    while True:
        
        i = input("Q to quit\r\nV for Version\r\nW for write test\r\nR for read test\r\nB for burst of memory\r\nInput: ")
        i = i.lower()
        if i == 'q':
            comm.close()
            break
        if i == 'v':
            comm.get_version()
        if i == 'w':
            r = np.random.randint(0, 2147483647)
            print("Writing random int",  r)
            comm.write_memory(1, r)
        if i == 'r':
            comm.read_memory(1)
        if i == 'b':
            comm.read_memory(17)
            


if __name__ == "__main__":
    main()

    
