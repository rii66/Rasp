/**
 * storage.cpp — RP2040 EEPROM only (no Preferences / NVS)
 * Port lean Dioscurios V1 slave
 */
#include <string.h>
#include <EEPROM.h>

#include "storage.h"
#include "config.h"
#include "GlobalState.h"
#include "tip.h"
#include "pages.h"
#include "boost.h"

Storage storage;

static const uint32_t STORAGE_MAGIC = 0x44314F53; // 'D1OS'
static const int EEPROM_SIZE = 512;

#ifndef STORAGE_COMMIT_DELAY_MS
#define STORAGE_COMMIT_DELAY_MS  3000UL
#endif

struct TipPersist {
    float kp, ki, kd;
    int   tempOffset;
    int   adcOffset;
};

struct PersistBlob {
    uint32_t magic;
    int  targetTemp;
    int  boostTemp;
    int  boostTimeSec;
    int  sleepTemp;
    int  sleepTimeSec;
    bool buzzerEnabled;
    int  tipMode;
    TipPersist tips[2];
    TipPersist custom;
    uint16_t sc_c0, sc_c1, sc_c2;
    uint16_t sc_temp;
    uint8_t  sc_fan;
    float    sc_kp, sc_ki, sc_kd;
};

static PersistBlob blob;
static PersistBlob blobCommitted;
static bool dirty = false;
static unsigned long dirtySince = 0;

static void defaultsBlob() {
    memset(&blob, 0, sizeof(blob));
    blob.magic = STORAGE_MAGIC;
    blob.targetTemp    = DEFAULT_TEMP;
    blob.boostTemp     = DEFAULT_BOOST_TEMP;
    blob.boostTimeSec  = DEFAULT_BOOST_TIME;
    blob.sleepTemp     = DEFAULT_SLEEP_TEMP;
    blob.sleepTimeSec  = 240;
    blob.buzzerEnabled = true;
    blob.tipMode       = TIP_ITEM_AUTO;
    blob.sc_c0 = 2348;
    blob.sc_c1 = 3004;
    blob.sc_c2 = 3400;
    blob.sc_temp = DEFAULT_HOTAIR_TEMP;
    blob.sc_fan  = 0;
    blob.sc_kp = 12.0f;
    blob.sc_ki = 0.6f;
    blob.sc_kd = 25.0f;
}

static void eepromLoad() {
    EEPROM.get(0, blob);
    if (blob.magic != STORAGE_MAGIC) {
        defaultsBlob();
        dirty = true;
        dirtySince = millis();
        memset(&blobCommitted, 0, sizeof(blobCommitted));
    } else {
        blobCommitted = blob;
        dirty = false;
    }
}

static void markDirty() {
    dirty = true;
    dirtySince = millis();
}

static void eepromFlush(bool force = false) {
    if (!dirty && !force) return;
    if (memcmp(&blob, &blobCommitted, sizeof(blob)) == 0) {
        dirty = false;
        return;
    }
    EEPROM.put(0, blob);
    if (EEPROM.commit()) {
        blobCommitted = blob;
        dirty = false;
    } else {
        Serial.println(F("[Storage] EEPROM commit FAILED"));
    }
}

void Storage::begin() {
    EEPROM.begin(EEPROM_SIZE);
    eepromLoad();
    if (dirty) eepromFlush(true);
    Serial.println(F("[Storage] EEPROM ready"));
}

void Storage::loadSiroccroCalibration(uint16_t &c0, uint16_t &c1, uint16_t &c2) {
    c0 = blob.sc_c0; c1 = blob.sc_c1; c2 = blob.sc_c2;
}
void Storage::saveSiroccroCalibration(uint16_t c0, uint16_t c1, uint16_t c2) {
    blob.sc_c0 = c0; blob.sc_c1 = c1; blob.sc_c2 = c2;
    markDirty();
}

void Storage::loadSiroccroSettings(uint16_t &temp, uint8_t &fan) {
    temp = blob.sc_temp; fan = blob.sc_fan;
}
void Storage::saveSiroccroSettings(uint16_t temp, uint8_t fan) {
    blob.sc_temp = temp; blob.sc_fan = fan;
    markDirty();
}

void Storage::loadSiroccroPID(float &kp, float &ki, float &kd) {
    kp = blob.sc_kp; ki = blob.sc_ki; kd = blob.sc_kd;
}
void Storage::saveSiroccroPID(float kp, float ki, float kd) {
    blob.sc_kp = kp; blob.sc_ki = ki; blob.sc_kd = kd;
    markDirty();
}

void Storage::loadSettings() {
    targetTemp    = blob.targetTemp;
    boostTemp     = blob.boostTemp;
    boostTimeSec  = blob.boostTimeSec;
    sleepTemp     = blob.sleepTemp;
    sleepTimeSec  = blob.sleepTimeSec;
    buzzerEnabled = blob.buzzerEnabled;
    currentTipMode = blob.tipMode;

    if (currentTipMode < TIP_ITEM_T12 || currentTipMode > TIP_ITEM_CUSTOM)
        currentTipMode = TIP_ITEM_AUTO;

    for (int i = 0; i < TOTAL_SUPPORTED_TIPS && i < 2; i++) {
        TipPersist &p = blob.tips[i];
        if (p.kp != 0.0f || p.ki != 0.0f || p.kd != 0.0f) {
            tipDatabase[i].kp = p.kp;
            tipDatabase[i].ki = p.ki;
            tipDatabase[i].kd = p.kd;
            tipDatabase[i].tempOffset = p.tempOffset;
            tipDatabase[i].adcOffset  = p.adcOffset;
        }
    }
    if (blob.custom.kp != 0.0f || blob.custom.ki != 0.0f || blob.custom.kd != 0.0f) {
        customTipProfile.kp = blob.custom.kp;
        customTipProfile.ki = blob.custom.ki;
        customTipProfile.kd = blob.custom.kd;
        customTipProfile.tempOffset = blob.custom.tempOffset;
        customTipProfile.adcOffset  = blob.custom.adcOffset;
    }

    switch (currentTipMode) {
        case TIP_ITEM_T12:    applyTipProfile(&tipDatabase[0]); break;
        case TIP_ITEM_C210:   applyTipProfile(&tipDatabase[1]); break;
        case TIP_ITEM_CUSTOM: applyTipProfile(&customTipProfile); break;
        default: activeTip = nullptr; break;
    }
}

void Storage::saveSettings() {
    blob.targetTemp = targetTemp;
    markDirty();
}

void Storage::loadPID() {
    if (!activeTip) return;
    kp = activeTip->kp;
    ki = activeTip->ki;
    kd = activeTip->kd;
}

void Storage::savePID() {
    if (!activeTip) return;
    activeTip->kp = kp;
    activeTip->ki = ki;
    activeTip->kd = kd;
    if (activeTip == &tipDatabase[0])
        blob.tips[0] = {kp, ki, kd, activeTip->tempOffset, activeTip->adcOffset};
    else if (TOTAL_SUPPORTED_TIPS > 1 && activeTip == &tipDatabase[1])
        blob.tips[1] = {kp, ki, kd, activeTip->tempOffset, activeTip->adcOffset};
    else if (activeTip == &customTipProfile)
        blob.custom = {kp, ki, kd, activeTip->tempOffset, activeTip->adcOffset};
    markDirty();
}

void Storage::saveActivePID() {
    if (!activeTip) return;
    kp = activeTip->kp;
    ki = activeTip->ki;
    kd = activeTip->kd;
    savePID();
}

void Storage::saveBoost() {
    blob.boostTemp = boostTemp;
    blob.boostTimeSec = boostTimeSec;
    markDirty();
}

void Storage::saveSleep() {
    blob.sleepTemp = sleepTemp;
    blob.sleepTimeSec = sleepTimeSec;
    markDirty();
}

void Storage::saveCal() {
    if (!activeTip) return;
    activeTip->tempOffset = tempOffset;
    activeTip->adcOffset  = adcOffset;
    if (activeTip == &tipDatabase[0]) {
        blob.tips[0].tempOffset = tempOffset;
        blob.tips[0].adcOffset  = adcOffset;
    } else if (TOTAL_SUPPORTED_TIPS > 1 && activeTip == &tipDatabase[1]) {
        blob.tips[1].tempOffset = tempOffset;
        blob.tips[1].adcOffset  = adcOffset;
    } else if (activeTip == &customTipProfile) {
        blob.custom.tempOffset = tempOffset;
        blob.custom.adcOffset  = adcOffset;
    }
    markDirty();
}

void Storage::saveTip() {
    blob.tipMode = currentTipMode;
    markDirty();
}

void Storage::saveBuzzer() {
    blob.buzzerEnabled = buzzerEnabled;
    markDirty();
}

void Storage::factoryReset() {
    defaultsBlob();
    dirty = true;
    eepromFlush(true);
}

void Storage::tick() {
    if (!dirty) return;
    if (millis() - dirtySince < STORAGE_COMMIT_DELAY_MS) return;
    eepromFlush(false);
}

void Storage::flush() {
    eepromFlush(true);
}

void loadSettings()  { storage.loadSettings(); }
void saveSettings()  { storage.saveSettings(); }
void loadPID()       { storage.loadPID(); }
void savePID()       { storage.savePID(); }
void saveActivePID() { storage.saveActivePID(); }
void saveBoost()     { storage.saveBoost(); }
void saveSleep()     { storage.saveSleep(); }
void saveCal()       { storage.saveCal(); }
void saveTip()       { storage.saveTip(); }
void saveBuzzer()    { storage.saveBuzzer(); }
void storageTick()   { storage.tick(); }
void storageFlush()  { storage.flush(); }
