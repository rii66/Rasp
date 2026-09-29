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
#include "oled_ui.h"
#include "platform_compat.h"

#define STATUS_LED 25

static const uint32_t CONTROL_MS = 50;
static const uint32_t TIP_MS     = 500;
static uint32_t lastControl = 0, lastTip = 0;
static uint32_t lastSerialAlive = 0;

static void bootMark(const __FlashStringHelper* label) {
    Serial.print(F("[BOOT] "));
    Serial.println(label);
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    delay(100);
}

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    Serial.begin(115200);
    Serial.ignoreFlowControl(true);
    delay(100);
    Serial.println(F("\r\n========== Dioscuri SLAVE RP2040 BOOT TRACE =========="));
    Serial.println(F("[BOOT 0] USB CDC serial ready"));

    bootMark(F("storage.begin ->"));
    storage.begin();
    bootMark(F("storage.begin OK; loadSettings ->"));
    storage.loadSettings();
    bootMark(F("loadSettings OK; setTipProfile ->"));
    setTipProfile(currentTipMode);
    bootMark(F("setTipProfile OK; initEncoder ->"));

    initEncoder();
    bootMark(F("initEncoder OK; buzzer pin ->"));
    pinMode(BUZZER_PIN, OUTPUT);

    bootMark(F("buzzer pin OK; initStations ->"));
    initStations();
    bootMark(F("initStations OK; detectTip ->"));
    detectTip();
    bootMark(F("detectTip OK; initMotion ->"));
    initMotion();
    bootMark(F("initMotion OK; initUartLink ->"));
    initUartLink();
    bootMark(F("initUartLink OK; initLcdTemps ->"));
    initLcdTemps();
    bootMark(F("initLcdTemps OK; initOledUI ->"));
    initOledUI();
    bootMark(F("initOledUI OK; beep ->"));

    lastActivity = millis();
    lastControl = lastTip = millis();
    lastSerialAlive = millis();

    beep();

    digitalWrite(STATUS_LED, HIGH);
    Serial.println(F("[OK] slave ready; entering loop"));
}

void loop() {
    const uint32_t now = millis();

    if (now - lastSerialAlive >= 1000) {
        lastSerialAlive = now;
        Serial.println(F("[RUN] slave alive"));
    }

    // EC1: solder temp / menu. Long hold SW1 opens/closes OLED menu.
    handleMenu(getEncoderDelta(), buttonPressed());
    menuClick = false;

    // EC2: hot-air temperature. SW2 short = heater ON/OFF.
    static bool lastSw2 = false;
    static uint32_t sw2Start = 0;
    const bool sw2 = button2Pressed();

    if (sw2 && !lastSw2) {
        sw2Start = now;
    }

    if (!sw2 && lastSw2) {
        const uint32_t hold = now - sw2Start;

        if (hold >= 2100) {
            inMenu = !inMenu;
            inEdit = false;
            isEditingValue = false;
            if (inMenu) {
                page = PAGE_SET;
                item = 0;
            }
            beepLong();
        } else if (hold > 50) {
            handleAirButton();
            activeStation = STATION_MODE_HOTAIR;
        }
    }

    lastSw2 = sw2;

    // EC2 rotation controls hot-air target outside OLED menu.
    const int airDelta = getEncoder2Delta();
    if (airDelta != 0 && !inMenu) {
        activeStation = STATION_MODE_HOTAIR;
        handleAirEncoder(airDelta);
    }

    // Fan speed is controlled ONLY by the physical potentiometer.
    updateAirFanFromPot();

    updateMotion();
    updateUartLink();

    if (now - lastControl >= CONTROL_MS) {
        lastControl = now;
        updateBoost();
        updateStations();
    }

    if (tipError || activeTip == nullptr) {
        detectTip();
    }

    storage.tick();
    updateLcdTemps();
    updateOledUI();
}