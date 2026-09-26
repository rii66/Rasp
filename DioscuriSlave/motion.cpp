#include <Arduino.h>

#include "motion.h"
#include "config.h"
#include "GlobalState.h"
#include "pid.h"
#include "tip.h"
#include "encoder.h"
#include "buzzer.h"

void initMotion() {
  pinMode(MOTION_PIN, INPUT_PULLUP);
  lastMotion = millis();
  sleepTimer = (unsigned long)sleepTimeSec * 1000UL;
}
// Motion //
void wakeFromSleep() {
  motionDetected = true;
  lastMotion = millis();

  if (sleeping) {
    sleeping = false;
    beepWake();
  }
}

void updateMotion() {
  bool motion = digitalRead(MOTION_PIN);

  /* Bangun Universal*/
  if (motion == LOW) {
    wakeFromSleep();
  }

  /* Keep sleep timeout in sync with setting (ms) */
  sleepTimer = (unsigned long)sleepTimeSec * 1000UL;

  /* AUTO SLEEP */
  if (
    sleepTimer > 0 &&
    millis() - lastMotion > sleepTimer
  ) {
    if (!sleeping) {
      sleeping = true;
      beepSleep();  // sleep beep
    }
  }
}