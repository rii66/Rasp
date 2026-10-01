#include "Nokia105Debug.h"

Nokia105Debug lcd1(1, 0, 6, 2);
Nokia105Debug lcd2(1, 0, 6, 3);

void idleCS() {
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  idleCS();

  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  delay(20);
  digitalWrite(6, HIGH);
  delay(150);

  idleCS();
  lcd1.initDisplay(false);
  lcd1.setRotation(1);
  idleCS();
  lcd1.backgroundColor(0x0000);
  lcd1.printString("LCD1", 4, 30, 0x07FF, 0x0000);
  idleCS();

  lcd2.initDisplay(false);
  lcd2.setRotation(1);
  idleCS();
  lcd2.backgroundColor(0x0000);
  lcd2.printString("LCD2", 4, 30, 0xF81F, 0x0000);
  idleCS();
}

void loop() {}
