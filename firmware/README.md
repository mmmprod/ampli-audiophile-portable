# Firmware

<p>
  <img src="https://img.shields.io/badge/MCU-ESP32--S3-orange?style=flat-square&logo=espressif" />
  <img src="https://img.shields.io/badge/Framework-Arduino-00979D?style=flat-square&logo=arduino" />
  <img src="https://img.shields.io/badge/I2C-400kHz-blue?style=flat-square" />
  <img src="https://img.shields.io/badge/Lines-1119-lightgrey?style=flat-square" />
</p>

ESP32-S3 firmware handling audio routing, I2C peripherals, thermal management, and user interface.

---

## Build

```bash
# Arduino IDE
Board:      ESP32-S3 Dev Module
Flash:      8MB (64Mb)
PSRAM:      OPI PSRAM
Partition:  Default 4MB with spiffs
USB CDC:    Enabled

# PlatformIO
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```

No external libraries required. Everything uses ESP-IDF drivers.

---

## Memory Map

```
┌─────────────────────────────────────┐ 0x3FC00000
│            PSRAM (8MB)              │
│  - Audio buffers                    │
│  - Display framebuffer              │
├─────────────────────────────────────┤ 0x3FC80000
│            SRAM (512KB)             │
│  - Stack                            │
│  - Heap                             │
│  - Static variables                 │
├─────────────────────────────────────┤
│            Flash (8MB)              │
│  - App (1.5MB)                      │
│  - NVS (Preferences)                │
│  - SPIFFS (future)                  │
└─────────────────────────────────────┘
```

---

## Pinout

```
                    ┌─────────────────────┐
                    │     ESP32-S3        │
                    │                     │
        I2C SDA ◄───┤ GPIO1         GPIO42├───► SAFE_EN (relay)
        I2C SCL ────┤ GPIO2         GPIO41├───► MUX_S0
       I2S BCK  ────┤ GPIO3         GPIO40├◄─── AMP_ERR
       I2S WS   ────┤ GPIO4         GPIO39├───► AMP_MUTE
       I2S DATA ◄───┤ GPIO5         GPIO38├───► AMP_EN
       ADC BATT ◄───┤ GPIO6         GPIO21├◄─── IR_RX
       ADC NTC  ◄───┤ GPIO7         GPIO20├◄─── ENC_SW
                    │                GPIO19├◄─── ENC_B
                    │                GPIO18├◄─── ENC_A
                    │                     │
                    │        USB          │
                    └─────────────────────┘

ADC_BATT: Voltage divider 220k/33k (ratio 7.666)
ADC_NTC:  10k pullup to 3V3, NTC 10k@25C to GND
```

---

## I2C Architecture

```c
#define I2C_ADDR_MA12070  0x20    // Class-D amplifier
#define I2C_ADDR_OLED     0x3C    // SSD1306 128x64
#define I2C_ADDR_TDA7439  0x44    // Audio processor

Wire.begin(PIN_SDA, PIN_SCL);
Wire.setClock(400000);  // Fast mode
```

### Bus Recovery (V1.9 Fix)

Critical bug fixed in V1.9. Previous code could short-circuit the bus:

```c
// WRONG (V1.8) - Forces 3.3V on bus, shorts if slave pulls LOW
pinMode(SDA, OUTPUT);
digitalWrite(SDA, HIGH);  // DANGER!

// CORRECT (V1.9) - Open-drain compliant
pinMode(SDA, INPUT);      // High-Z, pullup takes line HIGH
```

Full recovery sequence: 9 clock pulses + STOP condition.

---

## State Machine

```
                    ┌─────────────┐
                    │  AMP_OFF    │
                    └──────┬──────┘
                           │ enable()
                           v
                    ┌─────────────┐
                    │  STARTING   │──────────────┐
                    └──────┬──────┘              │
                           │ init OK             │ init fail
                           v                     v
    ┌──────────┐    ┌─────────────┐       ┌─────────────┐
    │  MUTED   │◄──►│  RUNNING    │       │   ERROR     │
    └──────────┘    └──────┬──────┘       └─────────────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
              v            v            v
       ┌───────────┐ ┌───────────┐ ┌───────────┐
       │THERMAL    │ │THERMAL    │ │ SHUTDOWN  │
       │WARNING    │ │CRITICAL   │ │           │
       │(vol -20%) │ │(vol -50%) │ │(all off)  │
       └───────────┘ └───────────┘ └───────────┘
```

---

## Thermal Protection

```c
#define TEMP_WARNING     50.0f   // Reduce volume 20%
#define TEMP_CRITICAL    65.0f   // Reduce volume 50%
#define TEMP_SHUTDOWN    75.0f   // Emergency shutdown
#define TEMP_FAIL_LOW   -20.0f   // NTC disconnected
#define TEMP_FAIL_HIGH  120.0f   // NTC shorted

// NTC fail-safe (V1.9): If sensor fails, limit to 50% volume
if (!tempStatus.isValid && currentVolume > VOLUME_FAILSAFE) {
    setVolume(VOLUME_FAILSAFE);  // 50
}
```

NTC calculation uses Beta equation (Steinhart-Hart simplified):

```c
float steinhart = ntcResistance / NTC_NOMINAL_R;   // 10k
steinhart = log(steinhart);
steinhart /= NTC_BETA;                              // 3950
steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);  // 25C
steinhart = 1.0f / steinhart;
steinhart -= 273.15f;  // Celsius
```

---

## ADC Filtering

Median filter to reject spikes (V1.9):

```c
#define ADC_MEDIAN_SIZE  5

uint16_t buffer[ADC_MEDIAN_SIZE];

uint16_t medianFilter(uint16_t* buffer, uint8_t size) {
    // Copy and sort
    uint16_t sorted[size];
    memcpy(sorted, buffer, size * sizeof(uint16_t));
    
    // Bubble sort (small array, fast enough)
    for (uint8_t i = 0; i < size - 1; i++) {
        for (uint8_t j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    return sorted[size / 2];
}
```

---

## Encoder Handling

Interrupt-driven with Gray code decoding and mutex protection:

```c
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR encoderISR(void) {
    portENTER_CRITICAL_ISR(&encoderMux);
    
    static uint8_t lastState = 0;
    uint8_t state = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
    
    // Transition table for Gray code
    static const int8_t trans[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    
    encoderPosition += trans[(lastState << 2) | state];
    lastState = state;
    
    portEXIT_CRITICAL_ISR(&encoderMux);
}
```

---

## TDA7439 Registers

```c
// Input selector
i2cWrite(0x44, 0x00, source);  // 0=IN1, 1=IN2, 2=IN3

// Volume (0x00 = 0dB, 0x38 = -56dB)
i2cWrite(0x44, 0x02, (100 - volume) * 56 / 100);

// Bass/Mid/Treble (-14dB to +14dB)
// Register value: 0x00 = -14dB, 0x07 = 0dB, 0x0E = +14dB
uint8_t regVal = (db + 14) / 2;
i2cWrite(0x44, 0x03, regVal);  // Bass
i2cWrite(0x44, 0x04, regVal);  // Mid
i2cWrite(0x44, 0x05, regVal);  // Treble
```

---

## Watchdog

5-second hardware watchdog with panic reset:

```c
#include <esp_task_wdt.h>

void initWatchdog(void) {
    esp_task_wdt_init(5, true);  // 5s timeout, panic on expire
    esp_task_wdt_add(NULL);       // Add current task
}

void loop() {
    esp_task_wdt_reset();  // Feed the dog
    // ...
}
```

---

## API Reference

```c
// Volume control (0-100, respects thermal limits)
void setVolume(uint8_t vol);

// Source selection (mutes during switch)
typedef enum { SOURCE_BLUETOOTH, SOURCE_AUX, SOURCE_PHONO } AudioSource_t;
void setSource(AudioSource_t source);

// EQ control (-14 to +14 dB)
void setTDA7439Bass(int8_t db);
void setTDA7439Mid(int8_t db);
void setTDA7439Treble(int8_t db);

// Amp control
void enableMA12070(bool enable);
void setMA12070Mute(bool mute);

// I2C low-level
bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t data);
bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* data);
bool i2cRecovery(void);
```

---

## Debug Output

```
=== AMPLI AUDIOPHILE V1.9 ===
Corrections: I2C open-drain, NTC failsafe, PTC nappe
I2C: OLED non detecte, tentative recovery...
I2C Recovery: Debut sequence...
I2C Recovery: SDA libre apres 3 clocks
I2C Recovery: SUCCES
Initialisation complete
```

Serial: 115200 baud.

---

## Changelog

| Version | Changes |
|---------|---------|
| **1.9** | I2C open-drain fix (was shorting bus!), NTC fail-safe, median ADC filter, portMUX encoder |
| 1.8 | I2C recovery, NTC monitoring, watchdog |
| 1.7 | TDA7439 integration, multi-source |
| 1.6 | NVS persistence, IR remote |

---

<p align="center">
  <sub>Full source: <code>Firmware_Ampli_V1_9.ino</code> (1119 lines)</sub>
</p>
