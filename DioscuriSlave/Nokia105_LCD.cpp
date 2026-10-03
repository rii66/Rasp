#include "Nokia105_LCD.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// PIO 9-bit SPI TX (3-wire) - FIXED shared RST + CS timing
// Added guard delays to prevent CS overlap on dual LCD bus.
// ============================================================

static const uint16_t spi9_program_instructions[] = {
    0x6001, // out pins, 1   side 0
    0xb042, // nop           side 1
};

static const struct pio_program spi9_program = {
    .instructions = spi9_program_instructions,
    .length = 2,
    .origin = -1,
};

bool  Nokia105::_pio_ok   = false;
PIO   Nokia105::_pio      = nullptr;
uint  Nokia105::_sm       = 0;
uint  Nokia105::_offset   = 0;
int   Nokia105::_bus_sda  = -1;
int   Nokia105::_bus_sck  = -1;

Nokia105::Nokia105(int sda, int sck, int rst, int cs)
  : _rst(rst), _cs(cs), _rotation(0)
{
  if (_bus_sda < 0) {
    _bus_sda = sda;
    _bus_sck = sck;
  }
}

void Nokia105::csLow()  { gpio_put(_cs, 0); }
void Nokia105::csHigh() { gpio_put(_cs, 1); }

void Nokia105::pioPut(uint16_t val9) {
  while (pio_sm_is_tx_fifo_full(_pio, _sm))
    tight_loop_contents();
  pio_sm_put(_pio, _sm, (uint32_t)val9 << 23);
}

void Nokia105::writeCmd(uint8_t cmd) {
  csLow();
  busy_wait_us(2);
  pioPut(cmd);
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
}

void Nokia105::writeData(uint8_t data) {
  csLow();
  busy_wait_us(2);
  pioPut(0x100 | data);
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
}

void Nokia105::writeData16(uint16_t color) {
  csLow();
  busy_wait_us(2);
  pioPut(0x100 | (color >> 8));
  pioPut(0x100 | (color & 0xFF));
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
}

void Nokia105::begin(uint32_t freq_hz) {
  static bool rst_done = false;

  if (!_pio_ok) {
    _pio = pio0;
    _sm = pio_claim_unused_sm(_pio, true);
    _offset = pio_add_program(_pio, &spi9_program);

    pio_gpio_init(_pio, _bus_sda);
    pio_gpio_init(_pio, _bus_sck);
    pio_sm_set_consecutive_pindirs(_pio, _sm, _bus_sda, 1, true);
    pio_sm_set_consecutive_pindirs(_pio, _sm, _bus_sck, 1, true);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, _offset + 0, _offset + 1);
    sm_config_set_sideset(&c, 1, false, false);
    sm_config_set_out_pins(&c, _bus_sda, 1);
    sm_config_set_sideset_pins(&c, _bus_sck);
    sm_config_set_out_shift(&c, false, true, 9);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    float div = (float)clock_get_hz(clk_sys) / (freq_hz * 2.0f);
    if (div < 1.0f) div = 1.0f;
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(_pio, _sm, _offset, &c);
    pio_sm_set_enabled(_pio, _sm, true);
    _pio_ok = true;
  }

  gpio_init(_cs);
  gpio_set_dir(_cs, GPIO_OUT);
  gpio_put(_cs, 1);

  gpio_init(_rst);
  gpio_set_dir(_rst, GPIO_OUT);
  gpio_put(_rst, 1);

  // Shared RST: keep proven working pattern from display/pio.
  if (!rst_done) {
    reset();
    rst_done = true;
  }

  initDisplaySoft();
}

void Nokia105::reset() {
  gpio_put(_rst, 0);
  delay(10);
  gpio_put(_rst, 1);
  delay(120);
}

void Nokia105::displayOn()  { writeCmd(NOKIA105_DISPON); }
void Nokia105::displayOff() { writeCmd(NOKIA105_DISPOFF); }

void Nokia105::invertDisplay(bool invert) {
  writeCmd(invert ? NOKIA105_INVON : NOKIA105_INVOFF);
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
  writeCmd(NOKIA105_MADCTL);
  writeData(mad);
}

void Nokia105::initDisplay() {
  reset();
  initDisplaySoft();
}

void Nokia105::initDisplaySoft() {
  writeCmd(NOKIA105_SWRESET);
  delay(120);
  writeCmd(NOKIA105_SPLOUT);
  delay(120);
  writeCmd(NOKIA105_COLMOD);
  writeData(0x05);
  setRotation(0);
  writeCmd(NOKIA105_NORON);
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
    default: break;
  }

  writeCmd(NOKIA105_CASET);
  writeData(0);
  writeData(x0 + NOKIA105_X_OFFSET);
  writeData(0);
  writeData(x1 + NOKIA105_X_OFFSET);

  writeCmd(NOKIA105_PASET);
  writeData(0);
  writeData(y0 + NOKIA105_Y_OFFSET);
  writeData(0);
  writeData(y1 + NOKIA105_Y_OFFSET);

  writeCmd(NOKIA105_RAMWR);
}

void Nokia105::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;
  setDrawPositionAxis(x, y, x, y);
  writeData16(color);
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

  csLow();
  busy_wait_us(2);
  uint16_t hi = 0x100 | (color >> 8);
  uint16_t lo = 0x100 | (color & 0xFF);
  int32_t count = (int32_t)w * h;
  while (count--) {
    pioPut(hi);
    pioPut(lo);
  }
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
}

void Nokia105::backgroundColor(uint16_t c) {
  setDrawPositionAxis(0, 0, WIDTH - 1, HEIGHT - 1);

  csLow();
  busy_wait_us(2);
  uint16_t hi = 0x100 | (c >> 8);
  uint16_t lo = 0x100 | (c & 0xFF);
  uint32_t n = (uint32_t)WIDTH * HEIGHT;
  while (n--) {
    pioPut(hi);
    pioPut(lo);
  }
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
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
  csLow();
  busy_wait_us(2);
  uint16_t hi = 0x100 | (color >> 8);
  uint16_t lo = 0x100 | (color & 0xFF);
  while (w--) {
    pioPut(hi);
    pioPut(lo);
  }
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
}

void Nokia105::lineVertical(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;
  int16_t y2 = y + h - 1;
  if (y2 < 0) return;
  if (y2 >= HEIGHT) h = HEIGHT - y;
  if (y < 0) { h += y; y = 0; }

  setDrawPositionAxis(x, y, x, y + h - 1);
  csLow();
  busy_wait_us(2);
  uint16_t hi = 0x100 | (color >> 8);
  uint16_t lo = 0x100 | (color & 0xFF);
  while (h--) {
    pioPut(hi);
    pioPut(lo);
  }
  while (!pio_sm_is_tx_fifo_empty(_pio, _sm))
    tight_loop_contents();
  busy_wait_us(3);
  csHigh();
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
  if (c < 0x20 || c > 0x7A) c = '?';
  c -= 0x20;

  setDrawPosition(x, y);

  for (uint8_t row = 0; row < 16; row++) {
    uint8_t bits = font8x16[c][row];
    for (uint8_t col = 0; col < 8; col++) {
      writeData16((bits & 0x01) ? fg : bg);
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
    delay(300);
  }
}
