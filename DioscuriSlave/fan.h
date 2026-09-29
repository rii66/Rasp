#ifndef FAN_H
#define FAN_H

#include <Arduino.h>
#include "config.h"

class Fan {
public:
  void begin() {
    /*=important=*/ PWM frequency is configured once by initPWM().
    pinMode(PIN_FAN_PWM, OUTPUT);
    analogWrite(PIN_FAN_PWM, 0);
    current_speed = 0;
  }

  void setSpeed(uint8_t speed) {
    speed = constrain(speed, 0, 255);

    if (speed > 0 && speed < FAN_MIN_SPEED) {
      speed = FAN_MIN_SPEED;
    }

    analogWrite(PIN_FAN_PWM, speed);
    current_speed = speed;
  }

  void off() {
    analogWrite(PIN_FAN_PWM, 0);
    current_speed = 0;
  }

  uint8_t getSpeed() const {
    return current_speed;
  }

  bool isRunning() const {
    return current_speed >= FAN_MIN_SPEED;
  }

private:
  uint8_t current_speed;
};

#endif