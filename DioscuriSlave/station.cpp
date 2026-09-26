#include "station.h"

#include "GlobalState.h"

#include "pid.h"
#include "pwm.h"
#include "ptc.h"

#include "handler.h"

void initStations() {
    // Solder
    initPWM();
    initPTC();

    // Hot Air
    initAirHandler();
}

void updateStations() {
    // Kedua station boleh berjalan bersamaan
    updatePID();
    updateAirHandler();
}


void handleStationEncoder(int delta) {
    if (delta == 0) return;

    switch (activeStation) {

        case STATION_MODE_SOLDER:
            break;

        case STATION_MODE_HOTAIR:
            handleAirEncoder(delta);
            break;
    }
}

void handleStationButton() {
    switch (activeStation) {

        case STATION_MODE_SOLDER:
            break;

        case STATION_MODE_HOTAIR:
            handleAirButton();
            break;
    }
}
      
