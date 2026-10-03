#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "Nokia105_LCD.h"
#include <string.h>

static Nokia105 lcdSolder(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);
static Nokia105 lcdHotAir(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS2);

static bool ready = false;
static uint32_t lastDraw = 0;
static const uint32_t DRAW_MS = 250;

static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static char lastAirMode[16] = "";

// ============================================================
// Helpers - 160x128 landscape
// ============================================================

static void printMini(Nokia105& lcd, const char* str, int x, int y,
                      uint16_t fg, uint16_t bg)
{
    const int len = strlen(str);
    lcd.fillRectangle(x, y, len * 4, 8, bg);

    for (int n = 0; n < len; ++n) {
        const unsigned char c = (unsigned char)str[n];
        if (c < 32 || c > 127) continue;

        const int glyph = c - 32;
        for (int sy = 0; sy < 16; sy += 2) {
            for (int sx = 0; sx < 8; sx += 2) {
                bool on = false;
                for (int yy = 0; yy < 2; ++yy)
                    for (int xx = 0; xx < 2; ++xx)
                        if ((font8x16[glyph][sy + yy] >> (7 - (sx + xx))) & 1)
                            on = true;

                if (on)
                    lcd.fillRectangle(x + n * 4 + sx / 2, y + sy / 2, 1, 1, fg);
            }
        }
    }
}

static void drawGauge(Nokia105& lcd, int16_t cx, int16_t cy, int16_t r,
                      const char* label, int value, const char* unit,
                      uint16_t ringColor)
{
    lcd.circle(cx, cy, r, ringColor);
    lcd.circle(cx, cy, r - 2, ringColor);

    int labelW = strlen(label) * 8;
    lcd.printString(label, cx - labelW / 2, cy - 16, LIGHTGREY, BLACK);

    char buf[12];
    snprintf(buf, sizeof(buf), "%d", value);
    int valueW = strlen(buf) * 8;
    lcd.printString(buf, cx - valueW / 2, cy - 2, ringColor, BLACK);

    int unitW = strlen(unit) * 8;
    lcd.printString(unit, cx - unitW / 2, cy + 14, LIGHTGREY, BLACK);
}

static void drawTop(Nokia105& lcd, const char* title, const char* mode, uint16_t titleColor)
{
    // Clear only header; full-screen clear causes visible blinking.
    lcd.fillRectangle(0, 0, 160, 14, BLACK);

    int titleW = strlen(title) * 4;
    printMini(lcd, title, 4, 2, titleColor, BLACK);

    int modeW = strlen(mode) * 4;
    printMini(lcd, mode, 156 - modeW, 2, WHITE, BLACK);

    lcd.lineHorizontal(4, 13, 152, DARKGREY);
}

static void drawTempGraph(Nokia105& lcd, int ct, int tt, uint16_t color)
{
    // Clear graph area only; do not blank the whole LCD every update.
    lcd.fillRectangle(4, 14, 152, 41, BLACK);

    lcd.lineHorizontal(5, 35, 150, DARKGREY);
    lcd.lineHorizontal(5, 41, 150, DARKGREY);
    lcd.lineHorizontal(5, 47, 150, DARKGREY);
    lcd.lineHorizontal(5, 53, 150, DARKGREY);
    lcd.lineVertical(5, 35, 19, LIGHTGREY);
    lcd.lineVertical(154, 35, 19, LIGHTGREY);
    lcd.lineHorizontal(5, 53, 150, LIGHTGREY);

    char buf[28];
    snprintf(buf, sizeof(buf), "ACT:%dC SET:%dC",
             ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcd.printString(buf, 8, 14, WHITE, BLACK);

    int pct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    int w = (146 * pct) / 100;
    if (w > 0)
        lcd.fillRectangle(7, 44, w, 6, color);

    // Target line + actual marker
    lcd.lineHorizontal(7, 41, 146, color);
    int marker = 7 + (146 * pct) / 100;
    if (marker > 7 && marker < 153)
        lcd.fillRectangle(marker, 42, 2, 10, color);
}

static void drawStatus(Nokia105& lcd, const char* a, const char* b, const char* c,
                       bool aOk, bool bOk, bool cOk)
{
    const int y = 117;
    const int h = 10;

    lcd.fillRectangle(3,   y, 44, h, aOk ? GREEN : DARKGREY);
    lcd.fillRectangle(50,  y, 60, h, bOk ? GREEN : DARKGREY);
    lcd.fillRectangle(114, y, 43, h, cOk ? GREEN : DARKGREY);

    printMini(lcd, a, 5,   y + 1, aOk ? BLACK : LIGHTGREY, aOk ? GREEN : DARKGREY);
    printMini(lcd, b, 52,  y + 1, bOk ? BLACK : LIGHTGREY, bOk ? GREEN : DARKGREY);
    printMini(lcd, c, 116, y + 1, cOk ? BLACK : LIGHTGREY, cOk ? GREEN : DARKGREY);
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

    lastSolderCt = ct;
    lastSolderTt = tt;
    lastSolderPwm = pwm;
    lastTipErr = tipError;
    lastSleep = sleeping;
    lastBoost = boostMode;
    lastOver = overHeat;
    lastTipMode = currentTipMode;

    const char* tipName = "T12";
    if (tipError)                  tipName = "ERR";
    else if (currentTipMode == 1)  tipName = "C210";
    else if (currentTipMode >= 2)  tipName = "CUST";

    drawTop(lcdSolder, "SOLDER", tipName, ORANGE);
    drawTempGraph(lcdSolder, ct, tt, ORANGE);

    drawGauge(lcdSolder, 47, 82, 25, "TEMP",
              ct < 0 ? 0 : ct, "C", ORANGE);

    drawGauge(lcdSolder, 113, 82, 25, "POWER",
              pwmPct, "%", CYAN);

    const bool ironOn = (pwm > 0 && !tipError && !sleeping && !overHeat);
    const bool readyOk = !tipError && !overHeat;
    const bool airUnused = true;

    drawStatus(lcdSolder,
               "READY",
               ironOn ? "HEAT ON" : "HEAT OFF",
               boostMode ? "BOOST" : (sleeping ? "SLEEP" : "POWER"),
               readyOk,
               ironOn,
               airUnused);
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
        strcmp(lastAirMode, (mode ? mode : "")) == 0)
        return;

    lastAirCt = ct;
    lastAirTt = tt;
    lastFan = fan;
    lastAirPower = power;
    lastAirOn = on;
    // Simpan mode string, bukan pointer
    strncpy(lastAirMode, (mode ? mode : ""), sizeof(lastAirMode) - 1);
    lastAirMode[sizeof(lastAirMode) - 1] = '\0';

    drawTop(lcdHotAir, "HOT AIR", mode ? mode : "AIR", MAGENTA);
    drawTempGraph(lcdHotAir, ct, tt, MAGENTA);

    drawGauge(lcdHotAir, 47, 82, 25, "TEMP",
              ct < 0 ? 0 : ct, "C", ORANGE);

    drawGauge(lcdHotAir, 113, 82, 25, "AIRFLOW",
              fanPct, "%", CYAN);

    const bool fanOk = fanPct > 5;
    drawStatus(lcdHotAir,
               "READY",
               on ? "HEAT ON" : "HEAT OFF",
               fanOk ? "AIR OK" : "FAN 0",
               true,
               on,
               fanOk);
}

// ============================================================
// Init & Update
// ============================================================
static void forceLcdCsHigh()
{
    pinMode(PIN_LCD_CS1, OUTPUT);
    pinMode(PIN_LCD_CS2, OUTPUT);
    digitalWrite(PIN_LCD_CS1, HIGH);
    digitalWrite(PIN_LCD_CS2, HIGH);
    delayMicroseconds(50);
}

void initLcdTemps()
{
    forceLcdCsHigh();

    lcdHotAir.begin(5000000);
    forceLcdCsHigh();
    delay(50);

    lcdSolder.begin(5000000);
    forceLcdCsHigh();
    delay(50);

    lcdSolder.initDisplaySoft();
    forceLcdCsHigh();
    delay(30);

    lcdHotAir.initDisplaySoft();
    forceLcdCsHigh();

    // Landscape 160x128
    ready = true;
    lastDraw = 0;

    lastSolderCt = -999;
    lastSolderTt = -999;
    lastSolderPwm = -999;
    lastTipErr = false;
    lastSleep = false;
    lastBoost = false;
    lastOver = false;
    lastTipMode = 255;

    lastAirCt = -999;
    lastAirTt = -999;
    lastFan = -999;
    lastAirPower = -999;
    lastAirOn = false;
    lastAirMode[0] = '\0';
}

void updateLcdTemps()
{
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;

    lastDraw = millis();

    drawSolderScreen(false);
    drawHotAirScreen(false);
}
