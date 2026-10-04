#include <Arduino.h>
#include "pwm.h"
#include "config.h"
#include "GlobalState.h"

void initPWM() {
    pinMode(PWM_PIN, OUTPUT);
    digitalWrite(PWM_PIN, LOW);

#if defined(ARDUINO_ARCH_RP2040)
    analogWriteRange(PWM_MAX_VAL);
    analogWriteFreq(PWM_FREQ);
#endif

    analogWrite(PWM_PIN, 0);
}

void heaterOff() {
    analogWrite(PWM_PIN, 0);
}

void heaterOn() {
    analogWrite(PWM_PIN, pwmOut);
}

void setPWM(int pwm) {
    if (pwm < 0) pwm = 0;
    int lim = (maxPwmLimit > 0) ? maxPwmLimit : PWM_MAX_VAL;
    if (pwm > lim) pwm = lim;
    if (pwm > PWM_MAX_VAL) pwm = PWM_MAX_VAL;
    pwmOut = pwm;
    analogWrite(PWM_PIN, pwmOut);
}

void startTempRead() {
    heaterOff();
    delayMicroseconds(250);
}

void endTempRead() {
    heaterOn();
}
