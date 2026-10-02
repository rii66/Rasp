// ============================================================
// Dual Nokia PIO + OLED SSD1306 MINIMAL (no Adafruit)
// OLED 128x64 addr 0x3C  SDA=GP4  SCL=GP5
// LCD CS1=GP2 CS2=GP3  RST=GP6  SDA=1 SCK=0
// ============================================================

#include "Nokia105_LCD.h"
#include <Wire.h>

// ----- Nokia -----
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LCD1_CS   2
#define LCD2_CS   3

// ----- OLED -----
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  5
#define OLED_ADDR     0x3C

Nokia105 display1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 display2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

// ====== SSD1306 minimal (128x64) ======
static uint8_t oled_buf[1024];  // 128*64/8

void oled_cmd(uint8_t c) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(c);
  Wire.endTransmission();
}

void oled_init() {
  Wire.setSDA(PIN_OLED_SDA);
  Wire.setSCL(PIN_OLED_SCL);
  Wire.begin();
  delay(100);

  oled_cmd(0xAE); // display off
  oled_cmd(0xD5); oled_cmd(0x80);
  oled_cmd(0xA8); oled_cmd(0x3F);
  oled_cmd(0xD3); oled_cmd(0x00);
  oled_cmd(0x40);
  oled_cmd(0x8D); oled_cmd(0x14); // charge pump
  oled_cmd(0x20); oled_cmd(0x00); // horizontal
  oled_cmd(0xA1);
  oled_cmd(0xC8);
  oled_cmd(0xDA); oled_cmd(0x12);
  oled_cmd(0x81); oled_cmd(0xCF);
  oled_cmd(0xD9); oled_cmd(0xF1);
  oled_cmd(0xDB); oled_cmd(0x40);
  oled_cmd(0xA4);
  oled_cmd(0xA6);
  oled_cmd(0xAF); // display on
}

void oled_clear() {
  memset(oled_buf, 0, sizeof(oled_buf));
}

void oled_setPixel(int x, int y, bool on) {
  if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
  if (on) oled_buf[x + (y / 8) * 128] |=  (1 << (y & 7));
  else    oled_buf[x + (y / 8) * 128] &= \~(1 << (y & 7));
}

// font 5x7 sederhana (hanya angka + huruf besar)
const uint8_t font5x7[][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
};

void oled_drawChar(int x, int y, char c) {
  if (c >= '0' && c <= '9') {
    const uint8_t* g = font5x7[c - '0'];
    for (int col = 0; col < 5; col++) {
      uint8_t line = g[col];
      for (int row = 0; row < 7; row++) {
        if (line & (1 << row)) oled_setPixel(x + col, y + row, true);
      }
    }
  }
}

void oled_print(int x, int y, const char* s) {
  while (*s) {
    oled_drawChar(x, y, *s);
    x += 6;
    s++;
  }
}

void oled_show() {
  oled_cmd(0x21); oled_cmd(0); oled_cmd(127); // col
  oled_cmd(0x22); oled_cmd(0); oled_cmd(7);   // page

  for (int i = 0; i < 1024; i += 16) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);
    for (int j = 0; j < 16; j++) Wire.write(oled_buf[i + j]);
    Wire.endTransmission();
  }
}

// ====== Nokia helpers ======
const uint16_t colors[] = {
  BLACK, NAVY, DARKGREEN, MAROON, PURPLE,
  BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, ORANGE
};
const char* colorNames[] = {
  "BLACK", "NAVY", "DKGRN", "MAROON", "PURPLE",
  "BLUE", "GREEN", "CYAN", "RED", "MAGNTA", "YELLOW", "WHITE", "ORANGE"
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
  delay(400);
  Serial.println("Dual Nokia + OLED minimal");

  oled_init();
  oled_clear();
  oled_print(0, 0, "OLED OK");
  oled_show();

  forceAllCS_High();
  display1.begin(20000000);
  forceAllCS_High();
  delay(30);
  display2.begin(20000000);
  forceAllCS_High();
  delay(30);
  display1.initDisplaySoft();
  forceAllCS_High();

  display1.backgroundColor(RED);
  display2.backgroundColor(BLUE);
  delay(600);
  display1.backgroundColor(GREEN);
  display2.backgroundColor(YELLOW);
  delay(600);
}

void loop() {
  static uint32_t last = 0;
  static int idx1 = 0, idx2 = 5;
  if (millis() - last < 1200) return;
  last = millis();

  forceAllCS_High();

  uint16_t bg1 = colors[idx1];
  uint16_t fg1 = (bg1 == WHITE || bg1 == YELLOW || bg1 == CYAN || bg1 == GREEN) ? BLACK : WHITE;
  display1.backgroundColor(bg1);
  display1.printString("LCD1", 40, 20, fg1, bg1);
  display1.printString(colorNames[idx1], 20, 50, fg1, bg1);

  forceAllCS_High();

  uint16_t bg2 = colors[idx2];
  uint16_t fg2 = (bg2 == WHITE || bg2 == YELLOW || bg2 == CYAN || bg2 == GREEN) ? BLACK : WHITE;
  display2.backgroundColor(bg2);
  display2.printString("LCD2", 40, 20, fg2, bg2);
  display2.printString(colorNames[idx2], 20, 50, fg2, bg2);

  forceAllCS_High();

  // OLED status (angka saja biar ringan)
  oled_clear();
  oled_print(0, 0, "1");
  oled_drawChar(10, 0, '0' + (idx1 % 10));
  oled_print(0, 16, "2");
  oled_drawChar(10, 16, '0' + (idx2 % 10));
  oled_show();

  idx1 = (idx1 + 1) % NUM_COLORS;
  idx2 = (idx2 + 1) % NUM_COLORS;
}
