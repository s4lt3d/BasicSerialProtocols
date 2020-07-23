// This is an extra library that will need to be installed. V1.1.3 by Agileware.  
#include <CircularBuffer.h>

#ifndef __FOURIENCOMM_H
#define __FOURIENCOMM_H

#define PREAMBLE               0x55
#define PREAMBLE_COUNT         4   // must recieve 4 preambles in a row to start a packet
#define ASCII_MESSAGE          'A' 
#define VERSION_MESSAGE        'V'
#define MEMORY_READ_MESSAGE    0x10
#define MEMORY_WRITE_MESSAGE   0x20

void serviceIncomingSerial();
void parseBuffer();
void sendPreamble();
void sendMemoryMessage(unsigned int address, unsigned long data, unsigned long millis_time = 0);
void burstMemoryMessage(unsigned int address, unsigned long memory_contents[], int memory_len, unsigned long millis_time);
void debug(String message);
void sendAsciiMessage(String message);
void sendVersion(String ver);

int incoming_byte;
int expected_size = 2;
int preamble_count = 0;

CircularBuffer<char, 2048> buffer;

void (*memory_write_callback)(unsigned int, unsigned long);
void (*memory_read_callback)(unsigned int);

void setMemoryWriteCallback(void (*f)(unsigned int, unsigned long))
{
  memory_write_callback = f;
}

void setMemoryReadCallback(void (*f)(unsigned int))
{
  memory_read_callback = f;
}

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
  //debug(String(type, 10));
  switch(type) {
    
    case VERSION_MESSAGE:
      
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
      
      memory_write_callback(address,data);
      
      break;

    case MEMORY_READ_MESSAGE:
      //debug("Read memory");
      address = (unsigned int)buffer.shift() << 8;
      address |= (unsigned int)buffer.shift();
      address &= 0xFFFF;      
      memory_read_callback(address);
      
      
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
void sendMemoryMessage(unsigned int address, unsigned long data, unsigned long millis_time = 0) {

  if(millis_time == 0)
    millis_time = millis();
  
  unsigned long mem[1];
  mem[0] = data;
  burstMemoryMessage(address, mem, 1, millis_time);
}

// Memory Read will include time in milliseconds
void burstMemoryMessage(unsigned int address, unsigned long memory_contents[], int memory_len, unsigned long millis_time) {
  sendPreamble();
  
  if(millis_time == 0)
    millis_time = millis();
  
  int i = 0;
  unsigned long data;

  int packet_len = 8 + (memory_len * 4);

  Serial.write((byte)((packet_len >> 8) & 0xFF));
  Serial.write((byte)(packet_len & 0xFF));
  Serial.write(MEMORY_READ_MESSAGE);
  Serial.write((byte)((millis_time >> 24) & 0xFF));
  Serial.write((byte)((millis_time >> 16) & 0xFF));
  Serial.write((byte)((millis_time >> 8) & 0xFF));
  Serial.write((byte)(millis_time & 0xFF));  
  Serial.write((byte)((address >> 8) & 0xFF));
  Serial.write((byte)((address) & 0xFF));
  for(int i = 0; i < memory_len; i++) {
    data = memory_contents[i];
    Serial.write((byte)((data >> 24) & 0xFF));
    Serial.write((byte)((data >> 16) & 0xFF));
    Serial.write((byte)((data >> 8) & 0xFF));
    Serial.write((byte)((data) & 0xFF));
  }  
  Serial.write(0);
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
  Serial.write(0);
}

void sendVersion(String ver)
{
  sendAsciiMessage(ver);  
}

#endif //__FOURIENCOMM_H
