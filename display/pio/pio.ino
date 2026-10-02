// ============================================================
// Nokia 105 LCD - Full PIO 9-bit SPI (RP2040)
// Dual display example
// ============================================================

#include "Nokia105_LCD.h"

// ====== SHARED BUS ======
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6     // shared reset

// ====== CS per display ======
#define LCD1_CS   12
#define LCD2_CS   14

Nokia105 display1(LCD_SDA, LCD_SCK, LCD_RST, LCD1_CS);
Nokia105 display2(LCD_SDA, LCD_SCK, LCD_RST, LCD2_CS);

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Nokia 105 PIO 9-bit SPI Dual");

  // First begin() will load the PIO program (40 MHz target)
  display1.begin(40000000);
  display2.begin(40000000);   // second call skips PIO init

  // ---- Display 1 ----
  display1.backgroundColor(BLACK);
  display1.printString("Display 1", 25, 30, WHITE, BLACK);
  display1.printString("PIO Fast", 30, 55, GREEN, BLACK);
  display1.printDigit(1, 50, 90, CYAN, BLACK);

  // ---- Display 2 ----
  display2.backgroundColor(NAVY);
  display2.printString("Display 2", 25, 30, WHITE, NAVY);
  display2.printString("Dual OK", 35, 55, YELLOW, NAVY);
  display2.printDigit(2, 50, 90, GREEN, NAVY);
}

void loop() {
  // free
}
