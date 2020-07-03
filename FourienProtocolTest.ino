// Runs on the teensy
// Walter Gordy
// Fourien, Inc

#define HWSERIAL Serial1
#include <CircularBuffer.h>
CircularBuffer<char, 255> buffer;

#define DEBUG 1

#define VERSION                "Ver 0.011 2020 07 03"
#define PREAMBLE               0x55
#define PREAMBLE_COUNT         4   // must recieve 4 preambles in a row to start a packet
#define DIGITAL_IO_MESSAGE     0x0E
#define ASCII_MESSAGE          'A'
#define VERSION_MESSAGE        85
#define MEMORY_READ_MESSAGE    0x10
#define MEMORY_WRITE_MESSAGE   0x20


unsigned long register_memory[255]; // 

void setup() {
  // Run usb serial at 8mb/s 
  Serial.begin(2000000);
  sendAsciiMessage(VERSION_MESSAGE);
  pinMode(13, OUTPUT);
  digitalWrite(13, 1);
  
}

int packet_flag = 0;

int incoming_byte;
int expected_size = 2;
int preamble_count = 0;

void loop() {
  serviceIncomingSerial(); 
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
      
      if(buffer.size() > 0) {
        if(buffer.size() == buffer[0]){
          parseBuffer();
        }
      }
    }
  }
}

void parseBuffer() {
  debug("Parsing Buffer");
  char len = buffer.shift();
  char type = buffer.shift();

  debug(String(len, DEC));
  debug(String(type, DEC));
  char pin;
  char val;

  unsigned int address = 0;
  unsigned long   data = 0;
  
  switch(type) {
    case DIGITAL_IO_MESSAGE:
      pin = buffer.shift();
      val = buffer.shift();
      debug("Digital IO");
      debug(String(pin, DEC));
      debug(String(val, DEC));
      if(val != 0)
        val = 1;
      if(pin == 13)
        digitalWrite(pin, val);
      break; // DIGITAL_IO_MESSAGE

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
      register_memory[address] = data;
      data = register_memory[address];
      debug("Set Register");
      debug(String(address, DEC));
      debug(String(data, DEC));
      break;

    case MEMORY_READ_MESSAGE:
      debug("Read memory");
      address = (unsigned int)buffer.shift() << 8;
      address |= (unsigned int)buffer.shift();
      address &= 0xFFFF;
      data = register_memory[address];
      sendPreamble();

      Serial.write(7);
      Serial.write(MEMORY_READ_MESSAGE);
      Serial.write((byte)((address >> 8) & 0xFF));
      Serial.write((byte)((address) & 0xFF));
      Serial.write((byte)((data >> 24) & 0xFF));
      Serial.write((byte)((data >> 16) & 0xFF));
      Serial.write((byte)((data >> 8) & 0xFF));
      Serial.write((byte)((data) & 0xFF));
      
      debug(String(address, DEC));
      debug(String(data, DEC));
      break;
  }
  
  buffer.clear();
}

void sendPreamble(){
  for(int i = 0; i < PREAMBLE_COUNT; i++)
    Serial.write(PREAMBLE);
}

void debug(String message) {
  #ifdef DEBUG
  sendAsciiMessage(message);
  #endif  
}

void sendAsciiMessage(String message) {
  sendPreamble();
  Serial.write(message.length() + 1);
  Serial.write(ASCII_MESSAGE);
  Serial.println(message);
} 
