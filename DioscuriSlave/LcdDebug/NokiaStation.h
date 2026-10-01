#ifndef DIOSCURI_NOKIA_STATION_H
#define DIOSCURI_NOKIA_STATION_H

#include "Arduino.h"
#include "fonts.h"

#define LCD_RES_High() digitalWrite(SPIDEVICE_RES, HIGH)
#define LCD_RES_Low()  digitalWrite(SPIDEVICE_RES, LOW)
#define LCD_CS_High()  digitalWrite(SPIDEVICE_CS, HIGH)
#define LCD_CS_Low()   digitalWrite(SPIDEVICE_CS, LOW)
#define LCD_SDA_High() digitalWrite(SPIDEVICE_SDA, HIGH)
#define LCD_SDA_Low()  digitalWrite(SPIDEVICE_SDA, LOW)
#define LCD_SCK_High() digitalWrite(SPIDEVICE_SCK, HIGH)
#define LCD_SCK_Low()  digitalWrite(SPIDEVICE_SCK, LOW)

#ifndef NOKIA105_WIDTH
#define NOKIA105_WIDTH 160
#endif
#ifndef NOKIA105_HEIGHT
#define NOKIA105_HEIGHT 128
#endif
#define WIDTH NOKIA105_WIDTH
#define HEIGHT NOKIA105_HEIGHT
#define nextLineEdge 152
#define spaceBetweenScanLines 16
#define fullLengthVertical 112
#define rotation 0
#define rotateBitmap90 0
#define LOG 0
#define totalPixals (WIDTH*HEIGHT)
#define RGB2BGR 1

#define BLACK 0x0000
#define NAVY 0x000F
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

class NokiaStation {
public:
  NokiaStation(int SDA, int SCLK, int RST, int CS);

  void setBacklightPin(int pin);
  void begin();
  void reset();
  void displayOn();
  void displayOff();
  void invertDisplay(bool invert = true);
  void setRotation(uint8_t r);
  void initDisplay(bool doReset = true);

  void PWMinit();
  void setLcdBrightness(uint16_t PWM);
  void setDrawPosition(unsigned char x, unsigned char y);
  void setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void image1d(uint16_t w, uint16_t h, uint16_t shiftX, uint16_t shiftY, const uint16_t image[]);
  void printDigit(unsigned int a, int16_t x, int16_t y, uint16_t fg, uint16_t bg);
  void drawtext(unsigned char c, unsigned char x, unsigned char y, uint16_t color);
  void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void smpteTest();
  void printBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
  void backgroundColor(uint16_t c);
  void colorPalletTest();
  void lineHorixontal(int16_t x, int16_t y, int16_t w, uint16_t color);
  void lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color);
  void circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void printSingleChar(unsigned char c, unsigned char x, unsigned char y, uint16_t fg, uint16_t bg);
  void printStringChar(const char *str, unsigned char x, unsigned char y, uint16_t fg, uint16_t bg);
  void printString(const char *str, uint8_t x, uint8_t y, uint16_t fg, uint16_t bg);
  void displayClear();

private:
  void writeNokiaCommand(unsigned char c);
  void writeNokiaData(unsigned char c);

  int SPIDEVICE_CS;
  int SPIDEVICE_RES;
  int SPIDEVICE_SDA;
  int SPIDEVICE_SCK;
  int backLightPin;
  uint8_t rotationValue;
};

#endif
