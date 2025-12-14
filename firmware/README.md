# Firmware Documentation V1.10

Documentation technique du firmware ESP32-S3 pour l'Ampli Audiophile Portable.

## Memory Map

```
┌─────────────────────────────────────────┐
│            ESP32-S3-WROOM-1-N8R8        │
├─────────────────────────────────────────┤
│ PSRAM:     8 MB (audio buffers)         │
│ SRAM:      512 KB                       │
│ Flash:     8 MB                         │
│ - App:     4 MB                         │
│ - SPIFFS:  1.5 MB                       │
│ - OTA:     1.5 MB                       │
│ - NVS:     64 KB (preferences)          │
└─────────────────────────────────────────┘
```

## GPIO Pinout

```
┌────────────────────────────────────────────────────────────┐
│                      ESP32-S3-WROOM                        │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  GPIO1  [SDA]     ←→  I2C Data (3.3V, via level shifter)  │
│  GPIO2  [SCL]     →   I2C Clock (3.3V, via level shifter) │
│  GPIO3  [BCK]     →   I2S Bit Clock                       │
│  GPIO4  [WS]      →   I2S Word Select                     │
│  GPIO5  [DATA]    ←   I2S Data In (from BT module)        │
│                                                            │
│  GPIO6  [ADC]     ←   Battery voltage (divided)           │
│  GPIO7  [ADC]     ←   NTC temperature sensor              │
│  GPIO8  [IN]      ←   Power Fail Detection [V1.10]        │
│                                                            │
│  GPIO18 [ENC_A]   ←   Encoder A (pull-up)                 │
│  GPIO19 [ENC_B]   ←   Encoder B (pull-up)                 │
│  GPIO20 [ENC_SW]  ←   Encoder Switch (pull-up)            │
│  GPIO21 [IR]      ←   IR Receiver                         │
│                                                            │
│  GPIO38 [AMP_EN]  →   MA12070 Enable                      │
│  GPIO39 [MUTE]    →   MA12070 Mute                        │
│  GPIO40 [ERR]     ←   MA12070 Error                       │
│  GPIO41 [MUX]     →   CD4053 Source Select                │
│  GPIO42 [RELAY]   →   Safety Relay Control                │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

## I2C Architecture [V1.10]

### Probleme Resolu

```
TDA7439 specifications:
  Vcc min = 7V, Vcc typ = 9V
  V_IH = 0.7 x Vcc = 6.3V @ 9V

ESP32 output:
  V_OH = 3.3V

Probleme: 3.3V < 6.3V --> TDA7439 ne voit jamais HIGH!
```

### Solution: BSS138 Level Shifter

```
          3.3V                              9V
           |                                 |
          [10k]                            [10k]
           |                                 |
SDA_3V3 ---+----[Source]   [Drain]----+--- SDA_9V
           |        |       |         |
           |       BSS138   |         |
           |        |       |         |
           +------[Gate]----+         |
           |                          |
          3.3V                        |
                                      |
                              To TDA7439/MA12070
```

**Fonctionnement:**
```c
// ESP32 envoie LOW (0V):
// V_GS = 3.3V - 0V = 3.3V > V_th (1.5V)
// BSS138 ON --> SDA_9V = GND (via Rds_on)

// ESP32 envoie HIGH (3.3V) ou INPUT:
// V_GS = 3.3V - 3.3V = 0V < V_th
// BSS138 OFF --> SDA_9V = 9V (via pull-up)

// TDA7439 tire LOW:
// Body diode BSS138 conduit
// SDA_3V3 = ~0.7V --> ESP32 voit LOW
```

### Adresses I2C

| Device | Address | Domain | Pull-up |
|--------|---------|--------|---------|
| OLED SSD1306 | 0x3C | 3.3V | 10k to 3V3 |
| TDA7439 | 0x44 | 9V | 10k to 9V |
| MA12070 | 0x20 | 9V | Via ribbon |

### I2C Recovery [V1.9]

```c
// CORRECT - Open-drain conforme
void i2cRecovery(void) {
    pinMode(SDA, INPUT);       // High-Z, pull-up tire HIGH
    pinMode(SCL, OUTPUT);
    
    for (int i = 0; i < 9; i++) {
        digitalWrite(SCL, HIGH);
        delayMicroseconds(5);
        if (digitalRead(SDA) == HIGH) break;
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

// FAUX - Court-circuit si slave tire LOW!
// pinMode(SDA, OUTPUT);
// digitalWrite(SDA, HIGH);  // DANGER!
```

## Sequence Extinction Anti-Plop [V1.10]

### Probleme

```
Extinction brutale:
1. Relay coupe +22V
2. Rails s'effondrent de facon asynchrone
3. MA12070 perd alimentation
4. Sorties transitent vers masse
5. Courant DC dans HP --> PLOP!
```

### Solution

```c
void startShutdownSequence(void) {
    // Etape 1: MUTE immediat (coupe audio)
    digitalWrite(PIN_AMP_MUTE, HIGH);
    delay(50);
    
    // Etape 2: Disable ampli (sorties haute-Z)
    digitalWrite(PIN_AMP_EN, LOW);
    delay(100);
    
    // Etape 3: Couper relay
    digitalWrite(PIN_SAFE_EN, LOW);
}
```

### Timing Diagram

```
          T=0      T=50ms    T=150ms
           |         |          |
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

### Power Fail ISR

```c
// Detection coupure alimentation (GPIO8)
// Active MUTE avant effondrement rails
void IRAM_ATTR powerFailISR(void) {
    powerFailDetected = true;
    digitalWrite(PIN_AMP_MUTE, HIGH);  // GPIO direct, pas I2C!
}
```

## State Machine

```
┌─────────────────────────────────────────────────────────────────┐
│                        AMP STATES                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│    ┌─────┐  startup()   ┌──────────┐  run ok   ┌─────────┐    │
│    │ OFF │────────────->│ STARTING │─────────->│ RUNNING │    │
│    └─────┘              └──────────┘           └────┬────┘    │
│       ^                                             │          │
│       │                                    temp>50  │          │
│       │                                             v          │
│       │                              ┌─────────────────────┐   │
│       │                              │  THERMAL_WARNING    │   │
│       │                              │  (volume -20%)      │   │
│       │                              └──────────┬──────────┘   │
│       │                                         │ temp>65     │
│       │                                         v              │
│       │                              ┌─────────────────────┐   │
│       │                              │  THERMAL_CRITICAL   │   │
│       │                              │  (volume -50%)      │   │
│       │                              └──────────┬──────────┘   │
│       │                                         │ temp>75     │
│       │ shutdown()                              │ OR button   │
│       │                                         v              │
│       │                              ┌─────────────────────┐   │
│       └──────────────────────────────│     SHUTDOWN        │   │
│                                      │ MUTE->EN->RELAY     │   │
│                                      └─────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Thermal Protection

### NTC Calculation (10k B3950)

```c
float readTemperature(void) {
    uint16_t adc = medianFilter(adcNtcBuffer, 5);
    float voltage = adc * 3.3f / 4095.0f;
    
    // Protection division par zero
    if (voltage < 0.1f) return TEMP_FAIL_HIGH;  // Court-circuit
    if (voltage > 3.2f) return TEMP_FAIL_LOW;   // Deconnecte
    
    // Resistance NTC
    float R_ntc = 10000.0f * voltage / (3.3f - voltage);
    
    // Steinhart-Hart (Beta simplifiee)
    float steinhart = R_ntc / 10000.0f;
    steinhart = log(steinhart);
    steinhart /= 3950.0f;
    steinhart += 1.0f / (25.0f + 273.15f);
    steinhart = 1.0f / steinhart;
    steinhart -= 273.15f;
    
    return steinhart;
}
```

### Fail-Safe [V1.9]

```c
// Si NTC defaillant: volume limite a 50%
if (tempStatus.isFailed && volume > VOLUME_FAILSAFE) {
    volume = VOLUME_FAILSAFE;
    Serial.println("NTC HS - Volume limite 50%");
}
```

## Median Filter [V1.9]

```c
// Rejette les spikes ADC (bruit EMI)
uint16_t medianFilter(uint16_t* buffer, uint8_t size) {
    uint16_t sorted[5];
    memcpy(sorted, buffer, size * sizeof(uint16_t));
    
    // Bubble sort (petit tableau)
    for (uint8_t i = 0; i < size - 1; i++) {
        for (uint8_t j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    
    return sorted[size / 2];  // Valeur mediane
}
```

## Encoder with Critical Section [V1.9]

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

## TDA7439 Registers

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

## Watchdog

```c
// Reset watchdog dans loop()
void loop() {
    esp_task_wdt_reset();
    
    // ... traitement ...
    
    delay(10);
}

// Configuration (setup)
esp_task_wdt_init(5, true);  // 5 secondes, panic si expire
esp_task_wdt_add(NULL);      // Ajouter tache courante
```

## Serial Debug Output

```
=== AMPLI AUDIOPHILE V1.10 ===
Corrections: Level Shifter I2C, Anti-Plop, Molex

GPIO: Initialise
I2C: Scan bus...
I2C: OLED trouve @ 0x3C
I2C: TDA7439 trouve @ 0x44
I2C: MA12070 trouve @ 0x20
TDA7439: Initialise
MA12070: Initialise
Relay: Active
Startup: Complete

Temp: 28.5C
Batt: 22.1V
Volume: 45

[Button press]
Shutdown: Debut sequence...
Shutdown: MUTE active
Shutdown: Ampli desactive
Shutdown: Relay coupe
Shutdown: Complete
```

## Build Configuration

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

## Changelog V1.10

| Fix | Description |
|-----|-------------|
| [A1] | Sequence extinction MUTE->EN->RELAY |
| [A2] | ISR POWER_FAIL pour MUTE immediat |
| [A3] | Timeout I2C augmente (50ms) pour level shifter |
| [A4] | GPIO8 pour detection coupure |

## Changelog V1.9

| Fix | Description |
|-----|-------------|
| [S1] | I2C recovery open-drain (INPUT pas OUTPUT+HIGH) |
| [D1] | NTC fail-safe: volume 50% max si capteur HS |
| [D2] | Flag i2cHardwareFault |
| [P0] | Filtre median ADC (5 samples) |
| [P1] | Sections critiques encoder (portMUX) |
| [P2] | Compteur erreurs I2C |
