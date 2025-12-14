# 💻 Firmware Documentation V1.10

Technical firmware documentation for the Portable Audiophile Amplifier ESP32-S3.

[![Firmware](https://img.shields.io/badge/Firmware-v1.10-green)](firmware/Firmware_Ampli_V1_10.ino)
[![MCU](https://img.shields.io/badge/MCU-ESP32--S3-red)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-teal)](https://www.arduino.cc/)

---

## 🧠 ESP32-S3 Memory Map

```
┌─────────────────────────────────────────┐
│         ESP32-S3-WROOM-1-N8R8           │
├─────────────────────────────────────────┤
│ 💾 PSRAM:    8 MB (audio buffers)       │
│ 💾 SRAM:     512 KB                     │
│ 💾 Flash:    8 MB                       │
│    ├── App:     4 MB                    │
│    ├── SPIFFS:  1.5 MB                  │
│    ├── OTA:     1.5 MB                  │
│    └── NVS:     64 KB (preferences)     │
└─────────────────────────────────────────┘
```

---

## 📍 GPIO Pinout

```
┌────────────────────────────────────────────────────────────┐
│                    🧠 ESP32-S3-WROOM                       │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  📡 I2C                                                    │
│  GPIO1  [SDA]     ↔   I2C Data (3.3V side)                │
│  GPIO2  [SCL]     →   I2C Clock (3.3V side)               │
│                                                            │
│  🎵 I2S                                                    │
│  GPIO3  [BCK]     →   Bit Clock                           │
│  GPIO4  [WS]      →   Word Select                         │
│  GPIO5  [DATA]    ←   Data In (from BT module)            │
│                                                            │
│  📊 ADC                                                    │
│  GPIO6  [VBAT]    ←   Battery voltage (divided)           │
│  GPIO7  [NTC]     ←   Temperature sensor                  │
│  GPIO8  [PFAIL]   ←   Power Fail Detection [V1.10]        │
│                                                            │
│  🎛️ User Interface                                         │
│  GPIO18 [ENC_A]   ←   Encoder A (pull-up)                 │
│  GPIO19 [ENC_B]   ←   Encoder B (pull-up)                 │
│  GPIO20 [ENC_SW]  ←   Encoder Switch (pull-up)            │
│  GPIO21 [IR]      ←   IR Receiver                         │
│                                                            │
│  🔊 Amplifier Control                                      │
│  GPIO38 [AMP_EN]  →   MA12070 Enable                      │
│  GPIO39 [MUTE]    →   MA12070 Mute                        │
│  GPIO40 [ERR]     ←   MA12070 Error                       │
│  GPIO41 [MUX]     →   CD4053 Source Select                │
│  GPIO42 [RELAY]   →   Safety Relay Control                │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## 🔌 I2C Architecture [V1.10]

### ❌ The Problem

```
TDA7439 @ 9V:  V_IH (min HIGH) = 0.7 × 9V = 6.3V
ESP32 output:  V_OH (max HIGH) = 3.3V

⚠️ 3.3V < 6.3V → TDA7439 NEVER recognizes HIGH!
                → I2C completely broken!
```

### ✅ BSS138 Level Shifter

```c
// ESP32 sends LOW (0V):
// V_GS = 3.3V - 0V = 3.3V > V_th (1.5V)
// BSS138 ON → SDA_9V = GND ✅

// ESP32 sends HIGH (3.3V) or INPUT:
// V_GS = 3.3V - 3.3V = 0V < V_th
// BSS138 OFF → SDA_9V = 9V (via pull-up) ✅

// TDA7439 pulls LOW:
// Body diode conducts
// SDA_3V3 = ~0.7V → ESP32 sees LOW ✅
```

### 📋 I2C Addresses

| Device | Address | Voltage Domain | Pull-up |
|--------|---------|----------------|---------|
| 📺 OLED SSD1306 | 0x3C | 3.3V | 10kΩ to 3V3 |
| 🎚️ TDA7439 | 0x44 | 9V | 10kΩ to 9V |
| 🔊 MA12070 | 0x20 | 9V | Via ribbon |

---

## 🔇 Anti-Plop Shutdown Sequence [V1.10]

### ❌ The Problem

```
Brutal power-off:
1. Relay cuts +22V
2. Rails collapse asynchronously
3. MA12070 loses power
4. Outputs transition to ground
5. DC current into speakers → PLOP! 💀
```

### ✅ The Fix

```c
void startShutdownSequence(void) {
    // Step 1: MUTE immediately (cuts audio)
    digitalWrite(PIN_AMP_MUTE, HIGH);
    delay(50);
    
    // Step 2: Disable amp (outputs go high-Z)
    digitalWrite(PIN_AMP_EN, LOW);
    delay(100);
    
    // Step 3: Cut relay
    digitalWrite(PIN_SAFE_EN, LOW);
}
```

### 📊 Timing Diagram

```
          T=0      T=50ms    T=150ms
           │         │          │
AMP_MUTE   ┌─────────────────────────
           │
           └─────────┐
                     │
AMP_EN     ──────────┼──────────┐
                     │          │
                     │          └─────────
SAFE_EN    ──────────┼──────────┼─────────┐
                     │          │         │
                     │          │         └─────────
                     │          │
HP_OUT     ~~~~~~~~~~│~~~~~~~~~~│~~~~~~~~~────────
           (audio)   (silence)  (silence)  (off)
```

### ⚡ Power Fail ISR [V1.10]

```c
// GPIO8 detects power loss
// Triggers MUTE BEFORE rails collapse
void IRAM_ATTR powerFailISR(void) {
    powerFailDetected = true;
    digitalWrite(PIN_AMP_MUTE, HIGH);  // Direct GPIO, no I2C!
}
```

---

## 🔄 State Machine

```
┌─────────────────────────────────────────────────────────────────┐
│                      🎵 AMP STATES                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│    ┌─────┐  startup()   ┌──────────┐  init ok  ┌─────────┐    │
│    │ OFF │─────────────▶│ STARTING │──────────▶│ RUNNING │    │
│    └─────┘              └──────────┘           └────┬────┘    │
│       ▲                                             │          │
│       │                                    temp>50  │          │
│       │                                             ▼          │
│       │                              ┌─────────────────────┐   │
│       │                              │ 🌡️ THERMAL_WARNING  │   │
│       │                              │   (volume -20%)     │   │
│       │                              └──────────┬──────────┘   │
│       │                                         │ temp>65     │
│       │                                         ▼              │
│       │                              ┌─────────────────────┐   │
│       │                              │ 🔥 THERMAL_CRITICAL │   │
│       │                              │   (volume -50%)     │   │
│       │                              └──────────┬──────────┘   │
│       │                                         │ temp>75     │
│       │ shutdown()                              │ OR button   │
│       │                                         ▼              │
│       │                              ┌─────────────────────┐   │
│       └──────────────────────────────│ 🔇 SHUTDOWN         │   │
│                                      │ MUTE→EN→RELAY       │   │
│                                      └─────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🌡️ NTC Temperature Reading

### 📐 Calculation (10kΩ B3950)

```c
float readTemperature(void) {
    uint16_t adc = medianFilter(adcNtcBuffer, 5);
    float voltage = adc * 3.3f / 4095.0f;
    
    // 🛡️ Fail-safe checks
    if (voltage < 0.1f) return TEMP_FAIL_HIGH;  // Short circuit
    if (voltage > 3.2f) return TEMP_FAIL_LOW;   // Disconnected
    
    // NTC resistance
    float R_ntc = 10000.0f * voltage / (3.3f - voltage);
    
    // Steinhart-Hart (Beta simplified)
    float steinhart = R_ntc / 10000.0f;
    steinhart = log(steinhart);
    steinhart /= 3950.0f;
    steinhart += 1.0f / (25.0f + 273.15f);
    steinhart = 1.0f / steinhart;
    steinhart -= 273.15f;
    
    return steinhart;
}
```

### 🛡️ Fail-Safe Mode [V1.9]

```c
// If NTC fails: volume capped at 50%
if (tempStatus.isFailed && volume > VOLUME_FAILSAFE) {
    volume = VOLUME_FAILSAFE;
    Serial.println("⚠️ NTC FAILED - Volume limited to 50%");
}
```

---

## 📊 Median Filter [V1.9]

Rejects ADC spikes (EMI noise):

```c
uint16_t medianFilter(uint16_t* buffer, uint8_t size) {
    uint16_t sorted[5];
    memcpy(sorted, buffer, size * sizeof(uint16_t));
    
    // Bubble sort (small array)
    for (uint8_t i = 0; i < size - 1; i++) {
        for (uint8_t j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    
    return sorted[size / 2];  // Median value
}
```

---

## 🎛️ Encoder with Critical Section [V1.9]

```c
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR encoderISR(void) {
    portENTER_CRITICAL_ISR(&encoderMux);
    
    static uint8_t lastState = 0;
    uint8_t state = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
    
    // Gray code transition table
    static const int8_t trans[] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
    encoderPosition += trans[(lastState << 2) | state];
    lastState = state;
    
    portEXIT_CRITICAL_ISR(&encoderMux);
}

int32_t readEncoderDelta(void) {
    portENTER_CRITICAL(&encoderMux);
    int32_t delta = encoderPosition;
    encoderPosition = 0;
    portEXIT_CRITICAL(&encoderMux);
    return delta;
}
```

---

## 🔌 I2C Recovery [V1.9]

### ✅ Correct (Open-Drain Compliant)

```c
void i2cRecovery(void) {
    pinMode(SDA, INPUT);       // High-Z, pull-up pulls HIGH
    pinMode(SCL, OUTPUT);
    
    for (int i = 0; i < 9; i++) {
        digitalWrite(SCL, HIGH);
        delayMicroseconds(5);
        if (digitalRead(SDA) == HIGH) break;  // Slave released
        digitalWrite(SCL, LOW);
        delayMicroseconds(5);
    }
    
    // STOP condition
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    pinMode(SDA, INPUT);       // Release SDA
}
```

### ❌ WRONG (Short Circuit Risk!)

```c
// NEVER DO THIS:
pinMode(SDA, OUTPUT);
digitalWrite(SDA, HIGH);  // 💀 If slave pulls LOW = short circuit!
```

---

## 🎚️ TDA7439 Registers

| Addr | Register | Range | Function |
|------|----------|-------|----------|
| 0x00 | Input Select | 0-3 | IN1/IN2/IN3/IN4 |
| 0x01 | Input Gain | 0-15 | 0 to +30dB (2dB steps) |
| 0x02 | Volume | 0x00-0x38 | 0 to -56dB |
| 0x03 | Bass | 0x00-0x0E | -14 to +14dB |
| 0x04 | Mid | 0x00-0x0E | -14 to +14dB |
| 0x05 | Treble | 0x00-0x0E | -14 to +14dB |
| 0x06 | L Speaker | 0x00-0x78 | Attenuation |
| 0x07 | R Speaker | 0x00-0x78 | Attenuation |

---

## 🐕 Watchdog

```c
// Reset watchdog in loop()
void loop() {
    esp_task_wdt_reset();
    
    // ... processing ...
    
    delay(10);
}

// Configuration (setup)
esp_task_wdt_init(5, true);  // 5 seconds, panic if expired
esp_task_wdt_add(NULL);      // Add current task
```

---

## 🖥️ Serial Debug Output

```
=== 🎵 AMPLI AUDIOPHILE V1.10 ===
Fixes: I2C Level Shifter, Anti-Plop, Molex

GPIO: Initialized
I2C: Scanning bus...
I2C: OLED found @ 0x3C ✅
I2C: TDA7439 found @ 0x44 ✅
I2C: MA12070 found @ 0x20 ✅
TDA7439: Initialized
MA12070: Initialized
Relay: Activated
Startup: Complete ✅

🌡️ Temp: 28.5°C
🔋 Batt: 22.1V
🔊 Volume: 45

[Button press]
Shutdown: Starting sequence...
Shutdown: MUTE activated
Shutdown: Amp disabled
Shutdown: Relay cut
Shutdown: Complete ✅
```

---

## 🔧 Build Configuration

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.flash_mode = dio
board_build.f_flash = 80000000L
build_flags = 
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
```

### 📚 Required Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| Adafruit_GFX | 1.11+ | Graphics core |
| Adafruit_SSD1306 | 2.5+ | OLED driver |
| IRremoteESP8266 | 2.8+ | IR remote |

---

## 📝 Changelog

### V1.10

| Tag | Fix |
|-----|-----|
| [A1] | 🔇 Shutdown sequence MUTE→EN→RELAY |
| [A2] | ⚡ ISR POWER_FAIL for instant MUTE |
| [A3] | ⏱️ I2C timeout increased (50ms) for level shifter |
| [A4] | 📍 GPIO8 for power loss detection |

### V1.9

| Tag | Fix |
|-----|-----|
| [S1] | 🔌 I2C recovery open-drain (INPUT not OUTPUT+HIGH) |
| [D1] | 🛡️ NTC fail-safe: 50% max volume if sensor dead |
| [D2] | 🚩 Flag i2cHardwareFault |
| [P0] | 📊 ADC median filter (5 samples) |
| [P1] | 🔒 Encoder critical sections (portMUX) |
| [P2] | 📈 I2C error counter |

---

**💻 Happy coding!**
