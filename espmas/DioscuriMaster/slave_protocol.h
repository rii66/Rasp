#pragma once
#include <Arduino.h>

/*=important=*/
// DioscuriSlave UART protocol. Keep in sync with DioscuriSlave/uart_link.cpp.

namespace SlaveProtocol {
static constexpr const char* HELLO = "HELLO";
static constexpr const char* PING  = "PING";
static constexpr const char* PONG  = "PONG";

static constexpr char CMD_S = 'S';       // status request
static constexpr char CMD_T = 'T';       // solder target
static constexpr char CMD_H = 'H';       // hot-air target
static constexpr char CMD_F = 'F';       // fan 0..255
static constexpr char CMD_P = 'P';       // hot-air power 0/1
static constexpr char CMD_B = 'B';       // solder boost

// OK,
// activeStation,currentTemp,targetTemp,pwmOut,tipError,
// airTemp,airTargetTemp,airFan,airOn,airHasAC,boostMode,sleeping
struct Status {
    int activeStation = 0;
    int currentTemp = 0;
    int targetTemp = 0;
    int pwmOut = 0;
    bool tipError = false;
    int airTemp = 0;
    int airTargetTemp = 0;
    int airFan = 0;
    bool airOn = false;
    bool airHasAC = false;
    bool boostMode = false;
    bool sleeping = false;
    bool valid = false;
};

inline String makeTarget(char cmd, int value) {
    return String(cmd) + ":" + String(value);
}

inline String makePower(bool on) {
    return String("P:") + (on ? "1" : "0");
}

} // namespace SlaveProtocol
