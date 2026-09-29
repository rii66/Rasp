#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "slave_link.h"

class WebDashboard {
public:
    void begin(SlaveLink* slave);
    void update();

private:
    void handleRoot();
    void handleStatus();
    void handleSolder();
    void handleHotAir();
    void handleFan();
    void handlePower();
    void handleBoost();

    WebServer server{80};
    SlaveLink* link = nullptr;
};
