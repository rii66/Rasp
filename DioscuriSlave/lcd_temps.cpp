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
static const uint32_t DRAW_MS = 300;

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
    delayMicroseconds(100);
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
// SOLDER
// ============================================================
static void drawSolderScreen(bool force)
{
    const int ct  = currentTemp;
    const int tt  = targetTemp;
    const int pwm = pwmOut;
    const int lim = (maxPwmLimit > 0) ? maxPwmLimit : 255;
    const int pwmPct = (lim > 0) ? constrain((pwm * 100) / lim, 0, 100) : 0;

    if (!force &&
        ct == lastSolderCt && tt == lastSolderTt && pwm == lastSolderPwm &&
        tipError == lastTipErr && sleeping == lastSleep &&
        boostMode == lastBoost && overHeat == lastOver &&
        currentTipMode == lastTipMode)
        return;

    lastSolderCt = ct; lastSolderTt = tt; lastSolderPwm = pwm;
    lastTipErr = tipError; lastSleep = sleeping;
    lastBoost = boostMode; lastOver = overHeat;
    lastTipMode = currentTipMode;

    lcdSolder.displayClear();

    // Header
    lcdSolder.lineHorizontal(2, 1, W - 4, ORANGE);
    lcdSolder.lineHorizontal(2, 18, W - 4, ORANGE);
    printCenter(lcdSolder, "SOLDER", 2, ORANGE);

    // Big temp
    char buf[16];
    if (ct < 0) snprintf(buf, sizeof(buf), "---");
    else        snprintf(buf, sizeof(buf), "%d", ct);
    printCenter(lcdSolder, buf, 28, ORANGE);
    printLeft(lcdSolder, "C", 108, 32, ORANGE);

    // SET
    snprintf(buf, sizeof(buf), "SET %dC", tt < 0 ? 0 : tt);
    printCenter(lcdSolder, buf, 52, LIGHTGREY);

    // TIP LIFE
    int tipLife = tipError ? 0 : (overHeat ? 20 : (pwmPct > 80 ? 70 : 95));
    printLeft(lcdSolder, "TIP", 8, 74, LIGHTGREY);
    snprintf(buf, sizeof(buf), "%d%%", tipLife);
    printLeft(lcdSolder, buf, 72, 74, WHITE);
    drawBar(lcdSolder, 8, 94, 112, 12, tipLife, tipError ? RED : GREEN);

    // Status pills
    const bool readyOk = !tipError && !overHeat;
    const bool heatOn  = (pwm > 0 && !tipError && !sleeping && !overHeat);
    const bool sleepOn = sleeping;

    lcdSolder.fillRectangle(4,  116, 38, 18, readyOk ? GREEN : DARKGREY);
    lcdSolder.fillRectangle(45, 116, 40, 18, heatOn  ? ORANGE : DARKGREY);
    lcdSolder.fillRectangle(88, 116, 36, 18, sleepOn ? CYAN : DARKGREY);

    printLeft(lcdSolder, readyOk ? "RDY" : "ERR", 8,  117, readyOk ? BLACK : LIGHTGREY);
    printLeft(lcdSolder, heatOn  ? "HOT" : "OFF", 50, 117, heatOn  ? BLACK : LIGHTGREY);
    printLeft(lcdSolder, sleepOn ? "SLP" : "RUN", 92, 117, sleepOn ? BLACK : LIGHTGREY);

    printCenter(lcdSolder, "DUAL", 140, DARKGREY);
}

// ============================================================
// HOT AIR
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

    if (!force &&
        ct == lastAirCt && tt == lastAirTt && fan == lastFan &&
        power == lastAirPower && on == lastAirOn &&
        strcmp(lastAirMode, mode ? mode : "") == 0)
        return;

    lastAirCt = ct; lastAirTt = tt; lastFan = fan;
    lastAirPower = power; lastAirOn = on;
    strncpy(lastAirMode, mode ? mode : "", sizeof(lastAirMode) - 1);
    lastAirMode[sizeof(lastAirMode) - 1] = '\0';

    lcdHotAir.displayClear();

    // Header
    lcdHotAir.lineHorizontal(2, 1, W - 4, ORANGE);
    lcdHotAir.lineHorizontal(2, 18, W - 4, ORANGE);
    printCenter(lcdHotAir, "HOT AIR", 2, ORANGE);

    // Big temp
    char buf[16];
    if (ct < 0) snprintf(buf, sizeof(buf), "---");
    else        snprintf(buf, sizeof(buf), "%d", ct);
    printCenter(lcdHotAir, buf, 28, ORANGE);
    printLeft(lcdHotAir, "C", 108, 32, ORANGE);

    // SET
    snprintf(buf, sizeof(buf), "SET %dC", tt < 0 ? 0 : tt);
    printCenter(lcdHotAir, buf, 52, LIGHTGREY);

    // FAN
    printLeft(lcdHotAir, "FAN", 8, 74, LIGHTGREY);
    snprintf(buf, sizeof(buf), "%d%%", fanPct);
    printLeft(lcdHotAir, buf, 72, 74, WHITE);
    drawBar(lcdHotAir, 8, 94, 112, 12, fanPct, CYAN);

    // Status
    const bool fanOk = fanPct > 5;
    lcdHotAir.fillRectangle(4,  116, 38, 18, GREEN);
    lcdHotAir.fillRectangle(45, 116, 40, 18, on ? ORANGE : DARKGREY);
    lcdHotAir.fillRectangle(88, 116, 36, 18, fanOk ? CYAN : DARKGREY);

    printLeft(lcdHotAir, "RDY", 8,  117, BLACK);
    printLeft(lcdHotAir, on ? "HOT" : "OFF", 50, 117, on ? BLACK : LIGHTGREY);
    printLeft(lcdHotAir, fanOk ? "AIR" : "FAN", 92, 117, fanOk ? BLACK : LIGHTGREY);

    printCenter(lcdHotAir, "DUAL", 140, DARKGREY);
}

// ============================================================
void initLcdTemps()
{
    forceCsHigh();

    lcdHotAir.begin(4000000);
    forceCsHigh();
    delay(60);

    lcdSolder.begin(4000000);
    forceCsHigh();
    delay(60);

    // Re-init solder (LCD1) after hotair – penting dual CS
    lcdSolder.initDisplaySoft();
    forceCsHigh();
    delay(30);

    lcdHotAir.initDisplaySoft();
    forceCsHigh();

    // Pastikan portrait
    lcdSolder.setRotation(0);
    forceCsHigh();
    lcdHotAir.setRotation(0);
    forceCsHigh();

    ready = true;
    lastDraw = 0;

    lastSolderCt = -999; lastSolderTt = -999; lastSolderPwm = -999;
    lastTipErr = false; lastSleep = false; lastBoost = false; lastOver = false;
    lastTipMode = 255;

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
