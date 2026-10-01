#include "config.h"
#include "Nokia105Debug.h"

Nokia105Debug lcd1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105Debug lcd2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

void idleCS() {
  digitalWrite(LCD1_CS, HIGH);
  digitalWrite(LCD2_CS, HIGH);
}

void setup() {
  pinMode(LCD1_CS, OUTPUT);
  pinMode(LCD2_CS, OUTPUT);
  idleCS();

  pinMode(LCD_RST, OUTPUT);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
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
