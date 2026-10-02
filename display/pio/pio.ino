// ============================================================
// Nokia 105 LCD - PIO Dual Debug v3
// ============================================================

#include "Nokia105_LCD.h"

#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LCD1_CS   2
#define LCD2_CS   3

Nokia105 display1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 display2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

const uint16_t colors[] = {
  BLACK, NAVY, DARKGREEN, MAROON, PURPLE,
  BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, ORANGE
};
const char* colorNames[] = {
  "BLACK", "NAVY", "DKGREEN", "MAROON", "PURPLE",
  "BLUE", "GREEN", "CYAN", "RED", "MAGENTA", "YELLOW", "WHITE", "ORANGE"
};
const int NUM_COLORS = 13;

void forceAllCS_High() {
  pinMode(LCD1_CS, OUTPUT);
  pinMode(LCD2_CS, OUTPUT);
  digitalWrite(LCD1_CS, HIGH);
  digitalWrite(LCD2_CS, HIGH);
  delayMicroseconds(50);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Nokia 105 PIO Dual Debug v3 ===");

  forceAllCS_High();

  display1.begin(20000000);
  forceAllCS_High();
  delay(50);

  display2.begin(20000000);
  forceAllCS_High();
  delay(50);

  // Re-init LCD1 setelah LCD2
  display1.initDisplaySoft();
  forceAllCS_High();
  delay(50);

  display1.backgroundColor(RED);
  display2.backgroundColor(BLUE);
  delay(800);

  display1.backgroundColor(GREEN);
  display2.backgroundColor(YELLOW);
  delay(800);
}

void loop() {
  static uint32_t last = 0;
  static int idx1 = 0;
  static int idx2 = 5;

  if (millis() - last < 1200) return;
  last = millis();

  forceAllCS_High();

  uint16_t bg1 = colors[idx1];
  uint16_t fg1 = (bg1 == WHITE || bg1 == YELLOW || bg1 == CYAN || bg1 == GREEN) ? BLACK : WHITE;
  display1.backgroundColor(bg1);
  display1.printString("LCD #1", 35, 15, fg1, bg1);
  display1.printString("CS GP12", 28, 40, fg1, bg1);
  display1.printString(colorNames[idx1], 20, 65, fg1, bg1);
  display1.printDigit(idx1 + 1, 55, 100, fg1, bg1);

  forceAllCS_High();

  uint16_t bg2 = colors[idx2];
  uint16_t fg2 = (bg2 == WHITE || bg2 == YELLOW || bg2 == CYAN || bg2 == GREEN) ? BLACK : WHITE;
  display2.backgroundColor(bg2);
  display2.printString("LCD #2", 35, 15, fg2, bg2);
  display2.printString("CS GP14", 28, 40, fg2, bg2);
  display2.printString(colorNames[idx2], 20, 65, fg2, bg2);
  display2.printDigit(idx2 + 1, 55, 100, fg2, bg2);

  forceAllCS_High();

  Serial.print("LCD1=");
  Serial.print(colorNames[idx1]);
  Serial.print("  LCD2=");
  Serial.println(colorNames[idx2]);

  idx1 = (idx1 + 1) % NUM_COLORS;
  idx2 = (idx2 + 1) % NUM_COLORS;
}
