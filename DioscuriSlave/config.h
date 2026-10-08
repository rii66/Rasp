#ifndef CONFIG_H

#define CONFIG_H

#include <stdint.h>

// ============================================================
// Dioscuri SLAVE — RP2040 
// Master ESP Psu Lab / WiFi / Web via UART 
// Port dari Dioscurios V1 —
// ============================================================

// FIRMWARE_ROLE_SLAVE  1

// ---- UART ----
#define PIN_UART_TX          20   //  physical   26
#define PIN_UART_RX          21   //  physical   27
#define UART_BAUD            115200

// ---- DISPLAY ----
#define PIN_LCD_CS1           0   //   physical   debug dulu
#define PIN_LCD_CS2           1   //   physical   debug dulu
#define PIN_LCD_SCK           2   //   physical   debug dulu
#define PIN_LCD_SDA           3   //   physical   debug dulu
// ---- OLED I2C ----
#define PIN_OLED_SDA          4   //   physical   6
#define PIN_OLED_SCL          5   //   physical   7

#define PIN_LCD_RESET         6   //   physical   debug dulu

// ---- Encoder dual ----
#define ENC_A                 7   //   physical   10
#define ENC_B                 8   //   physical   11
#define ENC_SW                9   //   physical   12

#define PIN_ENC2_A           10   //  physical   14
#define PIN_ENC2_B           11   //  physical   15
#define PIN_ENC2_SW          12

// ---- Analog / sleep ----
#define PIN_POT_FAN          28   //  physical   34
#define MOTION_PIN           15   //  physical   20
#define PIN_SLEEP2           16   //  physical   21

#define TEMP_PIN             26   //  physical   31
#define PIN_HOTAIR_ADC       27   //  physical   32

// ---- PWM / heater ----
#define PWM_PIN              17   //  physical   22
#define PIN_HEATER_AC        18   //  physical   24
#define PIN_FAN_PWM          19
#define PIN_ZERO_CROSS       22   //   physical   29

// ---- Buzzer / future relay ----
#define BUZZER_PIN           13   //   physical   17
//#define PIN_AC_RELAY or LCD REST ( JIKA GLITCH )       ×

// ---- PID defaults ----
#define PID_KP_T12           3.2f
#define PID_KI_T12           0.12f
#define PID_KD_T12           1.8f
#define PID_KP_C210          2.8f
#define PID_KI_C210          0.10f
#define PID_KD_C210          1.4f
#define PID_KP_CUSTOM        3.0f
#define PID_KI_CUSTOM        0.11f
#define PID_KD_CUSTOM        1.6f

// ---- Temp limits (single source) ----
#define TEMP_MIN             100
#define TEMP_MAX_T12         450
#define TEMP_MAX_C210        380
#define TEMP_MAX_CUSTOM      600
#define TEMP_MAX_HOTAIR      500
#define TEMP_MIN_C           TEMP_MIN
#define TEMP_MAX_C           TEMP_MAX_HOTAIR

#define DEFAULT_TEMP         320
#define DEFAULT_BOOST_TEMP   470
#define DEFAULT_SLEEP_TEMP   195
#define DEFAULT_BOOST_TIME   12
#define DEFAULT_HOTAIR_TEMP  300

#define OFFSET_TEMP_T12      0
#define OFFSET_TEMP_C210     0
#define OFFSET_TEMP_CUSTOM   0
#define OFFSET_ADC_T12       0
#define OFFSET_ADC_C210      0
#define OFFSET_ADC_CUSTOM    0

#define ADC_NO_TIP           150
#define ADC_NO_TIP_PTC       4000

// PTC/CUSTOM ADC calibration endpoints
#define ADC_CUSTOM_MIN       700
#define ADC_CUSTOM_MAX       1515

#define PWM_FREQ             20000
#define PWM_RES              8
#define PWM_MAX_VAL          255
#define MAX_PWM_T12          255
#define MAX_PWM_C210         71
#define MAX_PWM_CUSTOM       255

#define POWER_PERIOD         100
#define HEATER_MAX_POWER     100
#define FAN_PWM_FREQ         PWM_FREQ
#define FAN_PWM_RES          8
#define FAN_MIN_SPEED        60
#define MAX_PWM_HOTAIR       255
#define MAX_FAN_PWM          255

#define TEMP_AMBIENT_C       28
const uint16_t TEMP_TIP[3] = {200, 300, 400};

#endif
