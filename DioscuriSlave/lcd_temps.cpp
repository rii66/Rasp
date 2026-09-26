#include "lcd_temps.h"
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "Nokia105_LCD.h"

// Shared SPI pins from config.h
// CS1 solder, CS2 hot-air

static Nokia105 lcdSolder(
    PIN_LCD_SDA,
    PIN_LCD_SCK,
    PIN_LCD_RESET,
    PIN_LCD_CS1
);

static Nokia105 lcdHotAir(
    PIN_LCD_SDA,
    PIN_LCD_SCK,
    PIN_LCD_RESET,
    PIN_LCD_CS2
);

static bool ready = false;
static unsigned long lastDraw = 0;
static const unsigned long DRAW_MS = 200;

static int lastSolderCt = -999;
static int lastSolderTt = -999;
static int lastAirCt = -999;
static int lastAirTt = -999;
static int lastFan = -999;
static bool lastAirOn = false;
static bool lastTipErr = false;
static bool lastBoost = false;

static void drawSolderScreen(bool force) {
    int ct = currentTemp;
    int tt = targetTemp;
    bool err = tipError;
    bool boost = boostMode;

    if (!force &&
        ct == lastSolderCt && tt == lastSolderTt &&
        err == lastTipErr && boost == lastBoost) {
        return;
    }
    lastSolderCt = ct;
    lastSolderTt = tt;
    lastTipErr = err;
    lastBoost = boost;

    lcdSolder.backgroundColor(BLACK);

    lcdSolder.printString("SOLDER", 28, 8, CYAN, BLACK);

    if (err) {
        lcdSolder.printString("NO TIP", 30, 60, RED, BLACK);
    } else {
        char buf[16];
        // Current large
        snprintf(buf, sizeof(buf), "%d", ct);
        lcdSolder.printString(buf, 36, 40, WHITE, BLACK);
        lcdSolder.printString("C", 90, 48, LIGHTGREY, BLACK);

        // Target
        snprintf(buf, sizeof(buf), "SET %d", tt);
        lcdSolder.printString(buf, 28, 90, GREEN, BLACK);

        if (boost) {
            lcdSolder.printString("BOOST", 40, 120, YELLOW, BLACK);
        } else if (sleeping) {
            lcdSolder.printString("SLEEP", 40, 120, BLUE, BLACK);
        } else {
            snprintf(buf, sizeof(buf), "PWM %d", pwmOut);
            lcdSolder.printString(buf, 32, 120, DARKGREY, BLACK);
        }
    }
}

static void drawHotAirScreen(bool force) {
    int ct = (int)airGetTemp();
    int tt = (int)airGetTargetTemp();
    int fan = (int)airGetFan();
    bool on = airIsOn();

    if (!force &&
        ct == lastAirCt && tt == lastAirTt &&
        fan == lastFan && on == lastAirOn) {
        return;
    }
    lastAirCt = ct;
    lastAirTt = tt;
    lastFan = fan;
    lastAirOn = on;

    lcdHotAir.backgroundColor(BLACK);

    lcdHotAir.printString("HOT AIR", 24, 8, MAGENTA, BLACK);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", ct);
    lcdHotAir.printString(buf, 36, 40, WHITE, BLACK);
    lcdHotAir.printString("C", 90, 48, LIGHTGREY, BLACK);

    snprintf(buf, sizeof(buf), "SET %d", tt);
    lcdHotAir.printString(buf, 28, 90, GREEN, BLACK);

    snprintf(buf, sizeof(buf), "FAN %d", fan);
    lcdHotAir.printString(buf, 28, 112, CYAN, BLACK);

    lcdHotAir.printString(on ? "ON " : "OFF", 48, 136,
                          on ? RED : DARKGREY, BLACK);
}

void initLcdTemps() {
    // CS3 / CS4 idle high (unused for now)
    pinMode(PIN_LCD_CS3, OUTPUT);
    pinMode(PIN_LCD_CS4, OUTPUT);
    digitalWrite(PIN_LCD_CS3, HIGH);
    digitalWrite(PIN_LCD_CS4, HIGH);

    // Shared bus init — first panel does full reset sequence
    lcdSolder.initDisplay();
    lcdSolder.backgroundColor(BLACK);
    lcdSolder.printString("SOLDER", 28, 60, WHITE, BLACK);

    // Second panel: shared RST already pulsed; still run init
    lcdHotAir.initDisplay();
    lcdHotAir.backgroundColor(BLACK);
    lcdHotAir.printString("HOT AIR", 24, 60, WHITE, BLACK);

    ready = true;
    lastDraw = 0;
    // force first full paint after delay
    delay(50);
    drawSolderScreen(true);
    drawHotAirScreen(true);

    Serial.println(F("[LCD] Nokia105 CS1=Solder CS2=HotAir ready"));
}

void updateLcdTemps() {
    if (!ready) return;
    if (millis() - lastDraw < DRAW_MS) return;
    lastDraw = millis();

    drawSolderScreen(false);
    drawHotAirScreen(false);
}
