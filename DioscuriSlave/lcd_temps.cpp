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

static int  lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false, lastOver = false;
static uint8_t lastTipMode = 255;

static int  lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static const char* lastAirMode = nullptr;

// ============================================================
// Helper: circular gauge
// ============================================================
static void drawGauge(Nokia105& lcd, int16_t cx, int16_t cy, int16_t r,
                      const char* label, int value, const char* unit,
                      uint16_t ringColor, uint16_t textColor)
{
    lcd.circle(cx, cy, r, ringColor);
    lcd.circle(cx, cy, r - 1, ringColor);

    int labelW = strlen(label) * 6;
    lcd.printString(label, cx - labelW / 2, cy - r - 11, LIGHTGREY, BLACK);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    int valW = strlen(buf) * 8;
    lcd.printString(buf, cx - valW / 2, cy - 6, textColor, BLACK);

    if (unit) {
        lcd.printString(unit, cx - 4, cy + 9, textColor, BLACK);
    }
}

// ============================================================
// Helper: progress bar
// ============================================================
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
// SOLDER (landscape) — PWM global untuk T12 & C210
// ============================================================
static void drawSolderScreen(bool force)
{
    const int ct  = currentTemp;
    const int tt  = targetTemp;
    const int pwm = pwmOut;

    // PWM % relatif ke maxPwmLimit (penting untuk C210 yang max 71)
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

    lcdSolder.fillRectangle(0, 0, 160, 128, BLACK);

    // Title + tip type
    lcdSolder.printString("SOLDERING IRON", 8, 3, CYAN, BLACK);

    // Tip type di kanan title
    const char* tipName = "T12";
    if (tipError)           tipName = "ERR";
    else if (currentTipMode == 1) tipName = "C210";
    else if (currentTipMode >= 2) tipName = "CUST";
    lcdSolder.printString(tipName, 130, 3, WHITE, BLACK);

    lcdSolder.lineHorizontal(0, 16, 160, DARKGREY);

    // Actual / Set
    char buf[36];
    snprintf(buf, sizeof(buf), "Actual:%dC / Set:%dC", ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcdSolder.printString(buf, 6, 20, WHITE, BLACK);

    // Progress bar (temp)
    int tempPct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    drawBar(lcdSolder, 6, 34, 148, 8, tempPct, ORANGE);

    // ===== Two gauges =====
    // Left: SOLD TEMP
    drawGauge(lcdSolder, 42, 78, 24, "SOLD TEMP", ct < 0 ? 0 : ct, "C", ORANGE, ORANGE);

    // Right: TIP LIFE (= PWM % global, valid T12 & C210)
    drawGauge(lcdSolder, 118, 78, 24, "TIP LIFE", pwmPct, "%", CYAN, CYAN);

    // ===== Status pills =====
    lcdSolder.lineHorizontal(0, 108, 160, DARKGREY);

    // READY
    lcdSolder.fillRectangle(4, 112, 36, 12, GREEN);
    lcdSolder.printString("READY", 8, 114, BLACK, GREEN);

    // IRON ON / OFF
    bool ironOn = (pwm > 0 && !tipError && !sleeping && !overHeat);
    if (ironOn) {
        lcdSolder.fillRectangle(44, 112, 40, 12, GREEN);
        lcdSolder.printString("IRON ON", 48, 114, BLACK, GREEN);
    } else {
        lcdSolder.fillRectangle(44, 112, 40, 12, DARKGREY);
        lcdSolder.printString("IRON OFF", 46, 114, LIGHTGREY, DARKGREY);
    }

    // Status kanan: prioritas ERROR > OVERHEAT > SLEEP > BOOST > normal
    if (tipError) {
        lcdSolder.fillRectangle(90, 112, 64, 12, RED);
        lcdSolder.printString("NO TIP", 102, 114, WHITE, RED);
    } else if (overHeat) {
        lcdSolder.fillRectangle(90, 112, 64, 12, RED);
        lcdSolder.printString("OVERHEAT", 96, 114, WHITE, RED);
    } else if (sleeping) {
        lcdSolder.fillRectangle(90, 112, 64, 12, BLUE);
        lcdSolder.printString("SLEEP MODE", 94, 114, WHITE, BLUE);
    } else if (boostMode) {
        lcdSolder.fillRectangle(90, 112, 64, 12, YELLOW);
        lcdSolder.printString("BOOST", 108, 114, BLACK, YELLOW);
    } else {
        lcdSolder.fillRectangle(90, 112, 64, 12, DARKGREY);
        lcdSolder.printString("---", 112, 114, LIGHTGREY, DARKGREY);
    }
}

// ============================================================
// HOT AIR (landscape)
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

    lcdHotAir.fillRectangle(0, 0, 160, 128, BLACK);

    // Title
    lcdHotAir.printString("HOT AIR", 52, 3, MAGENTA, BLACK);
    lcdHotAir.lineHorizontal(0, 16, 160, DARKGREY);

    // Actual / Set
    char buf[36];
    snprintf(buf, sizeof(buf), "Actual:%dC / Set:%dC", ct < 0 ? 0 : ct, tt < 0 ? 0 : tt);
    lcdHotAir.printString(buf, 6, 20, WHITE, BLACK);

    // Progress bar
    int tempPct = (tt > 0) ? constrain((ct * 100) / tt, 0, 100) : 0;
    drawBar(lcdHotAir, 6, 34, 148, 8, tempPct, ORANGE);

    // ===== Two gauges =====
    drawGauge(lcdHotAir, 42, 78, 24, "TEMP", ct < 0 ? 0 : ct, "C", ORANGE, ORANGE);
    drawGauge(lcdHotAir, 118, 78, 24, "AIRFLOW", fanPct, "%", CYAN, CYAN);

    // ===== Status pills =====
    lcdHotAir.lineHorizontal(0, 108, 160, DARKGREY);

    // READY
    lcdHotAir.fillRectangle(4, 112, 36, 12, GREEN);
    lcdHotAir.printString("READY", 8, 114, BLACK, GREEN);

    // HEATER ON / OFF
    if (on) {
        lcdHotAir.fillRectangle(44, 112, 50, 12, GREEN);
        lcdHotAir.printString("HEATER ON", 48, 114, BLACK, GREEN);
    } else {
        lcdHotAir.fillRectangle(44, 112, 50, 12, DARKGREY);
        lcdHotAir.printString("HEATER OFF", 46, 114, LIGHTGREY, DARKGREY);
    }

    // AIRFLOW OK
    if (fanPct > 5) {
        lcdHotAir.fillRectangle(100, 112, 54, 12, CYAN);
        lcdHotAir.printString("AIRFLOW OK", 102, 114, BLACK, CYAN);
    } else {
        lcdHotAir.fillRectangle(100, 112, 54, 12, DARKGREY);
        lcdHotAir.printString("FAN 0%", 112, 114, LIGHTGREY, DARKGREY);
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

    // Landscape 160x128
    lcdSolder.setRotation(1);
    lcdHotAir.setRotation(1);

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
