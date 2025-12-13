# 🎵 Amplificateur Audiophile Portable

Amplificateur stéréo Class-D 2×20W avec préampli phono, Bluetooth LDAC, égaliseur 3 bandes et contrôle numérique.

![Hardware](https://img.shields.io/badge/Hardware-v1.4-blue)
![Firmware](https://img.shields.io/badge/Firmware-v1.4-green)
![Status](https://img.shields.io/badge/status-active-success)

## ✅ Versions recommandées

| Hardware | Firmware | Statut | Notes |
|----------|----------|--------|-------|
| **V1.4** | **V1.4** | ✅ Recommandé | TDA7439 (EQ 3 bandes), volume intégré, corrections fiabilité |
| **V1.3** | **V1.3** | 📦 Archive | PT2314 + MCP4261 (legacy) |

> Utilisez le firmware correspondant à votre carte pour éviter les incompatibilités (voir section firmware ci-dessous).

## ✨ Caractéristiques

- **Puissance** : 2 × 20W RMS @ 8Ω (MA12070 Class-D).
- **Sources** : Bluetooth LDAC/aptX HD (BTM525 QCC5125), AUX 3.5mm, Phono MM (préampli RIAA OPA2134).
- **Égaliseur 3 bandes (V1.4)** : Bass/Mid/Treble ±14dB (pas 2dB), loudness automatique, effet spatial, 8 presets.
- **Volume & gain** : contrôle intégré TDA7439 (0 à -47dB + mute), gain d'entrée ajustable 0-30dB (V1.4) ou MCP4261 (V1.3).
- **Contrôle** : encodeur rotatif + OLED 128×64 + télécommande IR.
- **Alimentation** : Batterie LiPo 6S (18-25V) avec BMS, autonomie 4-6h.

## 🚀 Démarrage rapide

### 1) Choisir la documentation hardware

| Version | Guide |
|---------|-------|
| **V1.4 (recommandée)** | [docs/Hardware_V1_4.md](docs/Hardware_V1_4.md) |
| **V1.3 (archive)** | [docs/Ampli_Audiophile_Portable_V1_3.md](docs/Ampli_Audiophile_Portable_V1_3.md) |
| **Outil de test** | [docs/Breakout_Box_V1.md](docs/Breakout_Box_V1.md) |

### 2) Sélectionner le firmware

| Votre hardware | Firmware à flasher |
|----------------|--------------------|
| **V1.4** (TDA7439) | `firmware/Firmware_Ampli_V1_4.ino` |
| **V1.3** (PT2314 + MCP4261) | `firmware/Ampli_V1_3.ino` |

1. Installer l'IDE Arduino + ESP32 Core 2.0+.
2. Ajouter les bibliothèques : `Adafruit_GFX`, `Adafruit_SSD1306`, `IRremoteESP8266`.
3. Ouvrir le fichier `.ino` correspondant, sélectionner **ESP32S3 Dev Module**, puis uploader.

### 3) Assemblage

- Architecture bi-carte :
  - **Carte 1** (80×100mm) : alimentation/BMS + MA12070.
  - **Carte 2** (80×120mm) : ESP32, Bluetooth, DAC, égaliseur, préampli phono.
- Liaison par nappe JST XH 14 pins. Détails dans [docs/Hardware_V1_4.md](docs/Hardware_V1_4.md).

## 🧪 Tests et diagnostics

Firmware V1.4 inclut des commandes série :

- `i2ctest` : détection des périphériques et comptage des erreurs.
- `adctest` : filtre médian sur 5 échantillons.
- `stats` : statistiques complètes et watchdog.

## 🤝 Contribution

Les contributions sont les bienvenues : ouverture d'issues, propositions d'amélioration et pull requests. Consultez la licence pour les conditions d'usage.

## 📜 Licence

Projet sous licence propriétaire à usage non commercial. Usage commercial sur demande. Voir le fichier LICENSE.

**🎵 Enjoy high-fidelity audio!**
