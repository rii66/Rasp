#include "Nokia105_LCD.h"
#include <stdio.h>   // snprintf

Nokia105::Nokia105(int sda, int sck, int rst, int cs)
  : _sda(sda), _sck(sck), _rst(rst), _cs(cs), _rotation(0) {}

void Nokia105::begin() {
  initDisplay();
}

void Nokia105::reset() {
  LCD_RES_Low();
  delay(10);
  LCD_RES_High();
  delay(120);
}

void Nokia105::displayOn() {
  writeNokiaCommand(NOKIA105_DISPON);
}

void Nokia105::displayOff() {
  writeNokiaCommand(NOKIA105_DISPOFF);
}

void Nokia105::invertDisplay(bool invert) {
  writeNokiaCommand(invert ? NOKIA105_INVON : NOKIA105_INVOFF);
}

void Nokia105::setRotation(uint8_t r) {
  _rotation = r & 3;
  uint8_t mad = 0x08;
  switch (_rotation) {
    case 0: mad = 0x08; break;
    case 1: mad = 0x68; break;
    case 2: mad = 0xC8; break;
    case 3: mad = 0xA8; break;
  }
  writeNokiaCommand(NOKIA105_MADCTL);
  writeNokiaData(mad);
}

// 9-bit SPI transfer (DC bit + 8 data bits)
void Nokia105::writeNokiaCommand(uint8_t cmd) {
  LCD_CS_Low();
  LCD_SDA_Low();          // DC = 0 (command)
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(_sda, (cmd & mask) ? HIGH : LOW);
    LCD_SCK_High();
    LCD_SCK_Low();
  }
  LCD_CS_High();
}

void Nokia105::writeNokiaData(uint8_t data) {
  LCD_CS_Low();
  LCD_SDA_High();         // DC = 1 (data)
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(_sda, (data & mask) ? HIGH : LOW);
    LCD_SCK_High();
    LCD_SCK_Low();
  }
  LCD_CS_High();
}

void Nokia105::initDisplay() {
  pinMode(_cs,  OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(_sda, OUTPUT);
  pinMode(_sck, OUTPUT);

  digitalWrite(_cs,  HIGH);
  digitalWrite(_sck, LOW);
  digitalWrite(_sda, LOW);

  reset();

  writeNokiaCommand(NOKIA105_SWRESET);
  delay(120);
  writeNokiaCommand(NOKIA105_SPLOUT);
  delay(120);
  writeNokiaCommand(NOKIA105_COLMOD);
  writeNokiaData(0x05);          // 16-bit color
  setRotation(0);
  writeNokiaCommand(NOKIA105_NORON);
  delay(10);
  displayOn();
  delay(10);
  displayClear();
}

void Nokia105::setDrawPosition(unsigned char x, unsigned char y) {
  setDrawPositionAxis(x, y, x + 7, y + 15);
}

void Nokia105::setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t t0, t1;
  switch (_rotation) {
    case 1:
      t0 = WIDTH - 1 - y1; t1 = WIDTH - 1 - y0;
      y0 = x0; x0 = t0; y1 = x1; x1 = t1;
      break;
    case 2:
      t0 = x0; x0 = WIDTH - 1 - x1; x1 = WIDTH - 1 - t0;
      t0 = y0; y0 = HEIGHT - 1 - y1; y1 = HEIGHT - 1 - t0;
      break;
    case 3:
      t0 = HEIGHT - 1 - x1; t1 = HEIGHT - 1 - x0;
      x0 = y0; y0 = t0; x1 = y1; y1 = t1;
      break;
    default:
      break;
  }

  writeNokiaCommand(NOKIA105_CASET);
  writeNokiaData(0);
  writeNokiaData(x0 + NOKIA105_X_OFFSET);
  writeNokiaData(0);
  writeNokiaData(x1 + NOKIA105_X_OFFSET);

  writeNokiaCommand(NOKIA105_PASET);
  writeNokiaData(0);
  writeNokiaData(y0 + NOKIA105_Y_OFFSET);
  writeNokiaData(0);
  writeNokiaData(y1 + NOKIA105_Y_OFFSET);

  writeNokiaCommand(NOKIA105_RAMWR);
}

void Nokia105::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;
  setDrawPositionAxis(x, y, x, y);
  writeNokiaData(color >> 8);
  writeNokiaData(color & 0xFF);
}

void Nokia105::fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if ((x >= WIDTH) || (y >= HEIGHT)) return;
  int16_t x2 = x + w - 1;
  int16_t y2 = y + h - 1;
  if ((x2 < 0) || (y2 < 0)) return;

  if (x2 >= WIDTH)  w = WIDTH  - x;
  if (x < 0)        { w += x; x = 0; }
  if (y2 >= HEIGHT) h = HEIGHT - y;
  if (y < 0)        { h += y; y = 0; }

  setDrawPositionAxis(x, y, x + w - 1, y + h - 1);

  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;
  int32_t count = (int32_t)w * h;
  while (count--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void Nokia105::backgroundColor(uint16_t c) {
  setDrawPositionAxis(0, 0, WIDTH - 1, HEIGHT - 1);
  uint8_t hi = c >> 8;
  uint8_t lo = c & 0xFF;
  uint32_t n = (uint32_t)WIDTH * HEIGHT;
  while (n--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void Nokia105::displayClear() {
  backgroundColor(BLACK);
}

void Nokia105::lineHorizontal(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if ((y < 0) || (y >= HEIGHT) || (x >= WIDTH)) return;
  int16_t x2 = x + w - 1;
  if (x2 < 0) return;
  if (x2 >= WIDTH) w = WIDTH - x;
  if (x < 0) { w += x; x = 0; }

  setDrawPositionAxis(x, y, x + w - 1, y);
  uint8_t hi = color >> 8, lo = color & 0xFF;
  while (w--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void Nokia105::lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;
  int16_t y2 = y + h - 1;
  if (y2 < 0) return;
  if (y2 >= HEIGHT) h = HEIGHT - y;
  if (y < 0) { h += y; y = 0; }

  setDrawPositionAxis(x, y, x, y + h - 1);
  uint8_t hi = color >> 8, lo = color & 0xFF;
  while (h--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}

void Nokia105::circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0 + r, color);
  drawPixel(x0, y0 - r, color);
  drawPixel(x0 + r, y0, color);
  drawPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

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

void Nokia105::printSingleChar(unsigned char c, unsigned char x, unsigned char y,
                               uint16_t fg, uint16_t bg) {
  if (c < 0x20 || c > 0x7A) c = '?';  // safety
  c -= 0x20;

  setDrawPosition(x, y);

  for (uint8_t row = 0; row < 16; row++) {
    uint8_t bits = font8x16[c][row];
    for (uint8_t col = 0; col < 8; col++) {
      if (bits & 0x01) {
        writeNokiaData(fg >> 8);
        writeNokiaData(fg & 0xFF);
      } else {
        writeNokiaData(bg >> 8);
        writeNokiaData(bg & 0xFF);
      }
      bits >>= 1;
    }
    setDrawPosition(x, ++y);
  }
}

void Nokia105::printString(const char *str, uint8_t x, uint8_t y,
                           uint16_t fg, uint16_t bg) {
  while (*str) {
    if (x > nextLineEdge - 8) {
      y += spaceBetweenScanLines;
      x = 0;
    }
    if (y > fullLengthVertical - 16) break;

    printSingleChar(*str, x, y, fg, bg);
    str++;
    x += 8;
  }
}

void Nokia105::printDigit(unsigned int value, int16_t x, int16_t y,
                          uint16_t fg, uint16_t bg) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%u", value);
  printString(buf, x, y, fg, bg);

  // simple padding to avoid leftover digits when number gets smaller
  size_t len = strlen(buf);
  for (size_t i = len; i < 5; i++) {
    printSingleChar(' ', x + (i * 8), y, bg, bg);
  }
}

void Nokia105::image1d(uint16_t w, uint16_t h, uint16_t shiftX, uint16_t shiftY,
                       const uint16_t image[]) {
  uint32_t idx = 0;
  for (uint16_t yy = 0; yy < h; yy++) {
    for (uint16_t xx = 0; xx < w; xx++) {
      drawPixel(xx + shiftX, yy + shiftY, image[idx++]);
    }
  }
}

void Nokia105::printBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                           int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      if (byte & 0x80)
        drawPixel(x + i, y, color);
    }
  }
}

void Nokia105::smpteTest() {
  fillRectangle(0,   0, 18, 160, WHITE);
  fillRectangle(18,  0, 18, 160, BLUE);
  fillRectangle(36,  0, 18, 160, RED);
  fillRectangle(54,  0, 18, 160, GREEN);
  fillRectangle(72,  0, 18, 160, CYAN);
  fillRectangle(90,  0, 18, 160, MAGENTA);
  fillRectangle(108, 0, 18, 160, YELLOW);
  fillRectangle(126, 0, 34, 160, BLACK);
}

void Nokia105::colorPalletTest() {
  const uint16_t palette[] = {
    WHITE, BLUE, RED, GREEN, CYAN, MAGENTA, YELLOW,
    NAVY, DARKGREEN, DARKCYAN, MAROON, PURPLE, OLIVE,
    LIGHTGREY, DARKGREY, ORANGE, PINK
  };
  for (uint8_t i = 0; i < 10; i++) {
    backgroundColor(palette[i]);
    delay(600);
  }
}
