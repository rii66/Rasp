// ============================================================
// Nokia 105 LCD - Resolution & Orientation Test
// Target: 128 x 160 portrait, X_OFFSET=2
// CS1=GP12  CS2=GP14  RST=GP6  SDA=GP1  SCK=GP0
// ============================================================

#include "Nokia105_LCD.h"

#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LCD1_CS   12
#define LCD2_CS   14

Nokia105 lcd1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 lcd2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

void forceCsHigh() {
  pinMode(LCD1_CS, OUTPUT);
  pinMode(LCD2_CS, OUTPUT);
  digitalWrite(LCD1_CS, HIGH);
  digitalWrite(LCD2_CS, HIGH);
  delayMicroseconds(80);
}

void drawTestScreen(Nokia105& lcd, const char* name, uint8_t rot) {
  lcd.setRotation(rot);
  lcd.displayClear();

  // Border penuh – kalau offset/size salah, border putus atau keluar
  lcd.lineHorizontal(0, 0, 128, WHITE);
  lcd.lineHorizontal(0, 159, 128, WHITE);
  lcd.lineVertical(0, 0, 160, WHITE);
  lcd.lineVertical(127, 0, 160, WHITE);

  // Corner markers
  lcd.fillRectangle(0, 0, 8, 8, RED);        // top-left
  lcd.fillRectangle(120, 0, 8, 8, GREEN);    // top-right
  lcd.fillRectangle(0, 152, 8, 8, BLUE);     // bottom-left
  lcd.fillRectangle(120, 152, 8, 8, YELLOW); // bottom-right

  // Info teks
  char buf[24];
  lcd.printString(name, 20, 20, ORANGE, BLACK);

  snprintf(buf, sizeof(buf), "ROT %d", rot);
  lcd.printString(buf, 36, 40, WHITE, BLACK);

  lcd.printString("128 x 160", 24, 60, CYAN, BLACK);

  // Garis tengah horizontal + vertikal
  lcd.lineHorizontal(0, 80, 128, MAGENTA);
  lcd.lineVertical(64, 0, 160, MAGENTA);

  // Angka besar di tengah
  lcd.printString("OK", 48, 100, GREEN, BLACK);

  // Footer
  lcd.printString("TOP", 48, 140, LIGHTGREY, BLACK);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("=== Nokia 105 size/rotation test ===");
  Serial.println("Expect: 128x160 portrait, red=TL green=TR blue=BL yellow=BR");

  forceCsHigh();

  lcd1.begin(4000000);
  forceCsHigh();
  delay(50);

  lcd2.begin(4000000);
  forceCsHigh();
  delay(50);

  // re-init LCD1 setelah LCD2 (penting di dual CS)
  lcd1.initDisplaySoft();
  forceCsHigh();
  delay(30);

  // Mulai dengan rotation 0 (native portrait di driver pio)
  drawTestScreen(lcd1, "LCD1", 0);
  forceCsHigh();
  drawTestScreen(lcd2, "LCD2", 0);
  forceCsHigh();

  Serial.println("Drawn ROT=0 on both. Check physical screen.");
  Serial.println("Red should be top-left corner of the glass.");
}

void loop() {
  static uint32_t last = 0;
  static uint8_t rot = 0;

  if (millis() - last < 3000) return;
  last = millis();

  rot = (rot + 1) & 3;   // cycle 0 → 1 → 2 → 3 → 0

  Serial.print("Switch to ROT=");
  Serial.println(rot);

  forceCsHigh();
  drawTestScreen(lcd1, "LCD1", rot);
  forceCsHigh();
  drawTestScreen(lcd2, "LCD2", rot);
  forceCsHigh();
}
