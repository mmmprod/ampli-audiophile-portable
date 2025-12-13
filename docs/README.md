# 📐 Documentation Hardware — Ampli Audiophile V1.6

> Documentation technique complète du hardware de l'amplificateur audiophile portable.

---

## 📋 Table des Matières

1. [Vue d'Ensemble](#vue-densemble)
2. [Carte 1 — Puissance](#carte-1--puissance)
3. [Carte 2 — Signal/Contrôle](#carte-2--signalcontrôle)
4. [Nappe Inter-Cartes](#nappe-inter-cartes)
5. [Règles PCB V1.6](#règles-pcb-v16)
6. [Bill of Materials](#bill-of-materials)
7. [WCCA — Analyse Pire Cas](#wcca--analyse-pire-cas)
8. [Schémas de Connexion](#schémas-de-connexion)

---

## Vue d'Ensemble

### Architecture Bi-Carte

| Carte | Dimensions | Fonction |
|-------|------------|----------|
| **Carte 1** | 80 × 100 mm | Puissance : BMS, sécurité, ampli MA12070, sorties HP |
| **Carte 2** | 80 × 120 mm | Signal : ESP32-S3, BT, DAC, EQ TDA7439, préampli phono |

### Spécifications Système

| Paramètre | Valeur |
|-----------|--------|
| Tension batterie | 18-25.2V (6S LiPo) |
| Puissance ampli | 2 × 20W @ 8Ω |
| THD+N | < 0.01% @ 1W |
| SNR | > 110dB |
| Consommation repos | < 50mA |
| Consommation max | ~2.5A |

---

## Carte 1 — Puissance

### C1-A : Module BMS

**Composant :** JBD SP22S003B (6S 20A)

| Connecteur | Fonction |
|------------|----------|
| C- (noir) | GND commun charge/décharge |
| B- (bleu) | Négatif pack batterie |
| P+ (rouge) | Positif sortie protégée |
| JST XH-7P | Balance cellules B0-B6 |
| JST PH-2P | Sonde NTC 10kΩ |

**Protections intégrées :**
- Surcharge cellule : 4.25V ±25mV
- Sous-décharge : 2.8V ±50mV
- Surintensité : 25A
- Court-circuit : < 100µs
- Sur-température : 60°C

---

### C1-B : Sécurité 5 Niveaux

```
+PACK → BMS → TCO → Relais K1 → Fusible F1 → D1+D2 → +22V_RAW
         N1    N2      N3           N4         N5
```

| Niveau | Composant | Specs |
|--------|-----------|-------|
| N1 | BMS JBD | 6S 20A, balance 50mA |
| N2 | TCO Aupo A4-1A-F | 72°C, 10A, réarmable |
| N3 | Relais HF46F-G/12 | 12V, 10A, SPST-NO |
| N4 | Fusible Littelfuse | 5A, **Fast-blow**, ATO |
| N5 | D1 SS54 + D2 SMBJ24CA | Anti-inversion + TVS |

**Driver relais (opto-isolé) :**
```
+3V3 → R_LED (1kΩ) → PC817 LED → ESP32 GPIO42
+BATT → R_PULL (10kΩ) → PC817 Collecteur
PC817 Émetteur → Si2302 Gate → K1 Bobine-
```

---

### C1-C : Protection PVDD (V1.5+)

**Problème :** MA12070 PVDD max = 26V, batterie pleine = 25.2V, back-EMF possible +1V

**Solution :** Schottky série D3

```
+22V_RAW → D3 (SS54, Vf=0.5V) → +PVDD_SAFE (24.7V max)
```

| Paramètre | Valeur |
|-----------|--------|
| Batterie pleine | 25.2V |
| Après D3 | 24.7V |
| Marge vs 26V | 1.3V |
| Back-EMF +1V | 25.7V < 26V ✅ |

---

### C1-D : Alimentations

#### Buck DC-DC (22V → 5V)

**Module :** MP1584EN 3A

```
+22V_RAW → C_IN (100µF + 10µF) → MP1584 VIN   [V1.6: C_IN ajouté]
MP1584 VOUT → L_FILT (10µH) → +5V
```

#### LDO (5V → 3.3V)

**Composant :** AMS1117-3.3 (SOT-223)

```
+5V → AMS1117 → +3V3
Découplage: 10µF entrée, 22µF + 100nF sortie
```

#### LDO Audio (22V → 5V) — [MODIFIÉ V1.6]

**Composant :** MCP1703A-5002E/TO (ultra-low noise)

```
+22V_RAW → R_DROP (47Ω 3W) → +12V_PRE → MCP1703 → +5V_ANALOG
                    ↑
          [V1.6] UPGRADE 1W → 3W (WCCA)
```

**Justification V1.6 :**
```
Pire cas (court-circuit LDO, protection 250mA) :
P_R_DROP = 47Ω × (0.25A)² = 2.94W > 1W ❌
→ R_DROP 3W obligatoire (marge 2%)
```

---

### C1-E : Amplificateur MA12070

**Composant :** Infineon MA12070 (QFN-48)

| Pin | Signal | Connexion |
|-----|--------|-----------|
| 1-4 | PVDD | +PVDD_SAFE via C 220µF |
| 44-47 | PGND | GND_PWR |
| 32 | IN_L | AUDIO_L via C 2.2µF film |
| 17 | IN_R | AUDIO_R via C 2.2µF film |
| 12 | SDA | I2C bus |
| 13 | SCL | I2C bus |
| 31 | /EN | GPIO15 |
| 30 | /MUTE | GPIO16 |
| 25/24 | OUT_L+/- | HP gauche via L 10µH |
| 26/23 | OUT_R+/- | HP droit via L 10µH |

---

### C1-F : Star Ground [NOUVEAU V1.6]

**Problème identifié :** Courant retour ampli (2A crête) peut moduler référence audio

**Solution :** Point étoile unique sur C_BULK négatif

```
⭐ STAR GROUND (C_BULK 220µF négatif)
    ├── GND_PWR (nappe pin 4)
    ├── GND_SIG (nappe pin 5)
    ├── MA12070 PGND (pins 44-47)
    ├── Buck MP1584 GND
    ├── LDO AMS1117 GND
    ├── LM7809 GND
    ├── D2 TVS cathode
    └── Connecteur batterie GND
```

---

## Carte 2 — Signal/Contrôle

### C2-A : Module Bluetooth BTM525

**Module :** BTM525 (QCC5125)  
**Codecs :** LDAC, aptX HD, aptX, AAC, SBC

| Pin | Signal | Connexion |
|-----|--------|-----------|
| 1, 8 | VCC | +3V3 |
| 7, 20 | GND | GND |
| 4 | I2S_BCLK | PCM5102A BCK |
| 5 | I2S_LRCK | PCM5102A LCK |
| 6 | I2S_DATA | PCM5102A DIN |
| 19 | STATUS | GPIO4 |

---

### C2-B : DAC PCM5102A

**Composant :** TI PCM5102A (TSSOP-20)

| Pin | Signal | Connexion |
|-----|--------|-----------|
| 1 | VCC | +3V3 |
| 12 | BCK | BTM525 BCLK |
| 11 | LCK | BTM525 LRCK |
| 13 | DIN | BTM525 DATA |
| 18 | FMT | GND (I2S standard) |
| 6 | OUTL | BT_AUDIO_L |
| 8 | OUTR | BT_AUDIO_R |

---

### C2-C : Préampli Phono RIAA

**Composant :** OPA2134PA (DIP-8)  
**Gain :** 38dB @ 1kHz

```
J_PHONO → C_IN (0.1µF FILM) → R_IN (1kΩ) → OPA2134 IN+
OPA2134 OUT → R1 (75kΩ) → C1 (100nF FILM) → OPA2134 IN-
OPA2134 IN- → R2 (750Ω) → C2 (3.3nF FILM) → GND
```

**⚠️ IMPORTANT :** Condensateurs FILM obligatoires (pas céramique X7R)
- Céramique X7R = effet piézoélectrique = THD 0.1-1%
- Film polypropylène = THD < 0.001%

---

### C2-D : Sélecteur Source CD4053

**Composant :** CD4053BE (DIP-16)

| Entrée | Signal |
|--------|--------|
| A0-0 | BT_AUDIO_L |
| A0-1 | AUX_L |
| B0-0 | BT_AUDIO_R |
| B0-1 | AUX_R |

**Contrôle :**
- GPIO5 (SRC_SEL0) : BT/AUX
- GPIO6 (SRC_SEL1) : Phono (via TDA7439 IN2)

---

### C2-E : Processeur Audio TDA7439

**Composant :** ST TDA7439 (DIP-30)  
**I2C :** Adresse 0x44

| Fonction | Plage | Pas |
|----------|-------|-----|
| Input Gain | 0 à +30dB | 2dB |
| Volume | 0 à -47dB | 1dB |
| Bass | ±14dB | 2dB |
| Mid | ±14dB | 2dB |
| Treble | ±14dB | 2dB |
| Speaker Att | 0 à -79dB | 1dB |

**Filtres externes (condensateurs FILM) :**
```
Bass filter:   3× 100nF par canal (T-filter)
Mid filter:    3× 22nF par canal (T-filter)
Treble filter: 1× 5.6nF par canal (high-pass)
```

---

### C2-G : ESP32-S3

**Module :** ESP32-S3-WROOM-1-N8R8

| GPIO | Fonction | Direction |
|------|----------|-----------|
| 1 | I2C_SDA | Bidirectionnel |
| 2 | I2C_SCL | Sortie |
| 4 | BT_STATUS | Entrée |
| 5 | SRC_SEL0 | Sortie |
| 6 | SRC_SEL1 | Sortie |
| 15 | AMP_EN | Sortie |
| 16 | AMP_MUTE | Sortie |
| 17 | AMP_ERR | Entrée |
| 18 | ENC_A | Entrée |
| 19 | ENC_B | Entrée |
| 20 | ENC_SW | Entrée |
| 21 | IR_RX | Entrée |
| 38 | ADC_BATT | ADC |
| 39 | ADC_NTC | ADC |
| 40 | ADC_AUDIO_L | ADC |
| 41 | ADC_AUDIO_R | ADC |
| 42 | SAFE_EN | Sortie |

---

## Nappe Inter-Cartes

**Connecteur :** JST XH 16 pins  
**Câble :** 100mm AWG24

| Pin | Signal | Direction | Note |
|-----|--------|-----------|------|
| 1 | 22V_SENSE | C1→C2 | Diviseur batterie |
| 2 | +5V | C1→C2 | Rail 5V |
| 3 | +3V3 | C1→C2 | Rail 3.3V |
| 4 | GND_PWR | - | Masse puissance |
| 5 | GND_SIG | - | Masse signal |
| 6 | **GND_SHIELD** | - | Blindage |
| 7 | AUDIO_L | C2→C1 | Audio gauche |
| 8 | **GND_SHIELD** | - | Blindage |
| 9 | AUDIO_R | C2→C1 | Audio droit |
| 10 | **GND_SHIELD** | - | Blindage |
| 11 | SDA | ↔ | I2C data |
| 12 | SCL | C2→C1 | I2C clock |
| 13 | AMP_EN | C2→C1 | Enable ampli |
| 14 | AMP_MUTE | C2→C1 | Mute ampli |
| 15 | AMP_ERR | C1→C2 | Erreur ampli |
| 16 | SAFE_EN | C2→C1 | Contrôle relais |

**Blindage V1.5+ :** GND entre chaque paire critique (Audio, I2C)

---

## Règles PCB V1.6

### Problème Crosstalk Identifié

**I2C (400kHz) près traces audio → couplage capacitif 50mV**

```
Calcul:
C_mutuelle = 0.5pF (traces 2mm, 10mm long)
dV/dt I2C = 10V/µs
Z_audio = 10kΩ
V_couplé = 0.5pF × 10V/µs × 10kΩ = 50mV ❌
```

### Règles Obligatoires Carte 2

| # | Règle |
|---|-------|
| 1 | Minimum **3mm** entre traces I2C et Audio analogique |
| 2 | Plan GND entre I2C et Audio si distance < 5mm |
| 3 | I2C face TOP, Audio face BOTTOM si PCB 2 couches |
| 4 | Pas de via I2C sous condensateurs RIAA |
| 5 | Plan GND sous traces I2S (BTM525 → PCM5102A) |
| 6 | I2S dans zone numérique, loin préampli phono |

### Zones PCB Carte 2

```
┌───────────────────────────────────────┐
│  ZONE NUMÉRIQUE    │  ZONE AUDIO      │
│  ESP32-S3          │  OPA2134 ×2      │
│  BTM525            │  CD4053          │
│  PCM5102A          │  TDA7439         │
│  I2C/I2S traces    │  RCA Phono       │
│                    │                  │
│  ───────── GND GUARD ─────────────── │
└───────────────────────────────────────┘
```

---

## Bill of Materials

### Semiconducteurs (~53€)

| Réf | Composant | Package | Qté | Prix |
|-----|-----------|---------|-----|------|
| U1 | MA12070 | QFN-48 | 1 | 8€ |
| U2 | OPA2134PA (RIAA) | DIP-8 | 1 | 4€ |
| U3 | TDA7439 | DIP-30 | 1 | 3€ |
| U4 | CD4053BE | DIP-16 | 1 | 0.30€ |
| U5 | OPA2134PA (Buffer) | DIP-8 | 1 | 4€ |
| U6 | AMS1117-3.3 | SOT-223 | 1 | 0.30€ |
| U7 | MCP1703A-5002 | TO-92 | 1 | 0.60€ |
| U8 | ESP32-S3-WROOM | Module | 1 | 5€ |
| U9 | BTM525 | Module | 1 | 20€ |
| U10 | PCM5102A | TSSOP-20 | 1 | 3€ |
| D1, D3 | SS54 | SMA | 2 | 0.60€ |
| D2 | SMBJ24CA | SMB | 1 | 0.50€ |

### Passifs — Résistances (~3€)

| Valeur | Qté | Note |
|--------|-----|------|
| **47Ω 3W** | 1 | R_DROP LDO [V1.6] |
| 100Ω 1W | 1 | R_K1 bobine |
| 1kΩ | 5 | Pull-up, LED |
| 4.7kΩ | 2 | I2C pull-up |
| 10kΩ | 15 | Pull-up/down |
| 75kΩ 1% | 2 | RIAA |
| 750Ω 1% | 2 | RIAA |
| 100kΩ | 3 | Diviseurs, bias |

### Passifs — Condensateurs (~15€)

| Type | Valeur | Qté | Usage |
|------|--------|-----|-------|
| Céramique | 100nF | 15 | Découplage |
| Céramique | 10µF | 12 | Découplage |
| Électro | 220µF 35V | 1 | C_BULK PVDD |
| Électro | 100µF 35V | 4 | Filtrage |
| **Film** | 0.1-2.2µF | 30 | **Couplage audio** |
| **Film** | 100nF | 12 | **Bass filter TDA** |
| **Film** | 22nF | 12 | **Mid filter TDA** |
| **Film** | 5.6nF | 2 | **Treble filter TDA** |

**⚠️ Tous condensateurs chemin audio = FILM (pas céramique)**

---

## WCCA — Analyse Pire Cas

| Composant | Usage Normal | Pire Cas | Rating | Marge | Status |
|-----------|--------------|----------|--------|-------|--------|
| R_DROP 47Ω | 0.12W | 2.94W | **3W** | 2% | ✅ V1.6 |
| D3 SS54 | 50mW | 1W | 2W | 50% | ✅ |
| LM7809 | 324mW | 810mW | 1W+ | >20% | ✅ |
| F1 5A | 5A | 10A | 35A break | >250% | ✅ |
| Diviseur ADC | 2.29V | 2.29V | 3.3V | 31% | ✅ |

### Températures Jonction (Ta=40°C)

| Composant | P_diss | Rth | Tj | Tj_max | Status |
|-----------|--------|-----|----|----|--------|
| MA12070 | 2W | 25°C/W | 90°C | 150°C | ✅ |
| LM7809 | 324mW | 50°C/W | 56°C | 125°C | ✅ |
| D3 SS54 | 50mW | 60°C/W | 43°C | 150°C | ✅ |

---

## Schémas de Connexion

### Chaîne Audio Complète

```
SOURCES:
  BTM525 ──I2S──► PCM5102A ──────────────────────┐
                                                 │
  AUX Jack ──────────────────────────────────────┼──► CD4053 ──► TDA7439 ──► Buffer ──► Nappe ──► MA12070
                                                 │      MUX       EQ 3-bd    OPA2134            Class-D
  Phono RCA ──► OPA2134 RIAA ────────────────────┘
```

### Chaîne Alimentation

```
Batterie 6S
     │
     ▼
   BMS ──► TCO ──► K1 ──► F1 ──► D1+D2 ──► +22V_RAW
                                              │
                    ┌─────────────────────────┼─────────────────────┐
                    │                         │                     │
                    ▼                         ▼                     ▼
               R_DROP 47Ω 3W              D3 SS54                MP1584
                    │                         │                     │
                    ▼                         ▼                     ▼
               MCP1703                   +PVDD_SAFE              +5V
                    │                         │                     │
                    ▼                         ▼                     ▼
              +5V_ANALOG                  MA12070               AMS1117
                    │                                               │
                    ▼                                               ▼
          OPA2134, CD4053, TDA7439                               +3V3
```

---

## Fichiers Disponibles

| Fichier | Description |
|---------|-------------|
| `Ampli_V1_6.md` | Schéma complet avec tous les blocs |
| `BOM.csv` | Bill of Materials export |
| `kicad/` | Fichiers KiCad (à venir) |
| `gerber/` | Fichiers fabrication (à venir) |

---

## Historique Versions Hardware

| Version | Date | Modifications majeures |
|---------|------|------------------------|
| V1.6 | 13/12/2025 | R_DROP 3W, Star Ground, règles PCB |
| V1.5 | 13/12/2025 | D3 PVDD, TVS SMBJ24CA, nappe blindée |
| V1.4 | 13/12/2025 | TDA7439 EQ 3 bandes |
| V1.3 | 12/12/2025 | Préampli phono OPA2134 |
| V1.2 | 12/12/2025 | Pinouts explicites tous modules |
| V1.1 | 11/12/2025 | Sécurité 5 niveaux |
| V1.0 | 11/12/2025 | Architecture initiale |

---

<p align="center">
  <b>📐 Documentation Hardware V1.6</b>
</p>
