#include "handler.h"
#include "menuHandlr.h"
#include "GlobalState.h"
#include "encoder.h"
#include "pages.h"

#include "boost.h"
#include "pid.h"
#include "tip.h"

#include "storage.h"
#include "motion.h"
#include "buzzer.h"

// API Hot Air (handler.cpp)
extern void airSetTemp(uint16_t celsius);
extern void airSwitchPower(bool on);
extern uint16_t airGetTargetTemp();
extern bool airIsOn();


  //====================================//
 //             .MENU HANDLER               //
//====================================//

// ===== Cursor tiap halaman =====
int mainMenuIndex  = 0;
int boostMenuIndex = 0;
int sleepMenuIndex = 0;
int pidMenuIndex   = 0;
int tipMenuIndex   = 0;
int buzMenuIndex   = 0;
int calMenuIndex   = 0;

// Cursor khusus submenu Station (Solder / Hot Air)
static int stationItem = STATION_SOLDER;
static bool stationMenu = false;

// =========================================================
// SIMPAN POSISI CURSOR SUBMENU
// =========================================================
void saveMenuCursor() {
  switch (page) {
    case PAGE_BOOST:  boostMenuIndex = item; break;
    case PAGE_SLEEP:  sleepMenuIndex = item; break;
    case PAGE_CAL:    calMenuIndex   = item; break;
    case PAGE_PID:    pidMenuIndex   = item; break;
    case PAGE_TIP:    tipMenuIndex   = item; break;
    case PAGE_BUZZER: buzMenuIndex   = item; break;
  }
}

// =========================================================
// EKSEKUSI PILIHAN MENU
// =========================================================
void executePageSelect() {

  saveMenuCursor();

  switch (page) {

    // ===== SETTING =====
    case PAGE_SET:
      if (item == SET_SAVE || item == SET_EXIT) {
        //saveSettings();
        beepSave();
        inEdit = false;
      }
      else if (item == SET_STATION) {
        // Masuk submenu Station
        stationItem = (activeStation == STATION_HOTAIR)
                      ? STATION_HOTAIR
                      : STATION_SOLDER;
        stationMenu = true;
      }
      else if (item == SET_TEMP) {
        isEditingValue = true;
      }
      // item lain (BOOST/SLEEP/...) — biarkan navigasi page via swipe
      break;

    // ===== BOOST =====
    case PAGE_BOOST:
      if (item == BOOST_SAVE || item == BOOST_EXIT) {
      //  saveBoost();
        beepSave();
        inEdit = false;
      } else {
        isEditingValue = true;
      }
      break;

    // ===== SLEEP =====
    case PAGE_SLEEP:
      if (item == SLEEP_SAVE || item == SLEEP_EXIT) {
      //  saveSleep();
        beepSave();
        inEdit = false;
      } else {
        isEditingValue = true;
      }
      break;

    // ===== CALIBRATION =====
    case PAGE_CAL:
      if (item == CAL_SAVE || item == CAL_EXIT) {
      //  saveCal();
        beepSave();
        inEdit = false;
      }
      else if (item == CAL_SOLDER || item == CAL_HOTAIR) {
        isEditingValue = true;
      }
      break;

    // ===== PID =====
    case PAGE_PID:
      if (item == PID_SAVE || item == PID_EXIT) {
      //  saveActivePID();
        beepSave();
        inEdit = false;
      }
      else if (item == PID_KP || item == PID_KI || item == PID_KD) {
        isEditingValue = true;
      }
      break;

    // ===== TIP =====
    case PAGE_TIP:
      if (item == TIP_ITEM_SAVE) {
        //saveTip();
        beepSave();
        inEdit = false;
      }
      else if (item == TIP_ITEM_EXIT) {
        inEdit = false;
      }
      else if (item == TIP_ITEM_T12) {
        setTipProfile(TIP_ITEM_T12);
      }
      else if (item == TIP_ITEM_C210) {
        setTipProfile(TIP_ITEM_C210);
      }
      else if (item == TIP_ITEM_AUTO) {
        currentTipMode = TIP_ITEM_AUTO;
        detectTip();
      }
      else if (item == TIP_ITEM_CUSTOM) {
        setTipProfile(TIP_ITEM_CUSTOM);
      }
      break;

    // ===== BUZZER =====
    case PAGE_BUZZER:
      if (item == BUZ_SAVE || item == BUZ_EXIT) {
     //   saveBuzzer();
        beepSave();
        inEdit = false;
      }
      else if (item == BUZ_ON) {
        buzzerEnabled = true;
      }
      else if (item == BUZ_OFF) {
        buzzerEnabled = false;
      }
      break;
  }
}

// =========================================================
// HANDLE MENU (LOOP)
// =========================================================
void handleMenu(int direction, bool pressed) {

  static bool lastBtn = false;

  // =====================================================
  // BUTTON LOGIC
  // =====================================================
  if (pressed && !lastBtn) {
    wakeFromSleep();
    btnPressStart = millis();
    btnHolding = true;
  }

  if (!pressed && lastBtn) {

    unsigned long holdTime = millis() - btnPressStart;

    if (holdTime > 50) {

      // ===== LONG PRESS =====
      if (holdTime >= 2100) {
        inMenu = !inMenu;
        beepLong();
        inEdit = false;
        isEditingValue = false;
        stationMenu = false;

        if (inMenu) {
          page = PAGE_SET;
          item = 0;
          stationMenu = false;
        }
      }

      // ===== SHORT PRESS =====
      else {
        if (!inMenu) {
          // Dashboard: Solder → Boost, HotAir → Power toggle
          if (activeStation == STATION_HOTAIR) {
            airSwitchPower(!airIsOn());
            beepSelect();
          } else {
            startBoost();
          }
        }
        else {
          // ----- Submenu Station -----
          if (stationMenu) {
            if (stationItem == STATION_SOLDER) {
              activeStation = STATION_MODE_SOLDER;
              beepSelect();
            }
            else if (stationItem == STATION_HOTAIR) {
              activeStation = STATION_MODE_HOTAIR;
              beepSelect();
            }
            else if (stationItem == STATION_SAVE) {
          //    saveSettings();
              beepSave();
              stationMenu = false;
            }
            else if (stationItem == STATION_EXIT) {
              stationMenu = false;
              beepSelect();
            }
          }
          else if (!inEdit) {
            beepSelect();
            inEdit = true;
            item = 0;
            stationMenu = false;
          }
          else if (!isEditingValue) {
            executePageSelect();
          }
          else {
            isEditingValue = false;
          }
        }
      }
    }

    btnHolding = false;
  }

  lastBtn = pressed;

  // =====================================================
  // ENCODER LOGIC
  // =====================================================
  if (direction != 0) {
    wakeFromSleep();
  }

  // =======================================
  //  SUBMENU STATION
  // =======================================
  if (stationMenu) {
    if (direction != 0) {
      stationItem += direction;

      if (stationItem < 0)
        stationItem = STATION_COUNT - 1;
      if (stationItem >= STATION_COUNT)
        stationItem = 0;

      beepMove();
    }
    return;
  }

  if (direction == 0) return;

  // =======================================
  //  DASHBOARD (bukan menu)
  // =======================================
  if (!inMenu) {
    // OLED dashboard: encoder hanya memilih station aktif.
    activeStation = (activeStation == STATION_MODE_SOLDER)
                    ? STATION_MODE_HOTAIR
                    : STATION_MODE_SOLDER;
    beepMove();
    return;
  }

  // ===== LEVEL 1 : GANTI PAGE =====
  if (!inEdit) {
    page += direction;

    if (page >= PAGE_TOTAL) page = 0;
    if (page < 0) page = PAGE_TOTAL - 1;
    beepMove();
  }

  // ===== LEVEL 2 : PINDAH CURSOR =====
  else if (!isEditingValue) {

    item += direction;

    int maxItems = 1;

    switch (page) {
      case PAGE_SET:    maxItems = SET_COUNT;   break;
      case PAGE_BOOST:  maxItems = BOOST_COUNT; break;
      case PAGE_SLEEP:  maxItems = SLEEP_COUNT; break;
      case PAGE_CAL:    maxItems = CAL_COUNT;   break;
      case PAGE_PID:    maxItems = PID_COUNT;   break;
      case PAGE_TIP:    maxItems = TIP_COUNT;   break;
      case PAGE_BUZZER: maxItems = BUZ_COUNT;   break;
    }

    if (item >= maxItems) item = 0;
    if (item < 0) item = maxItems - 1;
  }

  // ===== LEVEL 3 : EDIT VALUE =====
  else {
    switch (page) {

      case PAGE_SET:
        if (item == SET_TEMP) {
          if (activeStation == STATION_MODE_HOTAIR) {
            int t = airGetTargetTemp() + (direction * 5);
            t = constrain(t, TEMP_MIN, TEMP_MAX_HOTAIR);
            airSetTemp((uint16_t)t);
          } else {
            targetTemp += (direction * 5);
            targetTemp = constrain(targetTemp, TEMP_MIN, maxTemp);
          }
        }
        break;

      case PAGE_BOOST:
        if (item == BOOST_TEMP) {
          boostTemp += (direction * 5);
        // boostTemp = constrain(boostTemp, TEMP_MIN, maxTemp);  // Old
        // New (izinkan di atas maxTemp )
        boostTemp = constrain(boostTemp, TEMP_MIN, TEMP_MAX_CUSTOM);  // 600
        }
        else if (item == BOOST_TIME) {
          boostTimeSec += direction;
          boostTimeSec = constrain(boostTimeSec, 10, 300);
        }
        break;

      case PAGE_SLEEP:
        if (item == SLEEP_TEMP) {
          sleepTemp += (direction * 5);
          sleepTemp = constrain(sleepTemp, TEMP_MIN, 250);
        }
        else if (item == SLEEP_TIME) {
          sleepTimeSec += (direction * 10);
          sleepTimeSec = constrain(sleepTimeSec, 10, 999);
        }
        break;

      case PAGE_PID:
        if (item == PID_KP) {
          kp = constrain(kp + (direction * 0.1), 0.0, 999.0);
        }
        else if (item == PID_KI) {
          ki = constrain(ki + (direction * 0.01), 0.0, 999.0);
        }
        else if (item == PID_KD) {
          kd = constrain(kd + (direction * 1.0), 0.0, 999.0);
        }
        break;

      case PAGE_CAL:
        // CAL_SOLDER → tempOffset solder, CAL_HOTAIR bisa offset HA terpisah
        if (item == CAL_SOLDER) {
          tempOffset += direction;
          tempOffset = constrain(tempOffset, -50, 50);
        }
        // CAL_HOTAIR: sesuaikan variabel offset hotair jika sudah ada
        break;
    }
  }
}

// Getter supaya UI bisa baca cursor station
int getStationItem() { return stationItem; }
bool isStationMenu() { return stationMenu; }
