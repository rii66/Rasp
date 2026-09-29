#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ============================================================
// Dioscuri SLAVE — RP2040 (real-time only)
// Master ESP32-C3: UI / WiFi / Web via UART
// Port dari Dioscurios V1 — tanpa umbrella / tanpa network
// ============================================================

#define FIRMWARE_ROLE_SLAVE  1

// ---- UART → master C3 ----
#define PIN_UART_TX          20   // GP20 = physical pin 26
#define PIN_UART_RX          21   // GP21 = physical pin 27
#define UART_BAUD            115200

// ---- Nokia LCD (shared SPI) ----
#define PIN_LCD_SCK           0   // GP0  = physical pin 1
#define PIN_LCD_SDA           1   // GP1  = physical pin 2
#define PIN_LCD_CS1           2   // GP2  = physical pin 4
#define PIN_LCD_CS2           3   // GP3  = physical pin 5
#define PIN_LCD_RESET         6   // GP6  = physical pin 9

// ---- OLED I2C ----
#define PIN_OLED_SDA          4   // GP4  = physical pin 6
#define PIN_OLED_SCL          5   // GP5  = physical pin 7

// ---- Encoder dual ----
#define ENC_A                 7   // GP7  = physical pin 10
#define ENC_B                 8   // GP8  = physical pin 11
#define ENC_SW                9   // GP9  = physical pin 12

#define PIN_ENC2_A           10   // GP10 = physical pin 14
#define PIN_ENC2_B           11   // GP11 = physical pin 15
#define PIN_ENC2_SW          12   // GP12 = physical pin 16

// ---- Analog / sleep ----
#define PIN_POT_FAN          14   // GP14 = physical pin 19
#define MOTION_PIN           15   // GP15 = physical pin 20
#define PIN_SLEEP2           16   // GP16 = physical pin 21

#define TEMP_PIN             26   // GP26/ADC0 = physical pin 31
#define PIN_HOTAIR_ADC       27   // GP27/ADC1 = physical pin 32

// ---- PWM / heater ----
#define PWM_PIN              17   // GP17 = physical pin 22
#define PIN_HEATER_AC        18   // GP18 = physical pin 24
#define PIN_FAN_PWM          19   // GP19 = physical pin 25
#define PIN_ZERO_CROSS       22   // GP22 = physical pin 29
#define BUZZER_PIN           28   // GP28/ADC2 = physical pin 34

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
#define ADC_CUSTOM_MIN       501
#define ADC_CUSTOM_MAX       599

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