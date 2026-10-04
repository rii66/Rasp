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
static const uint32_t DRAW_MS = 200;

static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static char lastAirMode[16] = "";

// Portrait 128 x 160
static const int16_t W = 128;
static const int16_t H = 160;

// ============================================================
// Mini 4x8 font (downsampled 8x16)
// ============================================================
static void printMini(Nokia105& lcd, const char* str, int x, int y,
                      uint16_t fg, uint16_t bg)
{
    const int len = strlen(str);
    if (len <= 0) return;
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

static void printCentered8(Nokia105& lcd, const char* str, int y,
                           uint16_t fg, uint16_t bg)
{
    int len = strlen(str);
    int x = (W - len * 8) / 2;
    if (x < 0) x = 0;
    lcd.printString(str, (uint8_t)x, (uint8_t)y, fg, bg);
}

static void printCenteredMini(Nokia105& lcd, const char* str, int y,
                              uint16_t fg, uint16_t bg)
{
    int len = strlen(str);
    int x = (W - len * 4) / 2;
    if (x < 0) x = 0;
    printMini(lcd, str, x, y, fg, bg);
}

// Progress bar with outer frame (like reference)
static void drawBar(Nokia105& lcd, int x, int y, int w, int h,
                    int pct, uint16_t fill, uint16_t frame)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    lcd.fillRectangle(x, y, w, h, BLACK);
    // outer frame
    lcd.lineHorizontal(x, y, w, frame);
    lcd.lineHorizontal(x, y + h - 1, w, frame);
    lcd.lineVertical(x, y, h, frame);
    lcd.lineVertical(x + w - 1, y, h, frame);
    int fillW = ((w - 4) * pct) / 100;
    if (fillW > 0)
        lcd.fillRectangle(x + 2, y + 2, fillW, h - 4, fill);
}

// Status pill matching [READY] style
static void drawPill(Nokia105& lcd, int x, int y, int w, int h,
                     const char* txt, bool active, uint16_t onColor)
{
    uint16_t bg = active ? onColor : 0x3186; // dark grey-blue
    uint16_t fg = active ? BLACK : LIGHTGREY;
    lcd.fillRectangle(x, y, w, h, bg);
    // thin border
    lcd.lineHorizontal(x, y, w, active ? onColor : DARKGREY);
    lcd.lineHorizontal(x, y + h - 1, w, active ? onColor : DARKGREY);
    int tw = strlen(txt) * 4;
    int tx = x + (w - tw) / 2;
    if (tx < x + 2) tx = x + 2;
    printMini(lcd, txt, tx, y + (h - 8) / 2, fg, bg);
}

// ============================================================
// Header bar – style "DUAL-CHANNEL D-REWORK v1.3"
// ============================================================
static void drawHeader(Nokia105& lcd, const char* mainTitle, const char* sub)
{
    // full black top
    lcd.fillRectangle(0, 0, W, 22, BLACK);

    // orange double frame like reference
    lcd.lineHorizontal(3, 2, W - 6, ORANGE);
    lcd.lineHorizontal(3, 19, W - 6, ORANGE);
    lcd.lineVertical(3, 2, 18, ORANGE);
    lcd.lineVertical(W - 4, 2, 18, ORANGE);

    // main title centered
    printCenteredMini(lcd, mainTitle, 4, ORANGE, BLACK);
    // sub version / mode
    if (sub && sub[0])
        printCenteredMini(lcd, sub, 12, LIGHTGREY, BLACK);
}

// ============================================================
// Big temperature block (xxx.0 °C look)
// ============================================================
static void drawBigTemp(Nokia105& lcd, int ct)
{
    char buf[12];
    if (ct < 0)
        snprintf(buf, sizeof(buf), "---.-");
    else
        snprintf(buf, sizeof(buf), "%d.0", ct);

    lcd.fillRectangle(0, 24, W, 40, BLACK);
    printCentered8(lcd, buf, 28, ORANGE, BLACK);

    // degree symbol + C on the right of the number
    // approximate position after the number
    int len = strlen(buf);
    int numW = len * 8;
    int startX = (W - numW) / 2;
    lcd.printString("C", (uint8_t)(startX + numW + 2), 32, ORANGE, BLACK);
}

// ============================================================
// SET line
// ============================================================
static void drawSetLine(Nokia105& lcd, int tt)
{
    char buf[20];
    lcd.fillRectangle(0, 64, W, 12, BLACK);
    snprintf(buf, sizeof(buf), "SET: %dC", tt < 0 ? 0 : tt);
    printMini(lcd, buf, 8, 66, LIGHTGREY, BLACK);
}

// ============================================================
// SOLDER screen
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

    if (force) lcdSolder.displayClear();

    // Header
    const char* tipName = "T12";
    if (tipError)                 tipName = "ERR";
    else if (currentTipMode == 1) tipName = "C210";
    else if (currentTipMode >= 2) tipName = "CUST";

    char sub[24];
    snprintf(sub, sizeof(sub), "SOLDER  %s", tipName);
    drawHeader(lcdSolder, "DUAL-CHANNEL v1.3", sub);

    // Big temp
    drawBigTemp(lcdSolder, ct);

    // SET
    drawSetLine(lcdSolder, tt);

    // TIP LIFE section
    lcdSolder.fillRectangle(0, 78, W, 32, BLACK);
    printMini(lcdSolder, "TIP LIFE", 8, 80, LIGHTGREY, BLACK);

    // simple life heuristic (no real counter in firmware yet)
    int tipLife = 98;
    if (tipError) tipLife = 0;
    else if (overHeat) tipLife = 15;
    else if (pwmPct > 80) tipLife = 70;
    else if (pwmPct > 50) tipLife = 85;

    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", tipLife);
    printMini(lcdSolder, buf, 92, 80, WHITE, BLACK);

    // small wrench-like mark (two crossed lines)
    lcdSolder.lineHorizontal(110, 84, 10, ORANGE);
    lcdSolder.lineVertical(115, 79, 10, ORANGE);

    drawBar(lcdSolder, 8, 92, 112, 12, tipLife,
            tipError ? RED : (tipLife < 30 ? ORANGE : GREEN), DARKGREY);

    // Status pills – match reference order
    lcdSolder.fillRectangle(0, 114, W, 46, BLACK);

    const bool readyOk = !tipError && !overHeat;
    const bool heatOn  = (pwm > 0 && !tipError && !sleeping && !overHeat);
    const bool sleepOn = sleeping || boostMode;

    drawPill(lcdSolder, 3,  120, 40, 16, "READY",     readyOk, GREEN);
    drawPill(lcdSolder, 45, 120, 42, 16, heatOn ? "HEATER ON" : "HEATER OFF", heatOn, ORANGE);
    drawPill(lcdSolder, 89, 120, 36, 16, sleepOn ? (boostMode ? "BOOST" : "SLEEP") : "RUN",
             sleepOn, CYAN);

    // footer
    printCenteredMini(lcdSolder, "DUAL-CHANNEL", 144, DARKGREY, BLACK);
}

// ============================================================
// HOT AIR screen
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
    strncpy(lastAirMode, (mode ? mode : ""), sizeof(lastAirMode) - 1);
    lastAirMode[sizeof(lastAirMode) - 1] = '\0';

    if (force) lcdHotAir.displayClear();

    // Header
    char sub[24];
    snprintf(sub, sizeof(sub), "HOT AIR  %s", mode ? mode : "AIR");
    drawHeader(lcdHotAir, "DUAL-CHANNEL v1.3", sub);

    // Big temp
    drawBigTemp(lcdHotAir, ct);

    // SET
    drawSetLine(lcdHotAir, tt);

    // FAN section
    lcdHotAir.fillRectangle(0, 78, W, 32, BLACK);
    printMini(lcdHotAir, "FAN", 8, 80, LIGHTGREY, BLACK);

    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", fanPct);
    printMini(lcdHotAir, buf, 92, 80, WHITE, BLACK);

    drawBar(lcdHotAir, 8, 92, 112, 12, fanPct, CYAN, DARKGREY);

    // Status pills
    lcdHotAir.fillRectangle(0, 114, W, 46, BLACK);

    const bool fanOk = fanPct > 5;
    drawPill(lcdHotAir, 3,  120, 40, 16, "READY", true, GREEN);
    drawPill(lcdHotAir, 45, 120, 42, 16, on ? "HEATER ON" : "HEATER OFF", on, ORANGE);
    drawPill(lcdHotAir, 89, 120, 36, 16, fanOk ? "AIR OK" : "FAN 0", fanOk, CYAN);

    printCenteredMini(lcdHotAir, "DUAL-CHANNEL", 144, DARKGREY, BLACK);
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
    delay(40);

    lcdSolder.begin(5000000);
    forceLcdCsHigh();
    delay(40);

    // ensure portrait
    lcdSolder.setRotation(0);
    forceLcdCsHigh();
    lcdHotAir.setRotation(0);
    forceLcdCsHigh();

    lcdSolder.initDisplaySoft();
    forceLcdCsHigh();
    delay(20);

    lcdHotAir.initDisplaySoft();
    forceLcdCsHigh();

    ready = true;
    lastDraw = 0;

    lastSolderCt = -999; lastSolderTt = -999; lastSolderPwm = -999;
    lastTipErr = false; lastSleep = false; lastBoost = false; lastOver = false;
    lastTipMode = 255;

    lastAirCt = -999; lastAirTt = -999; lastFan = -999; lastAirPower = -999;
    lastAirOn = false; lastAirMode[0] = '\0';

    drawSolderScreen(true);
    forceLcdCsHigh();
    drawHotAirScreen(true);
    forceLcdCsHigh();
}

void updateLcdTemps()
{
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;
    lastDraw = millis();

    drawSolderScreen(false);
    forceLcdCsHigh();
    drawHotAirScreen(false);
    forceLcdCsHigh();
}
