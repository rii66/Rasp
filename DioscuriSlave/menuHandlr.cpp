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

//====================================//
//             MENU HANDLER            //
//====================================//

// Cursor tiap halaman (untuk restore posisi)
int boostMenuIndex = 0;
int sleepMenuIndex = 0;
int pidMenuIndex   = 0;
int tipMenuIndex   = 0;
int buzMenuIndex   = 0;
int calMenuIndex   = 0;

// =========================================================
// SIMPAN POSISI CURSOR
// =========================================================
void saveMenuCursor() {
  switch (page) {
    case PAGE_BOOST:  boostMenuIndex = item; break;
    case PAGE_SLEEP:  sleepMenuIndex = item; break;
    case PAGE_CAL:    calMenuIndex   = item; break;
    case PAGE_PID:    pidMenuIndex   = item; break;
    case PAGE_TIP:    tipMenuIndex   = item; break;
    case PAGE_BUZZER: buzMenuIndex   = item; break;
    default: break;
  }
}

// =========================================================
// EKSEKUSI PILIHAN MENU
// =========================================================
void executePageSelect() {

  saveMenuCursor();

  switch (page) {

    // ===== SET =====
case PAGE_SET:
  if (item == SET_SAVE) {
    saveSettings();
    storageFlush();
    beepSave();
    inEdit = false;
    confirmBootsel = false;
  }
  else if (item == SET_BOOTSEL) {
    if (!confirmBootsel) {
      // Masuk mode konfirmasi
      confirmBootsel = true;
      item = 0;               // default pilih "Yes"
      beepSelect();
    } else {
      // Sudah di mode konfirmasi
      if (item == 0) {
        // Yes → masuk BOOTSEL
        beepLong();
        delay(400);
        rp2040.rebootToBootloader();
      } else {
        // Exit → kembali ke menu SET
        confirmBootsel = false;
        item = SET_BOOTSEL;
        beepSelect();
      }
    }
  }
  else if (item == SET_EXIT) {
    confirmBootsel = false;
    inEdit = false;
  }
  break;

    // ===== BOOST =====
    case PAGE_BOOST:
      if (item == BOOST_SAVE || item == BOOST_EXIT) {
        saveBoost();
        storageFlush();
        beepSave();
        inEdit = false;
      } else {
        isEditingValue = true;
      }
      break;

    // ===== SLEEP =====
    case PAGE_SLEEP:
      if (item == SLEEP_SAVE || item == SLEEP_EXIT) {
        saveSleep();
        storageFlush();
        beepSave();
        inEdit = false;
      } else {
        isEditingValue = true;
      }
      break;

    // ===== CALIBRATION =====
    case PAGE_CAL:
      if (item == CAL_SAVE || item == CAL_EXIT) {
        saveCal();
        storageFlush();
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
        saveActivePID();
        storageFlush();
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
        saveTip();
        storageFlush();
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
        saveBuzzer();
        storageFlush();
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
  static bool longPressDone = false;

  // =====================================================
  // BUTTON PRESS START
  // =====================================================
  if (pressed && !lastBtn) {
    wakeFromSleep();
    btnPressStart = millis();
    btnHolding = true;
    longPressDone = false;
  }

  // =====================================================
  // LONG PRESS: 3 detik = toggle menu (tetap seperti Dioscuri)
  // =====================================================
  if (pressed && btnHolding && !longPressDone) {
    const unsigned long heldTime = millis() - btnPressStart;

    if (heldTime >= 3000) {
      longPressDone = true;

      inMenu = !inMenu;
      beepLong();

      inEdit = false;
      isEditingValue = false;

      if (inMenu) {
        page = PAGE_SET;
        item = 0;
      }
    }
  }

  // =====================================================
  // BUTTON RELEASE
  // =====================================================
  if (!pressed && lastBtn) {
    const unsigned long holdTime = millis() - btnPressStart;

    // Long press sudah dieksekusi saat hold → abaikan release
    if (holdTime > 50 && !longPressDone) {

      // ===== SHORT PRESS =====
      if (!inMenu) {
        // Dashboard: SW1 = Boost solder
        activeStation = STATION_MODE_SOLDER;
        startBoost();
      } else {

        if (!inEdit) {
          // Masuk mode edit halaman saat ini
          beepSelect();
          inEdit = true;
          item = 0;
        }
        else if (!isEditingValue) {
          // Eksekusi item yang dipilih
          executePageSelect();
        }
        else {
          // Selesai edit value
          isEditingValue = false;
        }
      }
    }

    btnHolding = false;
    longPressDone = false;
  }

  lastBtn = pressed;

  // =====================================================
  // ENCODER LOGIC
  // =====================================================
  if (direction != 0) {
    wakeFromSleep();
  }

  if (direction == 0) return;

  // =======================================
  // DASHBOARD: EC1 langsung kontrol suhu solder
  // =======================================
  if (!inMenu) {
    activeStation = STATION_MODE_SOLDER;
    targetTemp += (direction * 5);
    targetTemp = constrain(targetTemp, TEMP_MIN, maxTemp);
    beepMove();
    return;
  }

  // ===== LEVEL 1 : PAGE (side-scroll) =====
  if (!inEdit) {
    page += direction;

    if (page >= PAGE_TOTAL) page = 0;
    if (page < 0) page = PAGE_TOTAL - 1;
    beepMove();
  }

  // ===== LEVEL 2 : ITEM =====
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
    beepMove();
  }

  // ===== LEVEL 3 : VALUE =====
  else {
    switch (page) {

      case PAGE_BOOST:
        if (item == BOOST_TEMP) {
          boostTemp += (direction * 5);
          boostTemp = constrain(boostTemp, TEMP_MIN, TEMP_MAX_CUSTOM);
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
          kp = constrain(kp + (direction * 0.1f), 0.0f, 999.0f);
        }
        else if (item == PID_KI) {
          ki = constrain(ki + (direction * 0.01f), 0.0f, 999.0f);
        }
        else if (item == PID_KD) {
          kd = constrain(kd + (direction * 1.0f), 0.0f, 999.0f);
        }
        break;

      case PAGE_CAL:
        if (item == CAL_SOLDER) {
          tempOffset += direction;
          tempOffset = constrain(tempOffset, -50, 50);
        }
        // CAL_HOTAIR masih placeholder (belum ada offset hotair)
        break;
    }
  }
}
