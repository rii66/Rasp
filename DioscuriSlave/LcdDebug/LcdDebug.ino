#include "config.h"
#include "Nokia105Debug.h"

Nokia105Debug lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== NOKIA SINGLE LCD TEST ===");

  lcd.initDisplay();
  lcd.setRotation(1);
  lcd.backgroundColor(BLACK);
  Serial.println("LCD READY");
}

void loop() {
  static uint8_t step = 0;
  if (step == 0) {
    lcd.backgroundColor(RED);
    lcd.printString("RED  128x160", 8, 20, WHITE, RED);
    Serial.println("RED");
  } else if (step == 1) {
    lcd.backgroundColor(GREEN);
    lcd.printString("GREEN", 8, 20, BLACK, GREEN);
    Serial.println("GREEN");
  } else if (step == 2) {
    lcd.backgroundColor(BLUE);
    lcd.printString("BLUE", 8, 20, WHITE, BLUE);
    Serial.println("BLUE");
  } else {
    lcd.backgroundColor(BLACK);
    lcd.printString("BLACK TEST", 8, 20, WHITE, BLACK);
    Serial.println("BLACK");
  }
  step = (step + 1) & 3;
  delay(1000);
}
