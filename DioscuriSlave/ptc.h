#ifndef PTC_H
#define PTC_H

#include <Arduino.h>

// ===============================
// PTC CONFIG
// ===============================
extern int ptc_adc_dingin;
extern int ptc_adc_panas;


// ===============================
// API
// ===============================
void initPTC();

int ptcToTemp(uint16_t adc);

void setPTCCal(int offset);
int getPTCCal();

#endif