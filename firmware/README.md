# 💾 Firmware ESP32-S3

Firmware Arduino pour l'ESP32-S3-WROOM-1-N8R8, avec deux variantes selon la révision hardware.

## 📄 Fichiers

- **[Firmware_Ampli_V1_4.ino](Firmware_Ampli_V1_4.ino)** : firmware recommandé pour le hardware V1.4 (TDA7439).
- **[Ampli_V1_3.ino](Ampli_V1_3.ino)** : firmware stable pour le hardware V1.3 (PT2314 + MCP4261).

## 🔧 Configuration

- **Board** : ESP32-S3 Dev Module (ESP32 Core 2.0+)
- **Flash** : 8MB
- **PSRAM** : 8MB

## 📚 Bibliothèques requises

Installez via le **Library Manager** de l'Arduino IDE :

```
Adafruit_GFX
Adafruit_SSD1306
IRremoteESP8266
```

## 🚀 Upload

1. Ouvrir le fichier `.ino` correspondant à votre révision hardware.
2. Sélectionner **ESP32S3 Dev Module** dans `Tools > Board`.
3. Port USB : **COMxx** (selon votre système).
4. Upload Speed : **921600**.
5. Flash Size : **8MB**.
6. Cliquer sur **Upload**.

## 🔗 Pinout et schémas

Consultez `/docs/Hardware_V1_4.md` ou `/docs/Ampli_Audiophile_Portable_V1_3.md` pour les pinouts détaillés selon la version.

## 📝 Notes de version

- **V1.4** : support TDA7439 (EQ 3 bandes), loudness, spatial, corrections fiabilité.
- **V1.3** : support PT2314 (EQ 2 bandes) et volume MCP4261, fonctionnalités stables.
