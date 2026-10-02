// ============================================================
// Nokia 105 PIO Dual + OLED SSD1306 Debug
// LCD1 CS=GP2  LCD2 CS=GP3  RST=GP6  SDA=1  SCK=0
// OLED SDA=GP4  SCL=GP5
// ============================================================

#include "Nokia105_LCD.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====== NOKIA SHARED BUS ======
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LCD1_CS   2
#define LCD2_CS   3

// ====== OLED I2C ======
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  5
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C   // coba 0x3D kalau blank

Nokia105 display1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 display2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint16_t colors[] = {
  BLACK, NAVY, DARKGREEN, MAROON, PURPLE,
  BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, ORANGE
};
const char* colorNames[] = {
  "BLACK", "NAVY", "DKGREEN", "MAROON", "PURPLE",
  "BLUE", "GREEN", "CYAN", "RED", "MAGENTA", "YELLOW", "WHITE", "ORANGE"
};
const int NUM_COLORS = 13;

bool oled_ok = false;

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
  Serial.println("=== Dual Nokia + OLED Debug ===");
  Serial.println("LCD CS1=GP2 CS2=GP3 | OLED SDA=4 SCL=5");

  // ----- OLED -----
  Wire.setSDA(PIN_OLED_SDA);
  Wire.setSCL(PIN_OLED_SCL);
  Wire.begin();

  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oled_ok = true;
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("OLED OK");
    oled.println("SDA=GP4 SCL=GP5");
    oled.println("Init Nokia...");
    oled.display();
    Serial.println("OLED init OK");
  } else {
    Serial.println("OLED GAGAL - cek wiring / alamat 0x3C vs 0x3D");
  }

  // ----- Nokia -----
  forceAllCS_High();

  Serial.println("Init LCD1...");
  display1.begin(20000000);
  forceAllCS_High();
  delay(50);

  Serial.println("Init LCD2...");
  display2.begin(20000000);
  forceAllCS_High();
  delay(50);

  display1.initDisplaySoft();
  forceAllCS_High();
  delay(50);

  // Solid test
  display1.backgroundColor(RED);
  display2.backgroundColor(BLUE);
  if (oled_ok) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("LCD1=RED");
    oled.println("LCD2=BLUE");
    oled.display();
  }
  delay(800);

  display1.backgroundColor(GREEN);
  display2.backgroundColor(YELLOW);
  if (oled_ok) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("LCD1=GREEN");
    oled.println("LCD2=YELLOW");
    oled.display();
  }
  delay(800);

  Serial.println("Cycle start...");
}

void loop() {
  static uint32_t last = 0;
  static int idx1 = 0;
  static int idx2 = 5;

  if (millis() - last < 1200) return;
  last = millis();

  forceAllCS_High();

  // ---- LCD 1 ----
  uint16_t bg1 = colors[idx1];
  uint16_t fg1 = (bg1 == WHITE || bg1 == YELLOW || bg1 == CYAN || bg1 == GREEN) ? BLACK : WHITE;
  display1.backgroundColor(bg1);
  display1.printString("LCD #1", 35, 15, fg1, bg1);
  display1.printString("CS GP2", 30, 40, fg1, bg1);
  display1.printString(colorNames[idx1], 20, 65, fg1, bg1);
  display1.printDigit(idx1 + 1, 55, 100, fg1, bg1);

  forceAllCS_High();

  // ---- LCD 2 ----
  uint16_t bg2 = colors[idx2];
  uint16_t fg2 = (bg2 == WHITE || bg2 == YELLOW || bg2 == CYAN || bg2 == GREEN) ? BLACK : WHITE;
  display2.backgroundColor(bg2);
  display2.printString("LCD #2", 35, 15, fg2, bg2);
  display2.printString("CS GP3", 30, 40, fg2, bg2);
  display2.printString(colorNames[idx2], 20, 65, fg2, bg2);
  display2.printDigit(idx2 + 1, 55, 100, fg2, bg2);

  forceAllCS_High();

  // ---- OLED status ----
  if (oled_ok) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("Dual + OLED");
    oled.print("LCD1: ");
    oled.println(colorNames[idx1]);
    oled.print("LCD2: ");
    oled.println(colorNames[idx2]);
    oled.print("t=");
    oled.print(millis() / 1000);
    oled.println("s");
    oled.display();
  }

  Serial.print("LCD1=");
  Serial.print(colorNames[idx1]);
  Serial.print("  LCD2=");
  Serial.println(colorNames[idx2]);

  idx1 = (idx1 + 1) % NUM_COLORS;
  idx2 = (idx2 + 1) % NUM_COLORS;
}
