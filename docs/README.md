# 📐 Documentation Hardware — Ampli Audiophile V1.7

> Documentation technique complète du hardware de l'amplificateur audiophile portable.

---

## 📋 Table des Matières

1. [Vue d'Ensemble](#vue-densemble)
2. [Carte 1 — Puissance](#carte-1--puissance)
3. [Carte 2 — Signal/Contrôle](#carte-2--signalcontrôle)
4. [Nappe Inter-Cartes](#nappe-inter-cartes)
5. [Règles PCB V1.7](#règles-pcb-v17)
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

### Changelog V1.7 — Audit ChatGPT

| Modification | Avant (V1.6) | Après (V1.7) | Raison |
|--------------|--------------|--------------|--------|
| Alimentation audio | R_DROP 47Ω → MCP1703A | LM7812 → MCP1703A | VIN max 18V absolu |
| R_DROP | 47Ω 3W | **SUPPRIMÉE** | Inutile avec LM7812 |
| D3 Protection PVDD | SS54 (Vf=0.5V) | 1N5822 (Vf=0.9V) | Marge PVDD +0.4V |

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
| N5 | D1 1N5822 + D2 SMBJ24CA | Anti-inversion + TVS |

**Driver relais (opto-isolé) :**
```
+3V3 → R_LED (1kΩ) → PC817 LED → ESP32 GPIO42
+BATT → R_PULL (10kΩ) → PC817 Collecteur
PC817 Émetteur → Si2302 Gate → K1 Bobine-
```

---

### C1-C : Protection PVDD (V1.7)

**Problème :** MA12070 PVDD max = 26V, batterie pleine = 25.2V, back-EMF possible +1.35V

**Solution V1.7 :** Diode série 1N5822 (Vf = 0.9V)

```
+22V_RAW → D3 (1N5822, Vf=0.9V) → +PVDD_SAFE (24.3V nominal)
```

| Paramètre | V1.6 (SS54) | V1.7 (1N5822) |
|-----------|-------------|---------------|
| Batterie pleine | 25.2V | 25.2V |
| Vf diode | 0.5V | 0.9V |
| PVDD nominal | 24.7V | **24.3V** |
| Back-EMF +1.35V | 26.05V ⚠️ | **25.65V** ✅ |
| Marge vs 26V | 0% | **1.3%** |

**Calcul back-EMF (Worst Case) :**
```
Inductance HP : L = 100µH (estimation HP 8Ω)
Courant crête : I = 2A
Énergie : E = ½ × L × I² = ½ × 100µH × 4A² = 200µJ
C_PVDD : 220µF
ΔV = √(2 × E / C) = √(2 × 200µJ / 220µF) = 1.35V
PVDD_max = 24.3V + 1.35V = 25.65V < 26V ✅
```

---

### C1-D : Alimentations

#### Buck DC-DC (22V → 5V)

**Module :** MP1584EN 3A

```
+22V_RAW → C_IN (100µF + 10µF) → MP1584 VIN
MP1584 VOUT → L_FILT (10µH) → +5V
```

#### LDO (5V → 3.3V)

**Composant :** AMS1117-3.3 (SOT-223)

```
+5V → AMS1117 → +3V3
Découplage: 10µF entrée, 22µF + 100nF sortie
```

#### LDO Audio (22V → 5V) — ⭐ REFONTE V1.7

**Problème V1.6 :** MCP1703A VIN max = 18V absolu, architecture précédente fournissait ~24V

**Solution V1.7 :** Double régulation LM7812 + MCP1703A

```
+22V_RAW → LM7812 → +12V_PRE → MCP1703A-5002E → +5V_ANALOG
           (TO-220)             (TO-92)
```

**Avantages :**
- VIN MCP1703A garanti 12V < 16V operating < 18V absolu ✅
- R_DROP 47Ω supprimée (simplification)
- Dissipation répartie sur 2 composants
- Fiabilité garantie toute la plage batterie

**Calculs thermiques V1.7 :**

| Composant | Courant | VIN | VOUT | P_diss | Tj @ 25°C |
|-----------|---------|-----|------|--------|-----------|
| LM7812 | 20mA | 25.2V | 12V | 0.26W | 53°C |
| MCP1703A | 20mA | 12V | 5V | 0.14W | 65°C |

```
LM7812:
  P = (25.2V - 12V) × 0.02A = 0.26W
  Rth(j-a) TO-220 = 65°C/W (sans radiateur)
  Tj = 25°C + 0.26W × 65°C/W = 42°C
  Avec marge (Rth = 110°C/W pire cas) : Tj = 53°C << 125°C ✅

MCP1703A:
  P = (12V - 5V) × 0.02A = 0.14W
  Rth(j-a) TO-92 = 200°C/W
  Tj = 25°C + 0.14W × 200°C/W = 53°C
  Avec marge 40°C : Tj = 65°C << 150°C ✅
```

**Découplage LM7812 :**
```
+22V_RAW → C_IN1 (100nF céramique) → LM7812 VIN
LM7812 GND → GND plan
LM7812 VOUT → C_OUT1 (10µF électrolytique) → +12V_PRE
```

**Découplage MCP1703A :**
```
+12V_PRE → C_IN2 (1µF céramique) → MCP1703A VIN
MCP1703A GND → GND plan (Star Ground)
MCP1703A VOUT → C_OUT2 (1µF céramique) → +5V_ANALOG
```

---

### C1-E : Amplificateur MA12070

**Composant :** Infineon MA12070 (QFN-48)

| Paramètre | Valeur |
|-----------|--------|
| PVDD | 4.5-26V (24.3V nominal V1.7) |
| Puissance | 2 × 20W @ 8Ω, THD 1% |
| Rendement | > 90% |
| I2C Addr | 0x20 |

**Connexions critiques :**
```
+PVDD_SAFE (24.3V) → MA12070 PVDD (pins multiples)
+5V → MA12070 VDD_5V0
+3V3 → MA12070 VDD_IO
GND → MA12070 GND (Star Ground)

I2C : SDA/SCL → ESP32 (via nappe)
MUTE → ESP32 GPIO (actif LOW)
ENABLE → ESP32 GPIO (actif HIGH)
```

**Sorties HP :**
```
MA12070 OUT_A+ → HP_L+
MA12070 OUT_A- → HP_L-
MA12070 OUT_B+ → HP_R+
MA12070 OUT_B- → HP_R-
```

---

## Carte 2 — Signal/Contrôle

### C2-A : ESP32-S3

**Module :** ESP32-S3-WROOM-1-N8R8

| Interface | GPIO | Fonction |
|-----------|------|----------|
| I2C SDA | GPIO1 | MA12070, TDA7439, OLED |
| I2C SCL | GPIO2 | Horloge I2C |
| SPI CS Volume | GPIO10 | MCP4261 (backup) |
| ADC Batterie | GPIO4 | Diviseur 1:6 |
| ADC NTC | GPIO5 | Thermistance 10kΩ |
| Encodeur A | GPIO6 | Rotation volume |
| Encodeur B | GPIO7 | Rotation volume |
| Encodeur SW | GPIO15 | Bouton poussoir |
| IR Receiver | GPIO16 | TSOP38238 |
| Relais Ctrl | GPIO42 | Opto PC817 |
| MA12070 MUTE | GPIO40 | Mute ampli |
| MA12070 EN | GPIO41 | Enable ampli |

### C2-B : Bluetooth BTM525

**Module :** QCC5125 LDAC

```
BTM525 I2S_BCLK → PCM5102A BCK
BTM525 I2S_LRCK → PCM5102A LRCK
BTM525 I2S_DATA → PCM5102A DIN
```

### C2-C : DAC PCM5102A

```
PCM5102A OUT_L → CD4053 X0
PCM5102A OUT_R → CD4053 Y0
```

### C2-D : Sélecteur CD4053

**3 entrées stéréo :**

| Source | Entrées CD4053 |
|--------|----------------|
| Bluetooth | X0/Y0 |
| AUX | X1/Y1 |
| Phono | X2/Y2 |

```
CD4053 OUT_X → TDA7439 IN_L
CD4053 OUT_Y → TDA7439 IN_R
```

### C2-E : EQ TDA7439

| Paramètre | Plage |
|-----------|-------|
| Bass | ±14dB @ 100Hz |
| Mid | ±14dB @ 1kHz |
| Treble | ±14dB @ 10kHz |
| Volume | -47dB à +15dB |

```
TDA7439 OUT_L → OPA2134 Buffer L
TDA7439 OUT_R → OPA2134 Buffer R
```

### C2-F : Préampli Phono RIAA

**Composant :** OPA2134 (×2)

```
Entrée phono → C_IN (100nF film) → OPA2134 #1 (gain + RIAA) → OPA2134 #2 (buffer)
```

---

## Nappe Inter-Cartes

### Connecteur 16 pins (IDC)

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1 | GND | - | Blindage |
| 2 | +22V_RAW | C1→C2 | Alim principale |
| 3 | GND | - | Blindage |
| 4 | +5V | C1→C2 | Buck régulé |
| 5 | GND | - | Blindage |
| 6 | +3V3 | C2→C1 | LDO |
| 7 | GND | - | Blindage |
| 8 | I2C_SDA | Bidirectionnel | Pull-up 4.7kΩ |
| 9 | I2C_SCL | Bidirectionnel | Pull-up 4.7kΩ |
| 10 | GND | - | Blindage |
| 11 | AUDIO_L | C2→C1 | Signal audio gauche |
| 12 | AUDIO_R | C2→C1 | Signal audio droit |
| 13 | GND | - | Blindage |
| 14 | MA_MUTE | C2→C1 | Contrôle mute |
| 15 | MA_EN | C2→C1 | Contrôle enable |
| 16 | GND | - | Blindage |

**Note V1.6+ :** 6 pins GND pour blindage (anti-crosstalk)

---

## Règles PCB V1.7

### Star Ground

```
         C_BULK (1000µF)
              │
    ┌─────────┼─────────┐
    │         │         │
    ▼         ▼         ▼
 GND_PWR   GND_CTRL  GND_AUDIO
(MA12070)  (ESP32)   (TDA7439)
```

**Impératif :** Toutes les masses convergent en UN SEUL point sur C_BULK

### Règles Placement

| Zone | Composants | Contraintes |
|------|------------|-------------|
| Puissance | MA12070, D3, F1 | Écart > 10mm du signal |
| Signal | TDA7439, OPA2134 | Blindage via GND |
| Numérique | ESP32, BTM525 | Loin des entrées analogiques |
| Thermique | LM7812, MA12070 | Cuivre 50mm² min, vias thermiques |

### Crosstalk Prevention

- Traces audio : ≥ 3× largeur d'écart entre L et R
- Guard traces GND autour des signaux sensibles
- Pas de trace numérique sous/sur traces analogiques
- Vias de découplage < 3mm des pins Vcc

---

## Bill of Materials

### Semiconducteurs

| Réf | Composant | Valeur | Package | Qté |
|-----|-----------|--------|---------|-----|
| U1 | MA12070 | Ampli Class-D | QFN-48 | 1 |
| U2 | ESP32-S3-WROOM-1 | N8R8 | Module | 1 |
| U3 | TDA7439 | EQ Audio | DIP-28 | 1 |
| U4 | PCM5102A | DAC I2S | TSSOP-20 | 1 |
| U5 | OPA2134 | Op-Amp Audio | DIP-8 | 2 |
| U6 | CD4053 | Mux Analog | DIP-16 | 1 |
| U7 | AMS1117-3.3 | LDO 3.3V | SOT-223 | 1 |
| U8 | MCP1703A-5002E | LDO 5V Audio | TO-92 | 1 |
| **U9** | **LM7812** | **Régulateur 12V** | **TO-220** | **1** |
| Q1 | Si2302 | N-MOS | SOT-23 | 1 |
| D1 | 1N5822 | Schottky 40V 3A | DO-201 | 1 |
| D2 | SMBJ24CA | TVS 24V | SMB | 1 |
| **D3** | **1N5822** | **Schottky PVDD** | **DO-201** | **1** |

### Passifs (Sélection)

| Réf | Valeur | Type | Qté |
|-----|--------|------|-----|
| C_BULK | 1000µF/35V | Électrolytique | 1 |
| C_PVDD | 220µF/35V | Électrolytique | 1 |
| C_dec | 100nF | Céramique X7R | 20 |
| R_I2C | 4.7kΩ | 0805 | 2 |
| R_LED | 1kΩ | 0805 | 2 |

### Modules

| Réf | Module | Specs |
|-----|--------|-------|
| MOD1 | BMS JBD SP22S003B | 6S 20A |
| MOD2 | MP1584EN | Buck 3A |
| MOD3 | BTM525 | Bluetooth LDAC |
| MOD4 | OLED 0.96" | SSD1306 I2C |

---

## WCCA — Analyse Pire Cas

### Températures Jonction V1.7

| Composant | P_diss | Rth(j-a) | Tj @ Ta=40°C |
|-----------|--------|----------|--------------|
| MA12070 | 4W max | 25°C/W | 140°C |
| LM7812 | 0.26W | 65°C/W | 57°C |
| MCP1703A | 0.14W | 200°C/W | 68°C |
| AMS1117 | 0.17W | 90°C/W | 55°C |
| 1N5822 (D3) | 1.8W crête | 50°C/W | 130°C |

### Marges Tension V1.7

| Rail | Nominal | Min | Max | Marge |
|------|---------|-----|-----|-------|
| +22V_RAW | 22.2V | 18V | 25.2V | - |
| +12V_PRE | 12V | 11.5V | 12.5V | 4% |
| +5V_ANALOG | 5.0V | 4.9V | 5.1V | 2% |
| +PVDD_SAFE | 24.3V | 23.5V | 25.65V | 1.3% vs 26V |

---

## Schémas de Connexion

### Bloc Alimentation Audio V1.7

```
+22V_RAW ─────┬───────────────────────────────────────────┐
              │                                           │
              ▼                                           │
         ┌─────────┐                                      │
         │ C_IN1   │ 100nF                                │
         │ ceramic │                                      │
         └────┬────┘                                      │
              │                                           │
              ▼                                           │
         ┌─────────┐                                      │
         │ LM7812  │ TO-220                               │
         │ VIN  OUT├──┬──────────────────┐                │
         │   GND   │  │                  │                │
         └────┬────┘  │                  ▼                │
              │       │             ┌─────────┐           │
              │       │             │ C_OUT1  │ 10µF      │
              ▼       │             │ elec    │           │
            GND       │             └────┬────┘           │
                      │                  │                │
                      ▼                  ▼                │
                 +12V_PRE ───────────────┘                │
                      │                                   │
                      ▼                                   │
                 ┌─────────┐                              │
                 │ C_IN2   │ 1µF                          │
                 │ ceramic │                              │
                 └────┬────┘                              │
                      │                                   │
                      ▼                                   │
                 ┌──────────┐                             │
                 │ MCP1703A │ TO-92                       │
                 │ VIN  OUT ├──┬──────────────┐           │
                 │   GND    │  │              │           │
                 └────┬─────┘  │              ▼           │
                      │        │         ┌─────────┐      │
                      │        │         │ C_OUT2  │ 1µF  │
                      ▼        │         │ ceramic │      │
                    GND        │         └────┬────┘      │
                               │              │           │
                               ▼              ▼           │
                          +5V_ANALOG ─────────┘           │
                               │                          │
                               ▼                          │
                     OPA2134 × 2, TDA7439, CD4053         │
                                                          │
                                                          │
         ┌────────────────────────────────────────────────┘
         │
         ▼
    ┌─────────┐
    │   D3    │ 1N5822 (Vf=0.9V)
    │ Schottky│
    └────┬────┘
         │
         ▼
    +PVDD_SAFE (24.3V nominal)
         │
         ▼
      MA12070
```

---

## Historique Versions Hardware

| Version | Date | Modifications |
|---------|------|---------------|
| **V1.7** | 13/12/2025 | LM7812 ajouté, R_DROP supprimée, D3→1N5822 |
| V1.6 | 13/12/2025 | R_DROP 3W, Star Ground, règles PCB |
| V1.5 | 13/12/2025 | D3 SS54, TVS SMBJ24CA, nappe blindée |
| V1.4 | 13/12/2025 | Filtrage ADC, découplages renforcés |
| V1.3 | 12/12/2025 | TDA7439 EQ intégré |
| V1.0-1.2 | 11-12/12/2025 | Architecture initiale |

---

<p align="center">
  <b>📐 Documentation Hardware V1.7 — Audit ChatGPT</b>
</p>
