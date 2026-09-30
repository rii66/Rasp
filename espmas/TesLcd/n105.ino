#include <Nokia105_LCD.h>

#define LCD_SDA  6
#define LCD_CS   7
#define LCD_SCK  4
#define LCD_RST  2

// #define LCD_BL   10

Nokia105 display(LCD_SDA, LCD_SCK, LCD_RST, LCD_CS);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Nokia 105 LCD / ESP32-C3 TEST ===");

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  Serial.println("[1] Init LCD...");
  display.initDisplay();
  Serial.println("[OK] Init command sent");

  Serial.println("[2] RED");
  display.backgroundColor(RED);
  delay(2000);

  Serial.println("[3] GREEN");
  display.backgroundColor(GREEN);
  delay(2000);

  Serial.println("[4] BLUE");
  display.backgroundColor(BLUE);
  delay(2000);

  Serial.println("[5] WHITE");
  display.backgroundColor(WHITE);
  delay(2000);

  Serial.println("[6] BLACK");
  display.backgroundColor(BLACK);
  delay(1000);
}

void loop() {
  Serial.println("TEXT: HELLO");
  display.backgroundColor(BLACK);
  display.printString("HELLO", 20, 30, WHITE, BLACK);
  display.printString("ESP32-C3", 15, 55, GREEN, BLACK);
  delay(2000);

  Serial.println("TEXT: NOKIA 105");
  display.backgroundColor(BLUE);
  display.printString("NOKIA 105", 10, 35, WHITE, BLUE);
  display.printString("128x160", 25, 60, YELLOW, BLUE);
  delay(2000);

  Serial.println("TEXT: HERMENEX");
  display.backgroundColor(RED);
  display.printString("HERMENEX", 15, 40, WHITE, RED);
  delay(2000);
}
