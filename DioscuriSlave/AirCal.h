#ifndef AIRCAL_H
#define AIRCAL_H

#include <Arduino.h>
#include "config.h"

class Calibration {
public:
  Calibration() {
    // Default (×4 dari nilai AVR asli)
    tip[0] = 2348;
    tip[1] = 3004;
    tip[2] = 3400;
  }

  void setTip(uint16_t t0, uint16_t t1, uint16_t t2) {
    tip[0] = t0;
    tip[1] = t1;
    tip[2] = constrain(t2, 0, 4095);
  }

  void getTip(uint16_t out[3]) {
    out[0] = tip[0];
    out[1] = tip[1];
    out[2] = tip[2];
  }

  // ADC (0-4095) → Celsius
  uint16_t toCelsius(uint16_t adc) const {
    if (adc <= 200) return TEMP_AMBIENT_C;

    if (adc < tip[0]) {
      return map(adc, 200, tip[0], TEMP_AMBIENT_C, TEMP_TIP[0]);
    } 
    else if (adc <= tip[1]) {
      return map(adc, tip[0], tip[1], TEMP_TIP[0], TEMP_TIP[1]);
    } 
    else if (adc <= tip[2]) {
      return map(adc, tip[1], tip[2], TEMP_TIP[1], TEMP_TIP[2]);
    } 
    else {
      // Past the highest calibration point, keep the last segment slope stable.
      return map(adc, tip[1], tip[2], TEMP_TIP[1], TEMP_TIP[2]);
    }
  }

  // Celsius → ADC
  uint16_t toADC(uint16_t celsius) {
    celsius = constrain(celsius, TEMP_MIN_C, TEMP_MAX_C);

    uint16_t left = 0;
    uint16_t right = 4095;
    uint16_t mid = map(celsius, TEMP_TIP[0], TEMP_TIP[2], tip[0], tip[2]);

    for (uint8_t i = 0; i < 25; i++) {
      uint16_t h = toCelsius(mid);
      if (h == celsius) return mid;

      if (h < celsius) {
        left = mid;
        mid = (left + right) / 2;
        if (mid == left) mid++;
      } else {
        right = mid;
        mid = (left + right) / 2;
        if (mid == right) mid--;
      }
    }
    return mid;
  }

private:
  uint16_t tip[3];
};

#endif
