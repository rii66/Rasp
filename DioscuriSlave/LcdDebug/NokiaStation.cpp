#include "NokiaStation.h"
#include "fonts.h"
#include "cmd.h"

NokiaStation::NokiaStation(int SDA, int SCLK, int RST, int CS)
  : SPIDEVICE_CS(CS), SPIDEVICE_RES(RST), SPIDEVICE_SDA(SDA),
    SPIDEVICE_SCK(SCLK), backLightPin(-1), rotationValue(0) {}

void NokiaStation::setBacklightPin(int pin) {
  backLightPin = pin;
  if (pin >= 0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void NokiaStation::reset() {
  LCD_RES_Low();
  delay(10);
  LCD_RES_High();
  delay(120);
}

void NokiaStation::begin() { initDisplay(); }

void NokiaStation::displayOn() { writeNokiaCommand(NOKIA105_DISPON); }
void NokiaStation::displayOff() { writeNokiaCommand(NOKIA105_DISPOFF); }
void NokiaStation::invertDisplay(bool invert) {
  writeNokiaCommand(invert ? NOKIA105_INVON : NOKIA105_INVOFF);
}

void NokiaStation::setRotation(uint8_t r) {
  rotationValue = r & 3;
  uint8_t mad;
  switch (rotationValue) {
    case 0: mad = 0x08; break;
    case 1: mad = 0x68; break;
    case 2: mad = 0xC8; break;
    case 3: mad = 0x68; break;
  }
  writeNokiaCommand(0x36);
  writeNokiaData(mad);
}

void NokiaStation::writeNokiaCommand(unsigned char Cmd) {
  LCD_CS_Low();
  LCD_SDA_Low();
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(SPIDEVICE_SDA, (Cmd & mask) ? HIGH : LOW);
    delayMicroseconds(3);
    delayMicroseconds(3);
    LCD_SCK_High();
    delayMicroseconds(3);
    LCD_SCK_Low();
    delayMicroseconds(3);
  }
  LCD_CS_High();
}

void NokiaStation::writeNokiaData(unsigned char Data) {
  LCD_CS_Low();
  LCD_SDA_High();
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(SPIDEVICE_SDA, (Data & mask) ? HIGH : LOW);
    delayMicroseconds(3);
    LCD_SCK_High();
    delayMicroseconds(3);
    LCD_SCK_Low();
    delayMicroseconds(3);
  }
  LCD_CS_High();
}

void NokiaStation::initDisplay(bool doReset) {
  pinMode(SPIDEVICE_CS, OUTPUT);
  pinMode(SPIDEVICE_RES, OUTPUT);
  pinMode(SPIDEVICE_SDA, OUTPUT);
  pinMode(SPIDEVICE_SCK, OUTPUT);

  digitalWrite(SPIDEVICE_CS, HIGH);
  digitalWrite(SPIDEVICE_SCK, LOW);
  digitalWrite(SPIDEVICE_SDA, LOW);

  if (doReset) reset();

  writeNokiaCommand(0x01);
  delay(150);
  writeNokiaCommand(0x11);
  delay(150);

  writeNokiaCommand(0xC1);
  writeNokiaData(0xFF);
  writeNokiaData(0x83);
  writeNokiaData(0x40);

  writeNokiaCommand(0xCA);
  writeNokiaData(0x70);
  writeNokiaData(0x00);
  writeNokiaData(0xD9);

  writeNokiaCommand(0xB0);
  writeNokiaData(0x01);
  writeNokiaData(0x11);

  writeNokiaCommand(0xC9);
  writeNokiaData(0x90);
  writeNokiaData(0x49);
  writeNokiaData(0x10);
  writeNokiaData(0x28);
  writeNokiaData(0x28);
  writeNokiaData(0x10);
  writeNokiaData(0x00);
  writeNokiaData(0x06);
  delay(20);

  writeNokiaCommand(0xC2);
  writeNokiaData(0x60);
  writeNokiaData(0x71);
  writeNokiaData(0x01);
  writeNokiaData(0x0E);
  writeNokiaData(0x05);
  writeNokiaData(0x02);
  writeNokiaData(0x09);
  writeNokiaData(0x31);
  writeNokiaData(0x0A);

  writeNokiaCommand(0xC3);
  writeNokiaData(0x67);
  writeNokiaData(0x30);
  writeNokiaData(0x61);
  writeNokiaData(0x17);
  writeNokiaData(0x48);
  writeNokiaData(0x07);
  writeNokiaData(0x05);
  writeNokiaData(0x33);
  delay(10);

  writeNokiaCommand(0xB5);
  writeNokiaData(0x35);
  writeNokiaData(0x20);
  writeNokiaData(0x45);

  writeNokiaCommand(0xB4);
  writeNokiaData(0x33);
  writeNokiaData(0x25);
  writeNokiaData(0x4C);
  delay(10);

  writeNokiaCommand(0x3A);
  writeNokiaData(0x05);

  setRotation(0);

  writeNokiaCommand(0x13);
  delay(10);
  writeNokiaCommand(0x29);
  delay(20);

  backgroundColor(BLACK);
}

void NokiaStation::displayClear() {
  backgroundColor(BLACK);
}

void NokiaStation::setDrawPosition(unsigned char x, unsigned char y) {
  setDrawPositionAxis(x, y, x + 7, y + 15);
}

void NokiaStation::setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  writeNokiaCommand(0x2A);
  writeNokiaData(0);
  writeNokiaData(x0);
  writeNokiaData(0);
  writeNokiaData(x1);

  writeNokiaCommand(0x2B);
  writeNokiaData(0);
  writeNokiaData(y0);
  writeNokiaData(0);
  writeNokiaData(y1);

  writeNokiaCommand(0x2C);
}

void NokiaStation::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;
  setDrawPositionAxis(x, y, x, y);
  writeNokiaData(color >> 8);
  writeNokiaData(color);
}

void NokiaStation::fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if ((x >= WIDTH) || (y >= HEIGHT)) return;

  int16_t x2, y2;
  if (((x2 = x + w - 1) < 0) || ((y2 = y + h - 1) < 0)) return;
  if (x2 >= WIDTH) w = WIDTH - x;
  if (x < 0) { w += x; x = 0; }
  if (y2 >= HEIGHT) h = HEIGHT - y;
  if (y < 0) { h += y; y = 0; }

  setDrawPositionAxis(x, y, x + w - 1, y + h - 1);

  uint8_t hi = color >> 8, lo = color;
  int32_t n = (int32_t)w * (int32_t)h;
  while (n--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void NokiaStation::backgroundColor(uint16_t c) {
  setDrawPositionAxis(0, 0, WIDTH - 1, HEIGHT - 1);

  uint8_t hi = c >> 8, lo = c;
  uint32_t n = (uint32_t)WIDTH * HEIGHT;
  while (n--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void NokiaStation::printSingleChar(unsigned char c, unsigned char x, unsigned char y,
                                   uint16_t fg, uint16_t bg) {
  if (c < 0x20) c = 0x20;
  c -= 0x20;

  setDrawPositionAxis(x, y, x + 7, y + 15);

  for (uint8_t row = 0; row < 16; row++) {
    uint8_t bits = font8x16[c][row];
    for (uint8_t col = 0; col < 8; col++) {
      uint16_t color = (bits & 0x01) ? fg : bg;
      writeNokiaData(color >> 8);
      writeNokiaData(color);
      bits >>= 1;
    }
  }
}

void NokiaStation::printString(const char *str, uint8_t x, uint8_t y,
                               uint16_t fg, uint16_t bg) {
  while (*str) {
    if (x > nextLineEdge) {
      y += spaceBetweenScanLines;
      x = 0;
    }
    if (y > fullLengthVertical) break;
    printSingleChar(*str++, x, y, fg, bg);
    x += 8;
  }
}

void NokiaStation::printStringChar(const char *str, unsigned char x, unsigned char y,
                                   uint16_t fg, uint16_t bg) {
  while (*str) {
    printSingleChar(*str++, x, y, fg, bg);
    x += 8;
  }
}

void NokiaStation::printDigit(unsigned int a, int16_t x, int16_t y,
                              uint16_t fg, uint16_t bg) {
  char cstr[11];
  snprintf(cstr, sizeof(cstr), "%u", a);
  printString(cstr, x, y, fg, bg);
}

void NokiaStation::lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;
  int16_t y2 = y + h - 1;
  if (y2 < 0) return;
  if (y2 >= HEIGHT) h = HEIGHT - y;
  if (y < 0) { h += y; y = 0; }

  setDrawPositionAxis(x, y, x, y + h - 1);
  uint8_t hi = color >> 8, lo = color;
  while (h--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void NokiaStation::lineHorixontal(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if ((y < 0) || (y >= HEIGHT) || (x >= WIDTH)) return;
  int16_t x2 = x + w - 1;
  if (x2 < 0) return;
  if (x2 >= WIDTH) w = WIDTH - x;
  if (x < 0) { w += x; x = 0; }

  setDrawPositionAxis(x, y, x + w - 1, y);
  uint8_t hi = color >> 8, lo = color;
  while (w--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void NokiaStation::circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;

  drawPixel(x0, y0 + r, color);
  drawPixel(x0, y0 - r, color);
  drawPixel(x0 + r, y0, color);
  drawPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++; ddF_x += 2; f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void NokiaStation::smpteTest() {
  fillRectangle(0,0,18,160,WHITE);
  fillRectangle(18,0,36,160,BLUE);
  fillRectangle(36,0,54,160,RED);
  fillRectangle(54,0,72,160,GREEN);
  fillRectangle(72,0,90,160,CYAN);
  fillRectangle(90,0,108,160,MAGENTA);
  fillRectangle(108,0,126,160,YELLOW);
  fillRectangle(126,0,190,160,BLACK);
}

void NokiaStation::printBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7) byte <<= 1;
      else byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      if (byte & 0x80) drawPixel(x + i, y, color);
    }
  }
}

void NokiaStation::image1d(uint16_t w, uint16_t h, uint16_t shiftX,
                           uint16_t shiftY, const uint16_t image[]) {
  int l = 0;
  for (uint16_t y = 0; y < h; y++) {
    for (uint16_t x = 0; x < w; x++) {
      drawPixel(x + shiftX, y + shiftY, pgm_read_word(&(image[l++])));
    }
  }
}

void NokiaStation::drawtext(unsigned char c, unsigned char x, unsigned char y, uint16_t color) {
  unsigned char k, Mline, Ctemp;
  setDrawPosition(x, y);
  c -= 0x20;

  for (Mline = 0; Mline < 16; Mline++) {
    Ctemp = text[c][Mline];
    for (k = 0; k < 8; k++) {
      uint16_t px = (Ctemp & 0x80) ? color : BLACK;
      writeNokiaData(px >> 8);
      writeNokiaData(px);
      Ctemp >>= 1;
    }
    setDrawPosition(x, ++y);
  }
}

void NokiaStation::colorPalletTest() {
  int colorPallete[] = {WHITE,BLUE,RED,GREEN,CYAN,MAGENTA,YELLOW,NAVY,DARKGREEN,DARKCYAN};
  for (int i = 0; i < 10; i++) {
    backgroundColor(colorPallete[i]);
    delay(800);
  }
}

void NokiaStation::PWMinit() {
  if (backLightPin >= 0) {
    pinMode(backLightPin, OUTPUT);
    analogWrite(backLightPin, 0);
  }
}

void NokiaStation::setLcdBrightness(uint16_t PWM) {
  if (backLightPin < 0) return;
  analogWrite(backLightPin, PWM > 255 ? 255 : PWM);
}
