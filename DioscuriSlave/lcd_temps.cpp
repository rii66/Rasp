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

static int lastSolderCt = -999, lastSolderTt = -999, lastSolderPwm = -999;
static bool lastTipErr = false, lastSleep = false, lastBoost = false;

static int lastAirCt = -999, lastAirTt = -999, lastFan = -999, lastAirPower = -999;
static bool lastAirOn = false;
static const char* lastAirMode = nullptr;

static void drawSolderScreen(bool force) {
    const int ct = currentTemp;
    const int tt = targetTemp;
    const int pwm = pwmOut;

    if (!force &&
        ct == lastSolderCt && tt == lastSolderTt && pwm == lastSolderPwm &&
        tipError == lastTipErr && sleeping == lastSleep && boostMode == lastBoost) {
        return;
    }

    lastSolderCt = ct;
    lastSolderTt = tt;
    lastSolderPwm = pwm;
    lastTipErr = tipError;
    lastSleep = sleeping;
    lastBoost = boostMode;

    lcdSolder.printString("SOLDER", 4, 2, CYAN, BLACK);
    lcdSolder.lineHorizontal(0, 20, 160, DARKGREY);

    lcdSolder.printString("ACT", 4, 27, LIGHTGREY, BLACK);
    lcdSolder.printDigit(ct < 0 ? 0 : (unsigned)ct, 30, 25, WHITE, BLACK);
    lcdSolder.printString("C", 88, 25, WHITE, BLACK);

    lcdSolder.printString("SET", 4, 51, LIGHTGREY, BLACK);
    lcdSolder.printDigit(tt < 0 ? 0 : (unsigned)tt, 30, 49, GREEN, BLACK);
    lcdSolder.printString("C", 88, 49, GREEN, BLACK);

    if (tipError) {
        lcdSolder.printString("NO TIP", 105, 27, RED, BLACK);
    } else if (sleeping) {
        lcdSolder.printString("SLEEP", 105, 27, BLUE, BLACK);
    } else if (boostMode) {
        lcdSolder.printString("BOOST", 105, 27, YELLOW, BLACK);
    } else {
        lcdSolder.printString(pwm > 0 ? "ON" : "OFF", 105, 27,
                             pwm > 0 ? GREEN : DARKGREY, BLACK);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "PWM %d%%", (pwm * 100) / 255);
    lcdSolder.printString(buf, 4, 77, CYAN, BLACK);

    lcdSolder.lineHorizontal(4, 101, 152, DARKGREY);
    lcdSolder.printString("TIP", 4, 105, LIGHTGREY, BLACK);

    if (tipError) {
        lcdSolder.printString("ERROR", 38, 105, RED, BLACK);
    } else {
        switch (currentTipMode) {
            case 0: lcdSolder.printString("T12", 38, 105, WHITE, BLACK); break;
            case 1: lcdSolder.printString("C210", 38, 105, WHITE, BLACK); break;
            default: lcdSolder.printString("CUSTOM", 38, 105, WHITE, BLACK); break;
        }
    }
}

static void drawHotAirScreen(bool force) {
    const int ct = (int)airGetTemp();
    const int tt = (int)airGetTargetTemp();
    const int fan = (int)airGetFan();
    const int power = (int)airGetPower();
    const bool on = airIsOn();
    const char* mode = airGetModeStr();

    if (!force &&
        ct == lastAirCt && tt == lastAirTt && fan == lastFan &&
        power == lastAirPower && on == lastAirOn && mode == lastAirMode) {
        return;
    }

    lastAirCt = ct;
    lastAirTt = tt;
    lastFan = fan;
    lastAirPower = power;
    lastAirOn = on;
    lastAirMode = mode;

    lcdHotAir.printString("HOT AIR", 4, 2, MAGENTA, BLACK);
    lcdHotAir.lineHorizontal(0, 20, 160, DARKGREY);

    lcdHotAir.printString("ACT", 4, 27, LIGHTGREY, BLACK);
    lcdHotAir.printDigit(ct < 0 ? 0 : (unsigned)ct, 30, 25, WHITE, BLACK);
    lcdHotAir.printString("C", 88, 25, WHITE, BLACK);

    lcdHotAir.printString("SET", 4, 51, LIGHTGREY, BLACK);
    lcdHotAir.printDigit(tt < 0 ? 0 : (unsigned)tt, 30, 49, GREEN, BLACK);
    lcdHotAir.printString("C", 88, 49, GREEN, BLACK);

    lcdHotAir.printString(mode, 105, 27,
                          on ? GREEN : DARKGREY, BLACK);

    char buf[16];
    snprintf(buf, sizeof(buf), "POWER %d%%", power);
    lcdHotAir.printString(buf, 4, 77, YELLOW, BLACK);

    snprintf(buf, sizeof(buf), "FAN %d%%", (fan * 100) / 255);
    lcdHotAir.printString(buf, 4, 93, CYAN, BLACK);

    lcdHotAir.lineHorizontal(4, 117, 152, DARKGREY);
    lcdHotAir.printString(on ? "HEATER ON" : "HEATER OFF", 4, 121,
                          on ? GREEN : DARKGREY, BLACK);
}

void initLcdTemps() {
    // Lightweight startup: no full-screen background/clear.
    lcdSolder.initDisplay();
    lcdSolder.setRotation(0);

    lcdHotAir.initDisplay();
    lcdHotAir.setRotation(0);

    ready = true;
    lastDraw = 0;
}

void updateLcdTemps() {
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;
    lastDraw = millis();

    drawSolderScreen(false);
    drawHotAirScreen(false);
}
