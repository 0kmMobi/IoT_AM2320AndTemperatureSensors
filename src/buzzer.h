
#include "Arduino.h"

class Buzzer {
  uint8_t pin;

  public:
    Buzzer(uint8_t _pin) {
      pin = _pin;
      pinMode(pin, OUTPUT);
    }

    void beep(int frequency, unsigned long durationMSec) {
      tone(pin, frequency, durationMSec);
      delay(durationMSec);
      noTone(pin);
    }
};