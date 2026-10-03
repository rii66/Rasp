#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "Nokia105_LCD.h"

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
static const char* lastAirMode = nullptr;

// ============================================================
// Helpers - 160x128 landscape
// ============================================================
static void drawGauge(Nokia105& lcd, int16_t cx, int16_t cy, int16_t r,
                      const char* label, int value, const char* unit,
                      uint16_t ringColor)
{
    lcd.circle(cx, cy, r, ringColor);
    lcd.circle(cx, cy, r - 2, ringColor);

    int labelW = strlen(label) * 8;
    lcd.printString(label, cx - labelW / 2, cy - 18, LIGHTGREY, BLACK);

    char buf[12];
    snprintf(buf, sizeof(buf), "%d", value);
    int valueW = strlen(buf) * 8;
    lcd.printString(buf, cx - valueW / 2, cy - 2, ringColor, BLACK);

    int unitW = strlen(unit) * 8;
    lcd.printString(unit, cx - unitW / 2, cy + 14, LIGHTGREY, BLACK);
}

static void drawTop(Nokia105& lcd, const char* title, const char* mode, uint16_t titleColor)
{
    lcd.fillRectangle(0, 0, 160, 128, BLACK);

    lcd.printString(title, 5, 1, titleColor, BLACK);

    int modeW = strlen(mode) * 8;
    lcd.printString(mode, 155 - modeW, 1, WHITE, BLACK);

    lcd.lineHorizontal(4, 17, 152, DARKGREY);
}

static void drawTempGraph(Nokia105& lcd, int ct, int tt, uint16_t color)
{
    lcd.lineHorizontal(5, 31, 150, DARKGREY);
    lcd.lineHorizontal(5, 38, 150, DARKGREY);
    lcd.lineHorizontal(5, 45, 150, DARKGREY);
    lcd.lineHorizontal(5, 52, 150, DARKGREY);
    lcd.lineVertical(5, 31, 22, LIGHTGREY);
    lcd.lineVertical(154, 31, 22, LIGHTGREY);
    lcd.lineHorizontal(5, 52, 150, LIGHTGREY);

    char buf[28];
    snprintf(buf, sizeof(buf), "Actual:%dC Set:%dC",
             ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcd.printString(buf, 8, 19, WHITE, BLACK);

    int pct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    int w = (146 * pct) / 100;
    if (w > 0)
        lcd.fillRectangle(7, 40, w, 6, color);

    // Target line + actual marker
    lcd.lineHorizontal(7, 36, 146, color);
    int marker = 7 + (146 * pct) / 100;
    if (marker > 7 && marker < 153)
        lcd.fillRectangle(marker, 34, 2, 10, color);
}

static void drawStatus(Nokia105& lcd, const char* a, const char* b, const char* c,
                       bool aOk, bool bOk, bool cOk)
{
    const int y = 112;
    const int h = 13;

    lcd.fillRectangle(3,   y, 48, h, aOk ? GREEN : DARKGREY);
    lcd.fillRectangle(56,  y, 48, h, bOk ? GREEN : DARKGREY);
    lcd.fillRectangle(109, y, 48, h, cOk ? GREEN : DARKGREY);

    lcd.printString(a, 7,   y + 1, aOk ? BLACK : LIGHTGREY, aOk ? GREEN : DARKGREY);
    lcd.printString(b, 60,  y + 1, bOk ? BLACK : LIGHTGREY, bOk ? GREEN : DARKGREY);
    lcd.printString(c, 113, y + 1, cOk ? BLACK : LIGHTGREY, cOk ? GREEN : DARKGREY);
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

    drawGauge(lcdSolder, 47, 84, 26, "TEMP",
              ct < 0 ? 0 : ct, "C", ORANGE);

    drawGauge(lcdSolder, 113, 84, 26, "POWER",
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
        power == lastAirPower && on == lastAirOn && mode == lastAirMode)
        return;

    lastAirCt = ct;
    lastAirTt = tt;
    lastFan = fan;
    lastAirPower = power;
    lastAirOn = on;
    lastAirMode = mode;

    drawTop(lcdHotAir, "HOT AIR", mode ? mode : "AIR", MAGENTA);
    drawTempGraph(lcdHotAir, ct, tt, MAGENTA);

    drawGauge(lcdHotAir, 47, 84, 26, "TEMP",
              ct < 0 ? 0 : ct, "C", ORANGE);

    drawGauge(lcdHotAir, 113, 84, 26, "AIRFLOW",
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
    lastAirMode = nullptr;
}

void updateLcdTemps()
{
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;

    lastDraw = millis();

    drawSolderScreen(false);
    drawHotAirScreen(false);
}
