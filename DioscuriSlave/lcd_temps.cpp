#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "tip.h"
#include "Nokia105_LCD.h"
#include "fonts.h"
#include <string.h>
#include <stdio.h>

static Nokia105 lcdSolder(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);
static Nokia105 lcdHotAir(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS2);

static bool ready = false;
static uint32_t lastDraw = 0;
static const uint32_t DRAW_MS = 350;

static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;
static TipID lastTipId = (TipID)255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static char lastAirMode[16] = "";

static const int16_t W = 128;
static const int16_t H = 160;
static const uint16_t HDR_BG = 0x2104;   // warna header bar

// ============================================================
static void forceCsHigh()
{
    pinMode(PIN_LCD_CS1, OUTPUT);
    pinMode(PIN_LCD_CS2, OUTPUT);
    digitalWrite(PIN_LCD_CS1, HIGH);
    digitalWrite(PIN_LCD_CS2, HIGH);
    delayMicroseconds(60);
}

static void clearRegion(Nokia105& lcd, int x, int y, int w, int h, uint16_t col = BLACK)
{
    lcd.fillRectangle(x, y, w, h, col);
}

static void printCenter(Nokia105& lcd, const char* s, int y, uint16_t fg)
{
    int len = strlen(s);
    int x = (W - len * 8) / 2;
    if (x < 0) x = 0;
    lcd.printString(s, (uint8_t)x, (uint8_t)y, fg, BLACK);
}

static void printLeft(Nokia105& lcd, const char* s, int x, int y, uint16_t fg, uint16_t bg = BLACK)
{
    lcd.printString(s, (uint8_t)x, (uint8_t)y, fg, bg);
}

// --- digit 2x (16x32) dari font8x16 ---
static void drawChar2x(Nokia105& lcd, char c, int x, int y, uint16_t fg, uint16_t bg)
{
    if (c < 0x20 || c > 0x7A) c = '?';
    const char* glyph = font8x16[c - 0x20];

    for (uint8_t row = 0; row < 16; row++) {
        uint8_t bits = (uint8_t)glyph[row];
        for (uint8_t col = 0; col < 8; col++) {
            uint16_t colr = (bits & 0x01) ? fg : bg;
            // 2x2 block
            lcd.fillRectangle(x + col * 2, y + row * 2, 2, 2, colr);
            bits >>= 1;
        }
    }
}

static void printTemp2x(Nokia105& lcd, int temp, int y)
{
    char buf[8];
    if (temp < 0) snprintf(buf, sizeof(buf), "---");
    else          snprintf(buf, sizeof(buf), "%d", temp);

    int len = strlen(buf);
    // tiap digit 16 px, derajat + C \~ 20 px
    int totalW = len * 16 + 20;
    int x = (W - totalW) / 2;
    if (x < 2) x = 2;

    clearRegion(lcd, 2, y, W - 4, 34, BLACK);

    for (int i = 0; i < len; i++) {
        drawChar2x(lcd, buf[i], x + i * 16, y, ORANGE, BLACK);
    }

    // derajat ° (lingkaran kecil) + C
    int dx = x + len * 16 + 2;
    int dy = y + 2;
    // lingkaran derajat
    lcd.circle(dx + 3, dy + 4, 3, ORANGE);
    lcd.circle(dx + 3, dy + 4, 2, BLACK);   // hollow
    // C biasa di samping
    printLeft(lcd, "C", dx + 8, y + 8, ORANGE, BLACK);
}

// Warna bar gradient
static uint16_t barColor(int pct)
{
    if (pct < 25) return GREEN;
    if (pct < 50) return YELLOW;
    if (pct < 75) return ORANGE;
    return RED;
}

static void drawBar(Nokia105& lcd, int x, int y, int w, int h, int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    lcd.fillRectangle(x, y, w, h, DARKGREY);
    lcd.fillRectangle(x + 1, y + 1, w - 2, h - 2, BLACK);
    int fw = ((w - 4) * pct) / 100;
    if (fw > 0)
        lcd.fillRectangle(x + 2, y + 2, fw, h - 4, barColor(pct));
}

static void drawPill(Nokia105& lcd, int x, int y, int w, int h,
                     const char* label, bool active, uint16_t activeCol)
{
    lcd.fillRectangle(x, y, w, h, active ? activeCol : DARKGREY);
    if (active)
        lcd.lineHorizontal(x + 1, y, w - 2, WHITE);

    int len = strlen(label);
    int tx = x + (w - len * 8) / 2;
    if (tx < x + 1) tx = x + 1;
    lcd.printString(label, (uint8_t)tx, (uint8_t)(y + 2),
                    active ? BLACK : LIGHTGREY,
                    active ? activeCol : DARKGREY);
}

static const char* tipLabel()
{
    if (activeTip && activeTip->name) {
        if (strcmp(activeTip->name, "CUSTOM") == 0) return "CUST";
        return activeTip->name;
    }
    switch (currentTip) {
        case TIP_T12:    return "T12";
        case TIP_C210:   return "C210";
        case TIP_CUSTOM: return "CUST";
        case TIP_AUTO:   return "AUTO";
        default:         return "TIP";
    }
}

// ============================================================
static void drawSolderStatic(Nokia105& lcd)
{
    lcd.displayClear();

    lcd.fillRectangle(0, 0, W, 18, HDR_BG);
    lcd.lineHorizontal(0, 0,  W, ORANGE);
    lcd.lineHorizontal(0, 17, W, ORANGE);
    printLeft(lcd, "SOLDER", 4, 1, ORANGE, HDR_BG);

    lcd.lineHorizontal(2, 72, W - 4, DARKGREY);
    lcd.lineHorizontal(2, 138, W - 4, DARKGREY);
}

static void drawHotAirStatic(Nokia105& lcd)
{
    lcd.displayClear();

    lcd.fillRectangle(0, 0, W, 18, HDR_BG);
    lcd.lineHorizontal(0, 0,  W, ORANGE);
    lcd.lineHorizontal(0, 17, W, ORANGE);
    printLeft(lcd, "HOT AIR", 4, 1, ORANGE, HDR_BG);

    lcd.lineHorizontal(2, 72, W - 4, DARKGREY);
    lcd.lineHorizontal(2, 138, W - 4, DARKGREY);
}

// ============================================================
static void drawSolderScreen(bool force)
{
    const int ct  = currentTemp;
    const int tt  = targetTemp;
    const int pwm = pwmOut;
    const int lim = (maxPwmLimit > 0) ? maxPwmLimit : 255;
    const int pwmPct = (lim > 0) ? constrain((pwm * 100) / lim, 0, 100) : 0;

    const bool tipErr  = tipError;
    const bool sleepOn = sleeping;
    const bool boostOn = boostMode;
    const bool overOn  = overHeat;
    const uint8_t tipMode = currentTipMode;
    const TipID tipId = currentTip;
    const bool heatOn = (pwm > 0 && !tipErr && !sleepOn && !overOn);

    const bool tempChanged = (ct != lastSolderCt);
    const bool setChanged  = (tt != lastSolderTt);
    const bool pwmChanged  = (pwm != lastSolderPwm);
    const bool tipChanged  = (tipId != lastTipId) || (tipMode != lastTipMode);
    const bool stChanged   = tipErr != lastTipErr || sleepOn != lastSleep ||
                             boostOn != lastBoost || overOn != lastOver ||
                             heatOn != (lastSolderPwm > 0 && !lastTipErr && !lastSleep && !lastOver);

    if (!force && !tempChanged && !setChanged && !pwmChanged && !tipChanged && !stChanged)
        return;

    if (force) drawSolderStatic(lcdSolder);

    char buf[20];

    // Header ON/OFF — clear dengan warna header (bukan hitam)
    if (force || stChanged || pwmChanged) {
        clearRegion(lcdSolder, 72, 1, 56, 16, HDR_BG);
        printLeft(lcdSolder, heatOn ? "ON " : "OFF", 88, 1,
                  heatOn ? GREEN : LIGHTGREY, HDR_BG);
    }

    // Suhu jumbo 2x
    if (force || tempChanged) {
        printTemp2x(lcdSolder, ct, 20);
    }

    // set : xxx C
    if (force || setChanged) {
        clearRegion(lcdSolder, 4, 56, 120, 16);
        snprintf(buf, sizeof(buf), "set : %d C", tt < 0 ? 0 : tt);
        printCenter(lcdSolder, buf, 56, LIGHTGREY);
    }

    // PWM % + bar
    if (force || pwmChanged || tipErr != lastTipErr || overOn != lastOver) {
        clearRegion(lcdSolder, 4, 76, 120, 16);
        snprintf(buf, sizeof(buf), "PWM %d%%", pwmPct);
        printLeft(lcdSolder, buf, 4, 76, WHITE);
        drawBar(lcdSolder, 4, 96, 120, 12, tipErr ? 0 : pwmPct);
    }

    // Pills
    if (force || stChanged || tipChanged || pwmChanged) {
        clearRegion(lcdSolder, 2, 114, 124, 24);

        const bool readyOk = !tipErr && !overOn;
        drawPill(lcdSolder, 3,  116, 38, 20, readyOk ? "RDY" : "ERR", readyOk, readyOk ? GREEN : RED);
        drawPill(lcdSolder, 44, 116, 40, 20, tipLabel(), true, 0x8410);

        if (boostOn)
            drawPill(lcdSolder, 87, 116, 38, 20, "BST", true, ORANGE);
        else if (sleepOn)
            drawPill(lcdSolder, 87, 116, 38, 20, "SLP", true, CYAN);
        else
            drawPill(lcdSolder, 87, 116, 38, 20, "RUN", heatOn, GREEN);
    }

    lastSolderCt = ct; lastSolderTt = tt; lastSolderPwm = pwm;
    lastTipErr = tipErr; lastSleep = sleepOn;
    lastBoost = boostOn; lastOver = overOn;
    lastTipMode = tipMode; lastTipId = tipId;
}

// ============================================================
static void drawHotAirScreen(bool force)
{
    const int ct     = (int)airGetTemp();
    const int tt     = (int)airGetTargetTemp();
    const int fan    = (int)airGetFan();
    const int power  = (int)airGetPower();
    const bool on    = airIsOn();
    const char* mode = airGetModeStr();
    const int fanPct = constrain((fan * 100) / 255, 0, 100);

    const bool tempChanged = (ct != lastAirCt);
    const bool setChanged  = (tt != lastAirTt);
    const bool fanChanged  = (fan != lastFan) || (power != lastAirPower);
    const bool stChanged   = (on != lastAirOn) || (fan != lastFan) ||
                             strcmp(lastAirMode, mode ? mode : "") != 0;

    if (!force && !tempChanged && !setChanged && !fanChanged && !stChanged)
        return;

    if (force) drawHotAirStatic(lcdHotAir);

    char buf[20];

    // Header ON/OFF — sama, clear dengan HDR_BG
    if (force || stChanged) {
        clearRegion(lcdHotAir, 80, 1, 48, 16, HDR_BG);
        printLeft(lcdHotAir, on ? "ON " : "OFF", 96, 1,
                  on ? GREEN : LIGHTGREY, HDR_BG);
    }

    // Suhu jumbo 2x
    if (force || tempChanged) {
        printTemp2x(lcdHotAir, ct, 20);
    }

    // set
    if (force || setChanged) {
        clearRegion(lcdHotAir, 4, 56, 120, 16);
        snprintf(buf, sizeof(buf), "set : %d C", tt < 0 ? 0 : tt);
        printCenter(lcdHotAir, buf, 56, LIGHTGREY);
    }

    // FAN % + bar
    if (force || fanChanged) {
        clearRegion(lcdHotAir, 4, 76, 120, 16);
        snprintf(buf, sizeof(buf), "FAN %d%%", fanPct);
        printLeft(lcdHotAir, buf, 4, 76, WHITE);
        drawBar(lcdHotAir, 4, 96, 120, 12, fanPct);
    }

    // Pills
    if (force || stChanged || fanChanged) {
        clearRegion(lcdHotAir, 2, 114, 124, 24);
        const bool fanOk = fanPct > 5;
        drawPill(lcdHotAir, 3,  116, 38, 20, "RDY", true, GREEN);
        drawPill(lcdHotAir, 44, 116, 40, 20, on ? "HEAT" : "OFF", on, ORANGE);
        drawPill(lcdHotAir, 87, 116, 38, 20, fanOk ? "AIR" : "FAN", fanOk, CYAN);
    }

    lastAirCt = ct; lastAirTt = tt; lastFan = fan;
    lastAirPower = power; lastAirOn = on;
    strncpy(lastAirMode, mode ? mode : "", sizeof(lastAirMode) - 1);
    lastAirMode[sizeof(lastAirMode) - 1] = '\0';
}

// ============================================================
void initLcdTemps()
{
    forceCsHigh();

    lcdHotAir.begin(4000000);
    forceCsHigh();
    delay(40);

    lcdSolder.begin(4000000);
    forceCsHigh();
    delay(40);

    lcdSolder.initDisplaySoft();
    forceCsHigh();
    delay(15);

    lcdHotAir.initDisplaySoft();
    forceCsHigh();

    lcdSolder.setRotation(0);
    forceCsHigh();
    lcdHotAir.setRotation(0);
    forceCsHigh();

    ready = true;
    lastDraw = 0;

    lastSolderCt = -999; lastSolderTt = -999; lastSolderPwm = -999;
    lastTipErr = false; lastSleep = false; lastBoost = false; lastOver = false;
    lastTipMode = 255; lastTipId = (TipID)255;

    lastAirCt = -999; lastAirTt = -999; lastFan = -999; lastAirPower = -999;
    lastAirOn = false; lastAirMode[0] = '\0';

    drawSolderScreen(true);
    forceCsHigh();
    drawHotAirScreen(true);
    forceCsHigh();
}

void updateLcdTemps()
{
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;
    lastDraw = millis();

    drawSolderScreen(false);
    forceCsHigh();
    drawHotAirScreen(false);
    forceCsHigh();
}
