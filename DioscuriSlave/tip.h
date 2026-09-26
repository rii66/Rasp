#ifndef PATRI_TIP_H
#define PATRI_TIP_H

#include <Arduino.h>

typedef enum {
    TIP_T12 = 0,
    TIP_C210,
    TIP_AUTO,
    TIP_CUSTOM
} TipID;

typedef struct {
    TipID tipID;

    int minADC;
    int maxADC;

    float kp;
    float ki;
    float kd;

    uint8_t maxPWM;
    int maxTemp;

    float slope;

    int tempOffset;
    int adcOffset;

    const char *name;
} TipConfig;


// Database profile bawaan
extern TipConfig tipDatabase[];
extern const int TOTAL_SUPPORTED_TIPS;

// Profile custom
extern TipConfig customTipProfile;

// Profile yang sedang aktif
extern TipConfig *activeTip;

// Hasil deteksi
extern TipID detectedTip;


// Terapkan profile ke sistem
void applyTipProfile(TipConfig *targetTip);

// Pilih profile dari menu
void setTipProfile(int mode);

// Deteksi tip otomatis
void detectTip();

#endif