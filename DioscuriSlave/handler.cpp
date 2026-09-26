#include "platform_compat.h"
#include "handler.h"
#include "hotgun.h"
#include "config.h"

static HotGun hotGun;

void initAirHandler() {
    hotGun.begin();

    attachInterrupt(
        digitalPinToInterrupt(PIN_ZERO_CROSS),
        handleAirZeroCross,
        FALLING
    );
}

void updateAirHandler() {
    hotGun.update();
}

void handleAirEncoder(int delta) {
    if (delta == 0) return;

    int temp = hotGun.getTargetTemp();
    temp += delta * 5;

    hotGun.setTemp(temp);
}

void handleAirButton() {
    hotGun.switchPower(!hotGun.isOn());
}

void IRAM_ATTR handleAirZeroCross() {
    hotGun.onZeroCross();
}

/* ================= API untuk WebSocket / UI ================= */

void airSetTemp(uint16_t celsius) {
    hotGun.setTemp(celsius);
}

void airSetFan(uint8_t speed) {
    hotGun.setFan(speed);
}

void airSwitchPower(bool on) {
    hotGun.switchPower(on);
}

void airSaveSettings() {
    hotGun.saveCurrentSettings();
}

uint16_t airGetTemp() {
    return hotGun.getTemp();
}

uint16_t airGetTargetTemp() {
    return hotGun.getTargetTemp();
}

uint8_t airGetPower() {
    return hotGun.getPower();
}

uint8_t airGetFan() {
    return hotGun.getFan();
}

bool airIsOn() {
    return hotGun.isOn();
}

bool airHasAC() {
    return hotGun.hasAC();
}

const char* airGetModeStr() {
    switch (hotGun.getMode()) {
        case HotGun::MODE_ON:      return "ON";
        case HotGun::MODE_FIXED:   return "FIXED";
        case HotGun::MODE_COOLING: return "COOLING";
        case HotGun::MODE_OFF:
        default:                   return "OFF";
    }
}
