#include "NokiaStation.h"

NokiaStation solder(1, 0, 6, 3);
NokiaStation hotAir(1, 0, 6, 2);

static void lcdIdle() {
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(6, OUTPUT);

  lcdIdle();

  digitalWrite(6, LOW);
  delay(20);
  digitalWrite(6, HIGH);
  delay(150);

  lcdIdle();

  solder.initDisplay(false);
  solder.setRotation(1);
  solder.backgroundColor(BLACK);
  solder.printString("SOLDER", 4, 20, CYAN, BLACK);

  lcdIdle();
  delay(50);

  hotAir.initDisplay(false);
  hotAir.setRotation(1);
  hotAir.backgroundColor(BLACK);
  hotAir.printString("HOT AIR", 4, 20, MAGENTA, BLACK);

  lcdIdle();
}

void loop() {}
