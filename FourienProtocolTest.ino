// Runs on the teensy

#define HWSERIAL Serial1
#include <CircularBuffer.h>
CircularBuffer<char, 255> buffer;

#define PREAMBLE            0x55
#define PREAMBLE_COUNT      4 // must see 4 preambles in a row to start a packet
#define START_BYTE          0xF0
#define END_BYTE            0xF7
#define DIGITAL_IO_MESSAGE  0x0E

void setup() {
  // Run usb serial at 8mb/s 
  Serial.begin(4000000);
}

int packet_flag = 0;
byte data = 0;
int incoming_byte;
int expected_size = 2;
int preamble_count = 0;

void loop() {
  serviceIncomingSerial(); 
}

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
  Serial.println("Parsing Buffer");
  while(buffer.size() > 0) {
    Serial.print(buffer.shift());
  }
  Serial.println("");
}
