#include "uart_link.h"
#include "config.h"
#include "GlobalState.h"
#include "boost.h"
#include "handler.h"
#include "motion.h"

#if defined(ARDUINO_ARCH_RP2040)
  #define LINK Serial1
#else
  #define LINK Serial
#endif

static char rxBuf[64];
static uint8_t rxLen = 0;
static unsigned long lastTelem = 0;

static void sendStatus() {
    LINK.printf("OK,%d,%d,%d,%d,%d,%u,%u,%u,%d,%d,%d,%d\n",
        (int)activeStation, currentTemp, targetTemp, pwmOut, tipError ? 1 : 0,
        (unsigned)airGetTemp(), (unsigned)airGetTargetTemp(), (unsigned)airGetFan(),
        airIsOn() ? 1 : 0, airHasAC() ? 1 : 0, boostMode ? 1 : 0, sleeping ? 1 : 0);
}

static void handleLine(char *line) {
    for (char *p = line; *p; p++) if (*p == '\r') { *p = 0; break; }
    if (!line[0]) return;
    char cmd = line[0];
    char *arg = (line[1] == ':') ? &line[2] : nullptr;

    switch (cmd) {
        case 'T': if (arg) {
                int lim = (maxTemp > 0) ? maxTemp : TEMP_MAX_T12;
                int t = constrain(atoi(arg), TEMP_MIN, lim);
                targetTemp = t;
                wakeFromSleep();
              } break;
        case 'H': if (arg) { airSetTemp((uint16_t)constrain(atoi(arg), TEMP_MIN_C, TEMP_MAX_HOTAIR)); } break;
        case 'F': if (arg) { airSetFan((uint8_t)constrain(atoi(arg), 0, 255)); } break;
        case 'P': if (arg) { airSwitchPower(atoi(arg) != 0); } break;
        case 'B': startBoost(); break;
        case 'S': sendStatus(); break;
        case 'Z': LINK.println(F("PONG")); break;
        default: break;
    }
}

void initUartLink() {
#if defined(ARDUINO_ARCH_RP2040)
    LINK.setTX(PIN_UART_TX);
    LINK.setRX(PIN_UART_RX);
#endif
    LINK.begin(UART_BAUD);
    Serial.println(F("[UART] slave link ready"));
}

void updateUartLink() {
    while (LINK.available()) {
        char c = (char)LINK.read();
        if (c == '\n') {
            rxBuf[rxLen] = 0;
            handleLine(rxBuf);
            rxLen = 0;
        } else if (rxLen < sizeof(rxBuf) - 1) {
            rxBuf[rxLen++] = c;
        } else rxLen = 0;
    }
    if (millis() - lastTelem >= 200) {
        lastTelem = millis();
        sendStatus();
    }
}
