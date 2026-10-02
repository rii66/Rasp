/**
 * Dioscuri SLAVE — RP2040
 * Port lean dari Dioscurios V1
 * master ESP opsional via UART
 */
#include <Arduino.h>
#include <string.h>
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
#include "monitoring.h"

#define STATUS_LED 25

static const uint32_t CONTROL_MS = 50;
static uint32_t lastControl = 0;

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    Serial.begin(115200);
    Serial.ignoreFlowControl(true);
    delay(100);

    Serial.println(F("\r\n========== Dioscuri SLAVE RP2040 =========="));
    monitorBoot(F("USB Serial"));

    storage.begin();
    bootMark(F("Storage"));

    storage.loadSettings();
    bootMark(F("Settings"));

    setTipProfile(currentTipMode);
    bootMark(F("Tip profile"));

    initEncoder();
    bootMark(F("EC1 / EC2"));

    pinMode(BUZZER_PIN, OUTPUT);
    bootMark(F("Buzzer"));

    initStations();
    bootMark(F("Stations / PWM / Hot-Air + Fan"));

    detectTip();
    bootMark(F("Tip detect"));

    initMotion();
    bootMark(F("Motion"));

    initUartLink();
    bootMark(F("UART slave / waiting master"));

    initLcdTemps();
    bootMark(F("Nokia 1 / 2"));

    initOledUI();
    bootMark(F("OLED"));

    lastActivity = millis();
    lastControl = millis();

    beep();

    digitalWrite(STATUS_LED, HIGH);
    Serial.println(F("--------------------------------------------"));
    Serial.println(F("[READY] Slave waiting for master..."));
}

void loop() {
    const uint32_t now = millis();

    // EC1: solder temp / menu. Long hold SW1 opens/closes OLED menu.
    const int enc1Delta = getEncoderDelta();
    const bool sw1 = buttonPressed();

    handleMenu(enc1Delta, sw1);
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
        }
        else if (hold > 50) {
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

    monitorInput(enc1Delta, airDelta);
    monitorRuntime();
}
