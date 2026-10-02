#ifndef LCD_DEBUG_H
#define LCD_DEBUG_H

#include <Arduino.h>
#include "config.h"

// ============================================================
// LCD Debug & Diagnostics Module
// Detects: CS pin state, SPI communication, Reset signal
// ============================================================

// Forward declarations
class Nokia105;

namespace LcdDebug {
  
  // Check pin voltage levels (requires analogRead on CS/RST pins if available)
  inline void checkPinStates() {
    Serial.println(F("\n[LCD_DBG] === PIN STATE CHECK ==="));
    
    // CS pins
    pinMode(PIN_LCD_CS1, INPUT);
    pinMode(PIN_LCD_CS2, INPUT);
    int cs1_state = digitalRead(PIN_LCD_CS1);
    int cs2_state = digitalRead(PIN_LCD_CS2);
    Serial.print(F("[LCD_DBG] CS1 (GP"));
    Serial.print(PIN_LCD_CS1);
    Serial.print(F("): "));
    Serial.println(cs1_state ? "HIGH" : "LOW");
    Serial.print(F("[LCD_DBG] CS2 (GP"));
    Serial.print(PIN_LCD_CS2);
    Serial.print(F("): "));
    Serial.println(cs2_state ? "HIGH" : "LOW");
    
    // Reset pin
    pinMode(PIN_LCD_RESET, INPUT);
    int rst_state = digitalRead(PIN_LCD_RESET);
    Serial.print(F("[LCD_DBG] RST (GP"));
    Serial.print(PIN_LCD_RESET);
    Serial.print(F("): "));
    Serial.println(rst_state ? "HIGH" : "LOW");
    
    // SPI bus pins
    Serial.print(F("[LCD_DBG] SDA (GP"));
    Serial.print(PIN_LCD_SDA);
    Serial.print(F("), SCK (GP"));
    Serial.print(PIN_LCD_SCK);
    Serial.println(F(")");
  }
  
  // Log timing sequence during init
  inline void logInitStart(int cs_num) {
    Serial.print(F("[LCD_DBG] === LCD"));
    Serial.print(cs_num);
    Serial.println(F(" INIT START ==="));
  }
  
  inline void logResetPulse(int cs_num, uint32_t hold_ms) {
    Serial.print(F("[LCD_DBG] LCD"));
    Serial.print(cs_num);
    Serial.print(F(" reset pulse (hold "));
    Serial.print(hold_ms);
    Serial.println(F(" ms)"));
  }
  
  inline void logCsToggle(int cs_num, bool high) {
    Serial.print(F("[LCD_DBG] LCD"));
    Serial.print(cs_num);
    Serial.print(F(" CS "));
    Serial.println(high ? "HIGH" : "LOW");
  }
  
  inline void logCommandWrite(int cs_num, uint8_t cmd) {
    Serial.print(F("[LCD_DBG] LCD"));
    Serial.print(cs_num);
    Serial.print(F(" writeCmd(0x"));
    if (cmd < 0x10) Serial.print('0');
    Serial.print(cmd, HEX);
    Serial.println(')');
  }
  
  inline void logInitComplete(int cs_num, bool success) {
    Serial.print(F("[LCD_DBG] LCD"));
    Serial.print(cs_num);
    Serial.println(success ? F(" INIT OK ✓") : F(" INIT FAILED ✗"));
  }
  
  // Check if display responds to commands (read display status register)
  // Returns: 0 = no response, 1 = response OK
  inline int checkDisplayResponse(int cs_num) {
    // Note: This is a diagnostic placeholder.
    // In production, you would implement RAMRD (0x2E) command
    // to read back display memory and verify initialization.
    Serial.print(F("[LCD_DBG] LCD"));
    Serial.print(cs_num);
    Serial.println(F(" response check (requires RAMRD impl)"));
    return -1; // Not implemented yet
  }
}

#endif
