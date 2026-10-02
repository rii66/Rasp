// ============================================================
// Nokia 105 LCD - PIO 9-bit Dual + CS Debug
// Ganti warna & teks tiap 1.5 detik (beda tiap LCD)
// ============================================================

#include "Nokia105_LCD.h"

// ====== SHARED BUS ======
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6     // shared (hanya dipulse 1x)

// ====== CS per display ======
#define LCD1_CS   12
#define LCD2_CS   14

Nokia105 display1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 display2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

// warna untuk cycle
const uint16_t colors[] = {
  BLACK, NAVY, DARKGREEN, MAROON, PURPLE,
  BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, ORANGE
};
const char* colorNames[] = {
  "BLACK", "NAVY", "DKGREEN", "MAROON", "PURPLE",
  "BLUE", "GREEN", "CYAN", "RED", "MAGENTA", "YELLOW", "WHITE", "ORANGE"
};
const int NUM_COLORS = 13;

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("=== Nokia 105 PIO Dual Debug ===");
  Serial.println("CS1=GP12  CS2=GP14  RST=GP6");

  display1.begin(30000000);
  display2.begin(30000000);

  Serial.println("Init done. Starting color cycle...");
}

void loop() {
  static uint32_t last = 0;
  static int idx1 = 0;
  static int idx2 = 3;   // mulai beda

  if (millis() - last < 1500) return;   // tiap 1.5 detik
  last = millis();

  // ---- Display 1 (CS=12) ----
  uint16_t bg1 = colors[idx1];
  uint16_t fg1 = (bg1 == WHITE || bg1 == YELLOW || bg1 == CYAN) ? BLACK : WHITE;

  display1.backgroundColor(bg1);
  display1.printString("LCD #1", 35, 20, fg1, bg1);
  display1.printString("CS=GP12", 30, 45, fg1, bg1);
  display1.printString(colorNames[idx1], 25, 70, fg1, bg1);
  display1.printDigit(idx1, 50, 100, fg1, bg1);

  // ---- Display 2 (CS=14) ----
  uint16_t bg2 = colors[idx2];
  uint16_t fg2 = (bg2 == WHITE || bg2 == YELLOW || bg2 == CYAN) ? BLACK : WHITE;

  display2.backgroundColor(bg2);
  display2.printString("LCD #2", 35, 20, fg2, bg2);
  display2.printString("CS=GP14", 30, 45, fg2, bg2);
  display2.printString(colorNames[idx2], 25, 70, fg2, bg2);
  display2.printDigit(idx2, 50, 100, fg2, bg2);

  // Serial debug
  Serial.print("LCD1: ");
  Serial.print(colorNames[idx1]);
  Serial.print("  |  LCD2: ");
  Serial.println(colorNames[idx2]);

  // next color (beda phase)
  idx1 = (idx1 + 1) % NUM_COLORS;
  idx2 = (idx2 + 1) % NUM_COLORS;
}
