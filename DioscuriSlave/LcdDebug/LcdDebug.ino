#include "config.h"
#include "Nokia105Debug.h"

Nokia105Debug lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

void setup() {
  Serial.begin(115200);
  delay(1000);

  lcd.initDisplay(true);
  lcd.setRotation(1);
  lcd.backgroundColor(BLACK);
}

void loop() {
  static int pwm = 0;
  static int dir = 5;
  static uint8_t phase = 0;

  lcd.backgroundColor(BLACK);

  char line1[24];
  char line2[24];

  snprintf(line1, sizeof(line1), "PWM %3d%%", pwm);

  if (phase == 0) snprintf(line2, sizeof(line2), "SOLDER  RUN");
  if (phase == 1) snprintf(line2, sizeof(line2), "HOT AIR RUN");
  if (phase == 2) snprintf(line2, sizeof(line2), "TEMP  %3d C", 350 + pwm / 2);
  if (phase == 3) snprintf(line2, sizeof(line2), "STATUS  OK");

  lcd.printString(line1, 8, 20, WHITE, BLACK);
  lcd.printString(line2, 8, 45, GREEN, BLACK);

  pwm += dir;
  if (pwm >= 100) { pwm = 100; dir = -5; }
  if (pwm <= 0)   { pwm = 0;   dir = 5; }

  phase = (phase + 1) & 3;
  delay(300);
}
