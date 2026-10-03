#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "Nokia105_LCD.h"

// LCD1 = SOLDER, LCD2 = HOT AIR
static Nokia105 lcdSolder(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);
static Nokia105 lcdHotAir(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS2);

static bool ready = false;
static uint32_t lastDraw = 0;
static const uint32_t DRAW_MS = 200;

static int lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false;

static int lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static const char* lastAirMode = nullptr;


/* ===== Dashboard typography: compact labels + bold 3x7 digits ===== */
static const uint8_t bigDigitFont[10][7] = {
    {0x1F,0x11,0x13,0x15,0x19,0x11,0x1F},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x1E,0x01,0x01,0x1E,0x10,0x10,0x1F},
    {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
    {0x12,0x12,0x12,0x1F,0x02,0x02,0x02},
    {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
    {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}
};

static void drawBigDigit(Nokia105& lcd, uint8_t digit, int16_t x, int16_t y,
                         uint16_t fg, uint16_t bg) {
    if (digit > 9) return;
    lcd.fillRectangle(x, y, 16, 23, bg);

    const uint8_t scale = 3;
    for (uint8_t row = 0; row < 7; row++) {
        uint8_t bits = bigDigitFont[digit][row];
        uint8_t col = 0;
        while (col < 5) {
            if (!(bits & (1 << (4 - col)))) {
                col++;
                continue;
            }
            uint8_t start = col;
            while (col < 5 && (bits & (1 << (4 - col)))) col++;
            lcd.fillRectangle(x + start * scale, y + row * scale,
                              (col - start) * scale, scale, fg);
        }
    }
}

static void drawBigTemp(Nokia105& lcd, int value, int16_t x, int16_t y,
                        uint16_t fg) {
    if (value < 0) value = 0;
    if (value > 999) value = 999;

    char b[4];
    snprintf(b, sizeof(b), "%03d", value);

    lcd.fillRectangle(x, y, 56, 28, BLACK);

    drawBigDigit(lcd, b[0] - '0', x,      y, fg, BLACK);
    drawBigDigit(lcd, b[1] - '0', x + 18, y, fg, BLACK);
    drawBigDigit(lcd, b[2] - '0', x + 36, y, fg, BLACK);

    /* degree symbol */
    lcd.fillRectangle(x + 52, y + 1, 7, 3, fg);
    lcd.fillRectangle(x + 52, y + 4, 3, 3, fg);
    lcd.fillRectangle(x + 56, y + 4, 3, 3, fg);
    lcd.fillRectangle(x + 52, y + 7, 7, 3, fg);
}

static void drawPwmBar(Nokia105& lcd, int pwm) {
    if (pwm < 0) pwm = 0;
    if (pwm > 255) pwm = 255;

    const int x = 112;
    const int y = 30;
    const int w = 10;
    const int h = 112;

    lcd.fillRectangle(x, y, w, h, DARKGREY);

    int filled = (pwm * h) / 255;
    if (filled > 0)
        lcd.fillRectangle(x, y + h - filled, w, filled, CYAN);

}

static void drawSolderScreen(bool force) {
    const int ct = currentTemp;
    const int tt = targetTemp;
    const int pwm = pwmOut;

    if (!force &&
        ct == lastSolderCt && tt == lastSolderTt && pwm == lastSolderPwm &&
        tipError == lastTipErr && sleeping == lastSleep && boostMode == lastBoost) return;

    lastSolderCt = ct; lastSolderTt = tt; lastSolderPwm = pwm;
    lastTipErr = tipError; lastSleep = sleeping; lastBoost = boostMode;

    lcdSolder.printString("SOLDER", 4, 2, CYAN, BLACK);

    const char* status = tipError ? "ERROR" :
                         sleeping ? "SLEEP" :
                         boostMode ? "BOOST" :
                         (pwm > 0 ? "ON" : "OFF");
    lcdSolder.printString("STATUS", 60, 2, LIGHTGREY, BLACK);
    lcdSolder.printString(status, 88, 2, tipError ? RED : WHITE, BLACK);

    lcdSolder.printString("TEMP", 4, 25, LIGHTGREY, BLACK);
    drawBigTemp(lcdSolder, ct, 4, 38, WHITE);

    lcdSolder.printString("SET", 4, 72, LIGHTGREY, BLACK);
    drawBigTemp(lcdSolder, tt, 4, 85, GREEN);

    drawPwmBar(lcdSolder, pwm);

    lcdSolder.printString(boostMode ? "BOOST" :
                         sleeping ? "SLEEP" :
                         tipError ? "NO TIP" : "READY",
                         4, 130,
                         tipError ? RED : (boostMode ? YELLOW : LIGHTGREY), BLACK);
}

static void drawHotAirScreen(bool force) {
    const int ct = (int)airGetTemp();
    const int tt = (int)airGetTargetTemp();
    const int fan = (int)airGetFan();
    const int power = (int)airGetPower();
    const bool on = airIsOn();
    const char* mode = airGetModeStr();

    if (!force && ct == lastAirCt && tt == lastAirTt && fan == lastFan &&
        power == lastAirPower && on == lastAirOn && mode == lastAirMode) return;

    lastAirCt = ct; lastAirTt = tt; lastFan = fan; lastAirPower = power;
    lastAirOn = on; lastAirMode = mode;

    lcdHotAir.printString("HOT AIR", 4, 2, MAGENTA, BLACK);
    lcdHotAir.printString("STATUS", 60, 2, LIGHTGREY, BLACK);
    lcdHotAir.printString(mode, 88, 2, on ? GREEN : DARKGREY, BLACK);

    lcdHotAir.printString("TEMP", 4, 25, LIGHTGREY, BLACK);
    drawBigTemp(lcdHotAir, ct, 4, 38, WHITE);

    lcdHotAir.printString("SET", 4, 72, LIGHTGREY, BLACK);
    drawBigTemp(lcdHotAir, tt, 4, 85, GREEN);

    drawPwmBar(lcdHotAir, fan);

    char buf[12];
    snprintf(buf, sizeof(buf), "PWR %d%%", power);
    lcdHotAir.printString(buf, 4, 130, on ? GREEN : DARKGREY, BLACK);
}

static void forceLcdCsHigh() {
    pinMode(PIN_LCD_CS1, OUTPUT);
    pinMode(PIN_LCD_CS2, OUTPUT);
    digitalWrite(PIN_LCD_CS1, HIGH);
    digitalWrite(PIN_LCD_CS2, HIGH);
    delayMicroseconds(50);
}

void initLcdTemps() {
    // Pattern yang terbukti bekerja di display/pio/pio.ino:
    // 1) init CS1
    // 2) init CS2
    // 3) re-init CS1 setelah CS2
    forceLcdCsHigh();

    lcdSolder.begin(20000000);
    forceLcdCsHigh();
    delay(50);

    lcdHotAir.begin(20000000);
    forceLcdCsHigh();
    delay(50);

    // Re-init LCD1 after LCD2, matching the known-good dual display pattern.
    lcdSolder.initDisplaySoft();
    forceLcdCsHigh();
    delay(50);

    ready = true;
    lastDraw = 0;

    lastSolderCt = -999;
    lastSolderTt = -999;
    lastSolderPwm = -999;
    lastTipErr = false;
    lastSleep = false;
    lastBoost = false;

    lastAirCt = -999;
    lastAirTt = -999;
    lastFan = -999;
    lastAirPower = -999;
    lastAirOn = false;
    lastAirMode = nullptr;
}

void updateLcdTemps() {
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;
    lastDraw = millis();
    drawSolderScreen(false);
    drawHotAirScreen(false);
}
