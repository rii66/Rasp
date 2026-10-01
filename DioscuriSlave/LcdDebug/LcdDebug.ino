#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 25
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(2000);

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("=== RP2040 USB SERIAL TEST ===");
  Serial.println("BOOT OK");
  Serial.println("LED ON");
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  Serial.println("LOOP OK");
  delay(1000);
}
