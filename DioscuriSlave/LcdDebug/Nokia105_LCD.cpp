#include "Nokia105_LCD.h"
#include "cmd.h"

Nokia105::Nokia105(int SDA, int SCLK, int RST, int CS)
  : SPIDEVICE_CS(CS), SPIDEVICE_RES(RST), SPIDEVICE_SDA(SDA),
    SPIDEVICE_SCK(SCLK), rotationValue(0) {}

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
  writeNokiaCommand(0x28);
}

void Nokia105::setRotation(uint8_t r) {
  rotationValue = r & 3;
  uint8_t mad = 0x08;
  switch (rotationValue) {
    case 0: mad = 0x08; break;
    case 1: mad = 0x68; break;
    case 2: mad = 0xC8; break;
    case 3: mad = 0xA8; break;
  }
  writeNokiaCommand(NOKIA105_MADCTL);
  writeNokiaData(mad);
}

void Nokia105::writeNokiaCommand(unsigned char Cmd) {
  LCD_CS_Low();
  LCD_SDA_Low();
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(SPIDEVICE_SDA, (Cmd & mask) ? HIGH : LOW);
    LCD_SCK_High();
    LCD_SCK_Low();
  }

  LCD_CS_High();
}

void Nokia105::writeNokiaData(unsigned char Data) {
  LCD_CS_Low();
  LCD_SDA_High();
  LCD_SCK_Low();
  LCD_SCK_High();
  LCD_SCK_Low();

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    digitalWrite(SPIDEVICE_SDA, (Data & mask) ? HIGH : LOW);
    LCD_SCK_High();
    LCD_SCK_Low();
  }

  LCD_CS_High();
}

void Nokia105::initDisplay(bool doReset) {
  pinMode(SPIDEVICE_CS, OUTPUT);
  pinMode(SPIDEVICE_RES, OUTPUT);
  pinMode(SPIDEVICE_SDA, OUTPUT);
  pinMode(SPIDEVICE_SCK, OUTPUT);

  digitalWrite(SPIDEVICE_CS, HIGH);
  digitalWrite(SPIDEVICE_SCK, LOW);
  digitalWrite(SPIDEVICE_SDA, LOW);

  if (doReset) reset();

  writeNokiaCommand(NOKIA105_SWRESET);
  delay(120);

  writeNokiaCommand(NOKIA105_SPLOUT);
  delay(120);

  writeNokiaCommand(NOKIA105_COLMOD);
  writeNokiaData(0x05);

  setRotation(0);

  writeNokiaCommand(NOKIA105_NORON);
  delay(10);

  displayOn();
  delay(10);

  backgroundColor(BLACK);
}

void Nokia105::setDrawPosition(unsigned char x, unsigned char y) {
  setDrawPositionAxis(x, y, x + 7, y + 15);
}

void Nokia105::setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t t0, t1;

  switch (rotationValue) {
    case 1:
      t0 = WIDTH - 1 - y1;
      t1 = WIDTH - 1 - y0;
      y0 = x0; x0 = t0;
      y1 = x1; x1 = t1;
      break;

    case 2:
      t0 = x0;
      x0 = WIDTH - 1 - x1;
      x1 = WIDTH - 1 - t0;
      t0 = y0;
      y0 = HEIGHT - 1 - y1;
      y1 = HEIGHT - 1 - t0;
      break;

    case 3:
      t0 = HEIGHT - 1 - x1;
      t1 = HEIGHT - 1 - x0;
      x0 = y0; y0 = t0;
      x1 = y1; y1 = t1;
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

void Nokia105::backgroundColor(uint16_t c) {
  uint8_t hi = c >> 8;
  uint8_t lo = c;

  setDrawPositionAxis(0, 0, WIDTH - 1, HEIGHT - 1);

  for (uint32_t n = (uint32_t)WIDTH * HEIGHT; n; --n) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}
