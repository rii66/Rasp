#ifndef _NOKIA105_LCD_H
#define _NOKIA105_LCD_H

#include "Arduino.h"
#include "fonts.h"
#include "cmd.h"
#include "clocks.h"

// ============================================================
//  Nokia 105 LCD - Full PIO 9-bit SPI for RP2040
//  Dual / Quad ready (1 SM shared bus + separate CS)
//  FIX: shared RST + longer CS timing
// ============================================================

#ifndef NOKIA105_WIDTH
#define NOKIA105_WIDTH  128
#endif
#ifndef NOKIA105_HEIGHT
#define NOKIA105_HEIGHT 160
#endif
#ifndef NOKIA105_X_OFFSET
#define NOKIA105_X_OFFSET 2
#endif
#ifndef NOKIA105_Y_OFFSET
#define NOKIA105_Y_OFFSET 0
#endif

#define WIDTH                 NOKIA105_WIDTH
#define HEIGHT                NOKIA105_HEIGHT
#define nextLineEdge          128
#define spaceBetweenScanLines 16
#define fullLengthVertical    160

#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F

class Nokia105 {
public:
  Nokia105(int sda, int sck, int rst, int cs);

  void begin(uint32_t freq_hz = 20000000);

  void initDisplay();
  void initDisplaySoft();   // software only (no hardware RST)
  void reset();
  void displayOn();
  void displayOff();
  void invertDisplay(bool invert = true);
  void setRotation(uint8_t r);

  void setDrawPosition(unsigned char x, unsigned char y);
  void setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void backgroundColor(uint16_t c);
  void displayClear();

  void lineHorizontal(int16_t x, int16_t y, int16_t w, uint16_t color);
  void lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color);
  void circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

  void printSingleChar(unsigned char c, unsigned char x, unsigned char y,
                       uint16_t fg, uint16_t bg);
  void printString(const char *str, uint8_t x, uint8_t y,
                   uint16_t fg, uint16_t bg);
  void printDigit(unsigned int value, int16_t x, int16_t y,
                  uint16_t fg, uint16_t bg);

  void image1d(uint16_t w, uint16_t h, uint16_t shiftX, uint16_t shiftY,
               const uint16_t image[]);
  void printBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                   int16_t w, int16_t h, uint16_t color);

  void smpteTest();
  void colorPalletTest();

private:
  void writeCmd(uint8_t cmd);
  void writeData(uint8_t data);
  void writeData16(uint16_t color);
  void csLow();
  void csHigh();
  void pioPut(uint16_t val9);

  static bool     _pio_ok;
  static PIO      _pio;
  static uint     _sm;
  static uint     _offset;
  static int      _bus_sda;
  static int      _bus_sck;

  int _rst;
  int _cs;
  uint8_t _rotation;
};

#endif
