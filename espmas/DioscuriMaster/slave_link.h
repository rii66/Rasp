#pragma once
#include <Arduino.h>
#include "slave_protocol.h"

class SlaveLink {
public:
    void begin();
    void update();

    bool connected() const;
    const SlaveProtocol::Status& status() const;

    void requestStatus();
    void setSolderTarget(int temp);
    void setHotAirTarget(int temp);
    void setFan(uint8_t speed);
    void setHotAirPower(bool on);
    void boost();

private:
    void sendLine(const String& line);
    void handleLine(char* line);
    bool parseStatus(char* line);

    HardwareSerial* port = nullptr;
    SlaveProtocol::Status state;

    char rxBuf[128] = {};
    uint8_t rxLen = 0;

    bool linkUp = false;
    uint32_t lastRx = 0;
    uint32_t lastHello = 0;
    uint32_t lastStatus = 0;

    static constexpr uint32_t LINK_TIMEOUT_MS = 5000;
};
