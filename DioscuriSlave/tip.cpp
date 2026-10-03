#include "tip.h"
#include "config.h"
#include "GlobalState.h"
#include "pages.h"
#include "pid.h"
#include "pwm.h"


// ============================================================
// TIP DATABASE
// ============================================================
// Catatan kalibrasi untuk gain \~201x @ 3.3V (LMV358 + clamp):
// T12 thermocouple \~ 10-15 µV/°C → di 400°C hanya \~0.9-1.2 V
// ADC 12-bit (0-4095) hanya memakai range rendah (\~100-1300).
//
// Nilai minADC / maxADC di bawah adalah ESTIMASI awal.
// Ukur ADC dingin & panas nyata, lalu ganti angka ini.
// ============================================================

TipConfig tipDatabase[] = {

    // T12  (estimasi gain 201x @ 3.3V)
    {
        TIP_T12,
        150,          // minADC  ≈ dingin / no-tip boundary (ukur nyata!)
        1250,         // maxADC  ≈ \~400°C (ukur nyata!)
        3.2f,         // kp
        0.12f,        // ki
        1.8f,         // kd
        255,          // maxPWM
        450,          // maxTemp
        0.0f,         // slope (dihitung di applyTipProfile)
        0,            // tempOffset
        0,            // adcOffset
        "T12"
    },

    // C210 (biasanya sinyal lebih kecil dari T12)
    {
        TIP_C210,
        80,           // minADC
        900,          // maxADC
        2.8f,
        0.10f,
        1.4f,
        71,
        380,
        0.0f,
        0,
        0,
        "C210"
    }
};

const int TOTAL_SUPPORTED_TIPS =
    sizeof(tipDatabase) / sizeof(tipDatabase[0]);


// ============================================================
// CUSTOM PROFILE (PTC)
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
    0.0f,
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

    // -------------------------------------------------------
    // Linear conversion untuk Thermocouple (T12 / C210)
    // Asumsi 2-titik:
    //   minADC  →  TEMP_AMBIENT_C  (≈ 25-28°C)
    //   maxADC  →  400°C
    //
    // Nanti kalau sudah punya termometer, ganti ke 3-titik
    // 
    // -------------------------------------------------------
    if (targetTip->tipID != TIP_CUSTOM && targetTip->maxADC > targetTip->minADC) {

        const float t_cold = (float)TEMP_AMBIENT_C;   // 28°C dari config.h
        const float t_hot  = 400.0f;                  // titik kalibrasi atas

        targetTip->slope = (t_hot - t_cold) /
                           ((float)targetTip->maxADC - (float)targetTip->minADC);

        // temp = slope * adc + tempOffset
        targetTip->tempOffset = t_cold - (targetTip->slope * (float)targetTip->minADC);
        targetTip->adcOffset  = 0;

    } else {
        targetTip->slope      = 0.0f;
        targetTip->tempOffset = 0;
        targetTip->adcOffset  = 0;
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
