#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "platform_compat.h"
#include "config.h"

void IRAM_ATTR encoderISR();
void IRAM_ATTR encoder2ISR();
void initEncoder();

extern volatile int encoderPos;
extern volatile int encoder2Pos;

int getEncoderDelta();
int getEncoder2Delta();

bool buttonPressed();
bool button2Pressed();
bool buttonClicked();

#endif
