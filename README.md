# 🎵 Amplificateur Audiophile Portable

**Amplificateur stéréo Class-D 2×20W haute fidélité avec préampli phono, Bluetooth LDAC, égaliseur 3 bandes et contrôle numérique.**

![Version](https://img.shields.io/badge/version-1.3-blue)
![License](https://img.shields.io/badge/license-Proprietary-orange)
![Status](https://img.shields.io/badge/status-active-success)

---

## ✨ Caractéristiques principales

- **Puissance** : 2 × 20W RMS @ 8Ω (MA12070 Class-D)
- **Sources** : 
  - 🔵 Bluetooth LDAC / aptX HD (BTM525 QCC5125)
  - 🎧 AUX 3.5mm stéréo
  - 🎼 Phono MM (préampli RIAA OPA2134)
- **Égaliseur 3 bandes** :  TDA7439 (Bass/Mid/Treble ±14dB)
- **Effets** : Loudness automatique + Spatial/Surround
- **Contrôle** : Encodeur rotatif + OLED 128×64 + Télécommande IR
- **Alimentation** :  Batterie LiPo 6S (18-25V) avec BMS + sécurité 5 niveaux
- **Autonomie** : 4-6h @ volume moyen

---

## 📂 Structure du projet

```
ampli-audiophile-portable/
├── docs/                       # 📚 Documentation technique
│   ├── Hardware_V1_3.md       # Schéma électronique complet, pinouts, BOM
│   ├── Breakout_Box_V1.md     # Outil de test/debug
│   └── README.md
├── firmware/                   # 💾 Firmware ESP32-S3
│   ├── Ampli_V1_3.ino         # Code Arduino V1.3
│   └── README.md
├── kicad/                      # 🔌 Fichiers PCB KiCad (à venir)
├── .gitignore
├── LICENSE
└── README.md                   # ← Vous êtes ici
```

---

## 🚀 Démarrage rapide

### **1️⃣ Matériel requis**

Voir [docs/Hardware_V1_3.md](docs/Hardware_V1_3.md) pour la **BOM complète** (~96€ hors PCB/batterie).

Composants clés :
- ESP32-S3-WROOM-1-N8R8
- MA12070 (ampli Class-D)
- TDA7439 (égaliseur 3 bandes)
- BTM525 (Bluetooth LDAC)
- PCM5102A (DAC I2S)
- OPA2134 (préampli phono + buffers)
- MCP4261 (volume SPI)
- BMS 6S 20A

### **2️⃣ Firmware**

1. Installer [Arduino IDE](https://www.arduino.cc/en/software) + ESP32 Core
2. Installer les bibliothèques (voir [firmware/README.md](firmware/README.md))
3. Ouvrir `firmware/Ampli_V1_3.ino`
4. Sélectionner **ESP32S3 Dev Module**
5. **Upload** 🚀

### **3️⃣ Assemblage**

Architecture **bi-carte** :
- **Carte 1** (80×100mm) : Puissance (BMS, alimentation, MA12070, HP)
- **Carte 2** (80×120mm) : Signal/Contrôle (ESP32, BT, DAC, EQ, préampli phono)
- **Liaison** : Nappe JST XH 14 pins

Voir [docs/Hardware_V1_3.md](docs/Hardware_V1_3.md) section **Architecture bi-carte**.

---

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [**Hardware V1.3**](docs/Hardware_V1_3.md) | Schéma complet, pinouts explicites, BOM, notes de conception |
| [**Breakout Box V1**](docs/Breakout_Box_V1.md) | Outil de test (réduit le temps de debug de 2h à 15min) |
| [**Firmware V1.3**](firmware/README.md) | Code ESP32-S3, commandes série, changelog |

---

## 🎛️ Fonctionnalités V1.3

### **Égaliseur 3 bandes (TDA7439)**
- Bass : ±14dB (pas de 2dB)
- Mid : ±14dB
- Treble : ±14dB
- **8 presets** :  Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom

### **Loudness automatique**
- Boost bass/treble automatique à faible volume
- Compense la courbe de Fletcher-Munson
- Configurable ON/OFF

### **Effet Spatial/Surround**
- 4 niveaux :  OFF, Light, Medium, Wide
- Élargissement de la scène stéréo

### **Contrôles**
- **Encodeur rotatif** :  Volume, navigation menu
- **OLED 128×64** : Affichage source, volume, EQ, VU-mètre, batterie
- **Télécommande IR** : Volume, source, mute, presets
- **Commandes série** : Debug et configuration avancée

---

## 🔧 Changelog

### **V1.3** (13/12/2025)
- ✅ Support TDA7439 DIP-30 (remplace PT2314)
- ✅ EQ 3 bandes : Bass / Mid / Treble
- ✅ Loudness automatique
- ✅ Effet Spatial/Surround virtuel
- ✅ 8 presets sonores (ajout Cinema, Live, Custom)
- ✅ Menu EQ étendu avec visualisation graphique
- ✅ Commandes série étendues

### **V1.2** (12/12/2025)
- Préampli Phono RIAA (OPA2134)
- Volume numérique MCP4261
- ESP32-S3 + OLED + Encodeur
- Headers test pour debug

### **V1.0** (11/12/2025)
- Architecture initiale
- Sécurité batterie 5 niveaux
- Ampli MA12070 + DAC PCM5102A

Voir [docs/Hardware_V1_3.md](docs/Hardware_V1_3.md) pour le changelog complet.

---

## 🛠️ Outils de développement

### **Breakout Box Test**
Outil de test hardware qui réduit le temps de validation de **2 heures à 15 minutes** :
- Bornes banane pour tous les points de mesure
- LEDs de status (alimentations, ampli, BT)
- Sélecteur 12 positions pour oscilloscope
- Compatible multimètre/oscilloscope

Voir [docs/Breakout_Box_V1.md](docs/Breakout_Box_V1.md)

---

## 📸 Photos

*(À ajouter :  photos du prototype assemblé, PCB, écran OLED en fonctionnement)*

---

## 🤝 Contribution

Les contributions sont les bienvenues ! N'hésitez pas à : 
- 🐛 Signaler des bugs (Issues)
- 💡 Proposer des améliorations
- 🔧 Soumettre des Pull Requests

---

## 📜 Licence

Ce projet est sous **licence propriétaire à usage non commercial**. 

- ✅ **Usage personnel/éducatif** :  Libre et gratuit
- ❌ **Usage commercial** : Requiert une licence payante

**Pour une licence commerciale**, contactez l'auteur. 

Voir [LICENSE](LICENSE) pour les détails complets.

---

## 👤 Auteur

**Mehdi** + Claude (assistant IA)

---

## ⭐ Remerciements

- Infineon (MA12070)
- Texas Instruments (OPA2134, PCM5102A)
- Qualcomm (QCC5125)
- Microchip (MCP4261)
- Espressif (ESP32-S3)

---

**🎵 Enjoy high-fidelity audio!**
