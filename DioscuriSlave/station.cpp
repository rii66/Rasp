#include "station.h"

#include "GlobalState.h"

#include "pid.h"
#include "pwm.h"
#include "ptc.h"
#include "boost.h"
#include "handler.h"
#include "config.h"

void initStations() {
    // Solder
    initPWM();
    initPTC();

    // Hot Air
    initAirHandler();
}

void updateStations() {
    // Kedua station boleh berjalan bersamaan.
    updatePID();
    updateAirHandler();
}

void handleStationEncoder(int delta) {
    if (delta == 0) return;

    switch (activeStation) {
        case STATION_MODE_SOLDER:
            targetTemp += delta * 5;
            targetTemp = constrain(targetTemp, TEMP_MIN, maxTemp);
            break;

        case STATION_MODE_HOTAIR:
            handleAirEncoder(delta);
            break;
    }
}

void handleStationButton() {
    switch (activeStation) {
        case STATION_MODE_SOLDER:
            startBoost();
            break;

        case STATION_MODE_HOTAIR:
            handleAirButton();
            break;
    }
}
