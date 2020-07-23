// Runs on the teensy 4.1
// Walter Gordy
// Fourien, Inc


#define DEBUG 1
#define VERSION                "Ver 0.019 2020/07/22"

#include "Adafruit_ZeroFFT.h"
#include "FourienComm.h"

// Only 4 timers available
IntervalTimer heatbeatTimer;

void sendHeartBeat();
void onMemoryWrite(unsigned int, unsigned long);
void onMemoryRead(unsigned int);

#define DEFAULT_ANALOG_SAMPLES 1    // STORE IN MEMORY 100
#define ANALOG_SAMPLE_ADDR     100

int packet_flag = 0;
int fastUpdate_flag = 0;
int heartBeat_flag = 0;
int randomGen_flag = 0;

unsigned long register_memory[256]; 

/*
 * Function:  setup 
 * --------------------
 *  Basic setup function which initializes memory, sets baud rate, analog resolution, digital pin outputs,
 *  and sets callback functions for communication
 *
 */
void setup() {
  for(int i = 0; i < 256; i++) 
  {
    register_memory[i] = 0;
  }
  register_memory[ANALOG_SAMPLE_ADDR] = DEFAULT_ANALOG_SAMPLES;

  pinMode(13, OUTPUT);
  analogWriteResolution(12);
  analogReadResolution(12);  
  analogReadAveraging(1); 
  digitalWrite(13, 1);
  
  // For teensy 4.1, the baud doesn't matter. It is over usb at high speed. 
  Serial.begin(115200);
  sendVersion(VERSION);
  
  heatbeatTimer.begin(sendHeartBeat, 10000000);  // microseconds

  // set callback functions for comm events
  setMemoryWriteCallback(onMemoryWrite);
  setMemoryReadCallback(onMemoryRead);
}
/*
 * Function:  loop 
 * --------------------
 * main program loop which services the mailbox system
 *
 */
void loop() 
{
  serviceIncomingSerial(); 
  if(heartBeat_flag > 0)
  {
    heartBeat_flag = 0;
    sendMemoryMessage(0, 0, 0);
  }
}
/*
 * Function:  onMemoryWrite 
 * --------------------
 * callback function when a memory write request is recieved over serial
 *
 *  address: the address to write, 16 bit
 *  
 *  data: the data to write, 32 bit
 */
void onMemoryWrite(unsigned int address, unsigned long data)
{
  register_memory[address] = data;
  sendMemoryMessage(address, register_memory[address], 0);
  if(address == 13) {
    if(data > 0) {
      digitalWrite(13, HIGH);  
    }
    else {
      digitalWrite(13, LOW);
    }
  }
}
/*
 * Function:  onMemoryReadWrite 
 * --------------------
 * callback function when a memory read request is recieved over serial
 * this function should then call sendMemoryMessage(address) or burstMemoryMessage(address, memory_contents[], memory_len)
 * to return data to the caller
 *
 *  address: the address a read was requested from, 16 bit
 */
void onMemoryRead(unsigned int address)
{
  if(address != 17)
  {
    sendMemoryMessage(address, register_memory[address], 0);
  }
  else
  {
    unsigned long data[20];
    for(int i = 0; i < 20; i++)
    {
      unsigned long p = millis();
      data[i] = (unsigned long)((sin(i + p) + 1) * 1000);
    }
    burstMemoryMessage(17, data, 20, 0);
  
  }
}

void sendHeartBeat() 
{
  heartBeat_flag = 1;
}

float sine = 0;
unsigned long longsine = 0;

void fastUpdate() 
{
  sine += 0.2;
  longsine = (sin(sine) + 1) * 2048;
  longsine = min(4095, max(0, longsine)); // clamp it 
  register_memory[17] = longsine;
  fastUpdate_flag = 1;
 // analogWrite(A21, (int)longsine);
}

int analogSample(int pin) {
  //long sample = 0;
  register_memory[ANALOG_SAMPLE_ADDR] = min(32768, max(1, register_memory[ANALOG_SAMPLE_ADDR]));
  analogReadAveraging(register_memory[ANALOG_SAMPLE_ADDR]);
  //register_memory[ANALOG_SAMPLE_ADDR] = min(32768, max(1, register_memory[ANALOG_SAMPLE_ADDR]));
  
  //for(int i = 0; i < register_memory[ANALOG_SAMPLE_ADDR]; i++) {
  //  sample += analogRead(pin);
  //}
  return analogRead(pin);
  //return (int)(sample / register_memory[ANALOG_SAMPLE_ADDR]);  
}
