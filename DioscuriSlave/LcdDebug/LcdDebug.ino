#include <Arduino.h>

#define LCD_SCK  0
#define LCD_SDA  1
#define LCD_CS   2
#define LCD_RST  6
#define LED_PIN  25

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LCD_SCK, OUTPUT);
  pinMode(LCD_SDA, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RST, OUTPUT);

  digitalWrite(LCD_SCK, HIGH);
  digitalWrite(LCD_SDA, HIGH);
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RST, HIGH);

  Serial.begin(115200);
  delay(2000);

  Serial.println("=== LCD GPIO ALL HIGH ===");
  Serial.println("GP0 SCK = HIGH");
  Serial.println("GP1 SDA = HIGH");
  Serial.println("GP2 CS  = HIGH");
  Serial.println("GP6 RST = HIGH");
  Serial.println("Measure LCD pins: each should be about 3.3V.");
}

void loop() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(1000);
}
