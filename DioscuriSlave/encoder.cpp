#include "platform_compat.h"
#include "config.h"
#include "encoder.h"

// ============================================================
// ROTARY ENCODER
// A/B = rotary only
// SW  = button only, handled separately with debounce
// ============================================================

volatile int encoderPos = 0;
volatile int encoder2Pos = 0;

volatile uint8_t lastState = 0;
volatile uint8_t lastState2 = 0;

// ------------------------------------------------------------
// Quadrature lookup table
// Valid transitions only. Invalid transitions are ignored.
// This keeps contact bounce on A/B from creating false steps.
// ------------------------------------------------------------
static const int8_t QUAD_TABLE[16] = {
   0, -1, +1,  0,
  +1,  0,  0, -1,
  -1,  0,  0, +1,
   0, +1, -1,  0
};

void IRAM_ATTR encoderISR() {
  const uint8_t a = digitalRead(ENC_A);
  const uint8_t b = digitalRead(ENC_B);
  const uint8_t current = (a << 1) | b;

  const uint8_t transition = (lastState << 2) | current;
  encoderPos += QUAD_TABLE[transition & 0x0F];
  lastState = current;
}

void IRAM_ATTR encoder2ISR() {
  const uint8_t a = digitalRead(PIN_ENC2_A);
  const uint8_t b = digitalRead(PIN_ENC2_B);
  const uint8_t current = (a << 1) | b;

  const uint8_t transition = (lastState2 << 2) | current;
  encoder2Pos += QUAD_TABLE[transition & 0x0F];
  lastState2 = current;
}

// ============================================================
// BUTTON DEBOUNCE
// SW is NOT part of the A/B interrupt system.
// ============================================================
static bool sw1Stable = false;
static bool sw1LastRaw = false;
static uint32_t sw1ChangedAt = 0;

static bool sw2Stable = false;
static bool sw2LastRaw = false;
static uint32_t sw2ChangedAt = 0;

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

void initEncoder() {
  // ---- EC1 ----
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  // ---- EC2 ----
  pinMode(PIN_ENC2_A, INPUT_PULLUP);
  pinMode(PIN_ENC2_B, INPUT_PULLUP);
  pinMode(PIN_ENC2_SW, INPUT_PULLUP);

  // Initialise A/B state BEFORE enabling interrupts.
  lastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  lastState2 = (digitalRead(PIN_ENC2_A) << 1) | digitalRead(PIN_ENC2_B);

  // Initialise switches as released.
  sw1Stable = false;
  sw1LastRaw = false;
  sw1ChangedAt = millis();

  sw2Stable = false;
  sw2LastRaw = false;
  sw2ChangedAt = millis();

  // ONLY A/B use interrupts.
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_A), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_B), encoder2ISR, CHANGE);
}

// ============================================================
// Debounced SW1
// Returns true only after the raw state has remained stable for
// BUTTON_DEBOUNCE_MS.
// ============================================================
bool buttonPressed() {
  const bool raw = (digitalRead(ENC_SW) == LOW);
  const uint32_t now = millis();

  if (raw != sw1LastRaw) {
    sw1LastRaw = raw;
    sw1ChangedAt = now;
  }

  if ((uint32_t)(now - sw1ChangedAt) >= BUTTON_DEBOUNCE_MS) {
    sw1Stable = raw;
  }

  return sw1Stable;
}

// ============================================================
// Debounced SW2
// ============================================================
bool button2Pressed() {
  const bool raw = (digitalRead(PIN_ENC2_SW) == LOW);
  const uint32_t now = millis();

  if (raw != sw2LastRaw) {
    sw2LastRaw = raw;
    sw2ChangedAt = now;
  }

  if ((uint32_t)(now - sw2ChangedAt) >= BUTTON_DEBOUNCE_MS) {
    sw2Stable = raw;
  }

  return sw2Stable;
}

// ============================================================
// One logical step per 4 valid quadrature transitions
// ============================================================
int getEncoderDelta() {
  static int lastLogicPos = 0;
  const int currentLogicPos = encoderPos / 4;
  const int diff = currentLogicPos - lastLogicPos;

  if (diff != 0) {
    lastLogicPos = currentLogicPos;
    return diff;
  }

  return 0;
}

int getEncoder2Delta() {
  static int lastLogicPos2 = 0;
  const int currentLogicPos2 = encoder2Pos / 4;
  const int diff = currentLogicPos2 - lastLogicPos2;

  if (diff != 0) {
    lastLogicPos2 = currentLogicPos2;
    return diff;
  }

  return 0;
}

// ============================================================
// Debounced click detection for EC1
// Uses the same debounced SW state as buttonPressed().
// ============================================================
bool buttonClicked() {
  static bool lastStable = false;
  const bool now = buttonPressed();
  const bool click = lastStable && !now;
  lastStable = now;
  return click;
}
