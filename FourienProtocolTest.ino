// Runs on the teensy
// Walter Gordy
// Fourien, Inc

#define HWSERIAL Serial1
#include <CircularBuffer.h>
CircularBuffer<char, 255> buffer;

#define DEBUG 1


#define PREAMBLE            0x55
#define PREAMBLE_COUNT      4   // must recieve 4 preambles in a row to start a packet
#define DIGITAL_IO_MESSAGE  0x0E
#define ASCII_MESSAGE       'A'

void setup() {
  // Run usb serial at 8mb/s 
  Serial.begin(8000000);

  pinMode(13, OUTPUT);
  digitalWrite(13, 1);
  
}

int packet_flag = 0;
byte data = 0;
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
  sendAsciiMessage("Parsing Buffer");
  char len = buffer.shift();
  char type = buffer.shift();
  char pin;
  char val;
  
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

      
    case '1':
      sendAsciiMessage("Packet 1");
      pin = buffer.shift();
      val = buffer.shift();

      if(pin == '2'){
        sendAsciiMessage("Pin 2");
        if(val == '0'){
          sendAsciiMessage("Off");
          digitalWrite(13, 0);
        }
        if(val == '1'){
          sendAsciiMessage("On");
          digitalWrite(13, 1);
        }
      break; // case '1'
    }
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

















