#include "Nokia105Debug.h"

Nokia105Debug lcd(1, 0, 6, 2);   // SDA, SCK, RST, CS

void setup() {
  Serial.begin(115200);
  delay(500);

  lcd.initDisplay(true);
  lcd.setRotation(1);
  lcd.backgroundColor(0x0000);

  lcd.printString("TEMP 320 C", 4, 8,  0xFFFF, 0x0000);
  lcd.printString("PWM 45%",    4, 32, 0x07FF, 0x0000);
  lcd.printString("STATUS OK",  4, 56, 0x07E0, 0x0000);
}

void loop() {}
