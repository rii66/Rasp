#include "slave_link.h"
#include "config.h"

static HardwareSerial slaveSerial(1);

void SlaveLink::begin() {
    port = &slaveSerial;

#if MASTER_UART_RX >= 0 && MASTER_UART_TX >= 0
    port->begin(MASTER_BAUD, SERIAL_8N1, MASTER_UART_RX, MASTER_UART_TX);
#else
    port->begin(MASTER_BAUD);
#endif

    linkUp = false;
    lastRx = 0;
    lastHello = 0;
    lastStatus = 0;
}

void SlaveLink::sendLine(const String& line) {
    if (!port) return;
    port->println(line);
}

void SlaveLink::update() {
    if (!port) return;

    while (port->available()) {
        char c = (char)port->read();

        if (c == '\n') {
            rxBuf[rxLen] = 0;
            handleLine(rxBuf);
            rxLen = 0;
        } else if (rxLen < sizeof(rxBuf) - 1) {
            rxBuf[rxLen++] = c;
        } else {
            rxLen = 0;
        }
    }

    const uint32_t now = millis();

    if (!linkUp) {
        if (now - lastHello >= WEB_HELLO_MS) {
            lastHello = now;
            sendLine(SlaveProtocol::HELLO);
        }
    } else {
        if (now - lastStatus >= WEB_STATUS_MS) {
            lastStatus = now;
            requestStatus();
        }

        if (now - lastRx >= LINK_TIMEOUT_MS) {
            linkUp = false;
            state.valid = false;
            Serial.println("[UART] Slave timeout");
        }
    }
}

void SlaveLink::handleLine(char* line) {
    while (*line == '\r' || *line == ' ') line++;
    if (!*line) return;

    if (!strcmp(line, SlaveProtocol::PONG)) {
        linkUp = true;
        lastRx = millis();
        Serial.println("[UART] Slave detected");
        return;
    }

    if (parseStatus(line)) {
        linkUp = true;
        lastRx = millis();
    }
}

bool SlaveLink::parseStatus(char* line) {
    if (strncmp(line, "OK,", 3) != 0) return false;

    // Slave sends exactly 12 fields after "OK,".
    char* fields[12] = {};
    uint8_t count = 0;
    char* p = line + 3;

    while (count < 12 && p) {
        fields[count++] = p;
        char* comma = strchr(p, ',');
        if (!comma) break;
        *comma = 0;
        p = comma + 1;
    }

    if (count != 12) return false;

    state.activeStation = atoi(fields[0]);
    state.currentTemp = atoi(fields[1]);
    state.targetTemp = atoi(fields[2]);
    state.pwmOut = atoi(fields[3]);
    state.tipError = atoi(fields[4]) != 0;
    state.airTemp = atoi(fields[5]);
    state.airTargetTemp = atoi(fields[6]);
    state.airFan = atoi(fields[7]);
    state.airOn = atoi(fields[8]) != 0;
    state.airHasAC = atoi(fields[9]) != 0;
    state.boostMode = atoi(fields[10]) != 0;
    state.sleeping = atoi(fields[11]) != 0;
    state.valid = true;
    return true;
}

bool SlaveLink::connected() const {
    return linkUp;
}

const SlaveProtocol::Status& SlaveLink::status() const {
    return state;
}

void SlaveLink::requestStatus() {
    sendLine("S");
}

void SlaveLink::setSolderTarget(int temp) {
    temp = constrain(temp, SLAVE_TEMP_MIN_C, SLAVE_TEMP_MAX_T12);
    sendLine(SlaveProtocol::makeTarget(SlaveProtocol::CMD_T, temp));
}

void SlaveLink::setHotAirTarget(int temp) {
    temp = constrain(temp, SLAVE_TEMP_MIN_C, SLAVE_TEMP_MAX_HOTAIR);
    sendLine(SlaveProtocol::makeTarget(SlaveProtocol::CMD_H, temp));
}

void SlaveLink::setFan(uint8_t speed) {
    sendLine(SlaveProtocol::makeTarget(SlaveProtocol::CMD_F,
                                        constrain((int)speed, SLAVE_FAN_MIN, SLAVE_FAN_MAX)));
}

void SlaveLink::setHotAirPower(bool on) {
    sendLine(SlaveProtocol::makePower(on));
}

void SlaveLink::boost() {
    sendLine("B");
}
