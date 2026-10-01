#include <Arduino.h>

#define LCD_SCK  0
#define LCD_SDA  1
#define LCD_CS   2
#define LCD_RST  6
#define LED_PIN  25

static void report(const char *name, int pin, bool level) {
  digitalWrite(pin, level ? HIGH : LOW);
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  Serial.print(name);
  Serial.print(" GP");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println(level ? "HIGH" : "LOW");
  delay(3000);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  pinMode(LCD_SCK, OUTPUT);
  pinMode(LCD_SDA, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RST, OUTPUT);

  digitalWrite(LCD_SCK, LOW);
  digitalWrite(LCD_SDA, LOW);
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RST, LOW);

  Serial.begin(115200);
  delay(2000);

  Serial.println("=== RP2040 LCD GPIO PATH TEST ===");
  Serial.println("Measure directly at LCD pins.");
  Serial.println("Each state remains for 3 seconds.");

  report("SCK", LCD_SCK, HIGH);
  report("SCK", LCD_SCK, LOW);

  report("SDA", LCD_SDA, HIGH);
  report("SDA", LCD_SDA, LOW);

  report("CS", LCD_CS, HIGH);
  report("CS", LCD_CS, LOW);

  report("RST", LCD_RST, HIGH);
  report("RST", LCD_RST, LOW);

  Serial.println("=== TEST COMPLETE ===");
}

void loop() {
  delay(1000);
  Serial.println("ALIVE");
}
