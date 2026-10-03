#include "tip.h"
#include "config.h"
#include "GlobalState.h"
#include "pages.h"
#include "pid.h"
#include "pwm.h"


// ============================================================
// TIP DATABASE
// ============================================================

TipConfig tipDatabase[] = {

    // T12
    {
        TIP_T12,
        600,
        900,
        3.2f,
        0.12f,
        1.8f,
        255,
        450,
        0.12f,
        0,
        0,
        "T12"
    },

    // C210
    {
        TIP_C210,
        100,
        500,
        2.8f,
        0.10f,
        1.4f,
        71,
        380,
        0.12f,
        0,
        0,
        "C210"
    }
};

const int TOTAL_SUPPORTED_TIPS =
    sizeof(tipDatabase) / sizeof(tipDatabase[0]);


// ============================================================
// CUSTOM PROFILE
// ============================================================

TipConfig customTipProfile = {
    TIP_CUSTOM,
    ADC_CUSTOM_MIN,
    ADC_CUSTOM_MAX,
    2.0f,
    0.05f,
    0.8f,
    255,
    600,
    0.12f,
    0,
    0,
    "CUSTOM"
};


// ============================================================
// ACTIVE / DETECTED
// ============================================================

TipConfig *activeTip = nullptr;

TipID detectedTip = TIP_T12;


// ============================================================
// APPLY PROFILE
// ============================================================

void applyTipProfile(TipConfig *targetTip)
{
    if (!targetTip)
        return;

    activeTip = targetTip;

    currentTip = targetTip->tipID;

    kp = targetTip->kp;
    ki = targetTip->ki;
    kd = targetTip->kd;

    maxTemp = targetTip->maxTemp;

    // Keep calibration values coherent even when the profile was created
    // with a raw database that did not initialize the linear conversion fields.
    if (targetTip->tipID != TIP_CUSTOM && targetTip->maxADC > targetTip->minADC) {
        targetTip->slope = ((float)TEMP_TIP[2] - (float)TEMP_TIP[0]) /
                           ((float)targetTip->maxADC - (float)targetTip->minADC);
        targetTip->tempOffset = TEMP_TIP[0];
        targetTip->adcOffset = 0;
    } else {
        targetTip->slope = 0.0f;
        targetTip->tempOffset = 0;
        targetTip->adcOffset = 0;
    }

    maxPwmLimit = constrain(targetTip->maxPWM, 0, PWM_MAX_VAL);

    if (maxTemp > 0 && targetTemp > maxTemp)
        targetTemp = maxTemp;
}


// ============================================================
// SET TIP PROFILE
// ============================================================

void setTipProfile(int mode)
{
    currentTipMode = mode;

    switch (mode)
    {
        case TIP_ITEM_T12:
            applyTipProfile(&tipDatabase[0]);
            break;

        case TIP_ITEM_C210:
            applyTipProfile(&tipDatabase[1]);
            break;

        case TIP_ITEM_CUSTOM:
            applyTipProfile(&customTipProfile);
            break;

        case TIP_ITEM_AUTO:
        default:
            break;
    }

    if (maxTemp > 0 && targetTemp > maxTemp)
        targetTemp = maxTemp;
}
