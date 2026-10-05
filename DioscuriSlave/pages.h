#ifndef PAGES_H
#define PAGES_H

#include <Arduino.h>

enum Pages {
    PAGE_SET,
    PAGE_BOOST,
    PAGE_SLEEP,
    PAGE_CAL,
    PAGE_PID,
    PAGE_TIP,
    PAGE_BUZZER,
    PAGE_TOTAL
};

// Settings page — hanya SAVE / EXIT (STATION & TEMP dihapus)
enum {
    SET_SAVE,
    SET_EXIT,
    SET_COUNT
};

// PID
enum {
    PID_KI,
    PID_KP,
    PID_KD,
    PID_SAVE,
    PID_EXIT,
    PID_COUNT
};

// Boost
enum {
    BOOST_TEMP,
    BOOST_TIME,
    BOOST_SAVE,
    BOOST_EXIT,
    BOOST_COUNT
};

// Sleep
enum {
    SLEEP_TEMP,
    SLEEP_TIME,
    SLEEP_SAVE,
    SLEEP_EXIT,
    SLEEP_COUNT
};

// Calibration
enum {
    CAL_SOLDER,
    CAL_HOTAIR,
    CAL_SAVE,
    CAL_EXIT,
    CAL_COUNT
};

// Tip
enum {
    TIP_ITEM_T12,
    TIP_ITEM_C210,
    TIP_ITEM_AUTO,
    TIP_ITEM_CUSTOM,
    TIP_ITEM_SAVE,
    TIP_ITEM_EXIT,
    TIP_COUNT
};

// Buzzer
enum {
    BUZ_ON,
    BUZ_OFF,
    BUZ_SAVE,
    BUZ_EXIT,
    BUZ_COUNT
};

extern bool inMenu;
extern bool inEdit;

#endif
