#include <Arduino.h>
#include "pwm.h"
#include "config.h"
#include "GlobalState.h"

void initPWM() {
    #if defined(ARDUINO_ARCH_RP2040)
      analogWriteRange(PWM_MAX_VAL);
      analogWriteFreq(PWM_FREQ);   
#endif
    pinMode(PWM_PIN, OUTPUT);
    analogWrite(PWM_PIN, 0);
}

void heaterOff() {
    analogWrite(PWM_PIN, 0);
}

void heaterOn() {
    if (sleeping) {
        analogWrite(PWM_PIN, 0);
        return;
    }
    analogWrite(PWM_PIN, pwmOut);
}

void setPWM(int pwm) {
    if (pwm < 0) pwm = 0;
    if (sleeping) {
        pwmOut = 0;
        analogWrite(PWM_PIN, 0);
        return;
    }
    int lim = (maxPwmLimit > 0) ? maxPwmLimit : PWM_MAX_VAL;
    if (pwm > lim) pwm = lim;
    if (pwm > PWM_MAX_VAL) pwm = PWM_MAX_VAL;
    pwmOut = pwm;
    analogWrite(PWM_PIN, pwmOut);
}

void startTempRead() {
    heaterOff();
    delayMicroseconds(200);
}

void endTempRead() {
    heaterOn();
}
