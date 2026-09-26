#include "platform_compat.h"
#include "config.h"
#include "encoder.h"

volatile int encoderPos = 0;
volatile int encoder2Pos = 0;
volatile uint8_t lastState = 0;
volatile uint8_t lastState2 = 0;

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);
  uint8_t encoded = (a << 1) | b;
  uint8_t sum = (lastState << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPos++;
  else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPos--;

  lastState = encoded;
}

void IRAM_ATTR encoder2ISR() {
  uint8_t a = digitalRead(PIN_ENC2_A);
  uint8_t b = digitalRead(PIN_ENC2_B);
  uint8_t encoded = (a << 1) | b;
  uint8_t sum = (lastState2 << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoder2Pos++;
  else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoder2Pos--;

  lastState2 = encoded;
}

void initEncoder() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  pinMode(PIN_ENC2_A, INPUT_PULLUP);
  pinMode(PIN_ENC2_B, INPUT_PULLUP);
  pinMode(PIN_ENC2_SW, INPUT_PULLUP);

  lastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  lastState2 = (digitalRead(PIN_ENC2_A) << 1) | digitalRead(PIN_ENC2_B);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_A), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_B), encoder2ISR, CHANGE);
}

bool buttonPressed() {
  return !digitalRead(ENC_SW);
}

bool button2Pressed() {
  return !digitalRead(PIN_ENC2_SW);
}

int getEncoderDelta() {
  static int lastLogicPos = 0;
  int currentLogicPos = encoderPos / 4;
  int diff = currentLogicPos - lastLogicPos;

  if (diff != 0) {
    lastLogicPos = currentLogicPos;
    return diff;
  }
  return 0;
}

int getEncoder2Delta() {
  static int lastLogicPos2 = 0;
  int currentLogicPos = encoder2Pos / 4;
  int diff = currentLogicPos - lastLogicPos2;

  if (diff != 0) {
    lastLogicPos2 = currentLogicPos;
    return diff;
  }
  return 0;
}

bool buttonClicked() {
  static bool last = false;
  bool now = buttonPressed();
  bool click = last && !now;
  last = now;
  return click;
}
