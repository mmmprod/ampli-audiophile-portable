# 💾 Firmware ESP32-S3

Firmware Arduino pour l'ESP32-S3-WROOM-1-N8R8

## 📄 Fichiers

- **[Ampli_V1_3.ino](Ampli_V1_3.ino)** : Code principal V1.3

## 🔧 Configuration

- **Board** : ESP32-S3-WROOM-1
- **Framework** : Arduino (ESP32 Core 2.0+)
- **Flash** : 8MB
- **PSRAM** : 8MB

## 📚 Bibliothèques requises

Installez via le **Library Manager** Arduino IDE : 

```
Adafruit_GFX
Adafruit_SSD1306
IRremoteESP8266
```

## 🚀 Upload

1. Sélectionner **ESP32S3 Dev Module** dans `Tools > Board`
2. Port USB :  **COMxx** (selon votre système)
3. Upload Speed : **921600**
4. Flash Size : **8MB**
5. Cliquer sur **Upload**

## 🔗 Pinout

Voir `/docs/Hardware_V1_3.md` section **C2-G — ESP32-S3**

## 📝 Changelog V1.3

- Support TDA7439 DIP-30 (égaliseur 3 bandes)
- Loudness automatique
- Effet Spatial/Surround
- 8 presets sonores
- Menu EQ étendu avec visualisation graphique
- Commandes série étendues
