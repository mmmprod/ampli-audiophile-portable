# 💾 Firmware ESP32-S3

Ce dossier contient le firmware pour l'Amplificateur Audiophile Portable.

## 📋 Index des Firmwares

| Version | Fichier | Hardware requis | Statut |
|---------|---------|-----------------|--------|
| **V1.5** | [Firmware_Ampli_V1_5.ino](Firmware_Ampli_V1_5.ino) | Carte V1.5 (TDA7439 + protections) | ✅ **Recommandé** |
| V1.4 | [Firmware_Ampli_V1_4.ino](Firmware_Ampli_V1_4.ino) | Carte V1.4 (TDA7439) | 📦 Stable |
| V1.3 | [Ampli_V1_3.ino](Ampli_V1_3.ino) | Carte V1.3 (PT2314 + MCP4261) | 📦 Archive |

> ⚠️ **Important** : Utilisez le firmware correspondant à votre version hardware !

## 🆕 Nouveautés V1.5

| Amélioration | Description |
|--------------|-------------|
| **I2C Timeout** | `Wire.setTimeOut(10ms)` anti-blocage bus |
| **Documentation** | Commentaires protection PVDD et nappe 16 pins |
| **Compatibilité** | Support nappe 16 pins blindée |

*Note : La logique reste identique à V1.4. V1.5 ajoute robustesse et documentation.*

## ✨ Fonctionnalités

### Audio
- 🎵 **3 sources** : Bluetooth LDAC, AUX, Phono MM
- 🎛️ **Égaliseur 3 bandes** : Bass/Mid/Treble ±14dB (pas 2dB)
- 🔊 **Loudness automatique** : Compensation Fletcher-Munson selon volume
- 🎚️ **Effet spatial** : Élargissement stéréo ajustable
- 🎵 **8 presets** : Flat, Rock, Jazz, Classical, Pop, Bass Boost, Vocal, Custom

### Interface
- 📺 **OLED 128×64** : Menus, VU-mètre, animations
- 🎮 **Encodeur rotatif** : Navigation + volume
- 📡 **Télécommande IR** : Contrôle à distance
- 💾 **Sauvegarde EEPROM** : Paramètres persistants

### Sécurité
- 🔋 **Monitoring batterie** : ADC avec filtre médian anti-spike
- ⚡ **Watchdog 5s** : Redémarrage auto si freeze
- 🔒 **Section critique** : Atomicité lecture encodeur
- 🔄 **I2C retry** : 3 tentatives avec backoff
- ⏱️ **I2C timeout** : 10ms anti-blocage (V1.5)

## 🛠️ Installation

### Prérequis

1. **Arduino IDE 2.x** (ou PlatformIO)
2. **ESP32 Core 2.0+** : 
   - Dans Arduino IDE : `Fichier > Préférences > URLs de gestionnaire de cartes`
   - Ajouter : `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - `Outils > Gestionnaire de cartes > ESP32`

3. **Bibliothèques requises** (via Gestionnaire de bibliothèques) :
   ```
   Adafruit GFX Library
   Adafruit SSD1306
   IRremoteESP8266
   ```

### Configuration

1. Ouvrir le fichier `.ino` correspondant à votre hardware
2. `Outils > Type de carte > ESP32S3 Dev Module`
3. `Outils > USB CDC On Boot > Enabled` (pour Serial debug)
4. `Outils > Port > [Votre port COM]`

### Upload

1. Mettre l'ESP32 en mode boot (si nécessaire : maintenir BOOT, presser RESET)
2. Cliquer sur `Téléverser`
3. Attendre "Hard resetting via RTS pin..."

## 🧪 Commandes Série (115200 baud)

| Commande | Description |
|----------|-------------|
| `help` | Liste des commandes disponibles |
| `status` | État général (source, volume, EQ) |
| `stats` | Statistiques (uptime, erreurs I2C, watchdog) |
| `i2ctest` | Scan bus I2C + test communication |
| `adctest` | Test ADC batterie avec filtre médian |
| `eqtest` | Test égaliseur TDA7439 |
| `reset` | Redémarrage logiciel |

## 📊 Mapping GPIO ESP32-S3

```cpp
// === ENCODEUR ===
#define PIN_ENC_A       4
#define PIN_ENC_B       5
#define PIN_ENC_SW      6

// === I2C ===
#define PIN_SDA         8
#define PIN_SCL         9

// === OLED ===
// Utilise I2C (adresse 0x3C)

// === CONTRÔLE AMPLI ===
#define PIN_AMP_EN      15
#define PIN_AMP_MUTE    16
#define PIN_AMP_ERR     17

// === SÉCURITÉ ===
#define PIN_SAFE_EN     42

// === IR ===
#define PIN_IR_RECV     7

// === ADC ===
#define PIN_BATT_SENSE  38

// === SÉLECTION SOURCE ===
#define PIN_MUX_A       10
#define PIN_MUX_B       11
```

## 📁 Structure du Code

```
Firmware_Ampli_V1_5.ino
├── [En-tête]           Infos version, changelog
├── [Includes]          Bibliothèques
├── [Defines]           Pins, constantes, seuils
├── [Variables]         État global, buffers
├── [Classes]           TDA7439, Menu, VUMeter
├── [ISR]               Interruptions (encodeur, IR)
├── [Fonctions]         Audio, I2C, ADC, OLED
├── setup()             Initialisation
└── loop()              Boucle principale
```

## ⚙️ Configuration Personnalisable

```cpp
// === AUDIO ===
#define DEFAULT_VOLUME      20      // Volume initial (0-63)
#define LOUDNESS_THRESHOLD  30      // Seuil activation loudness
#define FADE_STEP_MS        10      // Vitesse fade in/out

// === BATTERIE ===
#define BATT_LOW_THRESHOLD  19.0    // Alerte batterie faible (V)
#define BATT_CRIT_THRESHOLD 18.0    // Arrêt critique (V)

// === I2C ===
#define I2C_TIMEOUT_MS      10      // Timeout anti-blocage
#define I2C_RETRY_COUNT     3       // Nombre de tentatives
#define I2C_RETRY_DELAY_MS  5       // Délai entre tentatives

// === WATCHDOG ===
#define WDT_TIMEOUT_S       5       // Timeout watchdog (secondes)
```

## 🐛 Dépannage

| Symptôme | Cause probable | Solution |
|----------|----------------|----------|
| Upload échoue | ESP32 pas en mode boot | Maintenir BOOT, presser RESET |
| Pas de Serial | CDC pas activé | `USB CDC On Boot > Enabled` |
| I2C timeout | Mauvais câblage | Vérifier SDA/SCL, pull-ups 4.7kΩ |
| OLED noir | Adresse I2C | Tester 0x3C et 0x3D |
| Encodeur erratique | Rebonds | Vérifier condensateurs 100nF |
| Watchdog reset | Boucle infinie | Vérifier I2C timeout activé |

## 🔗 Liens Utiles

- [Documentation Hardware](../docs/)
- [ESP32-S3 Pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)
- [Arduino-ESP32 Wiki](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)

## 📜 Changelog

### V1.5 (Décembre 2025)
- ⏱️ I2C timeout 10ms anti-blocage
- 📖 Documentation nappe 16 pins et protection PVDD
- 🔧 Compatibilité hardware V1.5

### V1.4 (Décembre 2025)
- 🎛️ TDA7439 EQ 3 bandes (remplace PT2314)
- 🔊 Loudness automatique Fletcher-Munson
- 🎚️ Effet spatial/surround
- 🎵 8 presets sonores
- 🛡️ Filtre médian ADC (5 échantillons)
- 🔒 Section critique encodeur (`portENTER_CRITICAL`)
- 🔄 I2C retry avec backoff exponentiel
- ⚡ Watchdog 5 secondes

### V1.3 (Novembre 2025)
- Version initiale
- PT2314 EQ 2 bandes + MCP4261 volume
- Support Bluetooth, AUX, Phono

---

*Dernière mise à jour : Décembre 2025 (V1.5)*
