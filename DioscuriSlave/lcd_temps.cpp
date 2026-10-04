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
static const uint32_t DRAW_MS = 350;

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
    delayMicroseconds(60);
}

static void clearRegion(Nokia105& lcd, int x, int y, int w, int h)
{
    lcd.fillRectangle(x, y, w, h, BLACK);
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

// Bar dengan border tebal + fill
static void drawBar(Nokia105& lcd, int x, int y, int w, int h, int pct, uint16_t col)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    // outer border
    lcd.fillRectangle(x, y, w, h, DARKGREY);
    // inner dark
    lcd.fillRectangle(x + 1, y + 1, w - 2, h - 2, BLACK);
    // fill
    int fw = ((w - 4) * pct) / 100;
    if (fw > 0)
        lcd.fillRectangle(x + 2, y + 2, fw, h - 4, col);
}

// Pill rounded-ish (isi + teks)
static void drawPill(Nokia105& lcd, int x, int y, int w, int h,
                     const char* label, bool active, uint16_t activeCol)
{
    lcd.fillRectangle(x, y, w, h, active ? activeCol : DARKGREY);
    // thin highlight top
    if (active)
        lcd.lineHorizontal(x + 1, y, w - 2, WHITE);

    int len = strlen(label);
    int tx = x + (w - len * 8) / 2;
    if (tx < x + 1) tx = x + 1;
    lcd.printString(label, (uint8_t)tx, (uint8_t)(y + 2),
                    active ? BLACK : LIGHTGREY, active ? activeCol : DARKGREY);
}

// ============================================================
// Static frame (sekali saja)
// ============================================================
static void drawSolderStatic(Nokia105& lcd)
{
    lcd.displayClear();

    // Header bar
    lcd.fillRectangle(0, 0, W, 20, 0x3186);          // dark blue-grey
    lcd.lineHorizontal(0, 0,  W, ORANGE);
    lcd.lineHorizontal(0, 19, W, ORANGE);
    printCenter(lcd, "SOLDER", 2, ORANGE);

    // unit C
    printLeft(lcd, "C", 108, 36, ORANGE);

    // label TIP
    printLeft(lcd, "TIP LIFE", 6, 72, LIGHTGREY);

    // footer
    lcd.lineHorizontal(4, 148, W - 8, DARKGREY);
    printCenter(lcd, "DUAL STATION", 150, DARKGREY);
}

static void drawHotAirStatic(Nokia105& lcd)
{
    lcd.displayClear();

    lcd.fillRectangle(0, 0, W, 20, 0x3186);
    lcd.lineHorizontal(0, 0,  W, ORANGE);
    lcd.lineHorizontal(0, 19, W, ORANGE);
    printCenter(lcd, "HOT AIR", 2, ORANGE);

    printLeft(lcd, "C", 108, 36, ORANGE);
    printLeft(lcd, "FAN", 6, 72, LIGHTGREY);

    lcd.lineHorizontal(4, 148, W - 8, DARKGREY);
    printCenter(lcd, "DUAL STATION", 150, DARKGREY);
}

// ============================================================
// SOLDER — partial
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
    const bool stChanged   = tipErr != lastTipErr || sleepOn != lastSleep ||
                             boostOn != lastBoost || overOn != lastOver ||
                             pwm != lastSolderPwm;

    if (!force && !tempChanged && !setChanged && !tipChanged && !stChanged)
        return;

    if (force) drawSolderStatic(lcdSolder);

    char buf[16];

    // Big temperature
    if (force || tempChanged) {
        clearRegion(lcdSolder, 4, 24, 100, 24);
        if (ct < 0) snprintf(buf, sizeof(buf), "---");
        else        snprintf(buf, sizeof(buf), "%d", ct);
        printCenter(lcdSolder, buf, 28, ORANGE);
    }

    // SET
    if (force || setChanged) {
        clearRegion(lcdSolder, 4, 50, 120, 18);
        snprintf(buf, sizeof(buf), "SET  %d C", tt < 0 ? 0 : tt);
        printCenter(lcdSolder, buf, 52, LIGHTGREY);
    }

    // TIP LIFE % + bar
    if (force || tipChanged) {
        int tipLife = tipErr ? 0 : (overOn ? 15 : (pwmPct > 85 ? 65 : 95));
        clearRegion(lcdSolder, 70, 70, 54, 18);
        snprintf(buf, sizeof(buf), "%d%%", tipLife);
        printLeft(lcdSolder, buf, 78, 72, WHITE);

        drawBar(lcdSolder, 6, 90, 116, 14, tipLife, tipErr ? RED : GREEN);
    }

    // Status pills
    if (force || stChanged) {
        const bool readyOk = !tipErr && !overOn;
        const bool heatOn  = (pwm > 0 && !tipErr && !sleepOn && !overOn);

        clearRegion(lcdSolder, 2, 112, 124, 28);

        drawPill(lcdSolder, 4,  114, 38, 20, readyOk ? "RDY" : "ERR", readyOk, GREEN);
        drawPill(lcdSolder, 45, 114, 40, 20, heatOn  ? "HEAT" : "OFF", heatOn,  ORANGE);
        drawPill(lcdSolder, 88, 114, 36, 20, sleepOn ? "SLP" : "RUN", sleepOn, CYAN);
    }

    lastSolderCt = ct; lastSolderTt = tt; lastSolderPwm = pwm;
    lastTipErr = tipErr; lastSleep = sleepOn;
    lastBoost = boostOn; lastOver = overOn; lastTipMode = tipMode;
}

// ============================================================
// HOT AIR — partial
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

    char buf[16];

    // Big temperature
    if (force || tempChanged) {
        clearRegion(lcdHotAir, 4, 24, 100, 24);
        if (ct < 0) snprintf(buf, sizeof(buf), "---");
        else        snprintf(buf, sizeof(buf), "%d", ct);
        printCenter(lcdHotAir, buf, 28, ORANGE);
    }

    // SET
    if (force || setChanged) {
        clearRegion(lcdHotAir, 4, 50, 120, 18);
        snprintf(buf, sizeof(buf), "SET  %d C", tt < 0 ? 0 : tt);
        printCenter(lcdHotAir, buf, 52, LIGHTGREY);
    }

    // FAN % + bar
    if (force || fanChanged) {
        clearRegion(lcdHotAir, 40, 70, 84, 18);
        snprintf(buf, sizeof(buf), "%d%%", fanPct);
        printLeft(lcdHotAir, buf, 78, 72, WHITE);

        drawBar(lcdHotAir, 6, 90, 116, 14, fanPct, CYAN);
    }

    // Status pills
    if (force || stChanged) {
        const bool fanOk = fanPct > 5;

        clearRegion(lcdHotAir, 2, 112, 124, 28);

        drawPill(lcdHotAir, 4,  114, 38, 20, "RDY",  true,  GREEN);
        drawPill(lcdHotAir, 45, 114, 40, 20, on    ? "HEAT" : "OFF", on,    ORANGE);
        drawPill(lcdHotAir, 88, 114, 36, 20, fanOk ? "AIR"  : "FAN", fanOk, CYAN);
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
