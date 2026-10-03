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

/* ===== Temperature panels ===== */
static void drawTempPanel(Nokia105& lcd, int value, int16_t y,
                          uint16_t accent, uint16_t digitColor) {
    // Full-width color accent + dark panel.
    lcd.fillRectangle(2, y, 104, 46, DARKGREY);
    lcd.fillRectangle(3, y + 1, 102, 44, BLACK);

    // Double accent border.
    lcd.fillRectangle(2, y, 104, 2, accent);
    lcd.fillRectangle(2, y + 44, 104, 2, accent);
    lcd.fillRectangle(2, y, 2, 46, accent);
    lcd.fillRectangle(104, y, 2, 46, accent);

    drawBigTemp(lcd, value, 6, y + 10, digitColor);
}

static uint16_t pwmColor(int pwm) {
    // 0% = green, 50% = yellow, 100% = red
    if (pwm < 0) pwm = 0;
    if (pwm > 255) pwm = 255;

    uint8_t r, g;
    if (pwm <= 128) {
        r = (uint8_t)((pwm * 255L) / 128);
        g = 255;
    } else {
        r = 255;
        g = (uint8_t)(((255 - pwm) * 255L) / 127);
    }

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3);
}

static void drawPwmBar(Nokia105& lcd, int pwm) {
    if (pwm < 0) pwm = 0;
    if (pwm > 255) pwm = 255;

    // Discrete vertical bar like battery / Wi-Fi indicator.
    const int x = 114;
    const int y = 30;
    const int w = 10;
    const int segH = 11;
    const int gap = 3;
    const int segs = 8;

    int level = (pwm * segs + 254) / 255;
    if (level > segs) level = segs;

    for (int i = 0; i < segs; i++) {
        const int sy = y + (segs - 1 - i) * (segH + gap);
        if (i < level) {
            int colorLevel = (i * 255) / (segs - 1);
            lcd.fillRectangle(x, sy, w, segH, pwmColor(colorLevel));
        } else {
            lcd.fillRectangle(x, sy, w, segH, DARKGREY);
        }
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (pwm * 100) / 255);
    lcd.printString(buf, 104, 136, LIGHTGREY, BLACK);
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

    // Compact header: STATUS no longer collides with ON/OFF/ERROR.
    const char* status = tipError ? "ERROR" :
                         sleeping ? "SLEEP" :
                         boostMode ? "BOOST" :
                         (pwm > 0 ? "ON" : "OFF");
    lcdSolder.printString("STAT", 58, 2, LIGHTGREY, BLACK);
    lcdSolder.printString(status, 90, 2, tipError ? RED :
                          sleeping ? YELLOW :
                          boostMode ? YELLOW :
                          (pwm > 0 ? GREEN : DARKGREY), BLACK);

    // Current temperature: only this value is large.
    drawTempPanel(lcdSolder, ct, 20, CYAN, WHITE);
    lcdSolder.printString("TEMP", 6, 22, LIGHTGREY, BLACK);
    drawBigTemp(lcdSolder, ct, 6, 34, WHITE);

    // Set temperature: compact label + normal-size value.
    drawTempPanel(lcdSolder, tt, 69, GREEN, GREEN);
    lcdSolder.printString("SET", 6, 72, LIGHTGREY, BLACK);
    lcdSolder.printDigit(tt, 6, 87, GREEN, BLACK);

    drawPwmBar(lcdSolder, pwm);

    // Bottom status / tip state.
    lcdSolder.printString(tipError ? "NO TIP" :
                         boostMode ? "BOOST" :
                         sleeping ? "SLEEP" : "TIP OK",
                         4, 130,
                         tipError ? RED : (boostMode ? YELLOW : GREEN), BLACK);
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
    lcdHotAir.printString("STAT", 58, 2, LIGHTGREY, BLACK);
    lcdHotAir.printString(mode, 90, 2, on ? GREEN : DARKGREY, BLACK);

    // Current temperature: only this value is large.
    drawTempPanel(lcdHotAir, ct, 20, CYAN, WHITE);
    lcdHotAir.printString("TEMP", 6, 22, LIGHTGREY, BLACK);
    drawBigTemp(lcdHotAir, ct, 6, 34, WHITE);

    // Set temperature: compact label + normal-size value.
    drawTempPanel(lcdHotAir, tt, 69, GREEN, GREEN);
    lcdHotAir.printString("SET", 6, 72, LIGHTGREY, BLACK);
    lcdHotAir.printDigit(tt, 6, 87, GREEN, BLACK);

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
