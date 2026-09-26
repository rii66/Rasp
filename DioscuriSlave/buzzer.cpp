#include "buzzer.h"
#include "GlobalState.h"
#include "config.h"

void beep(int freq, int dur) {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, freq, dur);
}
//========================//
      //  Event buzzer //
//========================//
extern bool buzzerEnabled;

// Select item
void beepSelect() {
  if (!buzzerEnabled) return;
  beep(3000, 30);
}

// Save setting
void beepSave() {
  if (!buzzerEnabled) return;
  beep(3500, 50);
}

// Exit / Cancel
void beepMove() {
  if (!buzzerEnabled) return;
  beep(2000, 30);
}

// Long press enter/exit menu
void beepLong() {
  if (!buzzerEnabled) return;
  beep(4000, 80);
}

// Boost ON (double beep cepat)
void beepBoost() {
  if (!buzzerEnabled) return;

  beep(3200, 40);
  delay(40);
  beep(3800, 40);
}

// Sleep mode masuk (nada turun)
void beepSleep() {
  if (!buzzerEnabled) return;

  beep(2800, 50);
  delay(40);
  beep(2200, 70);
}

// Bangun dari sleep (nada naik)
void beepWake() {
  if (!buzzerEnabled) return;

  beep(2200, 40);
  delay(40);
  beep(3200, 60);
}
/* BOOT ANIMASI EVENT */
// Dino Jump!
void beepJump() {
  if (!buzzerEnabled) return;
  beep(2500, 40);
}
// Connected! (nada gembira naik)
void beepConnected() {
  if (!buzzerEnabled) return;
  beep(2500, 40);
  delay(50);
  beep(4000, 100);
}
