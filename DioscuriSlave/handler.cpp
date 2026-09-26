#include "platform_compat.h"
#include "handler.h"
#include "hotgun.h"
#include "config.h"

static HotGun hotGun;

void initAirHandler() {
    hotGun.begin();
    pinMode(PIN_POT_FAN, INPUT);

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
    temp = constrain(temp, TEMP_MIN_C, TEMP_MAX_C);
    hotGun.setTemp((uint16_t)temp);
}

void handleAirButton() {
    hotGun.switchPower(!hotGun.isOn());
}

void updateAirFanFromPot() {
    int raw = analogRead(PIN_POT_FAN);
    int speed = map(raw, 0, 4095, 0, 255);

    if (speed < FAN_MIN_SPEED / 2) speed = 0;

    airSetFan((uint8_t)speed);
}

void IRAM_ATTR handleAirZeroCross() {
    hotGun.onZeroCross();
}

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
