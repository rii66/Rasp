#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "slave_link.h"
#include "web_dashboard.h"

SlaveLink slave;
WebDashboard web;

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println("========== Dioscuri MASTER ESP32-C3 ==========");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WEB_AP_SSID, WEB_AP_PASS);

    Serial.print("[WEB] AP: ");
    Serial.println(WEB_AP_SSID);
    Serial.print("[WEB] IP: ");
    Serial.println(WiFi.softAPIP());

    slave.begin();
    Serial.println("[UART] Waiting for Dioscuri Slave...");

    web.begin(&slave);

    Serial.println("[READY] Master + Web Dashboard");
}

void loop() {
    slave.update();
    web.update();
}
