#include "config.h"
#include "Nokia105Debug.h"

Nokia105Debug lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

void setup() {
  Serial.begin(115200);
  delay(1000);

  lcd.initDisplay(true);
  lcd.setRotation(0);

  lcd.backgroundColor(RED);
  delay(2000);

  lcd.backgroundColor(GREEN);
  delay(2000);

  lcd.backgroundColor(BLUE);
  delay(2000);

  lcd.backgroundColor(WHITE);
  delay(2000);

  lcd.backgroundColor(BLACK);
}

void loop() {
}
