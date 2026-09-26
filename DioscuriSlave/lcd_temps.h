#ifndef LCD_TEMPS_H
#define LCD_TEMPS_H

#include <Arduino.h>

// Nokia 105 local displays on RP2040 slave
// Shared bus: SCK / SDA / RESET
// CS1 = solder, CS2 = hot-air (CS3/CS4 reserved)

void initLcdTemps();
void updateLcdTemps();   // call from loop (~5–10 Hz is enough)

#endif
