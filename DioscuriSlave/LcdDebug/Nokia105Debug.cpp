#include "Nokia105Debug.h"
#include "fonts.h"
#include "cmd.h"

Nokia105Debug::Nokia105Debug(int SDA, int SCLK, int RST, int CS)
  : SPIDEVICE_CS(CS), SPIDEVICE_RES(RST), SPIDEVICE_SDA(SDA),
    SPIDEVICE_SCK(SCLK), backLightPin(-1), rotationValue(0) {}

void Nokia105Debug::setBacklightPin(int pin) {
  backLightPin = pin;
  if (pin >= 0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void Nokia105Debug::reset() {
  LCD_RES_Low();
  delay(10);
  LCD_RES_High();
  delay(120);
}

void Nokia105Debug::begin() {
  initDisplay();
}

void Nokia105Debug::displayOn() {
  writeNokiaCommand(NOKIA105_DISPON);
}

void Nokia105Debug::displayOff() {
  writeNokiaCommand(NOKIA105_DISPOFF);
}

void Nokia105Debug::invertDisplay(bool invert) {
  writeNokiaCommand(invert ? NOKIA105_INVON : NOKIA105_INVOFF);
}

void Nokia105Debug::setRotation(uint8_t r) {
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
  backgroundColor(BLACK);
}

void Nokia105Debug::writeNokiaCommand(unsigned char Cmd) {
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

void Nokia105Debug::writeNokiaData(unsigned char Data) {
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

void Nokia105Debug::displayClear(void) {
  setDrawPositionAxis(0, 0, WIDTH - 1, HEIGHT - 1);
  uint32_t n = (uint32_t)WIDTH * HEIGHT;
  while (n--) {
    writeNokiaData(0);
    writeNokiaData(0);
  }
}

void Nokia105Debug::initDisplay(bool doReset) {
  pinMode(SPIDEVICE_CS, OUTPUT);
  pinMode(SPIDEVICE_RES, OUTPUT);
  pinMode(SPIDEVICE_SDA, OUTPUT);
  pinMode(SPIDEVICE_SCK, OUTPUT);

  digitalWrite(SPIDEVICE_CS, HIGH);
  digitalWrite(SPIDEVICE_SCK, LOW);
  digitalWrite(SPIDEVICE_SDA, LOW);

  if (doReset) {
    reset();
  }

  writeNokiaCommand(0x01);   // SWRESET
  delay(150);

  writeNokiaCommand(0x11);   // Sleep Out
  delay(150);

  // Extended command
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

  // Drive ability
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

  // Gamma
  writeNokiaCommand(0xC2);   // SETGAMMAP
  writeNokiaData(0x60);
  writeNokiaData(0x71);
  writeNokiaData(0x01);
  writeNokiaData(0x0E);
  writeNokiaData(0x05);
  writeNokiaData(0x02);
  writeNokiaData(0x09);
  writeNokiaData(0x31);
  writeNokiaData(0x0A);

  writeNokiaCommand(0xC3);   // SETGAMMAN
  writeNokiaData(0x67);
  writeNokiaData(0x30);
  writeNokiaData(0x61);
  writeNokiaData(0x17);
  writeNokiaData(0x48);
  writeNokiaData(0x07);
  writeNokiaData(0x05);
  writeNokiaData(0x33);
  delay(10);

  // Power
  writeNokiaCommand(0xB5);
  writeNokiaData(0x35);
  writeNokiaData(0x20);
  writeNokiaData(0x45);

  writeNokiaCommand(0xB4);
  writeNokiaData(0x33);
  writeNokiaData(0x25);
  writeNokiaData(0x4C);
  delay(10);

  writeNokiaCommand(0x3A);   // COLMOD
  writeNokiaData(0x05);      // 16-bit

  setRotation(0);

  writeNokiaCommand(0x13);   // NORON
  delay(10);

  writeNokiaCommand(0x29);   // DISPON
  delay(20);

  backgroundColor(BLACK);    // paksa isi layar
}


void Nokia105Debug::setDrawPosition(unsigned char x, unsigned char y) {
  setDrawPositionAxis(x, y, x + 7, y + 15);
}

void Nokia105Debug::setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t t0, t1;
  switch (rotationValue) {
    case 1: {
      // 90 CW: logical 160x128 -> physical 128x160
      uint8_t nx0 = HEIGHT - 1 - y1;
      uint8_t nx1 = HEIGHT - 1 - y0;
      uint8_t ny0 = x0;
      uint8_t ny1 = x1;
      x0 = nx0; x1 = nx1; y0 = ny0; y1 = ny1;
      break;
    }
    case 2:
      x0 = WIDTH - 1 - x0;
      x1 = WIDTH - 1 - x1;
      y0 = HEIGHT - 1 - y0;
      y1 = HEIGHT - 1 - y1;
      break;
    case 3: {
      // 270 CW: logical 160x128 -> physical 128x160
      uint8_t nx0 = y0;
      uint8_t nx1 = y1;
      uint8_t ny0 = WIDTH - 1 - x1;
      uint8_t ny1 = WIDTH - 1 - x0;
      x0 = nx0; x1 = nx1; y0 = ny0; y1 = ny1;
      break;
    }
    default: break;
  }
  writeNokiaCommand(NOKIA105_CASET);
  writeNokiaData(0); writeNokiaData(x0 + NOKIA105_X_OFFSET);
  writeNokiaData(0); writeNokiaData(x1 + NOKIA105_X_OFFSET);
  writeNokiaCommand(NOKIA105_PASET);
  writeNokiaData(0); writeNokiaData(y0 + NOKIA105_Y_OFFSET);
  writeNokiaData(0); writeNokiaData(y1 + NOKIA105_Y_OFFSET);
  writeNokiaCommand(NOKIA105_RAMWR);
}

void Nokia105Debug:: drawPixel(int16_t x, int16_t y, uint16_t color) {
if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT))
    return;

setDrawPositionAxis(x, y, x, y);
writeNokiaData(color >> 8);
writeNokiaData(color);
}


void Nokia105Debug:: image1d (uint16_t w, uint16_t h, uint16_t shiftX,uint16_t shiftY, const uint16_t image[] ) {
int l = 0;
for (int y = 0; y < h; y++) {
  for (int x = 0; x < w; x++) {
    drawPixel( x+shiftX, y+shiftY, pgm_read_word(&(image[l])));
    l++;
    }
  }
}

/* void Nokia105Debug:: image2d ... */

/* beta */
void Nokia105Debug:: drawtext(unsigned char c, unsigned char x, unsigned char y ,uint16_t color) {
unsigned char k,Mline,Ctemp;
setDrawPosition(x,y);

c -= 0x20;

for (Mline = 0; Mline < 16; Mline++) {
  Ctemp = text[c][Mline];
  for(k = 0; k < 8; k++) {
    if(Ctemp & 0x80) {
      writeNokiaData(color>>8);
      writeNokiaData(color);
    } else {
      writeNokiaData(0x00>>8);
      writeNokiaData(0x00);
    }
    Ctemp=Ctemp>>1;
    }
    setDrawPosition(x,++y);
  }
}


void Nokia105Debug:: fillRectangle (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
if((x >= WIDTH) || (y >= HEIGHT)) return;
int16_t x2, y2;
if(((x2 = x + w - 1) < 0) ||
   ((y2 = y + h - 1) < 0)) return;
if(x2 >= WIDTH)  w = WIDTH  - x;
if(x < 0) { w += x; x = 0; }
if(y2 >= HEIGHT) h = HEIGHT - y;
if(y < 0) { h += y; y = 0; }

setDrawPositionAxis(x, y, x+w-1, y+h-1);

uint8_t hi = color >> 8, lo = color;
int32_t i  = (int32_t)w * (int32_t)h;

while(i--) {
   writeNokiaData(hi);
   writeNokiaData(lo);
}
}

void Nokia105Debug:: smpteTest() {
fillRectangle(0,0,18,160,WHITE);
fillRectangle(18,0,36,160,BLUE);
fillRectangle(36,0,54,160,RED);
fillRectangle(54,0,72,160,GREEN);
fillRectangle(72,0,90,160,CYAN);
fillRectangle(90,0,108,160,MAGENTA);
fillRectangle(108,0,126,160,YELLOW);
fillRectangle(126,0,190,160,BLACK);
}

void Nokia105Debug:: printBitmap(int16_t x, int16_t y, const uint8_t bitmap[],int16_t w, int16_t h, uint16_t color) {
int16_t byteWidth = (w + 7) / 8;
uint8_t byte = 0;

for (int16_t j = 0; j < h; j++, y++) {
  for (int16_t i = 0; i < w; i++) {
    if (i & 7)
      byte <<= 1;
    else
      byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
    if (byte & 0x80) {
        drawPixel(x + i, y, color);
    }
  }
}
}

void Nokia105Debug:: backgroundColor(uint16_t c) {
uint8_t x, y, hi = c >> 8, lo = c;
setDrawPositionAxis(0, 0, WIDTH-1, HEIGHT-1);

for( y = HEIGHT; y > 0; y--) {
  for(x = WIDTH; x > 0; x--) {
    writeNokiaData(hi);
    writeNokiaData(lo);
  }
}
}

void Nokia105Debug:: colorPalletTest() {
int colorPallete[] = {WHITE,BLUE,RED,GREEN,CYAN,MAGENTA,YELLOW,NAVY,DARKGREEN,DARKCYAN,MAROON,PURPLE,OLIVE,LIGHTGREY,DARKGREY,ORANGE,PINK};
  for(int i = 0; i < 10; i++) {
    backgroundColor(colorPallete[i]);
    delay(800);
  }
}

void Nokia105Debug:: circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
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

void Nokia105Debug::printDigit(unsigned int a, int16_t x, int16_t y, uint16_t forgroundColor, uint16_t backgroundColor) {
  char cstr[11];
  snprintf(cstr, sizeof(cstr), "%u", a);
  printString(cstr, x, y, forgroundColor, backgroundColor);

  if (a < 10) {
    for (int i = 1; i < 4; i++)
      printString(" ", x + (i * 8), y, backgroundColor, backgroundColor);
  } else if (a < 100) {
    printString(" ", x + 16, y, backgroundColor, backgroundColor);
  } else if (a < 1000) {
    printString(" ", x + 24, y, backgroundColor, backgroundColor);
  } else if (a < 10000) {
    printString(" ", x + 32, y, backgroundColor, backgroundColor);
  }
}

void Nokia105Debug:: lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color) {
if ((x < 0) || (x >= WIDTH ) || (y >= HEIGHT)) return;
int16_t y2 = y + h - 1;

if (y2 < 0) return;

if (y2 >= HEIGHT) {
  h = HEIGHT - y;
}

if (y < 0) {
  h += y; y = 0;
}

setDrawPositionAxis(x, y, x, y+h-1);

uint8_t hi = color >> 8, lo = color;
while (h--) {
  writeNokiaData(hi);
  writeNokiaData(lo);
}
}


void Nokia105Debug:: lineHorixontal(int16_t x, int16_t y, int16_t w,uint16_t color) {
if((y < 0) || (y >= HEIGHT )|| (x >= WIDTH)) return;

int16_t x2 = x + w - 1;

if (x2 < 0) return;

if (x2 >= WIDTH) {
  w = WIDTH - x;
}
if (x < 0) {
  w += x; x = 0;
}

setDrawPositionAxis(x, y, x+w-1, y);

uint8_t hi = color >> 8, lo = color;
while (w--) {
  writeNokiaData(hi);
  writeNokiaData(lo);
}
}

void Nokia105Debug:: printSingleChar ( unsigned char c,unsigned char x,unsigned char y,uint16_t forgroundColor,uint16_t backgroundColor) {
unsigned char k,Mline,Ctemp;
setDrawPosition(x,y);
c -= 0x20;

for (Mline = 0; Mline < 16; Mline++) {
  Ctemp = font8x16[c][Mline];
  for(k = 0; k < 8; k++) {
    if(Ctemp & 0x01) {
      writeNokiaData(forgroundColor>>8);
      writeNokiaData(forgroundColor);
    } else {
      writeNokiaData(backgroundColor>>8);
      writeNokiaData(backgroundColor);
    }
    Ctemp=Ctemp>>1;
  }
  setDrawPosition(x,++y);
}
}

void Nokia105Debug:: printStringChar(const char *String,unsigned char x,unsigned char y,uint16_t forgroundColor,uint16_t backgroundColor) {
while (*String) {
  printSingleChar(*String++,x,y,forgroundColor,backgroundColor);
  x+=8;
}
}

void Nokia105Debug:: printString(const char *str,uint8_t x,uint8_t y,uint16_t forgroundColor,uint16_t backgroundColor) {
while(*str!=0) {
  if (x > nextLineEdge) {
    y += spaceBetweenScanLines;
    x = 0;
    if (LOG){
      Serial.println("RESET X = 0 ");
    }
  }
  if (y > fullLengthVertical)
    break;
  if(LOG) {
    Serial.println("Single Char: ");
    Serial.println(*str);
  }
  printSingleChar(*str,x,y,forgroundColor,backgroundColor);
  str++;
  x+=8;
}
}

void Nokia105Debug::PWMinit() {
  if (backLightPin >= 0) {
    pinMode(backLightPin, OUTPUT);
    analogWrite(backLightPin, 0);
  }
}

void Nokia105Debug::setLcdBrightness(uint16_t PWM) {
  if (backLightPin < 0) return;
  uint16_t duty = PWM > 255 ? 255 : PWM;
  analogWrite(backLightPin, (int)duty);
}