# FIRMWARE - AMPLIFICATEUR AUDIOPHILE

## Informations

| Parametre | Valeur |
|-----------|--------|
| Version | 1.9 |
| MCU | ESP32-S3-WROOM-1-N8R8 |
| Framework | Arduino ESP32 |
| Date | 14 decembre 2025 |

## Fonctionnalites

- Controle volume digital (TDA7439 + MA12070)
- Egaliseur 3 bandes (+/-14dB)
- Selection source (Bluetooth/AUX/Phono)
- Monitoring batterie et temperature
- Protection thermique automatique
- Interface OLED + encodeur rotatif
- Telecommande IR
- Sauvegarde parametres (NVS)
- Watchdog 5 secondes

## Architecture

```
+------------------+
|    ESP32-S3      |
|                  |
|  +------------+  |     I2C Bus (400kHz)
|  |   Core 0   |  | ----------------------+
|  | Audio Task |  |                       |
|  +------------+  |     +--------+        |
|                  | <-> | MA12070| 0x20   | Ampli Class-D
|  +------------+  |     +--------+        |
|  |   Core 1   |  |                       |
|  | UI + Mon.  |  |     +--------+        |
|  +------------+  | <-> |TDA7439 | 0x44   | EQ + Volume
|                  |     +--------+        |
+------------------+                       |
       |                 +--------+        |
       | I2S             | OLED   | 0x3C   | Affichage
       v                 +--------+--------+
+------------------+
|    BTM525        |
|  Bluetooth LDAC  |
+------------------+
```

## Pinout ESP32-S3

### Communication

| GPIO | Fonction | Direction | Note |
|------|----------|-----------|------|
| 1 | SDA | I/O | I2C Data |
| 2 | SCL | OUT | I2C Clock |
| 3 | I2S_BCK | OUT | Bit Clock |
| 4 | I2S_WS | OUT | Word Select |
| 5 | I2S_DATA | IN | Audio Data |

### Interface utilisateur

| GPIO | Fonction | Direction | Note |
|------|----------|-----------|------|
| 18 | ENC_A | IN | Encodeur A (pull-up) |
| 19 | ENC_B | IN | Encodeur B (pull-up) |
| 20 | ENC_SW | IN | Bouton encodeur |
| 21 | IR_RX | IN | Recepteur IR |

### Controle ampli

| GPIO | Fonction | Direction | Note |
|------|----------|-----------|------|
| 38 | AMP_EN | OUT | Enable MA12070 |
| 39 | AMP_MUTE | OUT | Mute (actif HIGH) |
| 40 | AMP_ERR | IN | Erreur ampli |
| 41 | MUX_S0 | OUT | Selection source |
| 42 | SAFE_EN | OUT | Controle relais |

### ADC

| GPIO | Fonction | Note |
|------|----------|------|
| 6 | ADC_BATT | Diviseur 220k/33k |
| 7 | ADC_NTC | Pull-up 10k |

## I2C Bus

### Adresses

| Device | Adresse | Fonction |
|--------|---------|----------|
| MA12070 | 0x20 | Amplificateur Class-D |
| SSD1306 | 0x3C | OLED 128x64 |
| TDA7439 | 0x44 | Processeur audio |

### Recovery I2C (V1.9)

```c
// CORRECTION CRITIQUE V1.9
// Avant (BUG): pinMode(SDA, OUTPUT); digitalWrite(SDA, HIGH);
// Apres (OK):  pinMode(SDA, INPUT);  // Open-drain correct

bool i2cRecovery(void) {
    Wire.end();
    
    pinMode(PIN_SCL, OUTPUT);
    pinMode(PIN_SDA, INPUT);  // Relache ligne (pull-up tire HIGH)
    
    // 9 clock pulses
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(PIN_SCL, HIGH);
        delayMicroseconds(5);
    }
    
    // Condition STOP
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(PIN_SDA, LOW);
    digitalWrite(PIN_SCL, HIGH);
    pinMode(PIN_SDA, INPUT);  // Relache pour STOP
    
    Wire.begin(PIN_SDA, PIN_SCL);
    return true;
}
```

## Protections

### NTC Fail-safe (V1.9)

```c
// Si NTC defaillant (deconnecte ou court-circuit)
// Volume limite a 50% pour securite thermique

#define VOLUME_FAILSAFE 50

if (!tempStatus.isValid) {
    ntcFailsafeActive = true;
    if (currentVolume > VOLUME_FAILSAFE) {
        setVolume(VOLUME_FAILSAFE);
    }
}
```

### Seuils Temperature

| Seuil | Valeur | Action |
|-------|--------|--------|
| Warning | 50C | Alerte affichage |
| Critical | 65C | Reduction volume 20% |
| Shutdown | 75C | Arret d'urgence |
| Fail Low | -20C | NTC deconnecte |
| Fail High | 120C | NTC court-circuit |

### Watchdog

```c
#define WDT_TIMEOUT_SEC 5

void initWatchdog(void) {
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
    esp_task_wdt_add(NULL);
}

void loop() {
    esp_task_wdt_reset();  // Reset a chaque iteration
    // ...
}
```

## API Fonctions Principales

### Volume

```c
void setVolume(uint8_t vol);  // 0-100
// Applique aux deux etages (TDA7439 + MA12070)
// Respecte limite failsafe si NTC defaillant
```

### Source

```c
typedef enum {
    SOURCE_BLUETOOTH = 0,
    SOURCE_AUX,
    SOURCE_PHONO
} AudioSource_t;

void setSource(AudioSource_t source);
// Mute pendant changement
// Configure MUX analogique
```

### Egaliseur

```c
void setTDA7439Bass(int8_t db);    // -14 to +14
void setTDA7439Mid(int8_t db);     // -14 to +14
void setTDA7439Treble(int8_t db);  // -14 to +14
```

### Monitoring

```c
void readBattery(void);     // Met a jour batteryStatus
void readTemperature(void); // Met a jour tempStatus

// Structures status
BatteryStatus_t batteryStatus;  // voltage, percent, isLow, isCritical
TempStatus_t tempStatus;        // celsius, isValid, isWarning, isCritical
```

## Compilation

### Prerequis

- Arduino IDE 2.x ou PlatformIO
- Board: ESP32-S3 Dev Module
- Partition: Default 4MB with spiffs

### Bibliotheques

```
- Wire.h (inclus)
- driver/i2s.h (inclus ESP-IDF)
- esp_task_wdt.h (inclus ESP-IDF)
- Preferences.h (inclus)
```

### Configuration Arduino IDE

```
Board: ESP32S3 Dev Module
USB CDC On Boot: Enabled
Flash Size: 8MB
Partition Scheme: Default 4MB with spiffs
PSRAM: OPI PSRAM
```

## Debuggage

### Serial Monitor

```
Baudrate: 115200

Messages:
"I2C Recovery: Debut sequence..."
"I2C Recovery: SDA libre apres X clocks"
"I2C Recovery: SUCCES/ECHEC"
"NTC: DEFAILLANCE DETECTEE"
"THERMAL WARNING: XX.XC"
"THERMAL SHUTDOWN: XX.XC"
"!!! EMERGENCY SHUTDOWN !!!"
```

### LED Status

| Etat | Signification |
|------|---------------|
| Vert fixe | Fonctionnement normal |
| Vert clignotant | Bluetooth en attente |
| Orange | Avertissement temperature |
| Rouge | Erreur (voir OLED) |

## Changelog Firmware

| Version | Modifications |
|---------|---------------|
| V1.9 | I2C open-drain fix, NTC fail-safe, filtre median ADC |
| V1.8 | I2C recovery, NTC monitoring, watchdog |
| V1.7 | Integration TDA7439, multi-sources |
| V1.6 | Sauvegarde NVS, IR remote |
| V1.5 | OLED display, encodeur |

## Fichiers

```
Firmware_Ampli_V1_9.ino    # Code principal (1119 lignes)
```

## Licence

Code open-source.

---

*Firmware ESP32-S3 pour amplificateur audiophile portable*
