#include <i2c_t3.h>
#include <Sound.h>
#include <Throb.h>

#define SERIAL_BAUD 115200

#define PACKET_LEN        6

#define HELLO             1
#define ALREADY_CONNECTED 2
#define SHUTDOWN          3
#define ERROR             4
#define WRITE_DEVICE      5 // from here down are I2C commands
#define READ_DEVICE       6
#define SET_DEVICE        7

#define POS_CMD            1
#define POS_RESPONSE       1
#define POS_DEVICE_ADDRESS 2
#define POS_DEVICE_SEND1   3
#define POS_DEVICE_SEND2   4

#define MCU_PIN    13
#define LED_PIN    14
#define SpkrPin    9

const uint8_t numChars = 24;
uint32_t lastReceive   = 0;
uint8_t inBuffer[numChars];
uint8_t sendBuffer[numChars];
uint8_t packetLen;
boolean collecting = false;
boolean is_connected = false;
boolean packetComplete = false;
uint8_t order_received = 0;
uint8_t idx = 0;
uint8_t i;

uint8_t device = 0;
uint8_t cmd = 0;

Sound sound(SpkrPin);
Throb throb(LED_PIN);


void setup(){
  Serial.begin(SERIAL_BAUD);

  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
  Wire.setDefaultTimeout(200000); // 200ms

  pinMode(MCU_PIN, OUTPUT);
  digitalWrite(MCU_PIN, LOW);
}

boolean packetReceived() {
  packetComplete = false;
  packetLen = 0;

  while (Serial.available() > 0) {
    uint8_t rc = Serial.read();
    if (idx > numChars - 1) {
      idx = 0;
    }
    if (rc == '>') {
      inBuffer[idx] = '>';
      inBuffer[idx + 1] = '\0';
      packetLen = idx + 1;
      idx = 0;
      if (inBuffer[0] == '<') {packetComplete = true;}
      collecting = false;
    }
    if (rc == '<') {
      idx = 0;
      collecting = true;
    }
    if (collecting) {
      inBuffer[idx] = rc;
      inBuffer[idx + 1] = '\0';
      idx++;
    }
  }

  return(packetComplete);
}

void loop(){
  throb.pulseOnTimer(lastReceive);

  if (packetReceived()) {
    lastReceive = millis();

    order_received =  inBuffer[POS_CMD];

    switch(order_received) {
    case HELLO: 
      // until master sends hello, dont proceed
      if(!is_connected) { 
	is_connected = true;
	inBuffer[POS_RESPONSE] = HELLO;
      }
      else {
	// already connected dont send "hello", avoid infinite loop
	inBuffer[POS_RESPONSE] = ALREADY_CONNECTED;
      }
      inBuffer[POS_RESPONSE + 1] = '>'; // close this for each case
      break;
    case ALREADY_CONNECTED: 
      inBuffer[POS_RESPONSE] = order_received;
      inBuffer[POS_RESPONSE + 1] = '>'; 
      break;
    case SHUTDOWN: 
      inBuffer[POS_RESPONSE] = order_received;
      inBuffer[POS_RESPONSE + 1] = '>'; 
      break;
    case WRITE_DEVICE:
      inBuffer[POS_RESPONSE] = order_received;
      device = inBuffer[POS_DEVICE_ADDRESS];

      i = 0;
      while (inBuffer[POS_DEVICE_SEND1 + i] != '>') {
	sendBuffer[i] = inBuffer[POS_DEVICE_SEND1 + i];
	sendBuffer[i+1] = '\0';
	i++;
      }

      Wire.beginTransmission(device);  
      Wire.write(sendBuffer,i);
      Wire.endTransmission();        

      if(Wire.getError()) inBuffer[POS_RESPONSE + 1] = 0; // sad
      else inBuffer[POS_RESPONSE + 1] = 1; // ok

      inBuffer[POS_RESPONSE + 2] = '>'; 
      packetLen = 4;

      break;
    case READ_DEVICE: 
      inBuffer[POS_RESPONSE] = order_received;
      device = inBuffer[POS_DEVICE_ADDRESS];

      Wire.requestFrom(device, PACKET_LEN);
      if(Wire.getError()) { // sad
	inBuffer[POS_RESPONSE + 1] = 0;
	inBuffer[POS_RESPONSE + 2] = '>'; 
	packetLen = 4;
      }
      else { // ok
	Wire.read(inBuffer, Wire.available());
	inBuffer[POS_RESPONSE + 1] = 1;
	for (i = 0; i < PACKET_LEN; i++) {
	  inBuffer[POS_DEVICE_ADDRESS + i + 1] = i;
	  inBuffer[POS_DEVICE_ADDRESS + i + 2] = '>'; 
	}
	packetLen += i;
      }

      break;
    case SET_DEVICE:
      inBuffer[POS_RESPONSE] = order_received;
      inBuffer[POS_RESPONSE + 3] = '>'; 

      device = inBuffer[POS_DEVICE_ADDRESS];
      cmd    = inBuffer[POS_DEVICE_SEND1];

      break;
    default:
      inBuffer[POS_RESPONSE] = ERROR;
      inBuffer[POS_RESPONSE + 1] = '>'; 
      return;
    }

    Serial.write((uint8_t*) inBuffer, packetLen);
  }
}
