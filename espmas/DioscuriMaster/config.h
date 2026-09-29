#pragma once

/*=important=*/
#define MASTER_NAME "Dioscuri Master"
#define MASTER_BAUD 115200

// UART pins intentionally left to board wiring.
// For ESP32-C3 Arduino, Serial1 can use board defaults until final pins are locked.
#define MASTER_UART_RX -1
#define MASTER_UART_TX -1

// Web dashboard
#define WEB_AP_SSID "Dioscuri-C3"
#define WEB_AP_PASS "12345678"
#define WEB_STATUS_MS 1000
#define WEB_HELLO_MS 2000

// Slave limits mirrored from DioscuriSlave/config.h
#define SLAVE_TEMP_MIN 100
#define SLAVE_TEMP_MAX_T12 450
#define SLAVE_TEMP_MIN_C 100
#define SLAVE_TEMP_MAX_HOTAIR 500
#define SLAVE_FAN_MIN 0
#define SLAVE_FAN_MAX 255
