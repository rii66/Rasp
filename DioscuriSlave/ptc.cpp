#include "ptc.h"
#include "GlobalState.h"
#include <Arduino.h>

// ====================================================================
// PTC CONFIG
// ====================================================================
int ptc_adc_dingin = 700;
int ptc_adc_panas  = 1515;

static int ptcCal = 0;


// ====================================================================
// INITIALIZATION
// ====================================================================
void initPTC()
{
    ptcCal = 0;
}


// ====================================================================
// ADC TO TEMPERATURE
// ====================================================================
int ptcToTemp(uint16_t adc)
{
    if (adc >= 4090) {
        tipError = true; 
        return 0; 
    }

    
    int suhu = map(adc,
                   ptc_adc_dingin,
                   ptc_adc_panas,
                   22,
                   30);

    suhu += ptcCal;

    return constrain(suhu, 0, 600);
}


// ====================================================================
// CALIBRATION
// ====================================================================
void setPTCCal(int offset)
{
    ptcCal = offset;
}


int getPTCCal()
{
    return ptcCal;
}