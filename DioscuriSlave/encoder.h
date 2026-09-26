#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config.h"

// ISR
void IRAM_ATTR encoderISR();
void initEncoder();

//FOR WEB ONLY
extern volatile int encoderPos;

// OUTPUT
int getEncoderDelta();
bool buttonPressed();
bool buttonClicked();

#endif