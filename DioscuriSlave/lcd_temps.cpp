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

// cache
static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static char lastAirMode[16] = "";

// ============================================================
// Mini font (4x8) - sudah ada di kode lama
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

// ============================================================
// SEMI-3D BORDER
// ============================================================
static void drawBorder3D(Nokia105& lcd, int x, int y, int w, int h)
{
    // Outer frame
    lcd.lineHorizontal(x,     y,     w, LIGHTGREY);
    lcd.lineHorizontal(x,     y+h-1, w, DARKGREY);
    lcd.lineVertical  (x,     y,     h, LIGHTGREY);
    lcd.lineVertical  (x+w-1, y,     h, DARKGREY);

    // Inner highlight (top + left)
    lcd.lineHorizontal(x+1, y+1, w-2, WHITE);
    lcd.lineVertical  (x+1, y+1, h-2, WHITE);

    // Inner shadow (bottom + right)
    lcd.lineHorizontal(x+2, y+h-2, w-3, DARKGREY);
    lcd.lineVertical  (x+w-2, y+2, h-3, DARKGREY);
}

// Progress bar dengan border 3D tipis
static void drawBar(Nokia105& lcd, int x, int y, int w, int h, int pct, uint16_t color)
{
    // frame
    lcd.fillRectangle(x, y, w, h, DARKGREY);
    lcd.lineHorizontal(x, y, w, LIGHTGREY);
    lcd.lineVertical  (x, y, h, LIGHTGREY);

    int fill = ((w - 2) * constrain(pct, 0, 100)) / 100;
    if (fill > 0)
        lcd.fillRectangle(x + 1, y + 1, fill, h - 2, color);
}

// Status pill
static void drawPill(Nokia105& lcd, int x, int y, int w, const char* txt, bool active)
{
    uint16_t bg = active ? GREEN : DARKGREY;
    uint16_t fg = active ? BLACK : LIGHTGREY;

    lcd.fillRectangle(x, y, w, 14, bg);
    // semi-3D kecil
    lcd.lineHorizontal(x, y, w, active ? WHITE : LIGHTGREY);
    lcd.lineVertical  (x, y, 14, active ? WHITE : LIGHTGREY);

    printMini(lcd, txt, x + 4, y + 3, fg, bg);
}

// ============================================================
// SOLDER SCREEN (1 LCD)
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

    // Clear full
    lcdSolder.fillRectangle(0, 0, 160, 128, BLACK);

    // Outer semi-3D border
    drawBorder3D(lcdSolder, 2, 2, 156, 124);

    // Title
    const char* tipName = tipError ? "ERR" :
                          (currentTipMode == 1) ? "C210" :
                          (currentTipMode >= 2) ? "CUST" : "T12";

    printMini(lcdSolder, "SOLDER STATION", 10, 8, ORANGE, BLACK);
    printMini(lcdSolder, tipName, 130, 8, LIGHTGREY, BLACK);

    // Big temperature
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", ct < 0 ? 0 : ct);
    lcdSolder.printString(buf, 18, 28, ORANGE, BLACK);
    lcdSolder.printString("C", 18 + strlen(buf)*8, 28, LIGHTGREY, BLACK);

    // SET
    snprintf(buf, sizeof(buf), "SET: %d C", tt);
    printMini(lcdSolder, buf, 18, 52, WHITE, BLACK);

    // POWER bar
    printMini(lcdSolder, "POWER", 18, 68, LIGHTGREY, BLACK);
    snprintf(buf, sizeof(buf), "%d%%", pwmPct);
    printMini(lcdSolder, buf, 120, 68, CYAN, BLACK);
    drawBar(lcdSolder, 18, 80, 124, 10, pwmPct, CYAN);

    // Status pills
    bool ironOn = (pwm > 0 && !tipError && !sleeping && !overHeat);
    bool readyOk = !tipError && !overHeat;

    drawPill(lcdSolder, 10,  100, 44, "READY",   readyOk);
    drawPill(lcdSolder, 58,  100, 50, ironOn ? "HEAT ON" : "HEAT OFF", ironOn);
    drawPill(lcdSolder, 112, 100, 40, sleeping ? "SLEEP" : (boostMode ? "BOOST" : "RUN"), sleeping || boostMode);
}

// ============================================================
// HOT AIR SCREEN (1 LCD)
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

    lcdHotAir.fillRectangle(0, 0, 160, 128, BLACK);

    drawBorder3D(lcdHotAir, 2, 2, 156, 124);

    // Title
    printMini(lcdHotAir, "HOT AIR STATION", 10, 8, MAGENTA, BLACK);
    printMini(lcdHotAir, mode ? mode : "AIR", 120, 8, LIGHTGREY, BLACK);

    // Big temperature
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", ct < 0 ? 0 : ct);
    lcdHotAir.printString(buf, 18, 28, ORANGE, BLACK);
    lcdHotAir.printString("C", 18 + strlen(buf)*8, 28, LIGHTGREY, BLACK);

    // SET
    snprintf(buf, sizeof(buf), "SET: %d C", tt);
    printMini(lcdHotAir, buf, 18, 52, WHITE, BLACK);

    // FAN bar
    printMini(lcdHotAir, "FAN", 18, 68, LIGHTGREY, BLACK);
    snprintf(buf, sizeof(buf), "%d%%", fanPct);
    printMini(lcdHotAir, buf, 120, 68, CYAN, BLACK);
    drawBar(lcdHotAir, 18, 80, 124, 10, fanPct, CYAN);

    // Status
    bool fanOk = fanPct > 5;
    drawPill(lcdHotAir, 10,  100, 44, "READY",   true);
    drawPill(lcdHotAir, 58,  100, 50, on ? "HEAT ON" : "HEAT OFF", on);
    drawPill(lcdHotAir, 112, 100, 40, fanOk ? "AIR OK" : "FAN 0", fanOk);
}

// ============================================================
// Init & Update (tetap sama)
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

    ready = true;
    lastDraw = 0;

    // reset cache
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
