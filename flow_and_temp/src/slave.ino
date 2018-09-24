// this slave measures blips coming from flow sensor and reads temperature

#include <i2c_t3.h>
#include <Sound.h>
#include <Throb.h>
#include <laser_systems.h>

// states defined from commands that come in from master
#define S_IDLE                1 // no action required
#define S_MAKE_ACTIVATE_SOUND 8 // make sound when master requests it
#define S_DEACTIVATE_SOUND    9 // make a different sound when master requests it

#define MCU_PIN 14
#define LED_PIN    13
#define SPK_PIN    5
#define TEMP_PIN      17
#define WATER_PIN     21

#define NUM_LEDS      24
#define PALETTE_RANGE 56

int temp;
float centigrade_reading;
unsigned long currentMillis;
long previousMillis = 0;  
long interval = 1000; 

int current_water_count = 0;
int water_count = 0;

int state = S_IDLE;

byte addr[2][8];

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
  inputBuf[i+1] = '\0'; // guess what? we dont use inputBuf

  switch (masterCommand) {
  case CMD_DONOTHING:
    state = S_IDLE;
    break;
  case CMD_ACTIVATE:
    state = S_MAKE_ACTIVATE_SOUND;
    break;
  case CMD_DEACTIVATE:
    state = S_DEACTIVATE_SOUND;
    break;
  default:
    // unknown state
    break;
  }

  masterCommand = CMD_DONOTHING;    
  lastReceive = millis();
}

// always send data, regardless of state
void requestEvent() {Wire.write(outputBuf,PACKET_LEN); lastReceive = millis();}

// dis bumpy dee counter for the water flow
void isrService() {
  cli();
  current_water_count += 1;
  sei();
}

double thermister(int RawADC) {
 double Temp;
 Temp = log(10000.0*((1024.0/RawADC-1))); 
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;            // Convert Kelvin to Celcius
 Temp = (Temp - 32)/ (9.0 / 5.0);
 return Temp;
}


void setup() {
  Serial.begin(SERIAL_SPEED);

  attachInterrupt(WATER_PIN, isrService, FALLING);

  pinMode(MCU_PIN, OUTPUT);
  digitalWrite(MCU_PIN, LOW);

  Wire.begin(I2C_SLAVE, FLOW_SLAVE_ADDRESS, I2C_PINS_18_19,
	     I2C_PULLUP_EXT, I2C_SPEED);
  Wire.onReceive(receiveDataPacket); 
  Wire.onRequest(requestEvent);
}

void loop() {
  throb.pulseOnTimer(lastReceive);

  currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;   
    water_count = current_water_count;
    current_water_count = 0;

    int val=analogRead(TEMP_PIN);
    double temp = thermister(val); 

    char result[5];
    dtostrf(temp, 5, 2, result); 

    outputBuf[0] = PACKET_START;   outputBuf[5] = PACKET_STOP;
    outputBuf[1] = 7; outputBuf[2] = 7;
    outputBuf[1] = (result[0] * 10) + result[1];
    outputBuf[2] = (result[3] * 10) + result[4];
    outputBuf[3] = lowByte(water_count);
    outputBuf[4] = highByte(water_count);

    outputBuf[0] = PACKET_START;   outputBuf[5] = PACKET_STOP;
    outputBuf[1] = 6; outputBuf[2] = 7;
    outputBuf[3] = 8; outputBuf[4] = 9;

  }

  switch (state) {
  case S_IDLE:
    state = S_IDLE;
    break;
  case S_MAKE_ACTIVATE_SOUND:
    sound.star_trek1();
    state = S_IDLE;
    break;
  case S_DEACTIVATE_SOUND:
    sound.tweetOFF();
    state = S_IDLE;
    break;
  default:
    // unknown state
    break;
  }
}
