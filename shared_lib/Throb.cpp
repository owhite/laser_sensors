#include "Arduino.h"
#include "Throb.h"

Throb::Throb(int pin) {
  pinMode(pin, OUTPUT);
  _pin = pin;
}

void Throb::pulse() {
  _loopCounter = (_loopCounter < 1000) ? _loopCounter+1 : 0;
  _pwmCount = (_loopCounter == 0) ? _pwmCount+1: _pwmCount;
  _pwmCount = (_pwmCount > 99) ? 0 : _pwmCount;
  pwmLED(_pin, _pwmArray[_pwmCount]);
}

boolean Throb::pulseOnTimer(uint32_t time) {
  _now = millis();
  boolean r = false;
  if ((_now - time) < _timerInterval) { Throb::pulse(); r = true; }
  else { Throb::goDark(); }
  return r;
}

void Throb::setBrightness(int val) {
  pwmLED(_pin, val);
}

void Throb::goDark() {
  digitalWrite(_pin, HIGH);
}

void Throb::fullOn() {
  digitalWrite(_pin, LOW);
}

void Throb::stop() {
  // do nothing, no change to _loopCounter or _pwmCounter
  pwmLED(_pin, _pwmArray[_pwmCount]);
}

// val ranges from 0 to 100
void Throb::pwmLED(int pin, int val) {

  _blinkUpTime = _blinkInterval * val / 255;
  _blinkNow = micros();

  _LEDstate = (val != 0 && (_blinkNow - _blinkDelta) < _blinkUpTime) ? true : false;

  digitalWrite(pin, _LEDstate);

  _blinkDelta = ((_blinkNow - _blinkDelta) > _blinkInterval) ? _blinkNow : _blinkDelta;
}
