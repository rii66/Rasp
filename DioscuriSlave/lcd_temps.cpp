#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "Nokia105_LCD.h"
#include <string.h>
#include <stdio.h>

static Nokia105 lcdSolder(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);
static Nokia105 lcdHotAir(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS2);

static bool ready = false;
static uint32_t lastDraw = 0;
static const uint32_t DRAW_MS = 400;   // sedikit lebih pelan biar stabil

// cache nilai terakhir
static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static char lastAirMode[16] = "";

static const int16_t W = 128;
static const int16_t H = 160;

// ============================================================
static void forceCsHigh()
{
    pinMode(PIN_LCD_CS1, OUTPUT);
    pinMode(PIN_LCD_CS2, OUTPUT);
    digitalWrite(PIN_LCD_CS1, HIGH);
    digitalWrite(PIN_LCD_CS2, HIGH);
    delayMicroseconds(80);
}

static void printCenter(Nokia105& lcd, const char* s, int y, uint16_t fg)
{
    int len = strlen(s);
    int x = (W - len * 8) / 2;
    if (x < 0) x = 0;
    lcd.printString(s, (uint8_t)x, (uint8_t)y, fg, BLACK);
}

static void printLeft(Nokia105& lcd, const char* s, int x, int y, uint16_t fg)
{
    lcd.printString(s, (uint8_t)x, (uint8_t)y, fg, BLACK);
}

// Hapus area tertentu dulu (anti sisa karakter lama)
static void clearRegion(Nokia105& lcd, int x, int y, int w, int h)
{
    lcd.fillRectangle(x, y, w, h, BLACK);
}

static void drawBar(Nokia105& lcd, int x, int y, int w, int h, int pct, uint16_t col)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    lcd.fillRectangle(x, y, w, h, DARKGREY);
    int fw = ((w - 2) * pct) / 100;
    if (fw > 0)
        lcd.fillRectangle(x + 1, y + 1, fw, h - 2, col);
}

// ============================================================
// Static frame (header + garis) — hanya digambar sekali
// ============================================================
static void drawSolderStatic(Nokia105& lcd)
{
    lcd.displayClear();
    lcd.lineHorizontal(2, 1,  W - 4, ORANGE);
    lcd.lineHorizontal(2, 18, W - 4, ORANGE);
    printCenter(lcd, "SOLDER", 2, ORANGE);

    printLeft(lcd, "C", 108, 32, ORANGE);
    printLeft(lcd, "TIP", 8, 74, LIGHTGREY);
    printCenter(lcd, "DUAL", 140, DARKGREY);
}

static void drawHotAirStatic(Nokia105& lcd)
{
    lcd.displayClear();
    lcd.lineHorizontal(2, 1,  W - 4, ORANGE);
    lcd.lineHorizontal(2, 18, W - 4, ORANGE);
    printCenter(lcd, "HOT AIR", 2, ORANGE);

    printLeft(lcd, "C", 108, 32, ORANGE);
    printLeft(lcd, "FAN", 8, 74, LIGHTGREY);
    printCenter(lcd, "DUAL", 140, DARKGREY);
}

// ============================================================
// SOLDER — partial update
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

    const bool tempChanged = (ct != lastSolderCt);
    const bool setChanged  = (tt != lastSolderTt);
    const bool tipChanged  = (pwm != lastSolderPwm) || tipErr != lastTipErr ||
                             overOn != lastOver || tipMode != lastTipMode;
    const bool stChanged   = (tipErr != lastTipErr) || (sleepOn != lastSleep) ||
                             (boostOn != lastBoost) || (overOn != lastOver) ||
                             (pwm != lastSolderPwm);

    if (!force && !tempChanged && !setChanged && !tipChanged && !stChanged)
        return;

    if (force) {
        drawSolderStatic(lcdSolder);
    }

    char buf[16];

    // --- Big temp ---
    if (force || tempChanged) {
        clearRegion(lcdSolder, 8, 26, 100, 22);
        if (ct < 0) snprintf(buf, sizeof(buf), "---");
        else        snprintf(buf, sizeof(buf), "%d", ct);
        printCenter(lcdSolder, buf, 28, ORANGE);
    }

    // --- SET ---
    if (force || setChanged) {
        clearRegion(lcdSolder, 8, 50, 112, 18);
        snprintf(buf, sizeof(buf), "SET %dC", tt < 0 ? 0 : tt);
        printCenter(lcdSolder, buf, 52, LIGHTGREY);
    }

    // --- TIP % + bar ---
    if (force || tipChanged) {
        int tipLife = tipErr ? 0 : (overOn ? 20 : (pwmPct > 80 ? 70 : 95));
        clearRegion(lcdSolder, 56, 72, 64, 18);
        snprintf(buf, sizeof(buf), "%d%%", tipLife);
        printLeft(lcdSolder, buf, 72, 74, WHITE);
        drawBar(lcdSolder, 8, 94, 112, 12, tipLife, tipErr ? RED : GREEN);
    }

    // --- Status pills ---
    if (force || stChanged) {
        const bool readyOk = !tipErr && !overOn;
        const bool heatOn  = (pwm > 0 && !tipErr && !sleepOn && !overOn);

        // clear 3 pills area
        clearRegion(lcdSolder, 2, 114, 124, 22);

        lcdSolder.fillRectangle(4,  116, 38, 18, readyOk ? GREEN  : DARKGREY);
        lcdSolder.fillRectangle(45, 116, 40, 18, heatOn  ? ORANGE : DARKGREY);
        lcdSolder.fillRectangle(88, 116, 36, 18, sleepOn ? CYAN   : DARKGREY);

        printLeft(lcdSolder, readyOk ? "RDY" : "ERR", 8,  117, readyOk ? BLACK : LIGHTGREY);
        printLeft(lcdSolder, heatOn  ? "HOT" : "OFF", 50, 117, heatOn  ? BLACK : LIGHTGREY);
        printLeft(lcdSolder, sleepOn ? "SLP" : "RUN", 92, 117, sleepOn ? BLACK : LIGHTGREY);
    }

    lastSolderCt  = ct;
    lastSolderTt  = tt;
    lastSolderPwm = pwm;
    lastTipErr    = tipErr;
    lastSleep     = sleepOn;
    lastBoost     = boostOn;
    lastOver      = overOn;
    lastTipMode   = tipMode;
}

// ============================================================
// HOT AIR — partial update
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
                             (strcmp(lastAirMode, mode ? mode : "") != 0);

    if (!force && !tempChanged && !setChanged && !fanChanged && !stChanged)
        return;

    if (force) {
        drawHotAirStatic(lcdHotAir);
    }

    char buf[16];

    // --- Big temp ---
    if (force || tempChanged) {
        clearRegion(lcdHotAir, 8, 26, 100, 22);
        if (ct < 0) snprintf(buf, sizeof(buf), "---");
        else        snprintf(buf, sizeof(buf), "%d", ct);
        printCenter(lcdHotAir, buf, 28, ORANGE);
    }

    // --- SET ---
    if (force || setChanged) {
        clearRegion(lcdHotAir, 8, 50, 112, 18);
        snprintf(buf, sizeof(buf), "SET %dC", tt < 0 ? 0 : tt);
        printCenter(lcdHotAir, buf, 52, LIGHTGREY);
    }

    // --- FAN % + bar ---
    if (force || fanChanged) {
        clearRegion(lcdHotAir, 56, 72, 64, 18);
        snprintf(buf, sizeof(buf), "%d%%", fanPct);
        printLeft(lcdHotAir, buf, 72, 74, WHITE);
        drawBar(lcdHotAir, 8, 94, 112, 12, fanPct, CYAN);
    }

    // --- Status pills ---
    if (force || stChanged) {
        const bool fanOk = fanPct > 5;

        clearRegion(lcdHotAir, 2, 114, 124, 22);

        lcdHotAir.fillRectangle(4,  116, 38, 18, GREEN);
        lcdHotAir.fillRectangle(45, 116, 40, 18, on    ? ORANGE : DARKGREY);
        lcdHotAir.fillRectangle(88, 116, 36, 18, fanOk ? CYAN   : DARKGREY);

        printLeft(lcdHotAir, "RDY", 8,  117, BLACK);
        printLeft(lcdHotAir, on    ? "HOT" : "OFF", 50, 117, on    ? BLACK : LIGHTGREY);
        printLeft(lcdHotAir, fanOk ? "AIR" : "FAN", 92, 117, fanOk ? BLACK : LIGHTGREY);
    }

    lastAirCt    = ct;
    lastAirTt    = tt;
    lastFan      = fan;
    lastAirPower = power;
    lastAirOn    = on;
    strncpy(lastAirMode, mode ? mode : "", sizeof(lastAirMode) - 1);
    lastAirMode[sizeof(lastAirMode) - 1] = '\0';
}

// ============================================================
void initLcdTemps()
{
    forceCsHigh();

    lcdHotAir.begin(4000000);
    forceCsHigh();
    delay(50);

    lcdSolder.begin(4000000);
    forceCsHigh();
    delay(50);

    // re-init solder setelah hotair (penting dual CS)
    lcdSolder.initDisplaySoft();
    forceCsHigh();
    delay(20);

    lcdHotAir.initDisplaySoft();
    forceCsHigh();

    lcdSolder.setRotation(0);
    forceCsHigh();
    lcdHotAir.setRotation(0);
    forceCsHigh();

    ready = true;
    lastDraw = 0;

    // reset cache
    lastSolderCt = -999; lastSolderTt = -999; lastSolderPwm = -999;
    lastTipErr = false; lastSleep = false; lastBoost = false; lastOver = false;
    lastTipMode = 255;

    lastAirCt = -999; lastAirTt = -999; lastFan = -999; lastAirPower = -999;
    lastAirOn = false; lastAirMode[0] = '\0';

    // full static + isi pertama
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
