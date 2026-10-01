#include "Nokia105Debug.h"

Nokia105Debug lcd1(1, 0, 6, 2);  // CS1 = GP2
Nokia105Debug lcd2(1, 0, 6, 3);  // CS2 = GP3

void lcdIdle() {
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(0, OUTPUT);  // SCK
  pinMode(1, OUTPUT);  // SDA
  pinMode(6, OUTPUT);  // RST
  lcdIdle();

  // RESET sekali untuk dua panel
  digitalWrite(6, LOW);
  delay(20);
  digitalWrite(6, HIGH);
  delay(150);

  lcdIdle();
  lcd1.initDisplay(false);   // reset sudah manual
  lcd1.setRotation(1);
  lcdIdle();
  lcd1.backgroundColor(0x0000);
  lcd1.printString("LCD1 SOLDER", 4, 20, 0x07FF, 0x0000);
  lcdIdle();
  delay(50);

  lcd2.initDisplay(false);
  lcd2.setRotation(1);
  lcdIdle();
  lcd2.backgroundColor(0x0000);
  lcd2.printString("LCD2 HOTAIR", 4, 20, 0xF81F, 0x0000);
  lcdIdle();
}

void loop() {
  // kosong dulu. dual harus diam bagus sebelum animasi
}
