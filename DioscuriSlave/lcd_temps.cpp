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
static const uint32_t DRAW_MS = 250;

static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static const char* lastAirMode = nullptr;

// ============================================================
// Helper
// ============================================================
static void drawGauge(Nokia105& lcd, int16_t cx, int16_t cy, int16_t r,
                      const char* label, int value, const char* unit,
                      uint16_t ringColor, uint16_t textColor)
{
    lcd.circle(cx, cy, r, ringColor);
    lcd.circle(cx, cy, r - 1, ringColor);

    int labelW = strlen(label) * 6;
    lcd.printString(label, cx - labelW / 2, cy - r - 14, LIGHTGREY, BLACK);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    int valW = strlen(buf) * 8;
    lcd.printString(buf, cx - valW / 2, cy - 8, textColor, BLACK);

    if (unit) lcd.printString(unit, cx - 4, cy + 8, textColor, BLACK);
}

static void drawBar(Nokia105& lcd, int16_t x, int16_t y, int16_t w, int16_t h,
                    int percent, uint16_t fillColor)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    lcd.fillRectangle(x, y, w, h, DARKGREY);
    int fw = (w * percent) / 100;
    if (fw > 0) lcd.fillRectangle(x, y, fw, h, fillColor);

    lcd.lineHorizontal(x, y, w, LIGHTGREY);
    lcd.lineHorizontal(x, y + h - 1, w, LIGHTGREY);
    lcd.lineVertical(x, y, h, LIGHTGREY);
    lcd.lineVertical(x + w - 1, y, h, LIGHTGREY);
}

// ============================================================
// SOLDER (portrait 128x160)
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

    lcdSolder.fillRectangle(0, 0, 128, 160, BLACK);

    lcdSolder.printString("SOLDER", 4, 4, CYAN, BLACK);

    const char* tipName = "T12";
    if (tipError)              tipName = "ERR";
    else if (currentTipMode == 1) tipName = "C210";
    else if (currentTipMode >= 2) tipName = "CUST";
    lcdSolder.printString(tipName, 90, 4, WHITE, BLACK);

    lcdSolder.lineHorizontal(0, 20, 128, DARKGREY);

    char buf[28];
    snprintf(buf, sizeof(buf), "A:%d  S:%d", ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcdSolder.printString(buf, 4, 26, WHITE, BLACK);

    int tempPct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    drawBar(lcdSolder, 4, 44, 120, 8, tempPct, ORANGE);

    drawGauge(lcdSolder, 34, 85, 22, "TEMP", ct < 0 ? 0 : ct, "C", ORANGE, ORANGE);
    drawGauge(lcdSolder, 94, 85, 22, "LIFE", pwmPct, "%", CYAN, CYAN);

    lcdSolder.lineHorizontal(0, 120, 128, DARKGREY);

    lcdSolder.fillRectangle(4, 126, 36, 12, GREEN);
    lcdSolder.printString("READY", 7, 128, BLACK, GREEN);

    bool ironOn = (pwm > 0 && !tipError && !sleeping && !overHeat);
    if (ironOn) {
        lcdSolder.fillRectangle(44, 126, 36, 12, GREEN);
        lcdSolder.printString("IRON", 50, 128, BLACK, GREEN);
    } else {
        lcdSolder.fillRectangle(44, 126, 36, 12, DARKGREY);
        lcdSolder.printString("OFF", 52, 128, LIGHTGREY, DARKGREY);
    }

    if (tipError) {
        lcdSolder.fillRectangle(84, 126, 40, 12, RED);
        lcdSolder.printString("NOTIP", 88, 128, WHITE, RED);
    } else if (overHeat) {
        lcdSolder.fillRectangle(84, 126, 40, 12, RED);
        lcdSolder.printString("OVRHT", 88, 128, WHITE, RED);
    } else if (sleeping) {
        lcdSolder.fillRectangle(84, 126, 40, 12, BLUE);
        lcdSolder.printString("SLEEP", 88, 128, WHITE, BLUE);
    } else if (boostMode) {
        lcdSolder.fillRectangle(84, 126, 40, 12, YELLOW);
        lcdSolder.printString("BOOST", 88, 128, BLACK, YELLOW);
    } else {
        lcdSolder.fillRectangle(84, 126, 40, 12, DARKGREY);
        lcdSolder.printString("---", 96, 128, LIGHTGREY, DARKGREY);
    }
}

// ============================================================
// HOT AIR (portrait 128x160)
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

    if (!force && ct == lastAirCt && tt == lastAirTt && fan == lastFan &&
        power == lastAirPower && on == lastAirOn && mode == lastAirMode)
        return;

    lastAirCt = ct; lastAirTt = tt; lastFan = fan; lastAirPower = power;
    lastAirOn = on; lastAirMode = mode;

    lcdHotAir.fillRectangle(0, 0, 128, 160, BLACK);

    lcdHotAir.printString("HOT AIR", 28, 4, MAGENTA, BLACK);
    lcdHotAir.lineHorizontal(0, 20, 128, DARKGREY);

    char buf[28];
    snprintf(buf, sizeof(buf), "A:%d  S:%d", ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcdHotAir.printString(buf, 4, 26, WHITE, BLACK);

    int tempPct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    drawBar(lcdHotAir, 4, 44, 120, 8, tempPct, ORANGE);

    drawGauge(lcdHotAir, 34, 85, 22, "TEMP", ct < 0 ? 0 : ct, "C", ORANGE, ORANGE);
    drawGauge(lcdHotAir, 94, 85, 22, "AIR", fanPct, "%", CYAN, CYAN);

    lcdHotAir.lineHorizontal(0, 120, 128, DARKGREY);

    lcdHotAir.fillRectangle(4, 126, 36, 12, GREEN);
    lcdHotAir.printString("READY", 7, 128, BLACK, GREEN);

    if (on) {
        lcdHotAir.fillRectangle(44, 126, 36, 12, GREEN);
        lcdHotAir.printString("HEAT", 50, 128, BLACK, GREEN);
    } else {
        lcdHotAir.fillRectangle(44, 126, 36, 12, DARKGREY);
        lcdHotAir.printString("OFF", 52, 128, LIGHTGREY, DARKGREY);
    }

    if (fanPct > 5) {
        lcdHotAir.fillRectangle(84, 126, 40, 12, CYAN);
        lcdHotAir.printString("AIROK", 88, 128, BLACK, CYAN);
    } else {
        lcdHotAir.fillRectangle(84, 126, 40, 12, DARKGREY);
        lcdHotAir.printString("FAN0", 92, 128, LIGHTGREY, DARKGREY);
    }
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

    // Portrait only — driver belum siap landscape
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
