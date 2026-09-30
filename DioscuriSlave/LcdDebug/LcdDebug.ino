#include "config.h"
#include "Nokia105_LCD.h"

Nokia105 lcd(PIN_LCD_SDA, PIN_LCD_SCK, PIN_LCD_RESET, PIN_LCD_CS1);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== NOKIA LCD BUS DEBUG ===");
  Serial.println("RP2040: CS=GP2 SDA=GP1 SCK=GP0 RST=GP6");

  Serial.println("[1] initDisplay()");
  lcd.initDisplay();
  Serial.println("[1] DONE");

  Serial.println("[2] RED FRAME");
  lcd.backgroundColor(RED);
  Serial.println("[2] DONE");

  Serial.println("[3] WHITE FRAME");
  delay(1000);
  lcd.backgroundColor(WHITE);
  Serial.println("[3] DONE");

  Serial.println("[4] BLACK FRAME");
  delay(1000);
  lcd.backgroundColor(BLACK);
  Serial.println("[4] DONE");

  Serial.println("=== TEST COMPLETE ===");
}

void loop() {
  delay(1000);
}
