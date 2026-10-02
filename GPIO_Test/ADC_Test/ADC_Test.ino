/*
  RP2040 RAW ADC TEST
  GP26 / ADC0
  Standalone sketch.
*/

const uint8_t ADC_PIN = 26;
const uint16_t SAMPLES = 100;

void setup() {
  Serial.begin(115200);
  delay(1000);
  analogReadResolution(12);

  Serial.println();
  Serial.println("=== RP2040 ADC TEST ===");
  Serial.println("GP26 / ADC0 / 12-bit");
  Serial.println("GP26 -> GND : near 0");
  Serial.println("GP26 -> 3V3 : near 4095");
  Serial.println();
}

void loop() {
  uint16_t minRaw = 4095;
  uint16_t maxRaw = 0;
  uint32_t sum = 0;

  for (uint16_t i = 0; i < SAMPLES; i++) {
    uint16_t raw = analogRead(ADC_PIN);
    if (raw < minRaw) minRaw = raw;
    if (raw > maxRaw) maxRaw = raw;
    sum += raw;
    delayMicroseconds(200);
  }

  Serial.print("AVG=");
  Serial.print(sum / SAMPLES);
  Serial.print(" MIN=");
  Serial.print(minRaw);
  Serial.print(" MAX=");
  Serial.print(maxRaw);
  Serial.print(" RANGE=");
  Serial.println(maxRaw - minRaw);

  delay(500);
}
