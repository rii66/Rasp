#include "ptc.h"
#include "GlobalState.h"
#include "config.h"
#include <Arduino.h>

// ====================================================================
// PTC CONFIG
// ====================================================================
int ptc_adc_dingin = ADC_CUSTOM_MIN;
int ptc_adc_panas  = ADC_CUSTOM_MAX;

static int ptcCal = 0;


// ====================================================================
// INITIALIZATION
// ====================================================================
void initPTC()
{
    ptcCal = 0;

#if defined(ARDUINO_ARCH_RP2040)
    analogReadResolution(12);
#endif
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
                   TEMP_MAX_CUSTOM);

    suhu += ptcCal;

    return constrain(suhu, 0, TEMP_MAX_CUSTOM);
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
