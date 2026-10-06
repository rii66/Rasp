#include <U8g2lib.h>
#include "ui_pages.h"
#include "pages.h"
#include "GlobalState.h"
#include "config.h"
#include "buzzer.h"
#include "tip.h"
#include "handler.h"   // airGet* jika diperlukan

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// ====================================================
// CURRENT TEMP (header tengah)
// ====================================================
void drawCurrentTemp(int x)
{
    char buf[8];
    sprintf(buf, "%dC", currentTemp);

    u8g2.setFont(u8g2_font_6x10_tf);
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr(x + (128 - w) / 2, 9, buf);
}

// ====================================================
// SET PAGE  Hapus switch station & tambah mode BOOTSEL
// ====================================================
void drawSetPage(int x)
{
    char buf[16];

    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 28, 38, "SET");

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    // ==============================
    // MODE KONFIRMASI BOOTSEL
    // ==============================
    if (confirmBootsel) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 28, 28, "BOOTSEL ?");

        // Yes
        if (item == 0) {
            u8g2.drawBox(x + 20, 42, 36, 12);
            u8g2.setDrawColor(0);
            u8g2.drawStr(x + 28, 51, "Yes");
            u8g2.setDrawColor(1);
        } else {
            u8g2.drawStr(x + 28, 51, "Yes");
        }

        // Exit
        if (item == 1) {
            u8g2.drawBox(x + 72, 42, 36, 12);
            u8g2.setDrawColor(0);
            u8g2.drawStr(x + 80, 51, "Exit");
            u8g2.setDrawColor(1);
        } else {
            u8g2.drawStr(x + 80, 51, "Exit");
        }
        return;
    }

    // ==============================
    // MODE NORMAL (SAVE / BOOTSEL / EXIT)
    // ==============================
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);

    // SAVE
    if (item == SET_SAVE) {
        u8g2.drawBox(x + 4, 2, 36, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 8, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 8, 9, "SAVE");
    }

    // BOOTSEL
    if (item == SET_BOOTSEL) {
        u8g2.drawBox(x + 46, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 50, 9, "BOOT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 50, 9, "BOOT");
    }

    // EXIT
    if (item == SET_EXIT) {
        u8g2.drawBox(x + 92, 2, 32, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 96, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 96, 9, "EXIT");
    }

    drawCurrentTemp(x);

    // Body info
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x + 16, 28, "SOLDER / HOTAIR");
    u8g2.drawStr(x + 16, 42, "via Encoder");

    // Footer
    u8g2.drawLine(x, 54, x + 127, 54);
    sprintf(buf, "MIN:%d", TEMP_MIN);
    u8g2.drawStr(x + 8, 63, buf);
    sprintf(buf, "MAX:%d", maxTemp);
    u8g2.drawStr(x + 74, 63, buf);
}

// ====================================================
// BOOST PAGE
// ====================================================
void drawBoostPage(int x)
{
    char buf[12];

    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 8, 38, "BosT");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (item == BOOST_SAVE) {
        u8g2.drawBox(x + 4, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 10, 9, "SAVE");
    }

    if (item == BOOST_EXIT) {
        u8g2.drawBox(x + 84, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 92, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 92, 9, "EXIT");
    }

    if (item == BOOST_TEMP) u8g2.drawStr(x + 4, 30, ">");
    u8g2.drawStr(x + 16, 30, "TEMP");
    sprintf(buf, "%dC", boostTemp);
    u8g2.drawStr(x + 88, 30, buf);

    if (item == BOOST_TIME) u8g2.drawStr(x + 4, 44, ">");
    u8g2.drawStr(x + 16, 44, "TIME");
    sprintf(buf, "%ds", boostTimeSec);
    u8g2.drawStr(x + 88, 44, buf);

    u8g2.drawLine(x, 63, x + 127, 63);
}

// ====================================================
// SLEEP PAGE
// ====================================================
void drawSleepPage(int x)
{
    char buf[12];

    if (!inEdit) {
        u8g2.setFont(u8g2_font_profont29_mr);
        u8g2.drawStr(x + 12, 36, "SLEEP");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (item == SLEEP_SAVE) {
        u8g2.drawBox(x + 4, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 10, 9, "SAVE");
    }

    if (item == SLEEP_EXIT) {
        u8g2.drawBox(x + 84, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 92, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 92, 9, "EXIT");
    }

    if (item == SLEEP_TEMP) u8g2.drawStr(x + 4, 30, ">");
    u8g2.drawStr(x + 16, 30, "TEMP");
    sprintf(buf, "%dC", sleepTemp);
    u8g2.drawStr(x + 88, 30, buf);

    if (item == SLEEP_TIME) u8g2.drawStr(x + 4, 44, ">");
    u8g2.drawStr(x + 16, 44, "TIME");
    sprintf(buf, "%ds", sleepTimeSec);
    u8g2.drawStr(x + 88, 44, buf);

    u8g2.drawLine(x, 63, x + 127, 63);
}

// ====================================================
// CAL PAGE
// ====================================================
void drawCalPage(int x)
{
    char buf[12];

    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 28, 40, "CAL");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (item == CAL_SAVE) {
        u8g2.drawBox(x + 4, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 10, 9, "SAVE");
    }

    if (item == CAL_EXIT) {
        u8g2.drawBox(x + 84, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 92, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 92, 9, "EXIT");
    }

    if (item == CAL_SOLDER) u8g2.drawStr(x + 4, 30, ">");
    u8g2.drawStr(x + 16, 30, "SOLDER");
    sprintf(buf, "%d", tempOffset);
    u8g2.drawStr(x + 88, 30, buf);

    if (item == CAL_HOTAIR) u8g2.drawStr(x + 4, 44, ">");
    u8g2.drawStr(x + 16, 44, "HOTAIR");
    u8g2.drawStr(x + 88, 44, "---");   // placeholder

    u8g2.drawLine(x, 54, x + 127, 54);
    u8g2.drawStr(x + 20, 63, "OFFSET C");
}

// ====================================================
// PID PAGE
// ====================================================
void drawPIDPage(int x)
{
    char buf[16];

    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 28, 40, "PID");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 34, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (!isEditingValue) {
        if (item == PID_SAVE) {
            u8g2.drawBox(x + 4, 2, 40, 8);
            u8g2.setDrawColor(0);
            u8g2.drawStr(x + 10, 9, "SAVE");
            u8g2.setDrawColor(1);
        } else {
            u8g2.drawStr(x + 10, 9, "SAVE");
        }

        if (item == PID_EXIT) {
            u8g2.drawBox(x + 84, 2, 40, 8);
            u8g2.setDrawColor(0);
            u8g2.drawStr(x + 92, 9, "EXIT");
            u8g2.setDrawColor(1);
        } else {
            u8g2.drawStr(x + 92, 9, "EXIT");
        }

        if (item == PID_KI) u8g2.drawStr(x + 10, 28, ">");
        u8g2.drawStr(x + 22, 28, "KI ────");

        if (item == PID_KP) u8g2.drawStr(x + 10, 42, ">");
        u8g2.drawStr(x + 22, 42, "KP ────");

        if (item == PID_KD) u8g2.drawStr(x + 10, 56, ">");
        u8g2.drawStr(x + 22, 56, "KD ────");
    } else {
        float val = 0;
        const char* label = "";

        if (item == PID_KP) { val = kp; label = "KP"; }
        else if (item == PID_KI) { val = ki; label = "KI"; }
        else if (item == PID_KD) { val = kd; label = "KD"; }

        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.drawStr(x + 92, 9, "EXIT");

        u8g2.setFont(u8g2_font_fub14_tf);
        u8g2.drawStr(x + 48, 28, label);

        u8g2.drawFrame(x + 18, 36, 92, 8);

        int slider = map((int)(val * 100), 0, 99900, 0, 88);
        if (slider < 0) slider = 0;
        if (slider > 88) slider = 88;
        u8g2.drawDisc(x + 20 + slider, 40, 3);

        u8g2.setFont(u8g2_font_6x12_tf);
        sprintf(buf, "%.2f", val);
        u8g2.drawStr(x + 42, 58, buf);
    }
}

// ====================================================
// TIP PAGE
// ====================================================
void drawTipPage(int x)
{
    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 24, 40, "TIP");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (item == TIP_ITEM_SAVE) {
        u8g2.drawBox(x + 4, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 10, 9, "SAVE");
    }

    if (item == TIP_ITEM_EXIT) {
        u8g2.drawBox(x + 84, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 92, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 92, 9, "EXIT");
    }

    if (item == TIP_ITEM_T12)  u8g2.drawStr(x + 4, 30, ">");
    u8g2.drawStr(x + 16, 30, "T12");

    if (item == TIP_ITEM_C210) u8g2.drawStr(x + 52, 30, ">");
    u8g2.drawStr(x + 64, 30, "C210");

    if (item == TIP_ITEM_AUTO) u8g2.drawStr(x + 4, 44, ">");
    u8g2.drawStr(x + 16, 44, "AUTO");

    if (item == TIP_ITEM_CUSTOM) u8g2.drawStr(x + 52, 44, ">");
    u8g2.drawStr(x + 64, 44, "CUSTOM");

    u8g2.drawLine(x, 54, x + 127, 54);
    u8g2.drawStr(x + 2, 61, "MODE:");

    if (currentTipMode == TIP_ITEM_AUTO)        u8g2.drawStr(x + 34, 61, "AUTO");
    else if (currentTipMode == TIP_ITEM_T12)    u8g2.drawStr(x + 34, 61, "T12");
    else if (currentTipMode == TIP_ITEM_C210)   u8g2.drawStr(x + 34, 61, "C210");
    else if (currentTipMode == TIP_ITEM_CUSTOM) u8g2.drawStr(x + 34, 61, "CUSTOM");
}

// ====================================================
// BUZZER PAGE
// ====================================================
void drawBuzzerPage(int x)
{
    if (!inEdit) {
        u8g2.setFont(u8g2_font_fub30_tf);
        u8g2.drawStr(x + 8, 32, "BuzeR");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x + 30, 60, "Click Enter");
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawLine(x, 12, x + 127, 12);
    drawCurrentTemp(x);

    if (item == BUZ_SAVE) {
        u8g2.drawBox(x + 4, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 10, 9, "SAVE");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 10, 9, "SAVE");
    }

    if (item == BUZ_EXIT) {
        u8g2.drawBox(x + 84, 2, 40, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 92, 9, "EXIT");
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(x + 92, 9, "EXIT");
    }

    if (item == BUZ_ON)  u8g2.drawStr(x + 10, 28, ">");
    u8g2.drawStr(x + 24, 28, "ON");

    if (item == BUZ_OFF) u8g2.drawStr(x + 10, 42, ">");
    u8g2.drawStr(x + 24, 42, "OFF");

    u8g2.drawLine(x, 54, x + 127, 54);

    if (buzzerEnabled)
        u8g2.drawStr(x + 36, 63, "STATE: ON");
    else
        u8g2.drawStr(x + 34, 63, "STATE: OFF");
}
