#include "Nokia105Debug.h"

Nokia105Debug lcd1(1, 0, 6, 2);   // SDA, SCK, RST, CS1 Solder
Nokia105Debug lcd2(1, 0, 6, 3);   // SDA, SCK, RST, CS2 Hot Air

int animX = 4;
int animDir = 1;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);

  lcd1.initDisplay(true);          // reset sekali
  lcd1.setRotation(1);
  lcd1.backgroundColor(0x0000);
  lcd1.printString("TEMP 320 C", 4, 8,  0xFFFF, 0x0000);
  lcd1.printString("PWM 45%",    4, 32, 0x07FF, 0x0000);
  lcd1.printString("STATUS OK",  4, 56, 0x07E0, 0x0000);

  digitalWrite(2, HIGH);

  lcd2.initDisplay(false);         // jangan reset lagi
  lcd2.setRotation(1);
  lcd2.backgroundColor(0x0000);
  lcd2.printString("TEMP 250 C", 4, 8,  0xFFFF, 0x0000);
  lcd2.printString("AIR 60%",    4, 32, 0xF81F, 0x0000);
  lcd2.printString("AIR RUN",    4, 56, 0xFFE0, 0x0000);

  digitalWrite(3, HIGH);
}

void loop() {
  lcd1.fillRectangle(0, 88, 160, 16, 0x0000);
  lcd1.printString("SolderxCastorS", animX, 88, 0xFFE0, 0x0000);

  lcd2.fillRectangle(0, 88, 160, 16, 0x0000);
  lcd2.printString("HotAirxPolluxS", animX, 88, 0x07FF, 0x0000);

  animX += animDir * 4;
  if (animX <= 0)  { animX = 0;  animDir = 1; }
  if (animX >= 96) { animX = 96; animDir = -1; }

  delay(120);
}
