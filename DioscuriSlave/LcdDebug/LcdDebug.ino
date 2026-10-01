#include <Arduino.h>

#define LCD_CS    2
#define LCD_SDA   1
#define LCD_SCK   0
#define LCD_RST   6
#define LED_PIN   25

static void mark(const char *s) {
  Serial.println(s);
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(50);
}

static void clockBit(uint8_t bit) {
  digitalWrite(LCD_SDA, bit ? HIGH : LOW);
  delayMicroseconds(5);
  digitalWrite(LCD_SCK, HIGH);
  delayMicroseconds(5);
  digitalWrite(LCD_SCK, LOW);
  delayMicroseconds(5);
}

static void send9(uint8_t dc, uint8_t data) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_SDA, dc ? HIGH : LOW);
  delayMicroseconds(5);
  digitalWrite(LCD_SCK, HIGH);
  delayMicroseconds(5);
  digitalWrite(LCD_SCK, LOW);
  delayMicroseconds(5);

  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    clockBit(data & mask);
  }

  digitalWrite(LCD_CS, HIGH);
}

static void sendCmd(uint8_t c) {
  send9(0, c);
}

static void sendData(uint8_t d) {
  send9(1, d);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_SDA, OUTPUT);
  pinMode(LCD_SCK, OUTPUT);
  pinMode(LCD_RST, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_SCK, LOW);
  digitalWrite(LCD_SDA, LOW);

  Serial.begin(115200);
  delay(2000);

  Serial.println("=== RP2040 NOKIA BUS DEBUG ===");
  Serial.println("CS=GP2 SDA=GP1 SCK=GP0 RST=GP6");

  mark("[RST] LOW");
  digitalWrite(LCD_RST, LOW);
  delay(20);

  mark("[RST] HIGH");
  digitalWrite(LCD_RST, HIGH);
  delay(120);

  mark("[CMD] SWRESET");
  sendCmd(0x01);
  delay(120);

  mark("[CMD] SPLOUT");
  sendCmd(0x11);
  delay(120);

  mark("[CMD] COLMOD");
  sendCmd(0x3A);
  sendData(0x05);
  delay(20);

  mark("[CMD] MADCTL");
  sendCmd(0x36);
  sendData(0x08);
  delay(20);

  mark("[CMD] NORON");
  sendCmd(0x13);
  delay(20);

  mark("[CMD] DISPON");
  sendCmd(0x29);
  delay(50);

  Serial.println("[FRAME] RED");
  digitalWrite(LED_PIN, HIGH);

  sendCmd(0x2A);
  sendData(0x00);
  sendData(0x02);
  sendData(0x00);
  sendData(0x81);

  sendCmd(0x2B);
  sendData(0x00);
  sendData(0x00);
  sendData(0x00);
  sendData(0x9F);

  sendCmd(0x2C);

  for (uint32_t i = 0; i < 128UL * 160UL; i++) {
    sendData(0xF8);
    sendData(0x00);
  }

  Serial.println("[FRAME] RED DONE");
  Serial.println("=== TEST COMPLETE ===");
}

void loop() {
  delay(1000);
  Serial.println("ALIVE");
}
