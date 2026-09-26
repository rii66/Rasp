#include "GlobalState.h"
#include "config.h"
#include "pages.h"

#include "pid.h"
#include "pwm.h"
#include "tip.h"
#include "ptc.h"
#include "boost.h"

//====================================================================//
// CONFIGS & CALIBRATION (Clean & Explicit)
//====================================================================//
#if defined(ARDUINO_ARCH_RP2040)
const uint8_t  ADC_SAMPLE_COUNT     = 6;
const uint16_t ADC_SAMPLE_DELAY_US  = 50;
const uint16_t SETTLING_DELAY_US    = 150;
#else
const uint8_t  ADC_SAMPLE_COUNT     = 10;
const uint16_t ADC_SAMPLE_DELAY_US  = 150;
const uint16_t SETTLING_DELAY_US    = 250;
#endif
const int      HEATER_HYSTERESIS    = 5;      // Batas toleransi pemanasan (Celsius)
const int      PID_INTEGRAL_LIMIT   = 500;    // Batasi angin-up pada integral

//====================================================================//
// PID INTERNAL & TARGET
//====================================================================//
float pidError      = 0;
float pidIntegral   = 0;
float pidDerivative = 0;
float lastError     = 0;

//====================================================================//
// CORE ADC READER (Optimized for RP2040 + multi-platform)
//====================================================================//
uint16_t getAverageADC() {
uint32_t totalRawAdc = 0; // Gunakan 32-bit untuk mencegah overflow saat penjumlahan
for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++) {
totalRawAdc += analogRead(TEMP_PIN);
delayMicroseconds(ADC_SAMPLE_DELAY_US);
}
return (uint16_t)(totalRawAdc / ADC_SAMPLE_COUNT);
}

//====================================================================//
// MULTI-TIP CALIBRATION ENGINE (UNIVERSAL & DYNAMIC)
//====================================================================//
int adcToTemp(uint16_t rawAdc) {

    if (activeTip == nullptr)
        return 0;

    // Sensor PTC
    if (activeTip->tipID == TIP_CUSTOM) {
        return ptcToTemp(rawAdc);
    }

    // Sensor T12 / C210
    int adc = rawAdc + activeTip->adcOffset;

    float temp =
        (adc * activeTip->slope) +
        activeTip->tempOffset;

    return (int)temp;
}

//====================================================================//
// SAFETY
//====================================================================//
void handleSafety() {
overHeat = (currentTemp > maxTemp);
}

//====================================================================//
// READ TEMP
//====================================================================//
int readTemp() {
startTempRead();

uint16_t rawAdc = getAverageADC();   
  
endTempRead();  

// Menggunakan fungsi kalibrasi dinamis sesuai jenis tip  
return adcToTemp(rawAdc);

}

//====================================================================//
// PID UPDATE
//====================================================================//
void updatePID() {
currentTemp = readTemp();
handleSafety();

// ===== SAFETY LOCK =====  
if (tipError || overHeat || activeTip == nullptr || heaterState == STATE_TIP) {  
    pwmOut      = 0;  
    pidIntegral = 0;  
    lastError   = 0;  
    heaterOff();  
    return;  
}  

// ===== TARGET SELECT =====  
int activeTarget = targetTemp;  
if (sleeping)  activeTarget = sleepTemp;  
if (boostMode) activeTarget = boostTemp;  

// ===== HEATER STATE =====  
if (currentTemp < (activeTarget - HEATER_HYSTERESIS)) {  
    heaterState = STATE_HEAT;  
} else {  
    heaterState = STATE_HOLD;  
}  

// ===== PID =====  
pidError      = activeTarget - currentTemp;  
  
// Integral limit menggunakan konstanta PID_INTEGRAL_LIMIT  
pidIntegral   = constrain(pidIntegral + pidError, -PID_INTEGRAL_LIMIT, PID_INTEGRAL_LIMIT);  
pidDerivative = pidError - lastError;  

float activeKp = activeTip->kp;
float activeKi = activeTip->ki;
float activeKd = activeTip->kd;

float output =
    (activeKp * pidError) +
    (activeKi * pidIntegral) +
    (activeKd * pidDerivative);

lastError     = pidError;  

{
  int lim = (maxPwmLimit > 0) ? maxPwmLimit : PWM_MAX_VAL;
  pwmOut = constrain((int)output, 0, lim);
  setPWM(pwmOut);
}


}

//====================================================================//
// AUTO / CUSTOM TIP DETECT
//====================================================================//
void detectTip() {

    heaterOff();  
    delayMicroseconds(SETTLING_DELAY_US);   

    uint16_t sensorValue = getAverageADC();   
    TipConfig *foundTip = nullptr;  

    // ====================================================
    // 1. DETEKSI TIDAK ADA TIP (Mendukung Thermocouple & PTC)
    // ====================================================
    // Jika mode Custom/PTC dan nilai melonjak ke atas (>= 4000)
    // ATAU jika mode T12/C210 dan nilai drop ke bawah (<= 260)
    if (sensorValue >= ADC_NO_TIP_PTC || sensorValue <= ADC_NO_TIP) {

        activeTip = nullptr;
        detectedTip = TIP_AUTO;
        tipError = true;

        pidIntegral = 0;
        lastError = 0;

        heaterState = STATE_TIP;
        heaterOff();
        return;
    }

    // CUSTOM (PTC)
    if (currentTipMode == TIP_ITEM_CUSTOM) {
        foundTip = &customTipProfile;
    }

    // AUTO / T12 / C210
    else {
        for (int i = 0; i < TOTAL_SUPPORTED_TIPS; i++) {

            // Mode manual T12
            if (currentTipMode == TIP_ITEM_T12 &&
                tipDatabase[i].tipID != TIP_T12)
                continue;

            // Mode manual C210
            if (currentTipMode == TIP_ITEM_C210 &&
                tipDatabase[i].tipID != TIP_C210)
                continue;

            // AUTO akan memeriksa semua profile
            if (sensorValue >= tipDatabase[i].minADC &&
                sensorValue <= tipDatabase[i].maxADC) {

                foundTip = &tipDatabase[i];
                break;
            }
        }
    }

    // ====================================================  
    // 2. ERROR HANDLING TERPUSAT  
    // ====================================================  
    if (foundTip == nullptr) {  
        activeTip   = nullptr;  
        detectedTip = TIP_AUTO;  
        tipError    = true;  
        heaterState = STATE_TIP;  
          
        pidIntegral = 0;  
        lastError   = 0;  
          
        heaterOff();  
        return;  
    }  

    // ====================================================  
    // 3. APPLY PROFILE  
    // ====================================================  
    detectedTip = foundTip->tipID;  

    if (foundTip != activeTip) {  
        applyTipProfile(foundTip);  
        pidIntegral = 0;  
        lastError   = 0;  
    }  

    tipError = false;  

    if (heaterState == STATE_TIP) {  
        heaterState = STATE_HEAT;  
    }
}
