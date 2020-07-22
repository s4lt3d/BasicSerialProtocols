// Runs on the teensy
// Walter Gordy
// Fourien, Inc

#define HWSERIAL Serial1
#include <CircularBuffer.h>
CircularBuffer<char, 2048> buffer;
// Only 4 timers available
IntervalTimer heatbeatTimer;
IntervalTimer fastUpdateTimer;
IntervalTimer analogUpdateTimer;

#include "Adafruit_ZeroFFT.h"


//#define SERIAL_BUFFER_SIZE  32768 // redefining an arduino define

#define DEBUG 1

#define VERSION                "Ver 0.018 2020/07/22"
#define PREAMBLE               0x55
#define PREAMBLE_COUNT         4   // must recieve 4 preambles in a row to start a packet
#define DIGITAL_WRITE_MESSAGE  0x0E
#define DIGITAL_READ_MESSAGE   0x1E
#define ANALOG_WRITE_MESSAGE   0x2E
#define ANALOG_READ_MESSAGE    0x3E
#define ASCII_MESSAGE          'A'
#define VERSION_MESSAGE        85
#define MEMORY_READ_MESSAGE    0x10
#define MEMORY_WRITE_MESSAGE   0x20

#define DEFAULT_ANALOG_SAMPLES 1    // STORE IN MEMORY 100
#define ANALOG_SAMPLE_ADDR     100

unsigned long register_memory[256]; 
unsigned long register_time[256]; // in millis()

int packet_flag = 0;
int fastUpdate_flag = 0;
int heartBeat_flag = 0;
int randomGen_flag = 0;
unsigned long mem[2096];

int16_t fft_buffer[2048];

const uint16_t samples = 2048; //This value MUST ALWAYS be a power of 2
const double signalFrequency = 1000;
const double samplingFrequency = 5000;
const uint8_t amplitude = 100;

void setup() {

  for(int i = 0; i < 256; i++) 
  {
    register_memory[i] = 0;
    register_time[i] = 0;
  }
  register_memory[ANALOG_SAMPLE_ADDR] = DEFAULT_ANALOG_SAMPLES;

  // Run usb serial at 8mb/s 
  Serial.begin(1600);
  sendAsciiMessage(VERSION_MESSAGE);
  pinMode(13, OUTPUT);
  analogWriteResolution(12);
  analogReadResolution(12);  
  analogReadAveraging(1); 
  digitalWrite(13, 1);
  heatbeatTimer.begin(sendHeartBeat, 50000); // 1s
  //fastUpdateTimer.begin(fastUpdate, 200000); // 100ms
//  analogUpdateTimer.begin(analogUpdate, 1);
}

void loop() {
  serviceIncomingSerial(); 

  if(fastUpdate_flag > 0) 
  {
    fastUpdate_flag = 0;
    sendAnalogReadMessage(1, analogSample(A1));
    sendMemoryMessage(17);
  }
  else if(heartBeat_flag > 0)
  {
    heartBeat_flag = 0;
    sendMemoryMessage(255);
  }
  else if(randomGen_flag > 0) 
  {
    randomGen_flag = 0;

    for(int i = 0; i < 2048; i++)
    {
      mem[i] = random(1000);
      fft_buffer[i] = random(1000) + (int)((sin(i) + 1) * 1000) + (int)((sin(i*.2) + 1) * 1000);
    }  

    if(register_memory[2] > 0)
      ZeroFFT(fft_buffer, 2048);
    for(int i = 0; i < 2048; i++)
      mem[i] = fft_buffer[i];
    

    burstMemoryMessage(1, mem, 1024);
  }
  delay(1000);
}

void sendHeartBeat() 
{
  heartBeat_flag = 1;
  randomGen_flag = 1;
}

float sine = 0;
unsigned long longsine = 0;

void fastUpdate() 
{
  sine += 0.2;

  longsine = (sin(sine) + 1) * 2048;
  longsine = min(4095, max(0, longsine)); // clamp it 
  
  register_memory[17] = longsine;
  register_time[17] = millis();
  fastUpdate_flag = 1;
 // analogWrite(A21, (int)longsine);
}

void analogUpdate() 
{
  sine += 0.1;

  longsine = (sin(sine) + 1) * 2048;
  longsine = min(4095, max(0, longsine)); // clamp it 
  
  register_memory[17] = longsine;
  register_time[17] = millis();
  fastUpdate_flag = 1;
  //analogWrite(A21, (int)longsine);
}

int incoming_byte;
int expected_size = 2;
int preamble_count = 0;

// Pushes incoming serail to a circular buffer. When the buffered packet is fully loaded we can parse it. 
void serviceIncomingSerial() {
  while (Serial.available() > 0) {
    incoming_byte = Serial.read();
    buffer.push(incoming_byte);
    if(incoming_byte == PREAMBLE) {
      preamble_count++;
    } else {
      
      if(preamble_count >= PREAMBLE_COUNT) {
        buffer.clear();
        buffer.push(incoming_byte);
      }
      
      preamble_count = 0;
      
      if(buffer.size() > 1) {
        int buffer_size = (buffer[0] << 8) + buffer[1];
        if(buffer.size() == buffer_size + 1){
          parseBuffer();
        }
      }
    }
  }
}

void parseBuffer() {
  unsigned int len = buffer.shift() << 8;
  len |= buffer.shift();
  unsigned int type = buffer.shift();
  unsigned int pin;
  unsigned int val;

  unsigned int address = 0;
  unsigned long   data = 0;
  
  switch(type) {
    case DIGITAL_WRITE_MESSAGE:
      pin = buffer.shift();
      val = buffer.shift();
      if(val != 0)
        val = 1;
      if(pin == 13)
        digitalWrite(pin, val);
      break; // DIGITAL_WRITE_MESSAGE

    case ANALOG_WRITE_MESSAGE:
      pin = buffer.shift();
      val  = ((unsigned int)buffer.shift()) << 8;
      val |= ((unsigned int)buffer.shift());
//      analogWrite(A21, (int)val);
      break;

    case ANALOG_READ_MESSAGE:
      debug(String(type, DEC));
      pin = buffer.shift();
      val = analogSample(A1); // read analog value
      sendAnalogReadMessage(pin, val);
      break;
      

    case VERSION_MESSAGE:
      debug("Version");
      sendAsciiMessage(VERSION);
      break;

    case MEMORY_WRITE_MESSAGE:
      address = ((unsigned int)buffer.shift()) << 8;
      address |= ((unsigned int)buffer.shift());

      address &= 0xFFFF;

      data  = ((unsigned long)buffer.shift()) << 24;
      data |= ((unsigned long)buffer.shift()) << 16;
      data |= ((unsigned long)buffer.shift()) << 8;
      data |= (unsigned long)buffer.shift();
      register_memory[address] = data;
      data = register_memory[address];
      break;

    case MEMORY_READ_MESSAGE:
      debug("Read memory");
      address = (unsigned int)buffer.shift() << 8;
      address |= (unsigned int)buffer.shift();
      address &= 0xFFFF;      
      sendMemoryMessage(address);
      
      if(address == 0xAAAA) {
        //_reboot_Teensyduino_();
      }
      break;
  }
  
  buffer.clear();
}

void sendPreamble(){
  for(int i = 0; i < PREAMBLE_COUNT; i++)
    Serial.write(PREAMBLE);
}

// Memory Read will include time in milliseconds
void sendMemoryMessage(unsigned int address) {
  unsigned long data;
  unsigned long t = 0;
  sendPreamble();
  
  if(register_time[address] == 0)
    register_time[address] = millis();
  
  t = register_time[address];
  
  data = register_memory[address];

  mem[0] = data;
  burstMemoryMessage(address, mem, 1);
}

// Memory Read will include time in milliseconds
void burstMemoryMessage(unsigned int address, unsigned long memory_contents[], int memory_len) {
  unsigned long t = 0;
  unsigned long data = 0;
  //byte output_stream[memory_len + 12];
  
  if(address < 256) {
    if(register_time[address] == 0)
      register_time[address] = millis();
    t = register_time[address];
  } else {
    t = millis();
  }

  //sendPreamble();
  int i = 0;

  
  int packet_len = 8 + (memory_len * 4);
/*
  output_stream[i++] = 0x55;
  output_stream[i++] = 0x55;
  output_stream[i++] = 0x55;
  output_stream[i++] = 0x55;
  output_stream[i++] = (byte)((packet_len >> 8) & 0xFF);
  output_stream[i++] = (byte)(packet_len & 0xFF);
  output_stream[i++] = MEMORY_READ_MESSAGE;
  output_stream[i++] = (byte)((t >> 24) & 0xFF);
  output_stream[i++] = (byte)((t >> 16) & 0xFF);
  output_stream[i++] = (byte)((t >> 8) & 0xFF);
  output_stream[i++] = (byte)((t) & 0xFF); 
  output_stream[i++] = (byte)((address >> 8) & 0xFF);
  output_stream[i++] = (byte)((address) & 0xFF);

  for(int j = 0; j < memory_len; j++) {
    data = memory_contents[j];
    output_stream[i++] = (byte)((data >> 24) & 0xFF);
    output_stream[i++] = (byte)((data >> 16) & 0xFF);
    output_stream[i++] = (byte)((data >> 8) & 0xFF);
    output_stream[i++] = (byte)((data) & 0xFF);
  }  

  Serial.write(output_stream, i);
  */
  Serial.write((byte)((packet_len >> 8) & 0xFF));
  Serial.write((byte)(packet_len & 0xFF));
  Serial.write(MEMORY_READ_MESSAGE);
  Serial.write((byte)((t >> 24) & 0xFF));
  Serial.write((byte)((t >> 16) & 0xFF));
  Serial.write((byte)((t >> 8) & 0xFF));
  Serial.write((byte)((t) & 0xFF));  
  Serial.write((byte)((address >> 8) & 0xFF));
  Serial.write((byte)((address) & 0xFF));
  for(int i = 0; i < memory_len; i++) {
    data = memory_contents[i];
    Serial.write((byte)((data >> 24) & 0xFF));
    Serial.write((byte)((data >> 16) & 0xFF));
    Serial.write((byte)((data >> 8) & 0xFF));
    Serial.write((byte)((data) & 0xFF));
  }  

  if(address < 256) {
    register_time[address] = 0;
  }
  //debug("No crash");
}

void sendAnalogReadMessage(int pin, int value) {
  //debug("Analog");
  unsigned long t = 0;
  sendPreamble();
  t = millis();
  Serial.write((byte)0);
  Serial.write(9);
  Serial.write(ANALOG_READ_MESSAGE);
  Serial.write((byte)((t >> 24) & 0xFF));
  Serial.write((byte)((t >> 16) & 0xFF));
  Serial.write((byte)((t >> 8) & 0xFF));
  Serial.write((byte)((t) & 0xFF));
  Serial.write((byte)pin);
  Serial.write((byte)((value >> 8) & 0xFF));
  Serial.write((byte)((value) & 0xFF));
}

void debug(String message) {
  #ifdef DEBUG
  sendAsciiMessage(message);
  #endif  
}

void sendAsciiMessage(String message) {
  sendPreamble();
  Serial.write(0);
  Serial.write(message.length() + 6);
  Serial.write(ASCII_MESSAGE);
  unsigned long t = millis();
  Serial.write((byte)((t >> 24) & 0xFF));
  Serial.write((byte)((t >> 16) & 0xFF));
  Serial.write((byte)((t >> 8) & 0xFF));
  Serial.write((byte)((t) & 0xFF));
  Serial.println(message);
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
