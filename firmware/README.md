# 💾 Documentation Firmware — Ampli Audiophile V1.6

> Documentation technique complète du firmware ESP32-S3 de l'amplificateur audiophile portable.

---

## 📋 Table des Matières

1. [Vue d'Ensemble](#vue-densemble)
2. [Installation](#installation)
3. [Architecture Logicielle](#architecture-logicielle)
4. [Configuration Matérielle](#configuration-matérielle)
5. [Fonctionnalités](#fonctionnalités)
6. [API et Registres](#api-et-registres)
7. [Corrections V1.6](#corrections-v16)
8. [Debug et Monitoring](#debug-et-monitoring)
9. [Commandes Série](#commandes-série)

---

## Vue d'Ensemble

### Informations Générales

| Paramètre | Valeur |
|-----------|--------|
| **Version** | 1.6 |
| **Date** | 13 décembre 2025 |
| **MCU** | ESP32-S3-WROOM-1-N8R8 |
| **Framework** | Arduino ESP32 Core 2.0+ |
| **Flash** | 8 MB |
| **PSRAM** | 8 MB |
| **Taille code** | ~1800 lignes |

### Changelog Résumé

| Version | Modifications clés |
|---------|-------------------|
| **V1.6** | Audit exhaustif fiabilité : shutdown sécurisé, anti-spam encodeur, validation NTC, pré-brownout |
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
   Ouvrir Firmware_V1_6.ino
   Croquis → Téléverser
   ```

### Installation PlatformIO

```ini
; platformio.ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit GFX Library@^1.11.0
    adafruit/Adafruit SSD1306@^2.5.0
    crankyoldgit/IRremoteESP8266@^2.8.0
```

---

## Architecture Logicielle

### Structure du Code

```
Firmware_V1_6.ino
├── INCLUDES
├── VERSION ET IDENTIFICATION
├── CONFIGURATION PINS GPIO
├── CONFIGURATION PÉRIPHÉRIQUES
│   ├── OLED
│   ├── MA12070
│   ├── TDA7439
│   └── MCP4261 (backup)
├── SEUILS BATTERIE [V1.6: BATT_CRITICAL ajouté]
├── SEUILS TEMPÉRATURE [V1.6: NTC validation]
├── CONFIGURATION V1.6
│   ├── Anti-spam encodeur
│   ├── ADC validation
│   ├── I2C backoff exponentiel
│   ├── Pré-brownout
│   └── NVS robustesse
├── STRUCTURES DE DONNÉES
│   ├── Equalizer
│   ├── Settings
│   ├── Stats [V1.6: champs étendus]
│   └── VUMeter
├── PRESETS ÉGALISEUR
├── VARIABLES D'ÉTAT
├── ISR (Interruptions)
│   ├── encoderISR() [V1.6: anti-spam]
│   └── buttonISR()
├── FONCTIONS ADC [V1.6: validation overflow]
├── FONCTIONS I2C [V1.6: backoff exponentiel]
├── FONCTIONS TDA7439
├── FONCTIONS ÉGALISEUR
├── FONCTIONS NVS [V1.6: gestion corruption]
├── FONCTIONS AMPLI MA12070
├── FONCTIONS MONITORING [V1.6: NTC validation, pré-brownout]
├── EMERGENCY SHUTDOWN [V1.6: refonte complète]
├── FONCTIONS DISPLAY
├── HANDLERS (Encodeur, IR, Serial)
├── SETUP
└── LOOP
```

### Flux Principal

```
setup()
    ├── Init GPIO
    ├── Init I2C (400kHz, timeout 10ms)
    ├── Init OLED
    ├── Init NVS + Load Settings
    ├── Splash Screen
    ├── Init TDA7439
    ├── Init IR
    ├── Connect Battery
    ├── Check Battery Level
    ├── Init MA12070
    ├── Attach Interrupts
    ├── Enable Amp
    └── Apply EQ

loop()
    ├── Reset Watchdog (5s)
    ├── Handle Serial Commands
    ├── Handle IR
    ├── Handle Encoder [V1.6: anti-spam]
    ├── Update Volume Fade
    ├── Update VU Meter
    ├── Update Monitoring [V1.6: NTC + brownout]
    ├── Update Display
    ├── Check Timeouts (menu, sleep, auto-save)
    └── delay(1ms)
```

---

## Configuration Matérielle

### Assignation GPIO

| GPIO | Fonction | Direction | Périphérique |
|------|----------|-----------|--------------|
| 1 | I2C_SDA | Bidir | MA12070, OLED, TDA7439 |
| 2 | I2C_SCL | Sortie | I2C Bus |
| 4 | BT_STATUS | Entrée | BTM525 |
| 5 | SRC_SEL0 | Sortie | CD4053 |
| 6 | SRC_SEL1 | Sortie | CD4053 |
| 7 | BT_RESET | Sortie | BTM525 |
| 15 | AMP_EN | Sortie | MA12070 /EN |
| 16 | AMP_MUTE | Sortie | MA12070 /MUTE |
| 17 | AMP_ERR | Entrée | MA12070 /ERR |
| 18 | ENC_A | Entrée | Encodeur |
| 19 | ENC_B | Entrée | Encodeur |
| 20 | ENC_SW | Entrée | Encodeur bouton |
| 21 | IR_RX | Entrée | Récepteur IR |
| 38 | ADC_BATT | ADC | Diviseur batterie |
| 39 | ADC_NTC | ADC | Diviseur NTC |
| 40 | ADC_AUDIO_L | ADC | VU-mètre L |
| 41 | ADC_AUDIO_R | ADC | VU-mètre R |
| 42 | SAFE_EN | Sortie | PC817 → Relais |
| 48 | LED_STATUS | Sortie | LED façade |

### Adresses I2C

| Périphérique | Adresse 7-bit | Adresse 8-bit |
|--------------|---------------|---------------|
| OLED SSD1306 | 0x3C | 0x78 |
| MA12070 | 0x20 | 0x40 |
| TDA7439 | 0x44 | 0x88 |

---

## Fonctionnalités

### Gestion Volume

| Paramètre | Valeur |
|-----------|--------|
| Plage | 0 à 47 (-47dB à 0dB) |
| Pas encodeur | 1dB |
| Pas IR | 2dB |
| Fade | 15ms par pas |
| Limite configurable | Oui |

### Égaliseur TDA7439

| Bande | Fréquence | Plage | Pas |
|-------|-----------|-------|-----|
| Bass | 100 Hz | ±14dB | 2dB |
| Mid | 1 kHz | ±14dB | 2dB |
| Treble | 10 kHz | ±14dB | 2dB |

### Presets Audio

| # | Nom | Bass | Mid | Treble |
|---|-----|------|-----|--------|
| 0 | Flat | 0dB | 0dB | 0dB |
| 1 | Bass+ | +10dB | -2dB | 0dB |
| 2 | Vocal | -4dB | +4dB | +6dB |
| 3 | Rock | +6dB | 0dB | +6dB |
| 4 | Jazz | +4dB | +2dB | +4dB |
| 5 | Cinema | +8dB | 0dB | +2dB |
| 6 | Live | +2dB | +4dB | +4dB |
| 7 | Custom | User | User | User |

### Loudness Automatique

Compensation Fletcher-Munson activée sous le seuil volume 30% :
- Boost bass progressif (+2dB à +6dB)
- Légère atténuation mid (-2dB) si boost > 4dB

### Sources Audio

| # | Source | Sélection |
|---|--------|-----------|
| 0 | Bluetooth | SEL0=LOW, SEL1=LOW, TDA IN1 |
| 1 | AUX | SEL0=HIGH, SEL1=LOW, TDA IN1 |
| 2 | Phono | SEL0=LOW, SEL1=HIGH, TDA IN2 |

---

## API et Registres

### TDA7439 — Registres

| Sub-Address | Fonction | Valeurs |
|-------------|----------|---------|
| 0x00 | Input Select | 0-3 (IN4-IN1) |
| 0x01 | Input Gain | 0-15 (0-30dB) |
| 0x02 | Volume | 0-48 (0 to -47dB, 48=mute) |
| 0x03 | Bass | 0-14 (-14dB à +14dB) |
| 0x04 | Mid | 0-14 (-14dB à +14dB) |
| 0x05 | Treble | 0-14 (-14dB à +14dB) |
| 0x06 | Speaker Att R | 0-79 (0 à -79dB) |
| 0x07 | Speaker Att L | 0-79 (0 à -79dB) |

**Note :** Pour EQ, valeur registre = 14 - dB_voulu (7 = 0dB flat)

### MA12070 — Registres Principaux

| Adresse | Fonction |
|---------|----------|
| 0x35 | Mode I2S |
| 0x40 | Volume Master |

### Fonctions API Principales

```cpp
// TDA7439
bool tda7439Detect();
void tda7439Init();
void tda7439SetInput(uint8_t input);      // 0-3
void tda7439SetInputGain(uint8_t gain);   // 0-15
void tda7439SetVolume(uint8_t vol);       // 0-48
void tda7439SetBass(uint8_t value);       // 0-14
void tda7439SetMid(uint8_t value);        // 0-14
void tda7439SetTreble(uint8_t value);     // 0-14
void eqApplyPreset(uint8_t presetIndex);  // 0-7
void eqApplyWithLoudness();

// Ampli
void ampInit();
void ampEnable(bool enable);
void ampSetMute(bool mute);
void ampToggleMute();

// Système
void emergencyShutdown(const char* reason);
void saveSettings();
void saveStats();
void batteryConnect(bool connect);
```

---

## Corrections V1.6

### [A1] Emergency Shutdown Sécurisé

**Problème :** Race condition — ISR continuaient pendant delay(3000) du shutdown

**Solution :**
```cpp
void emergencyShutdown(const char* reason) {
  // ÉTAPE 1: Désactiver ISR EN PREMIER
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_A));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_B));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_SW));
  
  // ÉTAPE 2: GPIO direct (pas I2C)
  digitalWrite(PIN_AMP_MUTE, LOW);   // Mute immédiat
  digitalWrite(PIN_AMP_EN, HIGH);    // Disable ampli
  
  // ÉTAPE 3: Sauvegarde NVS (maintenant sécurisé)
  saveSettings();
  delay(50);
  saveStats();
  
  // ÉTAPE 4: Display + Sleep
  // ...
}
```

### [A2] Encodeur Anti-Spam

**Problème :** Bruit électrique → accumulation illimitée → volume saute

**Solution :**
```cpp
void IRAM_ATTR encoderISR() {
  // ...
  portENTER_CRITICAL_ISR(&encoderMux);
  
  // Saturation anti-spam
  int32_t newDelta = encoderDelta + delta;
  encoderDelta = constrain(newDelta, -5, 5);  // Max ±5 pas/cycle
  
  portEXIT_CRITICAL_ISR(&encoderMux);
}
```

**Paramètres :**
```cpp
#define ENCODER_MAX_DELTA       5       // Max pas par cycle
#define ENCODER_CYCLE_MS        50      // Période traitement
```

### [A3] Validation NTC

**Problème :** NTC déconnectée → ADC flottant → lecture aléatoire

**Solution :**
```cpp
void checkTemperature() {
  tempRaw = readADCFiltered(PIN_ADC_NTC);
  
  // Validation plages
  if (tempRaw < 100) {           // Court-circuit NTC
    emergencyShutdown("NTC CC");
    return;
  }
  if (tempRaw > 3900) {          // NTC déconnectée
    emergencyShutdown("NTC OPEN");
    return;
  }
  
  // Traitement normal...
}
```

**Seuils :**
```cpp
#define NTC_SHORT_CIRCUIT   100     // ADC < 100
#define NTC_DISCONNECTED    3900    // ADC > 3900
```

### [A4] ADC Overflow Validation

**Problème :** ADC peut retourner valeurs > 4095 (overflow)

**Solution :**
```cpp
uint16_t readADCFiltered(uint8_t pin) {
  for (int i = 0; i < ADC_FILTER_SAMPLES; i++) {
    uint16_t raw = analogRead(pin);
    
    // Validation plage 12-bit
    if (raw > 4095) {
      raw = 4095;
      stats.adcSpikesFiltered++;
    }
    samples[i] = raw;
  }
  // Tri + médiane...
}
```

### [A5] I2C Backoff Exponentiel

**Problème :** Retry fixe inefficace si bus perturbé

**Solution :**
```cpp
bool i2cWriteWithRetry(uint8_t addr, uint8_t reg, uint8_t data) {
  uint16_t delayMs = 10;  // Base
  
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data);
    if (Wire.endTransmission() == 0) return true;
    
    delay(delayMs);
    delayMs *= 2;  // 10 → 20 → 40ms
  }
  return false;
}
```

**Délais :** 10ms → 20ms → 40ms = 70ms total max

### [A6] Pré-Brownout Detection

**Problème :** BMS coupe à 18.0V, pas le temps de sauvegarder NVS

**Solution :**
```cpp
#define BATT_CRITICAL   2700    // 18.2V (> BMS 18.0V)

void checkBattery() {
  if (batteryRaw < BATT_CRITICAL) {
    brownoutCounter++;
    
    if (brownoutCounter >= 3) {
      // Sauvegarde urgente AVANT coupure BMS
      saveSettings();
      delay(50);
      saveStats();
      emergencyShutdown("BATT CRITIQUE");
    }
  }
}
```

### [A7] NVS Corruption Handling

**Problème :** NVS corrompue → crash au démarrage

**Solution :**
```cpp
bool initNVS() {
  for (uint8_t i = 0; i < 3; i++) {
    if (preferences.begin("ampli", false)) {
      nvsInitialized = true;
      nvsDegraded = false;
      return true;
    }
    delay(100);
  }
  
  // Mode dégradé
  nvsInitialized = false;
  nvsDegraded = true;
  return false;
}
```

**Mode dégradé :** Valeurs par défaut, pas de sauvegarde, indicateur OLED "NVS!"

---

## Debug et Monitoring

### Structure Stats V1.6

```cpp
struct Stats {
  uint32_t totalOnTime;           // Temps total ON (secondes)
  uint32_t sessionStart;          // Timestamp début session
  uint16_t powerCycles;           // Nombre de démarrages
  uint16_t errorCount;            // Erreurs générales
  uint8_t maxTempReached;         // Température max atteinte
  uint16_t i2cErrors;             // Erreurs I2C
  uint16_t i2cRetries;            // Retries I2C
  uint16_t adcSpikesFiltered;     // Spikes ADC filtrés
  uint16_t ntcErrors;             // [V1.6] Erreurs NTC
  uint16_t encoderSpamFiltered;   // [V1.6] Spam encodeur filtré
  uint16_t brownoutWarnings;      // [V1.6] Alertes pré-brownout
};
```

### Indicateurs OLED

| Indicateur | Signification |
|------------|---------------|
| `TEMP!` | Throttle thermique activé |
| `I2C!` | Seuil erreurs I2C atteint (>10) |
| `NVS!` | Mode dégradé NVS |
| `NTC!` | Erreur capteur température |
| `%!` | Batterie basse |

---

## Commandes Série

Connecter via USB à 115200 baud.

| Commande | Description |
|----------|-------------|
| `help` | Liste des commandes |
| `stats` | Affiche statistiques V1.6 |
| `save` | Force sauvegarde NVS |
| `vol` | Affiche/modifie volume |
| `src` | Affiche/modifie source |
| `eq` | Affiche état égaliseur |
| `reset` | Reset paramètres défaut |
| `test` | Mode test (diagnostic) |

### Exemple Sortie `stats`

```
=== STATS V1.6 ===
Uptime: 3600s
Total: 125h
I2C errors: 3 retries: 12
ADC spikes: 5
NTC errors: 0
Encoder spam: 2
Brownout warnings: 0
NVS: OK
```

---

## Codes IR Télécommande

| Bouton | Code HEX | Action |
|--------|----------|--------|
| POWER | 0x00FF00FF | Toggle On/Off |
| MUTE | 0x00FF807F | Toggle Mute |
| VOL+ | 0x00FF40BF | Volume +2dB |
| VOL- | 0x00FFC03F | Volume -2dB |
| SOURCE | 0x00FF20DF | Cycle source |
| EQ | 0x00FF22DD | Menu EQ |
| LOUD | 0x00FF32CD | Toggle Loudness |

---

## Fichiers

| Fichier | Description |
|---------|-------------|
| `Firmware_V1_6.ino` | Code source complet |
| `libraries/` | Dépendances locales (optionnel) |

---

## Historique Versions Firmware

| Version | Lignes | Modifications |
|---------|--------|---------------|
| V1.6 | ~1800 | Audit fiabilité : shutdown, anti-spam, NTC, brownout |
| V1.5 | ~2900 | I2C timeout, support PVDD |
| V1.4 | ~2800 | Filtre médian, section critique, I2C retry, WDT |
| V1.3 | ~2500 | TDA7439 EQ, loudness, spatial |
| V1.2 | ~2000 | Support nappe 16 pins |
| V1.1 | ~1500 | Sécurité 5 niveaux |
| V1.0 | ~1200 | Version initiale |

---

## Troubleshooting

### Problème : OLED noir au démarrage

1. Vérifier alimentation 3.3V
2. Vérifier adresse I2C (0x3C)
3. Scanner I2C : `Wire.beginTransmission(0x3C); Wire.endTransmission();`

### Problème : TDA7439 non détecté

1. Vérifier alimentation 9V (LM7809)
2. Vérifier adresse I2C (0x44)
3. Pull-up I2C présents (4.7kΩ)

### Problème : Volume ne répond pas

1. Vérifier encodeur (pins 18, 19, 20)
2. Mode debug : `debugMode = true;`
3. Vérifier stats spam : commande `stats`

### Problème : Shutdown intempestif

1. Vérifier batterie (> 18V)
2. Vérifier NTC connectée (ADC 1000-3500)
3. Vérifier stats brownout : commande `stats`

---

<p align="center">
  <b>💾 Documentation Firmware V1.6</b>
</p>
