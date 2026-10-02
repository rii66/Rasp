/*
  RP2040 GPIO ALL-PIN TEST
  Tests internal PULLDOWN and PULLUP on every RP2040 GPIO 0..29.
  No external resistor required.

  Output:
    PD GPIOx = 0  -> expected
    PU GPIOx = 1  -> expected

  GPIO23/24/25/29 are Pico-board/internal-use pins:
    GP23 = SMPS PS
    GP24 = VBUS sense
    GP25 = onboard LED
    GP29 = VSYS/ADC3 sense
  They are included in the scan but should not be externally forced.

  Open Serial Monitor at 115200.
*/

#include <Arduino.h>

static const uint8_t TEST_PINS[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29
};

static const size_t PIN_COUNT = sizeof(TEST_PINS) / sizeof(TEST_PINS[0]);

void readAll(const char* mode, uint8_t pinModeValue, int expected) {
  Serial.println();
  Serial.println(mode);
  Serial.println("GPIO\tREAD\tSTATUS");

  for (size_t i = 0; i < PIN_COUNT; ++i) {
    uint8_t pin = TEST_PINS[i];

    pinMode(pin, pinModeValue);
    delay(2);

    int v = digitalRead(pin);

    Serial.print("GP");
    Serial.print(pin);
    Serial.print("\t");
    Serial.print(v);
    Serial.print("\t");
    Serial.println(v == expected ? "OK" : "NOISY/UNEXPECTED");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("================================");
  Serial.println(" RP2040 ALL GPIO INPUT TEST");
  Serial.println(" Internal pull-up / pull-down");
  Serial.println(" No external resistors needed");
  Serial.println("================================");

  readAll("=== INPUT_PULLDOWN: expected 0 ===",
           INPUT_PULLDOWN, 0);

  delay(500);

  readAll("=== INPUT_PULLUP: expected 1 ===",
           INPUT_PULLUP, 1);

  Serial.println();
  Serial.println("=== IMPORTANT ===");
  Serial.println("GP23/24/25/29 are Pico board/internal-use pins.");
  Serial.println("Do NOT connect external signals to those pins.");
  Serial.println("GP26-28 are also ADC-capable.");
  Serial.println();
  Serial.println("Test finished. Repeating every 2 seconds...");
}

void loop() {
  readAll("=== INPUT_PULLDOWN: expected 0 ===",
           INPUT_PULLDOWN, 0);

  delay(500);

  readAll("=== INPUT_PULLUP: expected 1 ===",
           INPUT_PULLUP, 1);

  delay(2000);
}
