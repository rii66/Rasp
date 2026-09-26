#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

// RP2040 (arduino-pico) has no IRAM_ATTR; ESP32 needs it for ISRs.
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// analogWriteFrequency / analogWriteRange are provided by arduino-pico.
// tone(), attachInterrupt(), EEPROM, analogReadResolution(12) are all supported.

#endif
