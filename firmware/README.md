# 💾 Documentation Firmware — Ampli Audiophile V1.7

> Documentation technique complète du firmware ESP32-S3 de l'amplificateur audiophile portable.

---

## 📋 Table des Matières

1. [Vue d'Ensemble](#vue-densemble)
2. [Installation](#installation)
3. [Architecture Logicielle](#architecture-logicielle)
4. [Configuration Matérielle](#configuration-matérielle)
5. [Fonctionnalités](#fonctionnalités)
6. [API et Registres](#api-et-registres)
7. [Corrections V1.7](#corrections-v17)
8. [Debug et Monitoring](#debug-et-monitoring)
9. [Commandes Série](#commandes-série)

---

## Vue d'Ensemble

### Informations Générales

| Paramètre | Valeur |
|-----------|--------|
| **Version** | 1.7 |
| **Date** | 13 décembre 2025 |
| **MCU** | ESP32-S3-WROOM-1-N8R8 |
| **Framework** | Arduino ESP32 Core 2.0+ |
| **Flash** | 8 MB |
| **PSRAM** | 8 MB |
| **Taille code** | ~1820 lignes |

### Changelog Résumé

| Version | Modifications clés |
|---------|-------------------|
| **V1.7** | Audit ChatGPT : esp_timer dans ISR, I2C bus recovery au boot |
| V1.6 | Audit exhaustif fiabilité : shutdown sécurisé, anti-spam encodeur, validation NTC, pré-brownout |
| V1.5 | Audit Gemini : I2C timeout 10ms, support PVDD protection |
| V1.4 | Audit Copilot : filtre médian ADC, section critique encodeur, I2C retry, WDT 5s |
| V1.3 | TDA7439 EQ 3 bandes, loudness, spatial, 8 presets |
| V1.2 | Pinouts explicites, support nappe 16 pins |
| V1.1 | Sécurité batterie 5 niveaux |
| V1.0 | Version initiale |

---

## Installation

### Prérequis

- **Arduino IDE** 2.x ou **PlatformIO**
- **ESP32 Board Package** v2.0.0 ou supérieur
- Bibliothèques requises (voir ci-dessous)

### Bibliothèques Requises

```
Adafruit GFX Library          @ ^1.11.0
Adafruit SSD1306              @ ^2.5.0
IRremoteESP8266               @ ^2.8.0
Preferences                   (inclus ESP32 core)
Wire                          (inclus ESP32 core)
SPI                           (inclus ESP32 core)
esp_timer                     (inclus ESP32 core)  // [V1.7]
```

### Installation Arduino IDE

1. **Ajouter le gestionnaire de cartes ESP32 :**
   ```
   Fichier → Préférences → URL de gestionnaire de cartes supplémentaires
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

2. **Installer ESP32 :**
   ```
   Outils → Type de carte → Gestionnaire de cartes
   Rechercher "ESP32" → Installer "esp32 by Espressif Systems"
   ```

3. **Installer les bibliothèques :**
   ```
   Croquis → Inclure une bibliothèque → Gérer les bibliothèques
   Installer : Adafruit GFX, Adafruit SSD1306, IRremoteESP8266
   ```

4. **Configurer la carte :**

| Paramètre | Valeur |
|-----------|--------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| Flash Size | 8MB (64Mb) |
| Partition Scheme | Default 4MB with spiffs |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600 |

5. **Upload :**
   ```
   Connecter ESP32-S3 via USB
   Sélectionner le port COM
   Cliquer Upload
   ```

---

## Architecture Logicielle

### Diagramme de Flux

```
┌─────────────────────────────────────────────────────────────┐
│                         BOOT                                │
├─────────────────────────────────────────────────────────────┤
│  1. Serial.begin(115200)                                    │
│  2. i2cBusRecovery()        [V1.7] Récupération bus         │
│  3. Wire.begin() + setTimeOut(10)                           │
│  4. loadSettings() NVS                                      │
│  5. initDisplay()                                           │
│  6. scanI2C() → détection périphériques                     │
│  7. initMA12070()                                           │
│  8. initTDA7439()                                           │
│  9. attachInterrupt() encodeur + bouton                     │
│ 10. esp_task_wdt_init(5s)                                   │
│ 11. systemReady = true                                      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                       LOOP (1ms)                            │
├─────────────────────────────────────────────────────────────┤
│  • esp_task_wdt_reset()                                     │
│  • handleSerialCommand()                                    │
│  • handleIR()                                               │
│  • handleEncoder()                                          │
│  • updateVolumeFade()                                       │
│  • updateVUMeter()                                          │
│  • updateMonitoring() → batterie, température               │
│  • updateDisplay()                                          │
│  • checkMenuTimeout()                                       │
│  • checkSleepTimer()                                        │
│  • checkAutoSleep()                                         │
│  • checkAutoSave()                                          │
└─────────────────────────────────────────────────────────────┘
```

### Interruptions (ISR)

| ISR | GPIO | Trigger | Fonction |
|-----|------|---------|----------|
| `encoderISR()` | 6, 7 | CHANGE | Rotation encodeur |
| `buttonISR()` | 15 | FALLING | Appui bouton |

**Note V1.7 :** Les ISR utilisent `esp_timer_get_time()` au lieu de `millis()` pour le timing.

---

## Configuration Matérielle

### Pinout GPIO

```cpp
// I2C
#define PIN_SDA         1
#define PIN_SCL         2

// SPI (Volume backup)
#define PIN_SPI_CS_VOL  10

// ADC
#define PIN_ADC_BATT    4   // Diviseur 1:6
#define PIN_ADC_NTC     5   // Thermistance

// Encodeur
#define PIN_ENC_A       6
#define PIN_ENC_B       7
#define PIN_ENC_SW      15

// IR
#define PIN_IR_RECV     16

// Contrôle ampli
#define PIN_MA_MUTE     40
#define PIN_MA_EN       41
#define PIN_RELAY       42

// Sélecteur source
#define PIN_MUX_A       11
#define PIN_MUX_B       12
#define PIN_MUX_INH     13
```

### Adresses I2C

| Périphérique | Adresse | Notes |
|--------------|---------|-------|
| MA12070 | 0x20 | Ampli Class-D |
| TDA7439 | 0x44 | EQ Audio |
| SSD1306 | 0x3C | OLED 128×64 |

---

## Fonctionnalités

### Sources Audio

| ID | Source | Sélection MUX |
|----|--------|---------------|
| 0 | Bluetooth | A=0, B=0 |
| 1 | AUX | A=1, B=0 |
| 2 | Phono | A=0, B=1 |

### Égaliseur TDA7439

| Bande | Fréquence | Plage |
|-------|-----------|-------|
| Bass | 100 Hz | ±14 dB |
| Mid | 1 kHz | ±14 dB |
| Treble | 10 kHz | ±14 dB |

**Presets :**

| ID | Nom | Bass | Mid | Treble |
|----|-----|------|-----|--------|
| 0 | Flat | 0 | 0 | 0 |
| 1 | Bass+ | +6 | 0 | 0 |
| 2 | Vocal | -2 | +4 | +2 |
| 3 | Rock | +4 | -2 | +4 |
| 4 | Jazz | +3 | 0 | +3 |
| 5 | Cinema | +5 | +2 | +1 |
| 6 | Live | +2 | +1 | +3 |
| 7 | Custom | User | User | User |

### Loudness Automatique

Compensation Fletcher-Munson active à bas volume :

```cpp
// Boost basses progressif selon volume
if (volume < 30) {
  bassBoost = map(volume, 0, 30, 8, 0);  // +8dB @ vol=0, 0dB @ vol=30
  applyLoudness(bassBoost);
}
```

### Gestion Batterie

| Seuil | Tension | Action |
|-------|---------|--------|
| FULL | > 24.5V | Affichage 100% |
| NOMINAL | 20-24.5V | Fonctionnement normal |
| LOW | < 20V | Avertissement |
| CRITICAL | < 18.5V | Extinction auto |

### Sleep Mode

| Mode | Condition | Consommation |
|------|-----------|--------------|
| Actif | Normal | ~50mA |
| Sleep léger | 5 min inactivité | ~10mA |
| Deep sleep | Batterie critique | < 1mA |

---

## API et Registres

### MA12070 (I2C 0x20)

```cpp
// Registres principaux
#define MA_REG_POWER      0x00  // Power mode
#define MA_REG_VOL_L      0x40  // Volume gauche
#define MA_REG_VOL_R      0x41  // Volume droit
#define MA_REG_MUTE       0x42  // Mute control
#define MA_REG_CONFIG     0x50  // Configuration

// Fonctions
void ma12070_setVolume(uint8_t vol);  // 0-255
void ma12070_mute(bool mute);
void ma12070_enable(bool en);
```

### TDA7439 (I2C 0x44)

```cpp
// Registres
#define TDA_REG_INPUT     0x00  // Sélection entrée
#define TDA_REG_GAIN      0x01  // Gain input
#define TDA_REG_VOL       0x02  // Volume master
#define TDA_REG_BASS      0x03  // Bass ±14dB
#define TDA_REG_MID       0x04  // Mid ±14dB
#define TDA_REG_TREBLE    0x05  // Treble ±14dB
#define TDA_REG_BALANCE_R 0x06  // Balance droite
#define TDA_REG_BALANCE_L 0x07  // Balance gauche

// Fonctions
void tda7439_setEQ(int8_t bass, int8_t mid, int8_t treble);
void tda7439_setVolume(uint8_t vol);
void tda7439_setInput(uint8_t input);
```

---

## Corrections V1.7

### [C1] ISR Timing avec esp_timer

**Problème V1.6 :** `millis()` peut être imprécis dans les ISR sur ESP32 car elle dépend de FreeRTOS tick counter.

**Solution V1.7 :** Utiliser `esp_timer_get_time()` qui lit directement le compteur hardware 64-bit.

```cpp
#include <esp_timer.h>

// Helper pour obtenir ms depuis esp_timer (µs → ms)
static inline uint32_t IRAM_ATTR getMillisISR() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void IRAM_ATTR encoderISR() {
  // [C1] V1.7: Utilise esp_timer au lieu de millis()
  uint32_t now = getMillisISR();
  
  if (now - lastEncoderTime > 2) {
    // ... traitement encodeur ...
    lastEncoderTime = now;
  }
}

void IRAM_ATTR buttonISR() {
  // [C1] V1.7: Utilise esp_timer au lieu de millis()
  uint32_t now = getMillisISR();
  
  if (now - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}
```

**Avantages :**
- Lecture directe compteur hardware (pas de scheduler overhead)
- Résolution microseconde
- Fiable dans contexte ISR
- Pas d'impact sur la latence d'interruption

---

### [C2] I2C Bus Recovery

**Problème :** Si un périphérique I2C reste bloqué (SDA LOW), le bus devient inutilisable.

**Solution V1.7 :** Procédure de récupération au boot selon NXP AN10216-01.

```cpp
#define I2C_RECOVERY_CLOCKS 9

void i2cBusRecovery() {
  debugLog("[C2] I2C Bus Recovery...");
  
  // Configurer les pins en GPIO
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, OUTPUT);
  
  // Vérifier si SDA est bloqué LOW
  if (digitalRead(PIN_SDA) == LOW) {
    debugLog("SDA bloqué LOW, envoi clocks recovery");
    
    // Envoyer 9 clocks SCL pour libérer SDA
    for (int i = 0; i < I2C_RECOVERY_CLOCKS; i++) {
      digitalWrite(PIN_SCL, LOW);
      delayMicroseconds(5);
      digitalWrite(PIN_SCL, HIGH);
      delayMicroseconds(5);
      
      // Vérifier si SDA est libéré
      if (digitalRead(PIN_SDA) == HIGH) {
        debugLog("SDA libéré après %d clocks", i + 1);
        break;
      }
    }
    
    // Générer condition STOP (SDA LOW→HIGH pendant SCL HIGH)
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(PIN_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_SDA, HIGH);
    delayMicroseconds(5);
    
    stats.i2cRecoveries++;
    debugLog("I2C recovery terminé");
  } else {
    debugLog("SDA OK, pas de recovery nécessaire");
  }
  
  // Remettre les pins en mode I2C
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, INPUT);
}
```

**Appel dans setup() :**

```cpp
void setup() {
  Serial.begin(115200);
  
  // [C2] V1.7: I2C Bus Recovery AVANT Wire.begin()
  i2cBusRecovery();
  
  // Maintenant initialiser I2C normalement
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setTimeOut(10);  // V1.5: Timeout 10ms
  
  // ... reste du setup ...
}
```

**Quand c'est utile :**
- Après un reset pendant une transaction I2C
- Périphérique défaillant qui maintient SDA
- Perturbations EMI ayant corrompu le bus
- Démarrage à froid avec esclave dans état inconnu

---

### Corrections Héritées (V1.6)

| Tag | Correction | Description |
|-----|------------|-------------|
| [A1] | emergencyShutdown() | detachInterrupt() EN PREMIER |
| [A2] | Encodeur anti-spam | Saturation ±5 pas/cycle |
| [A3] | NTC validation | Détection déconnexion/CC |
| [A4] | ADC overflow | Check > 4095 |
| [A5] | I2C backoff | 10+20+40ms (70ms total) |
| [A6] | Pré-brownout | Sauvegarde avant coupure BMS |
| [A7] | NVS corruption | Mode dégradé si erreur |
| [A8] | Shutdown séquence | Mute→Disable→Save→Display |

---

## Debug et Monitoring

### Mode Debug

Activer via commande série :
```
debug on
```

### Logs Disponibles

```
[12345] Boot V1.7
[12346] [C2] I2C Bus Recovery...
[12346] SDA OK, pas de recovery nécessaire
[12350] I2C scan: MA12070@0x20, TDA7439@0x44, OLED@0x3C
[12400] Système prêt! V1.7 Audit ChatGPT
```

### Statistiques Runtime

```
stats
```

Affiche :
```
=== STATISTIQUES ===
Uptime: 3h 24m 15s
Loop count: 12345678
I2C errors: 2
I2C retries: 5
I2C recoveries: 1 [V1.7]
ADC overflows: 0
NVS writes: 23
WDT resets: 0
```

---

## Commandes Série

| Commande | Description |
|----------|-------------|
| `help` | Liste des commandes |
| `status` | État système complet |
| `stats` | Statistiques runtime |
| `debug on/off` | Mode debug |
| `vol [0-100]` | Régler volume |
| `mute` | Toggle mute |
| `source [0-2]` | Changer source |
| `eq bass/mid/treble [±14]` | Régler EQ |
| `preset [0-7]` | Charger preset |
| `save` | Sauvegarder settings |
| `reset` | Reset factory |
| `i2c scan` | Scanner bus I2C |
| `i2c recovery` | Forcer recovery [V1.7] |
| `reboot` | Redémarrer |

---

## Sécurité et Robustesse

### Watchdog

- Timeout : 5 secondes
- Reset automatique si loop() bloquée
- Désactivé pendant flash OTA

### Protection I2C

```cpp
bool i2cWriteWithRetry(uint8_t addr, uint8_t reg, uint8_t val) {
  for (int attempt = 0; attempt < 3; attempt++) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    
    if (Wire.endTransmission() == 0) {
      return true;  // Succès
    }
    
    // [A5] Backoff exponentiel
    delay(10 * (1 << attempt));  // 10, 20, 40ms
    stats.i2cRetries++;
  }
  
  stats.i2cErrors++;
  return false;
}
```

### Emergency Shutdown (V1.6+)

```cpp
void emergencyShutdown() {
  // [A1] CRITIQUE: Désactiver interruptions EN PREMIER
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_A));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_B));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_SW));
  
  // Mute immédiat via GPIO (pas I2C)
  digitalWrite(PIN_MA_MUTE, LOW);
  
  // Désactiver ampli
  digitalWrite(PIN_MA_EN, LOW);
  
  // Couper relais principal
  digitalWrite(PIN_RELAY, LOW);
  
  // Tenter sauvegarde (peut échouer si brownout)
  saveSettings();
  
  // Afficher état
  displayShutdown();
  
  // Deep sleep
  esp_deep_sleep_start();
}
```

---

## Historique Versions Firmware

| Version | Date | Lignes | Modifications |
|---------|------|--------|---------------|
| **V1.7** | 13/12/2025 | 1820 | esp_timer ISR, I2C recovery |
| V1.6 | 13/12/2025 | 1798 | Shutdown sécurisé, anti-spam, NTC valid |
| V1.5 | 13/12/2025 | 1750 | I2C timeout, PVDD support |
| V1.4 | 13/12/2025 | 1700 | Filtre médian, section critique |
| V1.3 | 12/12/2025 | 1600 | TDA7439 EQ complet |
| V1.0-1.2 | 11-12/12/2025 | 1400 | Base fonctionnelle |

---

<p align="center">
  <b>💾 Documentation Firmware V1.7 — Audit ChatGPT</b>
</p>
