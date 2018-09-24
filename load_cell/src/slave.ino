// slave to monitor load cells. load cells detect if something
//   is pushing on a laser optic and complains if there
//   is too much pressure. The program also rotates a stepper motor when the user
//   hits a switch.

#include <i2c_t3.h>
#include <RunningAverage.h>
#include <SPI.h>
#include <laser_systems.h>
#include <Sound.h>
#include <Throb.h>

#define S_IDLE                1
#define S_MOTOR_INIT          2
#define S_MOTOR_RUN           3
#define S_MOTOR_OFF           4
#define S_AUTO_MOTOR_INIT     5
#define S_AUTO_MOTOR_RUN      6
#define S_CALIBRATE           7
#define S_MAKE_ACTIVATE_SOUND 8
#define S_DEACTIVATE_SOUND    9
#define S_REPORT              10

#define FORWARD    1
#define REVERSE    3

#define MCU_PIN    13
#define LED_PIN    14
#define DIR_PIN    2
#define STEP_PIN   3
#define GECKOENBL  4
#define SPKR_PIN   9
#define PB_PIN     6

#define LED1_r     5
#define LED1_g     8
#define LED2_r     23
#define LED2_g     22
#define LED3_r     21
#define LED3_g     20

#define SW_PIN1    10
#define SW_PIN2    11

#define LC_PIN1    15
#define LC_PIN2    16
#define LC_PIN3    17


int state = S_IDLE;
int auto_motor_run_time = 0;
int motorCount = 0;
float decayN = 0.0;
int decayTime = 0;
int direction = 0;
int LC1 = 0; int LC2 = 0; int LC3 = 0;

// sampling parameters
int RAsamples = 500;        // number of samples in rolling buffer
int delta = 18;             // degree of change in rolling buffer to prompt a response
int averageThreshold = 30;  // must be over this level to be counted
int touchThreshold =   80;  // level where lights go green
int compressed_counter = 0;

// parameters used to ramp up the motor slowly. 
// this create a decay function to eat away at the number of cycles
// that serve as a delay to pulse the motor. this is
// a great way to have the motor slowly ramp up in speed. 
// these parameters will make no sense unless you plot them. 
int accelTime  = 600;       // Number of loops to arrive at velocity
int startDelay = 8000;	    // Number of atoms at t = 0, starting velocity
int finalDelay = 500;       // Number of atoms to decay to, ending velocity
float decayConstant = .02;  // Impacts the degree of decay at each step

// motor variables
int rotF = HIGH; // pin setting to rotate motor forward
int rotR = LOW;

// decay function variables
int leftSpan;            
int rightSpan;
int endPoint;

int sw1; // measure the switch status
int sw2; 
int pb;

Sound sound(SPKR_PIN);
Throb throb(LED_PIN);

// silence the squealer from talking
int silenceFlag = 0;
// if the device is queried, send bad news
int badnewssinceQuery = 0;

RunningAverage RA1(RAsamples);
RunningAverage RA2(RAsamples);
RunningAverage RA3(RAsamples);

void initDecay() {
  // find final decay point using the above parameters
  float N = float(startDelay);
  int t;

  for (t = 0; t < accelTime; t++) {
    N = N - (decayConstant * N);
    t += 1;
  }

  int endPoint = int(N);

  // The issue is the decay of our atoms may be much larger or smaller
  // than what we want. But presumably we love the rate. So
  // Scale our final end point to our desired endpoint
  // do this by getting ranges
  leftSpan = startDelay - endPoint;
  rightSpan = startDelay - finalDelay;
}

int scaleDecay(float N) {
  // scale our current value to our desired endpoint
  float s = (N - float(endPoint)) / float(leftSpan);

  return (int(finalDelay + (s * rightSpan)));
}

void LED_OFF (int L_r, int L_g) {
  digitalWrite(L_r, HIGH);
  digitalWrite(L_g, HIGH);
  pinMode(L_r, INPUT);
  pinMode(L_g, INPUT);
}

void LED_GO (int L_r, int L_g) {
  pinMode(L_r, OUTPUT);
  pinMode(L_g, OUTPUT);
  digitalWrite(L_r, LOW);
  digitalWrite(L_g, HIGH);
}

void LED_STOP (int L_r, int L_g) {
  pinMode(L_r, OUTPUT);
  pinMode(L_g, OUTPUT);
  digitalWrite(L_r, HIGH);
  digitalWrite(L_g, LOW);
}

void setup() {
  Serial.begin(SERIAL_SPEED);

  Wire.begin(I2C_SLAVE, LOADCELL_SLAVE_ADDRESS, I2C_PINS_18_19,
	     I2C_PULLUP_EXT, I2C_SPEED);
  Wire.onReceive(receiveDataPacket); 
  Wire.onRequest(requestEvent);

  initDecay();

  RA1.clear(); RA2.clear(); RA3.clear();

  analogReference(EXTERNAL);

  pinMode(MCU_PIN, OUTPUT);
  digitalWrite(MCU_PIN, LOW);

  pinMode(SW_PIN1, INPUT);
  pinMode(SW_PIN2, INPUT);
  pinMode(PB_PIN, INPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(GECKOENBL, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(LED1_r, OUTPUT);
  pinMode(LED1_g, OUTPUT);
  pinMode(LED2_r, OUTPUT);
  pinMode(LED2_g, OUTPUT);
  pinMode(LED3_r, OUTPUT);
  pinMode(LED3_g, OUTPUT);

  digitalWrite(STEP_PIN, HIGH);
  digitalWrite(DIR_PIN, HIGH);
  digitalWrite(GECKOENBL, LOW);
  digitalWrite(LED1_r, HIGH);
  digitalWrite(LED1_g, HIGH);
  digitalWrite(LED2_r, HIGH);
  digitalWrite(LED2_g, HIGH);
  digitalWrite(LED3_r, HIGH);
  digitalWrite(LED3_g, HIGH);
}

void receiveDataPacket(size_t howMany){
  masterCommand = Wire.read();          

  inputSize = howMany;
  inputBuf[0] = '\0';
  int i;
  // inputbuf not used
  //  we're just being polite by collecting data
  for (i = 0; i < howMany - 1; i++) {
    inputBuf[i] = Wire.read();
    if (i > 19) { break; }
  }
  inputBuf[i+1] = '\0';

  switch (masterCommand) {
  case CMD_DONOTHING:
    state = S_IDLE;
    break;
  case CMD_RESET:
    badnewssinceQuery = 0; // keep sending a complaint until master
    state = S_IDLE;        //  sends a reset
    break;
  case CMD_REPORT:
    state = S_REPORT;
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

  lastReceive = millis();
  masterCommand = CMD_DONOTHING;    
}

void requestEvent() {
  outputBuf[1] = badnewssinceQuery;
  Wire.write(outputBuf,PACKET_LEN); 
  lastReceive = millis();
}

void loop() {
  throb.pulseOnTimer(lastReceive);

  outputBuf[0] = PACKET_START;   outputBuf[5] = PACKET_STOP;
  outputBuf[1] = 0;  outputBuf[2] = 0;
  outputBuf[3] = 0;  outputBuf[4] = 0;

  sw1 = digitalRead(SW_PIN1);
  sw2 = digitalRead(SW_PIN2);
  if (state != S_MOTOR_RUN) {
    if (sw1 == HIGH) {
      delay(20);
      state = S_MOTOR_INIT;
      direction = FORWARD;
    }
    if (sw2 == HIGH) {
      delay(20);
      state = S_MOTOR_INIT;
      direction = REVERSE;
    }
  }

  switch (state) {
  case S_MOTOR_INIT:
    if (direction == FORWARD) {
      LED_GO(LED3_r, LED3_g);
      digitalWrite(DIR_PIN, rotR);
    }
    if (direction == REVERSE) {
      LED_GO(LED1_r, LED1_g);
      digitalWrite(DIR_PIN, rotF);
    }
    digitalWrite(GECKOENBL, HIGH); // fire up the motor driver
    motorCount = 0;
    decayN = float(startDelay);
    decayTime = 0;
    state = S_MOTOR_RUN;
    break;
  case S_MOTOR_RUN:
    if (motorCount < accelTime) {
      decayN = decayN - (decayConstant * decayN);
      decayTime = scaleDecay(decayN);
    }
    else {
      decayTime = finalDelay;
    }

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(decayTime); 
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(decayTime); 
    motorCount += 1;
    if (sw1 != HIGH && sw2 != HIGH) {
      state = S_MOTOR_OFF;
    }

    break;
  case S_AUTO_MOTOR_INIT:
    if (direction == FORWARD) {
      LED_GO(LED3_r, LED3_g);
      digitalWrite(DIR_PIN, rotR);
    }
    if (direction == REVERSE) {
      LED_GO(LED1_r, LED1_g);
      digitalWrite(DIR_PIN, rotF);
    }
    digitalWrite(GECKOENBL, HIGH); // fire up the motor driver
    motorCount = 0;
    decayN = float(startDelay);
    decayTime = 0;
    state = S_AUTO_MOTOR_RUN;
    break;
  case S_AUTO_MOTOR_RUN:
    if (motorCount < accelTime) {
      decayN = decayN - (decayConstant * decayN);
      decayTime = scaleDecay(decayN);
    }
    else {
      decayTime = finalDelay;
    }
    motorCount += 1;

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(decayTime); 
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(decayTime); 

    auto_motor_run_time -= 1;

    if (auto_motor_run_time == 0) {
      state = S_MOTOR_OFF;
    }

    break;

  case S_CALIBRATE:
    // ** CALIBRATE IS DEACTIVATED. MUST FIX BY TESTING STATUS OF PB_PIN **

    auto_motor_run_time = 0;
    // see if load cells are receiving a certain amount of pressure. 
    LC1 = analogRead(LC_PIN1);  LC2 = analogRead(LC_PIN2);  LC3 = analogRead(LC_PIN3);

    if (LC1 > touchThreshold) { // go green when above threshold amount
      LED_GO(LED1_r, LED1_g);
    }
    if (LC2 > touchThreshold) {
      LED_GO(LED2_r, LED2_g);
    }
    if (LC3 > touchThreshold) {
      LED_GO(LED3_r, LED3_g);
    }
    pb = digitalRead(PB_PIN);
    if (pb != HIGH) {
      state = S_IDLE;
    }
    break;
  case S_MOTOR_OFF:
    auto_motor_run_time = 0;
    digitalWrite(GECKOENBL, LOW);
    LED_OFF(LED1_r, LED1_g);
    LED_OFF(LED3_r, LED3_g);
    delayMicroseconds(400000); // snooze before ending
    state = S_IDLE;
    break;
  case S_IDLE:
    // read what the load cells are doing and complain if needed
    LC1 = analogRead(LC_PIN1);  LC2 = analogRead(LC_PIN2);  LC3 = analogRead(LC_PIN3);

    RA1.addValue(LC1);  RA2.addValue(LC2);  RA3.addValue(LC3);

    Serial.printf("%d :: %d :: %d\n", LC1, LC2, LC3);

    if (abs(RA1.getAverage() - LC1) > delta &&
	RA1.getAverage() > averageThreshold) {
      // the magic numbers in tone have to do with the range of the load cells
      //  and the range of audible tones coming out of the speaker
      if (silenceFlag == 0) {
	tone(SPKR_PIN, map(LC1, 50, 1000, 2000, 6000));  // make noise
      }
      compressed_counter++;
      LED_STOP(LED1_r, LED1_g);
    }
    else if (abs(RA2.getAverage() - LC2) > delta &&
	     RA2.getAverage() > averageThreshold) {

      if (silenceFlag == 0) {
	tone(SPKR_PIN, map(LC2, 50, 1000, 2000, 6000));
      }
      compressed_counter++;
      LED_STOP(LED2_r, LED2_g);
    }
    else if (abs(RA3.getAverage() - LC3) > delta &&
	     RA3.getAverage() > averageThreshold) {

      if (silenceFlag == 0) {
	tone(SPKR_PIN, map(LC3, 50, 1000, 2000, 6000));
      }
      compressed_counter++;
      LED_STOP(LED3_r, LED3_g);
    }
    else {
      // no problem, no noise
      noTone(SPKR_PIN);
      LED_OFF(LED1_r, LED1_g); LED_OFF(LED2_r, LED2_g); LED_OFF(LED3_r, LED3_g);
    }

    if (compressed_counter > 100) {
      // this will report there is a problem
      badnewssinceQuery = 1;
      compressed_counter = 0;
    }
    break;

  case S_MAKE_ACTIVATE_SOUND:
    sound.ray_gun();
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
