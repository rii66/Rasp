#ifndef _NOKIA105_LCD_H
#define _NOKIA105_LCD_H

#include <Arduino.h>
#include "fonts.h"

#define LCD_RES_High() digitalWrite(SPIDEVICE_RES, HIGH)
#define LCD_RES_Low()  digitalWrite(SPIDEVICE_RES, LOW)
#define LCD_CS_High()  digitalWrite(SPIDEVICE_CS, HIGH)
#define LCD_CS_Low()   digitalWrite(SPIDEVICE_CS, LOW)
#define LCD_SDA_High() digitalWrite(SPIDEVICE_SDA, HIGH)
#define LCD_SDA_Low()  digitalWrite(SPIDEVICE_SDA, LOW)
#define LCD_SCK_High() digitalWrite(SPIDEVICE_SCK, HIGH)
#define LCD_SCK_Low()  digitalWrite(SPIDEVICE_SCK, LOW)

#define NOKIA105_WIDTH  128
#define NOKIA105_HEIGHT 160
#define NOKIA105_X_OFFSET 2
#define NOKIA105_Y_OFFSET 0
#define WIDTH  NOKIA105_WIDTH
#define HEIGHT NOKIA105_HEIGHT

#define BLACK 0x0000
#define BLUE  0x001F
#define GREEN 0x07E0
#define RED   0xF800
#define WHITE 0xFFFF

class Nokia105 {
public:
  Nokia105(int SID, int SCLK, int RST, int CS);
  void initDisplay(bool doReset = true);
  void reset();
  void displayOn();
  void displayOff();
  void setRotation(uint8_t r);
  void backgroundColor(uint16_t c);
  void setDrawPosition(unsigned char x, unsigned char y);
  void setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

private:
  void writeNokiaCommand(unsigned char c);
  void writeNokiaData(unsigned char c);
  int SPIDEVICE_CS;
  int SPIDEVICE_RES;
  int SPIDEVICE_SDA;
  int SPIDEVICE_SCK;
  uint8_t rotationValue;
};

#endif
