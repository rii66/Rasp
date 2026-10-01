void setup() {
  Serial.begin(115200);

  pinMode(3, OUTPUT);

  digitalWrite(3, HIGH);
  Serial.println("GP3 HIGH");
  delay(2000);

  digitalWrite(3, LOW);
  Serial.println("GP3 LOW");
  delay(2000);

  digitalWrite(3, HIGH);
  Serial.println("GP3 HIGH");
}

void loop() {}
