#include "Nokia105Debug.h"

Nokia105Debug lcd(1, 0, 6, 3);

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);  // pin 2 jangan mengambang

  lcd.initDisplay(true);
  lcd.setRotation(1);
  lcd.backgroundColor(0x0000);
  lcd.printString("CS3 OK", 4, 40, 0xFFFF, 0x0000);
}

void loop() {}
