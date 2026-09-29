#include <Arduino.h>

void setup() {
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);

  Serial.begin(115200);
  delay(1000);
  Serial.println(F("RP2040 USB OK"));
}

void loop() {
  digitalWrite(25, !digitalRead(25));
  Serial.println(F("USB ALIVE"));
  delay(1000);
}
