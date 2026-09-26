
/**
 * Dioscuri SLAVE — RP2040
 * Port lean dari Dioscurios V1
 * master ESP opsional via UART
 */
#include <Arduino.h>
#include "config.h"
#include "GlobalState.h"
#include "storage.h"
#include "pwm.h"
#include "ptc.h"
#include "tip.h"
#include "pid.h"
#include "boost.h"
#include "buzzer.h"
#include "encoder.h"
#include "motion.h"
#include "handler.h"
#include "station.h"
#include "menuHandlr.h"
#include "pages.h"
#include "uart_link.h"
#include "lcd_temps.h"
#include "platform_compat.h"

static const uint32_t CONTROL_MS = 50;
static const uint32_t TIP_MS     = 500;
static uint32_t lastControl = 0, lastTip = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("========== Dioscuri SLAVE RP2040 (V1 port) =========="));

    // Satu-satunya pemilik init EEPROM
    storage.begin();
    storage.loadSettings();
    setTipProfile(currentTipMode);

    initEncoder();
    pinMode(BUZZER_PIN, OUTPUT);

    // initStations → PWM/PTC + initAirHandler → hotGun.begin()
    // (hotGun HANYA load setting, tidak storage.begin lagi)
    initStations();
    detectTip();
    initMotion();
    initUartLink();
    initLcdTemps();

    lastActivity = millis();
    lastControl = lastTip = millis();
    beep();
    Serial.println(F("[OK] slave ready — single storage.begin()"));
}

void loop() {
    const uint32_t now = millis();

    // FAST: input + UART RX
    handleMenu(getEncoderDelta(), buttonPressed());
    menuClick = false;
    updateMotion();
    updateUartLink();

    // CONTROL ~20 Hz (satu jalur station)
    if (now - lastControl >= CONTROL_MS) {
        lastControl = now;
        updateBoost();
        updateStations();   // updatePID + updateAirHandler
    }

    // TIP detect ~2 Hz (satu panggilan)
    if (now - lastTip >= TIP_MS) {
        lastTip = now;
        detectTip();
    }

    storage.tick();
    updateLcdTemps();
}
