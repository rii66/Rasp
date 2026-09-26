#ifndef PWM_H
#define PWM_H

void initPWM();
void heaterOn();
void heaterOff();
void setPWM(int pwm);

void startTempRead();
void endTempRead();

#endif