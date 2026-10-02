// Nokia 105 LCD test for Raspberry Pi Pico (Arduino-Pico core)
// Board: Raspberry Pi Pico / Pico W
// Soft 9-bit SPI (no D/C pin)

#include "Nokia105_LCD.h"

// ====== PIN MAPPING (sesuai permintaan) ======
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LCD_CS    14
// Backlight langsung ke VCC 2-3v (tidak dipakai di kode)
// =============================================

Nokia105 display(LCD_SDA, LCD_SCK, LCD_RST, LCD_CS);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Nokia 105 LCD - Pico RP2040");

  display.begin();                     // init + clear
  display.backgroundColor(BLACK);

  display.printString("Hello Pico", 20, 20, WHITE, BLACK);
  display.printString("Nokia 105 LCD", 10, 50, GREEN, BLACK);
  display.printString("Library OK!", 20, 80, YELLOW, BLACK);

  // contoh angka (pakai snprintf)
  display.printDigit(12345, 30, 110, CYAN, BLACK);
}

void loop() {
  // kosong / tambah animasi sendiri
}
