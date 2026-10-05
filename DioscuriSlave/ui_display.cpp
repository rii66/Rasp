#include <Wire.h>
#include <U8g2lib.h>
#include "ui_display.h"
#include "ui_pages.h"
#include "pages.h"
#include "GlobalState.h"
#include "config.h"
#include "oled_dashboard.h"
// U8g2 instance (HW I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static int animX   = 0;
static int targetX = 0;

void initDisplay()
{
    Wire.setSDA(PIN_OLED_SDA);
    Wire.setSCL(PIN_OLED_SCL);
    Wire.begin();
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void drawUI()
{
    u8g2.clearBuffer();

    if (inMenu) {
        targetX = page * 128;

        int diff = targetX - animX;
        if (abs(diff) > 1)
            animX += diff / 3;
        else
            animX = targetX;

        drawSetPage   (  0 - animX);
        drawBoostPage (128 - animX);
        drawSleepPage (256 - animX);
        drawCalPage   (384 - animX);
        drawPIDPage   (512 - animX);
        drawTipPage   (640 - animX);
        drawBuzzerPage(768 - animX);
    }
    else 
        drawOledDashboard();
    

    u8g2.sendBuffer();  
}
