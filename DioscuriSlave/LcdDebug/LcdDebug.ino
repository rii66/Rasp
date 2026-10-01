#include "Nokia105Debug.h"

Nokia105Debug lcd(1, 0, 6, 2);   // SDA, SCK, RST, CS

int animX = 4;
int animDir = 1;

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

void loop() {
  // Animasi teks bergerak kiri-kanan.
  lcd.fillRectangle(0, 88, 160, 16, 0x0000);
  lcd.printString("HERMENEX", animX, 88, 0xFFE0, 0x0000);

  animX += animDir * 4;

  if (animX <= 0) {
    animX = 0;
    animDir = 1;
  }

  if (animX >= 96) {
    animX = 96;
    animDir = -1;
  }

  delay(120);
}
