// this slave sends signals to a bank of relays
// slave only receives input. no output is sent back
// one requirement - it shuts off all relays unless it
//  it receives a periodic signal from master

#include <i2c_t3.h>
#include <Sound.h>
#include <Throb.h>
#include <laser_systems.h>

#define S_IDLE                1
#define S_SETPINS             3
#define S_MAKE_ACTIVATE_SOUND 8
#define S_SHUTDOWN            9

#define SPK_PIN      20
#define MCU_PIN      14
#define LED_PIN      13
#define OUTPUT_RANGE 12 // total number of relays

int state = S_IDLE;

Sound sound(SPK_PIN);
Throb throb(LED_PIN);

void receiveDataPacket(size_t howMany){
  masterCommand = Wire.read();          

  inputSize = howMany;
  inputBuf[0] = '\0';
  int i;
  for (i = 0; i < howMany - 1; i++) {
    inputBuf[i] = Wire.read();
    if (i > 19) { break; }
  }
  inputBuf[i+1] = '\0';

  switch (masterCommand) {
  case CMD_DONOTHING:
    state = S_IDLE;
    break;
  case CMD_SETPINS:
    state = S_SETPINS;
    break;
  case CMD_ACTIVATE:
    state = S_MAKE_ACTIVATE_SOUND;
    break;
  case CMD_SHUTDOWN:
    state = S_SHUTDOWN;
    break;
  default:
    // unknown state
    break;
  }

  masterCommand = CMD_DONOTHING;    
  lastReceive = millis();
}

void requestEvent() { Wire.write(outputBuf,PACKET_LEN); }

void setup() {
  Serial.begin(SERIAL_SPEED);

  outputBuf[0] = PACKET_START;   outputBuf[5] = PACKET_STOP;
  outputBuf[1] = 0; outputBuf[2] = 0;
  outputBuf[3] = 0;  outputBuf[4] = 0;

  for(int i=0; i < OUTPUT_RANGE; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  pinMode(MCU_PIN, OUTPUT);
  digitalWrite(MCU_PIN, LOW);

  Wire.begin(I2C_SLAVE, RELAYDRIVER_SLAVE_ADDRESS, I2C_PINS_18_19,
	     I2C_PULLUP_EXT, I2C_SPEED);
  Wire.onReceive(receiveDataPacket); 
  Wire.onRequest(requestEvent);
}

void loop() {
  if (throb.pulseOnTimer(lastReceive) == false) {
    // no signal, then shut off all relays
    for(int i=0; i < OUTPUT_RANGE; i++){
      digitalWrite(i, LOW);
    }
  }

  switch (state) {
  case S_IDLE:
    state = S_IDLE;
    break;
  case S_SETPINS:
    // input buf has to be the right length
    // to set the pins
    if (OUTPUT_RANGE + 1 == inputSize) {
      for(int i = 0; i < inputSize; i++) {
	// inputbuf has to have the right values to set LED
	digitalWrite(i, (inputBuf[i] == 1) ? HIGH : LOW);
      }
    }
    state = S_IDLE;
    break;
  case S_MAKE_ACTIVATE_SOUND:
    sound.ray_gun();
    state = S_IDLE;
    break;
  case S_SHUTDOWN:
    // the master can also send a shutdown
    sound.tweetOFF();
    for(int i=0; i < OUTPUT_RANGE; i++){
      digitalWrite(i, LOW);
    }
    state = S_IDLE;
    break;
  default:
    // unknown state
    break;
  }
}



