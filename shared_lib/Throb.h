#ifndef Throb_h
#define Throb_h

#include "Arduino.h"

class Throb {
  public:
    Throb(int pin);
    void pulse();
    boolean pulseOnTimer(uint32_t time);
    void stop();
    void goDark();
    void fullOn();
    void setBrightness(int val);

  private:
    boolean _blinkOn = false;
    uint32_t _blinkDelta = 0;
    uint32_t _blinkInterval = 2000; 
    uint32_t _blinkNow;
    uint32_t _blinkDuty;
    uint32_t _blinkUpTime;

    uint32_t _now;
    uint32_t _timerInterval = 2000; 

    boolean _LEDstate;
    int _loopCounter;
    int _pwmCount;

    // int _pwmArray[100] = { 1, 2, 4,  6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 98, 96, 94, 92, 90, 88, 86, 84, 82, 80, 78, 76, 74, 72, 70, 68, 66, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0 };

    int _pwmArray[100] = {200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200};
    int _pin;
    void pwmLED(int pin, int val);
};

#endif
