#include "config.h"
#include "Nokia105Debug.h"
#include <string.h>

Nokia105Debug lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

static bool ready = false;
static unsigned long lastDraw = 0;
static int lastPwm = -1;
static int lastTemp = -1;
static char lastStatus[16] = "";

static int pwmOut = 0;
static int currentTemp = 25;
static bool tipError = false;
static bool sleeping = false;

void drawField(Nokia105Debug& lcd, int x, int y, int w, int h,
               const char* text, uint16_t fg, uint16_t bg) {
  lcd.fillRectangle(x, y, w, h, bg);
  lcd.printString(text, x, y, fg, bg);
}

void initLcdTemps() {
  lcd.initDisplay(true);
  lcd.setRotation(1);
  lcd.backgroundColor(BLACK);
  lcd.printString("SOLDER RUN", 4, 4, CYAN, BLACK);
  lcd.printString("HOT AIR RUN", 4, 22, MAGENTA, BLACK);
  ready = true;
  lastDraw = 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  initLcdTemps();
}

void loop() {
  if (!ready) return;
  if (millis() - lastDraw < 300) return;
  lastDraw = millis();

  static int dir = 5;
  pwmOut += dir;
  if (pwmOut >= 255) { pwmOut = 255; dir = -5; }
  if (pwmOut <= 0)   { pwmOut = 0;   dir = 5; }

  currentTemp = 350 + (pwmOut * 100) / 255;

  char buf[24];
  int pwmPct = (pwmOut * 100) / 255;

  if (pwmPct != lastPwm) {
    lastPwm = pwmPct;
    snprintf(buf, sizeof(buf), "PWM %3d%%", pwmPct);
    drawField(lcd, 4, 70, 120, 16, buf, CYAN, BLACK);
  }

  if (currentTemp != lastTemp) {
    lastTemp = currentTemp;
    snprintf(buf, sizeof(buf), "TEMP %3d C", currentTemp);
    drawField(lcd, 4, 40, 140, 16, buf, WHITE, BLACK);
  }

  static uint8_t phase = 0;
  phase++;
  const char* st = (phase % 30 < 10) ? "STATUS OK" :
                   (phase % 30 < 20) ? "SLEEP" : "NO TIP";
  if (strcmp(st, lastStatus) != 0) {
    lastStatus[sizeof(lastStatus) - 1] = '\0';
    strncpy(lastStatus, st, sizeof(lastStatus) - 1);
    drawField(lcd, 4, 100, 140, 16, st,
              strcmp(st, "NO TIP") == 0 ? RED : (strcmp(st, "SLEEP") == 0 ? YELLOW : GREEN),
              BLACK);
  }
}
