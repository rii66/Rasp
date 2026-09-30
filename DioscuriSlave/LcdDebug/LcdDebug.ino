#include "../config.h"
#include "../Nokia105_LCD.h"

Nokia105 lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== NOKIA LCD DEBUG ===");

  lcd.initDisplay();
  Serial.println("LCD INIT OK");

  lcd.backgroundColor(RED);
  Serial.println("RED OK");
}

void loop() {}
