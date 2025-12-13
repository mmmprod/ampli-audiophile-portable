# 🎵 Amplificateur Audiophile Portable

**Amplificateur stéréo Class-D 2×20W haute fidélité avec préampli phono, Bluetooth LDAC, égaliseur 3 bandes et contrôle numérique.**

![Hardware](https://img.shields.io/badge/Hardware-v1.4-blue)
![Firmware](https://img.shields.io/badge/Firmware-v1.3-orange)
![License](https://img.shields.io/badge/license-Proprietary-orange)
![Status](https://img.shields.io/badge/status-active-success)

---

## 📌 Compatibilité versions

| Hardware | Firmware | Statut | Notes |
|----------|----------|--------|-------|
| **V1.4** | **V1.3** | ⚠️ **Partiel** | TDA7439 non supporté, utiliser firmware V1.4 (en dev) |
| **V1.3** | **V1.3** | ✅ **OK** | Pleinement compatible (PT2314/MCP4261) |
| **V1.2** | **V1.3** | ⚠️ **Partiel** | Manque préampli phono, encodeur, OLED |
| **V1.1** | **V1.0-V1.2** | ❌ **Non** | Architecture différente |

### ⚙️ **Configuration actuelle recommandée**

Pour un système fonctionnel **immédiatement** : 
- ✅ **Hardware V1.3** + **Firmware V1.3** = 100% fonctionnel
- 🔄 **Hardware V1.4** + **Firmware V1.4** = En développement (TDA7439 support en cours)

### 🔜 **Firmware V1.4 - Roadmap**

- [ ] Support TDA7439 I2C (remplace PT2314)
- [ ] Suppression code MCP4261 (volume géré par TDA7439)
- [ ] Ajout presets 8 positions (Cinema, Live)
- [ ] Loudness automatique
- [ ] Spatial control
- [ ] Mise à jour commandes série

📅 **Sortie estimée** :  Fin décembre 2025

---

> ⚠️ **Versions actuelles** :   
> - 🔌 **Hardware V1.4** (13/12/2025) - TDA7439 3-band EQ  
> - 💾 **Firmware V1.3** (13/12/2025) - Compatible V1.3 hardware, **V1.4 en cours**

---

## ✨ Caractéristiques principales

- **Puissance** : 2 × 20W RMS @ 8Ω (MA12070 Class-D)
- **Sources** : 
  - 🔵 Bluetooth LDAC / aptX HD (BTM525 QCC5125)
  - 🎧 AUX 3.5mm stéréo
  - 🎼 Phono MM (préampli RIAA OPA2134)
- **🆕 Égaliseur 3 bandes** :   TDA7439 DIP-30
  - Bass / Mid / Treble :  ±14dB (pas de 2dB)
  - 8 presets :  Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom
  - Loudness automatique (compensation Fletcher-Munson)
  - Effet Spatial/Surround (élargissement stéréo)
- **Volume** :  Contrôle intégré TDA7439 (0 à -47dB + mute)
- **Gain d'entrée** :  Ajustable 0-30dB par logiciel
- **Contrôle** : Encodeur rotatif + OLED 128×64 + Télécommande IR
- **Alimentation** :   Batterie LiPo 6S (18-25V) avec BMS + sécurité 5 niveaux
- **Autonomie** : 4-6h @ volume moyen
- **THD+N** : < 0,01% @ 1W (chaîne complète)
- **SNR** : > 110dB (ampli), > 90dB (TDA7439), > 65dB (phono)

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

---

## 🎛️ Égaliseur 3 bandes (TDA7439) — Nouveau V1.4

### **Processeur audio intégré**

Le TDA7439 est un processeur audio analogique haute qualité qui remplace le PT2314 de la V1.3 :

| Fonction | Plage | Résolution | THD+N |
|----------|-------|------------|-------|
| **Volume** | 0 à -47dB + mute | 1dB | < 0,01% |
| **Bass** | ±14dB | 2dB | < 0,01% |
| **Mid** | ±14dB | 2dB | < 0,01% |
| **Treble** | ±14dB | 2dB | < 0,01% |
| **Gain d'entrée** | 0 à +30dB | 2dB | < 0,01% |
| **Balance L/R** | 0 à -79dB | 1dB | - |

### **8 Presets audio**

| Preset | Bass | Mid | Treble | Usage |
|--------|------|-----|--------|-------|
| **Flat** | 0dB | 0dB | 0dB | Neutre, haute fidélité |
| **Bass+** | +10dB | -2dB | 0dB | Electronic, Hip-Hop |
| **Vocal** | -4dB | +4dB | +6dB | Podcast, Voix parlée |
| **Rock** | +6dB | 0dB | +6dB | Rock, Metal |
| **Jazz** | +4dB | +2dB | +4dB | Jazz, Acoustique |
| **Cinema** | +8dB | 0dB | +2dB | Films, Effets graves |
| **Live** | +2dB | +4dB | +4dB | Concerts live |
| **Custom** | Réglable | Réglable | Réglable | Créez votre profil |

### **Loudness automatique**

Compensation psychoacoustique de Fletcher-Munson : 
- ✅ Boost automatique bass/treble à faible volume
- ✅ Activable ON/OFF via menu
- ✅ Proportionnel au niveau d'écoute

### **Effet Spatial**

Élargissement de la scène stéréo par atténuation différentielle :
- **OFF** : Stéréo standard
- **Light** : Léger élargissement (+1dB diff)
- **Medium** : Élargissement moyen (+2dB diff)
- **Wide** : Effet surround prononcé (+3dB diff)

### **Contrôle I2C**

Tous les paramètres sont contrôlés par ESP32-S3 via I2C (adresse 0x44).  
Voir [firmware/README.md](firmware/README.md) pour les commandes série.

---

### **1️⃣ Matériel requis**

Voir [docs/Hardware_V1_4.md](docs/Hardware_V1_4.md) pour la **BOM complète** (~98€ hors PCB/batterie).

📦 **Versions précédentes** :  [V1.3](docs/Hardware_V1_3.md) | [V1.2](docs/Hardware_V1_2.md) (si disponible)

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

#### ⚠️ **IMPORTANT - Choisir la bonne version**

| Votre hardware | Firmware à utiliser | Fichier |
|----------------|---------------------|---------|
| **V1.4** (TDA7439) | 🔄 **V1.4** (en dev) | `firmware/Ampli_V1_4.ino` (à venir) |
| **V1.3** (PT2314) | ✅ **V1.3** (stable) | `firmware/Ampli_V1_3.ino` |

#### 📥 **Installation Firmware V1.3 (Hardware V1.3)**

1. Installer [Arduino IDE](https://www.arduino.cc/en/software) + ESP32 Core
2. Installer les bibliothèques (voir [firmware/README.md](firmware/README.md))
3. Ouvrir `firmware/Ampli_V1_3.ino`
4. ⚠️ **Vérifier dans le code** : 
   ```cpp
   // Ligne ~50 du fichier . ino
   #define HARDWARE_VERSION "1.3"  // Doit correspondre à votre carte !
   
Sélectionner ESP32S3 Dev Module
Upload 🚀
🔄 Pour Hardware V1.4 - Utilisation temporaire
En attendant le firmware V1.4, vous pouvez :

Utiliser firmware V1.3 MAIS :
❌ TDA7439 non fonctionnel (pas de volume/EQ)
✅ Sources BT/AUX/Phono OK
✅ Ampli MA12070 OK
Ou attendre la sortie du firmware V1.4 (fin décembre)
📝 Vérifier la compatibilité
Avant de flasher, vérifiez dans le Serial Monitor (115200 bauds) :

=================================
AMPLI AUDIOPHILE V1.3
Hardware:  V1.3 (PT2314/MCP4261)
=================================
EQ chip: PT2314 detecte @ 0x44
Volume chip: MCP4261 OK
Si vous avez V1.4 hardware, vous verrez :

Code
EQ chip: PT2314 NON detecte  ⚠️
TDA7439 detecte @ 0x44       ⚠️ (non supporté V1.3 firmware)
Code

---

### **3️⃣ Assemblage**

Architecture **bi-carte** :
- **Carte 1** (80×100mm) : Puissance (BMS, alimentation, MA12070, HP)
- **Carte 2** (80×120mm) : Signal/Contrôle (ESP32, BT, DAC, EQ, préampli phono)
- **Liaison** : Nappe JST XH 14 pins

Voir [docs/Hardware_V1_3.md](docs/Hardware_V1_3.md) section **Architecture bi-carte**.

---

## 📖 Documentation

| Document | Description | Statut |
|----------|-------------|--------|
| [**Hardware V1.4**](docs/Hardware_V1_4.md) | **⭐ Version actuelle** - TDA7439 3-band EQ, Loudness, Spatial | ✅ Actif |
| [**Hardware V1.3**](docs/Hardware_V1_3.md) | Version précédente (PT2314 2-band) | 📦 Archive |
| [**Breakout Box V1**](docs/Breakout_Box_V1.md) | Outil de test (réduit debug 2h→15min) | ✅ Actif |
| [**Firmware V1.3**](firmware/README.md) | Code ESP32-S3 (à mettre à jour pour V1.4) | 🔄 En cours |

---

## 🎛️ Fonctionnalités V1.4

### **Égaliseur 3 bandes (TDA7439)**
- **Bass** : ±14dB @ ~100Hz (réglable, pas de 2dB)
- **Mid** : ±14dB @ ~1kHz (réglable, pas de 2dB)
- **Treble** : ±14dB @ ~10kHz (réglable, pas de 2dB)
- **8 presets** :   Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom
- **THD+N** : < 0,01% @ 1kHz

### **Loudness automatique** ⭐ NOUVEAU
- Boost automatique bass (+6dB) à faible volume (< -30dB)
- Légère atténuation mid (-2dB) pour clarté
- Compensation psychoacoustique de Fletcher-Munson
- Activable ON/OFF via menu

### **Effet Spatial/Surround** ⭐ NOUVEAU
- **4 niveaux** :   OFF, Light (+1dB diff L/R), Medium (+2dB), Wide (+3dB)
- Élargissement de la scène stéréo sans DSP externe
- Basé sur atténuation différentielle L/R du TDA7439

### **Volume & Gain**
- **Volume** : 0 à -47dB + mute (TDA7439 intégré)
- **Gain d'entrée** : 0 à +30dB ajustable (pour compenser sources faibles)
- **Balance L/R** : 0 à -79dB par canal

### **Contrôles**
- **Encodeur rotatif** :   Volume, navigation menu
- **OLED 128×64** : Affichage source, volume, EQ graphique, VU-mètre, batterie
- **Télécommande IR** : Volume, source, mute, presets, loudness, spatial
- **Commandes série** : Configuration avancée, debug (voir firmware/README.md)

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

### **V1.4** (13/12/2025) ⭐ **VERSION ACTUELLE**

#### 🎛️ **Audio**
- ✅ **TDA7439 DIP-30** : Processeur audio 3 bandes (remplace PT2314)
- ✅ **Égaliseur 3 bandes** : Bass / Mid / Treble ±14dB (pas de 2dB)
- ✅ **8 presets** : Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom
- ✅ **Loudness automatique** : Compensation Fletcher-Munson à faible volume
- ✅ **Effet Spatial** :  4 niveaux (OFF, Light, Medium, Wide)
- ✅ **Volume intégré** : 0 à -47dB + mute (suppression MCP4261)
- ✅ **Gain d'entrée** : 0-30dB ajustable par logiciel (4 sources disponibles)

#### 🔧 **Architecture**
- ✅ Chaîne audio simplifiée :  CD4053 → TDA7439 → Buffer OPA2134 → MA12070
- ✅ 4 entrées disponibles : IN1 (BT/AUX), IN2 (Phono), IN3/IN4 (futures évolutions)
- ✅ THD+N maintenu < 0,01% sur toute la chaîne
- ✅ SNR > 90dB (TDA7439)

#### 💰 **BOM**
- ➕ +3€ TDA7439
- ➖ -1,50€ suppression MCP4261
- ➕ +0,50€ condensateurs filtres EQ (Bass/Mid/Treble)
- 📊 **Total V1.4** : ~98€ (vs 96€ V1.3) = **+2€** pour EQ 3 bandes

#### 📝 **Documentation**
- ✅ [Hardware_V1_4.md](docs/Hardware_V1_4.md) : Pinouts TDA7439, schéma filtres
- 🔄 Firmware à mettre à jour (support TDA7439 I2C)

---

**Avantages V1.4 :**
- ✅ TDA7439 centralise Volume + EQ + Balance (moins de composants)
- ✅ 4 entrées disponibles (IN3/IN4 libres pour évolutions)
- ✅ Loudness et Spatial sans DSP externe
- ✅ THD+N < 0,01% maintenu sur toute la chaîne
- ✅ Contrôle total I2C (flexibilité firmware)

### **V1.3** (13/12/2025) 📦 Archive
- ✅ Support PT2314 (égaliseur 2 bandes)
- ✅ Volume MCP4261
- ✅ Préampli Phono RIAA
- ✅ ESP32-S3 + OLED + Encodeur

---

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

Mehdi

---

**🎵 Enjoy high-fidelity audio!**
