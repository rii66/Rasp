#ifndef MONITORING_H
#define MONITORING_H

#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "GlobalState.h"
#include "handler.h"

inline void monitorBoot(const __FlashStringHelper* label) {
    Serial.print(F("[BOOT] "));
    Serial.print(label);
    Serial.println(F(" ✓"));
}

inline void monitorInput(int ec1Delta, int ec2Delta) {
    if (ec1Delta != 0) {
        Serial.print(F("[INPUT] EC1 "));
        if (ec1Delta > 0) Serial.print('+');
        Serial.println(ec1Delta);
    }

    if (ec2Delta != 0) {
        Serial.print(F("[INPUT] EC2 "));
        if (ec2Delta > 0) Serial.print('+');
        Serial.println(ec2Delta);
    }
}

inline void monitorRuntime() {
    static bool first = true;
    static StationMode lastStation = STATION_MODE_SOLDER;
    static bool lastAirOn = false;
    static bool lastAirAc = false;
    static bool lastAirFan = false;
    static int lastAirPower = -1;
    static const char* lastAirMode = nullptr;
    static bool lastSleeping = false;
    static bool lastBoost = false;
    static int lastPotBucket = -1;
    static bool lastTipError = false;

    if (first) {
        lastStation = activeStation;
        lastAirOn = airIsOn();
        lastAirAc = airHasAC();
        lastAirFan = airGetFan() >= FAN_MIN_SPEED;
        lastAirPower = airGetPower();
        lastAirMode = airGetModeStr();
        lastSleeping = sleeping;
        lastBoost = boostMode;
        lastTipError = tipError;

        const int potPercent = airGetFanPotPercent();
        lastPotBucket = ((potPercent + 2) / 5) * 5;

        Serial.print(F("[STATION] "));
        Serial.println(activeStation == STATION_MODE_HOTAIR
                     ? F("HOT AIR active ✓")
                     : F("SOLDER active ✓"));
        first = false;
        return;
    }

    if (activeStation != lastStation) {
        lastStation = activeStation;
        Serial.print(F("[STATION] "));
        Serial.println(activeStation == STATION_MODE_HOTAIR
                     ? F("HOT AIR active ✓")
                     : F("SOLDER active ✓"));
    }

    const bool airOn = airIsOn();
    if (airOn != lastAirOn) {
        lastAirOn = airOn;
        Serial.println(airOn ? F("[AIR] HOT AIR ON") : F("[AIR] HOT AIR OFF"));
    }

    const bool airAc = airHasAC();
    if (airAc != lastAirAc) {
        lastAirAc = airAc;
        Serial.println(airAc ? F("[AIR] AC status ✓") : F("[AIR] AC lost"));
    }

    const bool airFan = airGetFan() >= FAN_MIN_SPEED;
    if (airFan != lastAirFan) {
        lastAirFan = airFan;
        Serial.println(airFan ? F("[AIR] Fan ON ✓") : F("[AIR] Fan OFF"));
    }

    const int airPower = airGetPower();
    if (airPower != lastAirPower) {
        lastAirPower = airPower;
        Serial.print(F("[AIR] Heater "));
        Serial.print(airPower);
        Serial.println('%');
    }

    const char* airMode = airGetModeStr();
    if (!lastAirMode || strcmp(airMode, lastAirMode) != 0) {
        lastAirMode = airMode;
        Serial.print(F("[AIR] Mode "));
        Serial.println(airMode);
    }

    // FAN POT is sampled only by updateAirFanFromPot().
    // Here we read the cached/filtered value, never the ADC directly.
    const int potPercent = airGetFanPotPercent();
    int potBucket = ((potPercent + 2) / 5) * 5;
    if (potBucket > 100) potBucket = 100;

    if (potBucket != lastPotBucket) {
        lastPotBucket = potBucket;
        Serial.print(F("[INPUT] FAN POT "));
        Serial.print(potBucket);
        Serial.println('%');
    }

    if (sleeping != lastSleeping) {
        lastSleeping = sleeping;
        Serial.println(sleeping ? F("[SLEEP] ON") : F("[SLEEP] OFF"));
    }

    if (boostMode != lastBoost) {
        lastBoost = boostMode;
        Serial.println(boostMode ? F("[BOOST] ON") : F("[BOOST] OFF"));
    }

    if (tipError != lastTipError) {
        lastTipError = tipError;
        Serial.println(tipError ? F("[TIP] ERROR") : F("[TIP] OK ✓"));
    }
}

#endif
