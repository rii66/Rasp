#include "GlobalState.h"
#include "config.h"
#include "pages.h"

#include "pid.h"
#include "pwm.h"
#include "tip.h"
#include "ptc.h"
#include "boost.h"

//====================================================================//
// CONFIGS & CALIBRATION
//====================================================================//
#if defined(ARDUINO_ARCH_RP2040)
const uint8_t  ADC_SAMPLE_COUNT     = 16;
const uint16_t ADC_SAMPLE_DELAY_US  = 120;
const uint16_t SETTLING_DELAY_US    = 230;
#else
const uint8_t  ADC_SAMPLE_COUNT     = 24;
const uint16_t ADC_SAMPLE_DELAY_US  = 40;
const uint16_t SETTLING_DELAY_US    = 270;
#endif
const int      HEATER_HYSTERESIS    = 5;
const int      PID_INTEGRAL_LIMIT   = 500;

//====================================================================//
// PID INTERNAL
//====================================================================//
float pidError      = 0;
float pidIntegral   = 0;
float pidDerivative = 0;
float lastError     = 0;

//====================================================================//
// CORE ADC READER
//====================================================================//
uint16_t getAverageADC() {
    // RP2040 ADC mux: discard first sample after channel switch
    analogRead(TEMP_PIN);
    delayMicroseconds(SETTLING_DELAY_US);

    uint32_t totalRawAdc = 0;
    for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++) {
        totalRawAdc += analogRead(TEMP_PIN);
        delayMicroseconds(ADC_SAMPLE_DELAY_US);
    }

    return (uint16_t)(totalRawAdc / ADC_SAMPLE_COUNT);
}

//====================================================================//
// MULTI-TIP CALIBRATION ENGINE
//====================================================================//
int adcToTemp(uint16_t rawAdc) {

    if (activeTip == nullptr)
        return 0;

    // Sensor PTC (Custom)
    if (activeTip->tipID == TIP_CUSTOM) {
        return ptcToTemp(rawAdc);
    }

    // Sensor Thermocouple (T12 / C210)
    int adc = (int)rawAdc + activeTip->adcOffset;

    float temp = (activeTip->slope * (float)adc) + activeTip->tempOffset;

    // Clamp ke range yang masuk akal
    if (temp < 0.0f)
        temp = 0.0f;
    if (temp > (float)(activeTip->maxTemp + 50))
        temp = (float)(activeTip->maxTemp + 50);

    return (int)(temp + 0.5f);   // round
}


// SAFETY
void handleSafety() {
  int limit = boostMode ? max(maxTemp, boostTemp) : maxTemp;
  overHeat = (currentTemp > limit + 20);
}


//====================================================================//
// READ TEMP
//====================================================================//
static int filteredTemp = -1;

int readTemp() {
    startTempRead();
    uint16_t rawAdc = getAverageADC();
    endTempRead();

    int t = adcToTemp(rawAdc);

    // EMA sederhana (alpha \~0.3)
    if (filteredTemp < 0) filteredTemp = t;
    else filteredTemp = (filteredTemp * 8 + t * 2) / 10; //80% 

    return filteredTemp;
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

    // ===== SLEEP = HEATER OFF TOTAL =====
    if (sleeping) {
        pwmOut      = 0;
        pidIntegral = 0;
        lastError   = 0;
        heaterState = STATE_SLEEP;
        heaterOff();
        return;
    }

    // ===== TARGET SELECT =====
    int activeTarget = targetTemp;
    if (boostMode) activeTarget = boostTemp;

    // ===== HEATER STATE =====
    if (currentTemp < (activeTarget - HEATER_HYSTERESIS)) {
        heaterState = STATE_HEAT;
    } else {
        heaterState = STATE_HOLD;
    }

    // ===== PID =====
    pidError      = activeTarget - currentTemp;

    pidIntegral   = constrain(pidIntegral + pidError, -PID_INTEGRAL_LIMIT, PID_INTEGRAL_LIMIT);
    pidDerivative = pidError - lastError;

    float activeKp = activeTip->kp;
    float activeKi = activeTip->ki;
    float activeKd = activeTip->kd;

    float output =
        (activeKp * pidError) +
        (activeKi * pidIntegral) +
        (activeKd * pidDerivative);

    lastError = pidError;

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
    // 1. DETEKSI TIDAK ADA TIP
    // ====================================================
    // Thermocouple open  → biasanya ADC sangat rendah  (≤ ADC_NO_TIP)
    // PTC open / short   → biasanya ADC sangat tinggi (≥ ADC_NO_TIP_PTC)
    // Nilai di config.h masih bisa di-tune setelah ukur nyata.
    // ====================================================
    if (sensorValue >= ADC_NO_TIP_PTC || sensorValue <= ADC_NO_TIP) {

        activeTip   = nullptr;
        detectedTip = TIP_AUTO;
        tipError    = true;

        pidIntegral = 0;
        lastError   = 0;

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

            if (currentTipMode == TIP_ITEM_T12 && tipDatabase[i].tipID != TIP_T12)
                continue;

            if (currentTipMode == TIP_ITEM_C210 && tipDatabase[i].tipID != TIP_C210)
                continue;

            if (sensorValue >= tipDatabase[i].minADC &&
                sensorValue <= tipDatabase[i].maxADC) {

                foundTip = &tipDatabase[i];
                break;
            }
        }
    }

    // ====================================================
    // 2. ERROR HANDLING
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
