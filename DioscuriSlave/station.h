#ifndef STATION_H
#define STATION_H

#include <Arduino.h>

void initStations();
void updateStations();   // updatePID + updateAirHandler
void handleStationEncoder(int delta);
void handleStationButton();

#endif
