#include <Wire.h>
#include <U8g2lib.h>
#include "oled_dashboard.h"
#include "GlobalState.h"
#include "config.h"
#include "handler.h"
#include "pages.h"       // ceklist in menu

// (dideklarasikan di ui_display.cpp)
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

static void drawOledDashboard();
{
    char buf[16];

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "UART SLAVE");
    u8g2.drawStr(74, 10, "ACTIVE");

    if (activeStation == STATION_MODE_SOLDER)
        u8g2.drawStr(110, 10, "S");
    else
        u8g2.drawStr(110, 10, "H");

    // ===== SOLDER =====
    u8g2.drawFrame(1, 14, 62, 49);
    if (activeStation == STATION_MODE_SOLDER)
        u8g2.drawFrame(2, 15, 60, 47);

    u8g2.drawStr(6, 24, "SOLDER");

    u8g2.setFont(u8g2_font_logisoso16_tf);
    sprintf(buf, "%d", currentTemp);
    u8g2.drawStr(6, 44, buf);

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(42, 42, "C");

    if (tipError)       u8g2.drawStr(6, 56, "NO TIP");
    else if (sleeping)  u8g2.drawStr(6, 56, "SLEEP");
    else if (boostMode) u8g2.drawStr(6, 56, "BOOST");
    else                u8g2.drawStr(6, 56, pwmOut ? "ON" : "OFF");

    sprintf(buf, "%dC", targetTemp);
    u8g2.drawStr(36, 56, buf);

    // ===== HOT AIR =====
    u8g2.drawFrame(65, 14, 62, 49);
    if (activeStation == STATION_MODE_HOTAIR)
        u8g2.drawFrame(66, 15, 60, 47);

    u8g2.drawStr(70, 24, "HOT AIR");

    u8g2.setFont(u8g2_font_logisoso16_tf);
    sprintf(buf, "%d", (int)airGetTemp());
    u8g2.drawStr(70, 44, buf);

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(106, 42, "C");

    const char* mode = airGetModeStr();
    if (mode) u8g2.drawStr(70, 56, mode);

    sprintf(buf, "%dC", (int)airGetTargetTemp());
    u8g2.drawStr(100, 56, buf);
}

void initOledDashboard()
{
    // Init sudah dilakukan di initDisplay() (ui_display.cpp)
}

void updateOledDashboard()
{
    // (drawUI di ui_display.cpp yang mengatur clearBuffer + sendBuffer)
}
