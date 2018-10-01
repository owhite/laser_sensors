// bridge between linux box and other I2C devices.
// the bridge serves as a slave to the serial input, and master to I2C devices. 
// using this strategy because so far the rtai OS on the linux
//  side bombs when it makes I2C requests directly. 

#include <i2c_t3.h>
#include <Throb.h>

#define SERIAL_BAUD 115200

#define PACKET_LEN        6 

// commands sent to and from bridge
#define HELLO             1 // wake up the bridge
#define ALREADY_CONNECTED 2 // checking bridge is woke
#define ERROR             4 // report back 'never heard of last command'
#define WRITE_DEVICE      5 // send data to I2C device
#define READ_DEVICE       6 // read data from I2C device

// byte positions of packets that come in
// all returned packets terminate with '>'
#define CMD            1 // incoming command from master
#define RESPONSE       1 // response from bridge in returning packet
#define DEVICE_ADDRESS 2 // this position is address of device
#define DEVICE_SEND    3 // starting byte for data returned by devices

#define MCU_PIN    23
#define LED_PIN    22

const uint8_t numChars = 24;
uint32_t lastReceive   = 0;
uint8_t serialBuf[numChars]; // used for serial communcations
uint8_t deviceBuf[numChars]; // used for I2C communications
uint8_t packetLen;
boolean collecting = false;
boolean is_connected = false;
boolean packetComplete = false;
uint8_t order_received = 0;
uint8_t idx = 0;
uint8_t i;
uint8_t device = 0;

Throb throb(LED_PIN);

void setup(){
  Serial1.begin(SERIAL_BAUD);

  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
  Wire.setDefaultTimeout(200000); 

  pinMode(MCU_PIN, OUTPUT); // if this teensy is woke, then turn this on
  digitalWrite(MCU_PIN, LOW);

  pinMode(LED_PIN, OUTPUT); // if this teensy is woke, then turn this on
  digitalWrite(LED_PIN, LOW);
}

boolean packetReceived() {
  packetComplete = false;
  packetLen = 0;

  // incoming packets can be of variable length
  while (Serial1.available() > 0) {
    uint8_t rc = Serial1.read();
    if (idx > numChars - 1) {
      idx = 0;
    }
    if (rc == '>') {
      serialBuf[idx] = '>';
      serialBuf[idx + 1] = '\0';
      packetLen = idx + 1;
      idx = 0;
      if (serialBuf[0] == '<') {packetComplete = true;}
      collecting = false;
    }
    if (rc == '<') {
      idx = 0;
      collecting = true;
    }
    if (collecting) { // only collect when you're between '<' and '>'
      serialBuf[idx] = rc; 
      serialBuf[idx + 1] = '\0';
      idx++;
    }
  }

  return(packetComplete);
}

void loop(){
  throb.pulseOnTimer(lastReceive); // if master is sending packets keep LED beating 

  if (packetReceived()) {
    lastReceive = millis();

    order_received =  serialBuf[CMD];

    switch(order_received) { // all case statements based on commands from master
    case HELLO: 
      // dont proceed until master sends hello
      if(!is_connected) { 
	is_connected = true;
	serialBuf[RESPONSE] = HELLO;
      }
      else {
	// no more hellos, tell master the bridge is active
	serialBuf[RESPONSE] = ALREADY_CONNECTED;
      }
      serialBuf[RESPONSE + 1] = '>'; // all response terminate with '>'
      break;
    case ALREADY_CONNECTED: 
      // tell master the bridge is active
      serialBuf[RESPONSE] = order_received;
      serialBuf[RESPONSE + 1] = '>'; 
      break;
    case WRITE_DEVICE: // write data to slave devices
      serialBuf[RESPONSE] = order_received;
      device = serialBuf[DEVICE_ADDRESS];

      i = 0;
      while (serialBuf[DEVICE_SEND + i] != '>') {
	deviceBuf[i] = serialBuf[DEVICE_SEND + i];
	deviceBuf[i+1] = '\0';
	i++;
      }

      Wire.beginTransmission(device);  
      Wire.write(deviceBuf,i);
      Wire.endTransmission();        

      // report to master that it ... 
      if(Wire.getError()) serialBuf[RESPONSE + 1] = 0; // broke, or...
      else serialBuf[RESPONSE + 1] = 1;                // was ok

      serialBuf[RESPONSE + 2] = '>'; 
      packetLen = 4;

      break;
    case READ_DEVICE: // request data from slave devices
      serialBuf[RESPONSE] = order_received;
      device = serialBuf[DEVICE_ADDRESS];

      Wire.requestFrom(device, PACKET_LEN);
      if(Wire.getError()) { // report to master it didnt work. 
	serialBuf[RESPONSE + 1] = 0;
	serialBuf[RESPONSE + 2] = '>'; 
	packetLen = 4;
      }
      else { // ok
	Wire.read(deviceBuf, Wire.available());
	serialBuf[RESPONSE + 1] = 1;
	for (i = 0; i < PACKET_LEN; i++) { // returned packet is always same fixed length
	  serialBuf[DEVICE_ADDRESS + i + 1] = deviceBuf[i];
	  serialBuf[DEVICE_ADDRESS + i + 2] = '>'; 
	}
	packetLen += i; // ** ?? BE SURE TO CHECK: SHOULD THIS NOT BE += i + 1 ?? **
      }

      break;
    default: // received an unknown command from master
      serialBuf[RESPONSE] = ERROR; 
      serialBuf[RESPONSE + 1] = '>'; 
      return;
    }

    // only writes back to master if a packet was received
    //  to avoid filling up the master's serial buffer
    Serial1.write((uint8_t*) serialBuf, packetLen); 
    // hopefully master will not send packets unless bridge says it's up
    //  to avoid filling up the bridge's serial buffer
  }
}
