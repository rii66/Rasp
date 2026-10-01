#include "config.h"
#include "Nokia105Debug.h"
#include <Wire.h>

Nokia105Debug lcd1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105Debug lcd2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

void idleCS() {
  digitalWrite(LCD1_CS, HIGH);
  digitalWrite(LCD2_CS, HIGH);
}

void oledCmd(uint8_t c) {
  Wire.beginTransmission(0x3C);
  Wire.write(0x00);
  Wire.write(c);
  Wire.endTransmission();
}

void oledData(uint8_t d) {
  Wire.beginTransmission(0x3C);
  Wire.write(0x40);
  Wire.write(d);
  Wire.endTransmission();
}

void oledInit() {
  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);
  Wire.begin();
  delay(20);

  const uint8_t init[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
    0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
    0xDA, 0x12, 0x81, 0x8F, 0xD9, 0xF1, 0xDB,
    0x40, 0xA4, 0xA6, 0xAF
  };

  for (uint8_t i = 0; i < sizeof(init); i++) oledCmd(init[i]);

  oledCmd(0x21); oledCmd(0); oledCmd(127);
  oledCmd(0x22); oledCmd(0); oledCmd(7);

  for (uint8_t page = 0; page < 8; page++) {
    oledCmd(0xB0 + page);
    oledCmd(0x00);
    oledCmd(0x10);
    for (uint16_t x = 0; x < 128; x++) oledData(0x00);
  }
}

void oledChar(uint8_t c) {
  static const uint8_t O[5] = {0x3E,0x41,0x41,0x41,0x3E};
  static const uint8_t L[5] = {0x7F,0x40,0x40,0x40,0x40};
  static const uint8_t E[5] = {0x7F,0x49,0x49,0x49,0x41};
  static const uint8_t D[5] = {0x7F,0x41,0x41,0x22,0x1C};
  static const uint8_t K[5] = {0x7F,0x08,0x14,0x22,0x41};

  const uint8_t *g = nullptr;
  switch (c) {
    case 'O': g = O; break;
    case 'L': g = L; break;
    case 'E': g = E; break;
    case 'D': g = D; break;
    case 'K': g = K; break;
    default: break;
  }

  if (!g) {
    for (uint8_t i = 0; i < 6; i++) oledData(0x00);
    return;
  }

  for (uint8_t i = 0; i < 5; i++) oledData(g[i]);
  oledData(0x00);
}

void oledOK() {
  oledCmd(0xB0);
  oledCmd(0x00);
  oledCmd(0x10);
  for (uint8_t i = 0; i < 5; i++) oledChar(' ');
  oledChar('O'); oledChar('L'); oledChar('E'); oledChar('D');
  oledChar(' ');
  oledChar('O'); oledChar('K');
}

void setup() {
  // 1. Matikan semua CS di awal
  pinMode(LCD1_CS, OUTPUT);
  pinMode(LCD2_CS, OUTPUT);
  digitalWrite(LCD1_CS, HIGH); 
  digitalWrite(LCD2_CS, HIGH); 
  pinMode(LCD_RST, OUTPUT);

  // 2. Init OLED kamu dulu sampai selesai
  oledInit();
  oledOK();

  // ==========================================
  // 3. INSIALISASI LAYAR 1 (Kanan/Kiri yang terhubung ke CS1)
  // ==========================================
  digitalWrite(LCD2_CS, HIGH); // Layar 2 wajib mati total
  digitalWrite(LCD1_CS, LOW);  // Aktifkan Layar 1

  // Reset fisik khusus untuk Layar 1 saat dia aktif
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);

  lcd1.initDisplay(false); // Jalankan init register
  lcd1.setRotation(1);
  lcd1.backgroundColor(0x0000);
  lcd1.printString("Cs1 Ok", 4, 30, 0x07FF, 0x0000);
  
  digitalWrite(LCD1_CS, HIGH); // Kunci mati Layar 1 setelah selesai
  delay(50); // Jeda napas SPI

  // ==========================================
  // 4. INISIALISASI LAYAR 2 (Layar yang blank putih)
  // ==========================================
  digitalWrite(LCD1_CS, HIGH); // Layar 1 wajib mati total
  digitalWrite(LCD2_CS, LOW);  // Aktifkan Layar 2

  // Reset fisik ulang khusus untuk Layar 2 sebelum dia di-init
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);

  lcd2.initDisplay(false); // Jalankan init register
  lcd2.setRotation(1);
  lcd2.backgroundColor(0x0000);
  lcd2.printString("Cs2 Ok", 4, 30, 0xF81F, 0x0000);
  
  digitalWrite(LCD2_CS, HIGH); // Kunci mati Layar 2 setelah selesai
}
  
void loop() {}
