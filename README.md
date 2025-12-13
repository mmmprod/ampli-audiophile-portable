# 🎵 Amplificateur Audiophile Portable

[![Version](https://img.shields.io/badge/version-1.7-blue.svg)](https://github.com/votre-repo)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Hardware](https://img.shields.io/badge/hardware-ESP32--S3-orange.svg)](docs/)
[![Status](https://img.shields.io/badge/status-En%20développement-yellow.svg)]()

> Amplificateur Hi-Fi portable 2×20W avec Bluetooth LDAC, entrée Phono vinyle, et égaliseur 3 bandes.

---

## 🎯 Caractéristiques

| Paramètre | Valeur |
|-----------|--------|
| **Puissance** | 2 × 20W RMS @ 8Ω |
| **THD+N** | < 0,01% @ 1W |
| **SNR** | > 110dB (ampli) / > 65dB (phono) |
| **Bluetooth** | LDAC, aptX HD, aptX, AAC, SBC |
| **Entrées** | Bluetooth, AUX 3.5mm, Phono MM |
| **Égaliseur** | 3 bandes ±14dB (Bass/Mid/Treble) |
| **Batterie** | LiPo 6S (22.2V nominal) |
| **Autonomie** | 4-6h @ volume moyen |

---

## 🏗️ Architecture Bi-Carte

```
┌─────────────────────────────────────────────────────────────┐
│                    CARTE 2 - SIGNAL                         │
│  ESP32-S3 │ BTM525 BT │ PCM5102A DAC │ TDA7439 EQ │ OPA2134 │
│                      80 × 120 mm                            │
└─────────────────────────┬───────────────────────────────────┘
                          │ Nappe 16 pins (blindée GND)
┌─────────────────────────┴───────────────────────────────────┐
│                    CARTE 1 - PUISSANCE                      │
│  BMS 6S │ Sécurité 5 niv │ MA12070 Class-D │ Sorties HP    │
│  ⭐ Star Ground sur C_BULK                                  │
│                      80 × 100 mm                            │
└─────────────────────────────────────────────────────────────┘
```

### Composants Principaux

| Composant | Fonction | Lien |
|-----------|----------|------|
| **MA12070** | Ampli Class-D 2×20W | [Infineon](https://www.infineon.com) |
| **TDA7439** | Processeur audio EQ 3 bandes | [ST](https://www.st.com) |
| **BTM525** | Module Bluetooth QCC5125 LDAC | AliExpress |
| **PCM5102A** | DAC I2S 32-bit 384kHz | [TI](https://www.ti.com) |
| **OPA2134** | Op-Amp audio faible bruit | [TI](https://www.ti.com) |
| **ESP32-S3** | MCU WiFi/BT, contrôle système | [Espressif](https://www.espressif.com) |

---

## 🔋 Sécurité Batterie 5 Niveaux

```
+PACK ──► BMS ──► TCO 72°C ──► Relais K1 ──► Fusible 5A ──► TVS ──► Circuit
          N1       N2            N3            N4           N5
```

| Niveau | Protection | Composant |
|--------|------------|-----------|
| N1 | Surcharge/décharge cellules | BMS JBD 6S 20A |
| N2 | Surchauffe pack | TCO Aupo 72°C réarmable |
| N3 | Coupure logicielle | Relais HF46F + opto PC817 |
| N4 | Surintensité | Fusible 5A Fast-blow ATO |
| N5 | Surtension/inversion | TVS SMBJ24CA + 1N5822 |

---

## 🎛️ Fonctionnalités Audio

### Égaliseur TDA7439
- **Bass** : ±14dB @ 100Hz
- **Mid** : ±14dB @ 1kHz  
- **Treble** : ±14dB @ 10kHz
- **8 Presets** : Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom

### Loudness Automatique
Compensation Fletcher-Munson à bas volume (boost basses progressif)

### Préampli Phono RIAA
- Gain 38dB @ 1kHz
- Condensateurs film polypropylène (THD < 0.001%)

---

## 📁 Structure du Repository

```
ampli-audiophile-portable/
├── README.md                 # Ce fichier
├── LICENSE
├── docs/
│   ├── README.md             # Documentation hardware détaillée
│   ├── Ampli_V1_7.md         # Schéma complet V1.7
│   └── BOM.csv               # Bill of Materials
├── firmware/
│   ├── README.md             # Documentation firmware détaillée
│   ├── Firmware_V1_7.ino     # Code source V1.7
│   └── libraries/            # Dépendances
├── hardware/
│   ├── kicad/                # Fichiers KiCad (à venir)
│   └── gerber/               # Fichiers fabrication (à venir)
└── tests/
    └── Protocole_Test.md     # Procédures de test
```

---

## 🚀 Démarrage Rapide

### Prérequis

- Arduino IDE 2.x ou PlatformIO
- ESP32 Board Package (v2.0+)
- Bibliothèques requises (voir [firmware/README.md](firmware/README.md))

### Installation Rapide

```bash
# Cloner le repo
git clone https://github.com/votre-user/ampli-audiophile-portable.git

# Ouvrir firmware/Firmware_V1_7.ino dans Arduino IDE
# Board : ESP32S3 Dev Module
# Upload !
```

---

## 📊 Changelog

### V1.7 (13/12/2025) — Audit ChatGPT ⭐⭐

**🔴 Hardware CRITIQUE :**
- **LM7812 ajouté** : pré-régulateur 12V (VIN MCP1703A garanti < 16V)
- **R_DROP 47Ω supprimée** : inutile avec LM7812
- **D3 SS54 → 1N5822** : Vf 0.9V → PVDD nominal 24.3V (marge +0.7V)

**🟡 Firmware Améliorations :**
- `esp_timer_get_time()` dans ISR (plus robuste que millis())
- `i2cBusRecovery()` au boot (récupère bus I2C bloqué)

**Justification V1.7 :**
```
BUG CRITIQUE V1.6: MCP1703A VIN max = 18V absolu
Avant: +22V_RAW → R_DROP → MCP1703A (VIN ≈ 24V) ❌ DÉTRUIT
Après: +22V_RAW → LM7812 → 12V → MCP1703A ✅ GARANTI
```

### V1.6 (13/12/2025) — Audit Exhaustif Fiabilité

**Hardware :**
- R_DROP 47Ω → 3W (WCCA validé)
- Star Ground explicite sur C_BULK
- Règles placement PCB anti-crosstalk

**Firmware :**
- `emergencyShutdown()` sécurisé (detachInterrupt first)
- Encodeur anti-spam (±5 pas/cycle max)
- NTC validation (détection déconnexion/CC)
- Pré-brownout (sauvegarde avant coupure BMS)

### V1.5 (13/12/2025) — Audit Gemini
- Protection PVDD Schottky D3 → 24.7V max
- TVS SMBJ24CA, nappe blindée, I2C timeout

### V1.4 (13/12/2025) — Audit Copilot
- Filtre médian ADC, section critique encodeur, I2C retry, WDT 5s

### V1.3 (12/12/2025) — TDA7439 EQ
- Égaliseur 3 bandes, 8 presets, loudness, spatial

### V1.0-1.2 (11-12/12/2025) — Base
- Architecture bi-carte, sécurité 5 niveaux, pinouts explicites

---

## 💰 Budget Estimé

| Catégorie | Coût |
|-----------|------|
| Semiconducteurs | ~55 € |
| Passifs | ~18 € |
| Connecteurs | ~9 € |
| Modules (BMS, Buck, OLED) | ~17 € |
| Divers | ~7 € |
| **TOTAL** | **~106 €** |

*(hors PCB, boîtier, batterie, haut-parleurs)*

---

## 🧪 Tests Critiques

| Test | Critère GO | Action si FAIL |
|------|------------|----------------|
| Cold-crank 6V | +5V_MCU > 4.75V | Vérifier buck |
| I_repos ampli OFF | < 1mA | Vérifier sleep mode |
| Protection backfeed | < 1V sur entrée | Vérifier D3 |
| PVDD nominal | 24.0-24.5V | Vérifier D3 1N5822 |
| LM7812 sortie | 11.5-12.5V | Vérifier LM7812 |

---

## 📜 Licence

MIT License — Voir [LICENSE](LICENSE)

---

## 🙏 Remerciements

- Infineon (MA12070), ST (TDA7439), Espressif (ESP32-S3)
- Audits : Copilot, Gemini, Claude, ChatGPT
- Communauté DIY audio

---

<p align="center">
  <b>🎧 Fait avec ❤️ pour les audiophiles DIY</b>
</p>
