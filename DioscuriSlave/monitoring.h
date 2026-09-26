#ifndef MONITORING_H
#define MONITORING_H

#include <Arduino.h>
#include "config.h"
#include "GlobalState.h"

extern uint16_t airGetTemp();
extern uint16_t airGetTargetTemp();
extern uint8_t airGetPower();
extern uint8_t airGetFan();
extern bool airIsOn();
extern bool airHasAC();
extern const char* airGetModeStr();

inline void monitorStations() {

    static unsigned long lastPrint = 0;

    if (millis() - lastPrint < 500)
        return;

    lastPrint = millis();

    // =========================
    // SOLDER / PATRI
    // =========================

    uint16_t solderRaw = analogRead(TEMP_PIN);

    Serial.println();
    Serial.println("========== HERMENEX ==========");

    Serial.print("[SOLDER] ADC Raw     : ");
    Serial.println(solderRaw);

    Serial.print("[SOLDER] Current Temp: ");
    Serial.print(currentTemp);
    Serial.println(" C");

    Serial.print("[SOLDER] Target Temp : ");
    Serial.print(targetTemp);
    Serial.println(" C");

    Serial.print("[SOLDER] PWM         : ");
    Serial.println(pwmOut);

    Serial.print("[SOLDER] Tip Error    : ");
    Serial.println(tipError ? "YES" : "NO");


    // =========================
    // HOT AIR / SIROCCRO
    // =========================

    Serial.println();

    Serial.print("[HOT AIR] Temp       : ");
    Serial.print(airGetTemp());
    Serial.println(" C");

    Serial.print("[HOT AIR] Target     : ");
    Serial.print(airGetTargetTemp());
    Serial.println(" C");

    Serial.print("[HOT AIR] Power      : ");
    Serial.print(airGetPower());
    Serial.println(" %");

    Serial.print("[HOT AIR] Fan        : ");
    Serial.print(airGetFan());
    Serial.println(" %");

    Serial.print("[HOT AIR] AC         : ");
    Serial.println(airHasAC() ? "YES" : "NO");

    Serial.print("[HOT AIR] State      : ");
    Serial.println(airIsOn() ? "ON" : "OFF");

    Serial.print("[HOT AIR] Mode       : ");
    Serial.println(airGetModeStr());

    Serial.println("==============================");
}

#endif