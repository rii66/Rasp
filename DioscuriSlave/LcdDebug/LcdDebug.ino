#include "Nokia105Debug.h"

Nokia105Debug lcd(1, 0, 6, 2);

void setup() {
  lcd.initDisplay(true);
  lcd.setRotation(1);
  lcd.backgroundColor(0x0000);
  lcd.printString("CS2 OK", 4, 40, 0xFFFF, 0x0000);
}

void loop() {}
