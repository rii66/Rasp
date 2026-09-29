#include "station.h"

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
    // Kedua station boleh berjalan bersamaan.
    updatePID();
    updateAirHandler();
}
