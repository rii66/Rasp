class HotGun {
public:
  enum Mode {
    MODE_OFF = 0,
    MODE_ON,
    MODE_FIXED,
    MODE_COOLING
  };

  void begin() {
    zc.begin();
    fan.begin();
    temp.begin();
    // storage.begin() HANYA di setup() — jangan double-init EEPROM

    // Load kalibrasi & setting dari flash
    uint16_t c0, c1, c2;
    storage.loadSiroccroCalibration(c0, c1, c2);
    calib.setTip(c0, c1, c2);

    uint16_t savedTemp;
    uint8_t savedFan;
    storage.loadSiroccroSettings(savedTemp, savedFan);

    float kp, ki, kd;
    storage.loadSiroccroPID(kp, ki, kd);
    pid.setTunings(kp, ki, kd);

    target_temp = savedTemp;
    fan_speed = savedFan;

    mode = MODE_OFF;
    fixed_power = 0;
    last_update = 0;

    pid.reset();
  }

  // ======================== Update (panggil di loop) ========================
  void update() {
    temp.update();
    temp.putHistory();

    if (!zc.isActive()) {
      // Keep the sampling history consistent even when line activity is low.
    }

    uint32_t now = millis();
    if (now - last_update < 100) return;
    last_update = now;

    float current_adc = temp.average();
    float current_c = calib.toCelsius(current_adc);
    float dt = 0.1f;

    switch (mode) {
      case MODE_ON:
        // Fan wajib saat heater
        if (!fan.isRunning()) {
          zc.setPower(0);
          break;
        }
        // Zero-cross / AC hilang
        if (!zc.hasAC()) {
          zc.setPower(0);
          break;
        }
        // Sensor ADC saturasi / short
        if (current_adc > 3800 || current_adc < 5) {
          mode = MODE_COOLING;
          zc.setPower(0);
          over_heat = true;
          break;
        }
        over_heat = false;
        {
          float output = pid.compute(target_temp, current_c, dt);
          zc.setPower((uint8_t)output);
        }
        break;

      case MODE_FIXED:
        if (fan.isRunning() && zc.hasAC()) zc.setPower(fixed_power);
        else zc.setPower(0);
        break;

      case MODE_COOLING:
        zc.setPower(0);
        fan.setSpeed(255);

        if (temp.isCold()) {
          mode = MODE_OFF;
          fan.off();
        }
        break;

      case MODE_OFF:
      default:
        zc.setPower(0);
        break;
    }
  }

  // ======================== API Kontrol ========================
  void setTemp(uint16_t celsius) {
    target_temp = constrain(celsius, TEMP_MIN_C, TEMP_MAX_C);
  }

  void setFan(uint8_t speed) {
    fan_speed = speed;
    if (mode == MODE_ON || mode == MODE_FIXED) {
      fan.setSpeed(fan_speed);
    }
  }

  void switchPower(bool on) {
    if (on) {
      if (mode == MODE_OFF || mode == MODE_COOLING) {
        mode = MODE_ON;
        fan.setSpeed(fan_speed);
        pid.reset();
      }
    } else {
      if (mode == MODE_ON || mode == MODE_FIXED) {
        mode = temp.isCold() ? MODE_OFF : MODE_COOLING;
      }
    }
  }

  void setFixedPower(uint8_t p) {
    fixed_power = constrain(p, 0, HEATER_MAX_POWER);
    mode = MODE_FIXED;
    fan.setSpeed(fan_speed);
  }

  // ======================== Kalibrasi ========================
  void setCalibration(uint16_t t0, uint16_t t1, uint16_t t2) {
    calib.setTip(t0, t1, t2);
    storage.saveSiroccroCalibration(t0, t1, t2);
  }

  void getCalibration(uint16_t out[3]) {
    calib.getTip(out);
  }

  // ======================== Simpan Setting ========================
  void saveCurrentSettings() {
    storage.saveSiroccroSettings(target_temp, fan_speed);
  }

  // ======================== Getter ========================
  uint16_t getTemp()        const { return calib.toCelsius(temp.average()); }  // dalam °C
  uint16_t getTempRaw()     const { return temp.average(); }                   // ADC mentah
  uint16_t getTargetTemp()  const { return target_temp; }
  uint8_t  getPower()       const { return zc.getPower(); }
  uint8_t  getFan()         const { return fan.getSpeed(); }
  bool     isOn()           const { return mode == MODE_ON || mode == MODE_FIXED; }
  bool     hasAC()          const { return zc.hasAC(); }
  Mode     getMode()        const { return mode; }
  bool     isCold()         const { return temp.isCold(); }

  // Untuk ISR
  void IRAM_ATTR onZeroCross() {
    zc.onZeroCross();
  }

private:
  ZeroCross       zc;
  Fan             fan;
  Temperature     temp;
  PIDController   pid;
  Calibration     calib;

  Mode            mode;
  uint16_t        target_temp;    // dalam Celsius
  uint8_t         fan_speed;
  uint8_t         fixed_power;
  uint32_t        last_update;
  bool            over_heat = false;
};
