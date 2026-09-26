#ifndef SIROCCRO_HANDLER_H
#define SIROCCRO_HANDLER_H

#include <Arduino.h>
#include "platform_compat.h"

void initAirHandler();
void updateAirHandler();

void handleAirEncoder(int delta);
void handleAirButton();

void airSetTemp(uint16_t celsius);
void airSetFan(uint8_t speed);
void airSwitchPower(bool on);
void airSaveSettings();
uint16_t airGetTemp();
uint16_t airGetTargetTemp();
uint8_t  airGetPower();
uint8_t  airGetFan();
bool     airIsOn();
bool     airHasAC();
const char* airGetModeStr();

void IRAM_ATTR handleAirZeroCross();

#endif