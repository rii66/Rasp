#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// ============================================================
// Storage — port dari Carberus/pandora/storage.cpp
// ESP32  : Preferences (NVS)
// RP2040 : EEPROM blob (compatible API)
// ============================================================

class Storage {
public:
    void begin();

    // ---- Hot Air (Siroccro) ----
    void loadSiroccroCalibration(uint16_t &c0, uint16_t &c1, uint16_t &c2);
    void saveSiroccroCalibration(uint16_t c0, uint16_t c1, uint16_t c2);

    void loadSiroccroSettings(uint16_t &temp, uint8_t &fan);
    void saveSiroccroSettings(uint16_t temp, uint8_t fan);

    void loadSiroccroPID(float &kp, float &ki, float &kd);
    void saveSiroccroPID(float kp, float ki, float kd);

    // ---- Solder (Patri / CastorS) ----
    void loadSettings();
    void saveSettings();

    void loadPID();
    void savePID();
    void saveActivePID();

    void saveBoost();
    void saveSleep();
    void saveCal();
    void saveTip();
    void saveBuzzer();

    void factoryReset();

    // RP2040: panggil tiap loop — commit tertunda setelah idle
    void tick();
    // Paksa commit sekarang (mis. sebelum sleep/reboot)
    void flush();
};

extern Storage storage;

// Bare API (menuHandlr / handler)
void loadSettings();
void saveSettings();
void loadPID();
void savePID();
void saveActivePID();
void saveBoost();
void saveSleep();
void saveCal();
void saveTip();
void saveBuzzer();

void storageTick();
void storageFlush();

#endif
