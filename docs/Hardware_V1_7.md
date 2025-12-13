# AMPLI AUDIOPHILE PORTABLE V1.7

## INFORMATIONS DOCUMENT

**Version:** 1.7  
**Date:** 13 décembre 2025  
**Auteur:** Mehdi  
**Status:** Corrections critiques audit externe  

---

## CHANGELOG V1.7

### 🔴 CORRECTIONS CRITIQUES (Audit ChatGPT vérifié)

| # | Bug | Gravité | Correction |
|---|-----|---------|------------|
| H1 | **MCP1703A VIN > 18V absolu** | DESTRUCTEUR | Ajout LM7812 pré-régulateur |
| H2 | **R_DROP inutile/dangereuse** | CRITIQUE | SUPPRIMÉE |
| H3 | **PVDD marge insuffisante** | CRITIQUE | D3 → 1N5822 (Vf 0.9V) |

### Détail Corrections

**[H1] MCP1703A en surtension**
```
AVANT V1.6 (BUG):
+22V_RAW → R_DROP (47Ω) → MCP1703A VIN
À 10mA: VIN = 25.2V - 0.47V = 24.7V >> 18V absolu ❌

APRÈS V1.7 (CORRIGÉ):
+22V_RAW → LM7812 → +12V_PRE → MCP1703A → +5V_ANALOG
VIN MCP1703A = 12V < 16V operating ✅
```

**[H2] R_DROP supprimée**
```
R_DROP 47Ω 3W n'est plus nécessaire avec LM7812
- Simplifie le circuit
- Élimine point chaud thermique
- Économie composant
```

**[H3] PVDD marge augmentée**
```
AVANT V1.6:
D3 = SS54 Schottky, Vf = 0.5V
PVDD_SAFE = 25.2V - 0.5V = 24.7V
Marge vs 26V = 1.3V (insuffisant avec back-EMF)

APRÈS V1.7:
D3 = 1N5822 standard, Vf = 0.9V
PVDD_SAFE = 25.2V - 0.9V = 24.3V
Marge vs 26V = 1.7V
Back-EMF +1.35V → PVDD_max = 25.65V < 26V ✅
```

---

## SPÉCIFICATIONS

| Paramètre | Valeur |
|-----------|--------|
| Puissance | 2 × 20W RMS @ 8Ω |
| THD+N | < 0.01% @ 1W |
| SNR | > 110dB (ampli) / > 65dB (phono) |
| Bluetooth | LDAC, aptX HD, aptX, AAC, SBC |
| Entrées | Bluetooth, AUX 3.5mm, Phono MM |
| Égaliseur | 3 bandes ±14dB (Bass/Mid/Treble) |
| Batterie | LiPo 6S 22.2V nominal (18-25.2V) |
| Autonomie | 4-6h @ volume moyen |

---

## ARCHITECTURE BI-CARTE

### Vue d'ensemble

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

---

# CARTE 1 — PUISSANCE (80 × 100 mm)

## C1-A : Module BMS

### Composant
JBD SP22S003B (6S 20A)

### Connexions

```
Pack 6S → B- (bleu) → BMS entrée négative
Pack 6S → B1-B6 (JST XH-7P) → Balance cellules
BMS C- (noir) → GND commun
BMS P+ (rouge) → +BATT_PROT
NTC 10kΩ (JST PH-2P) → Sonde température pack
```

### Protections intégrées

| Protection | Seuil |
|------------|-------|
| Surcharge cellule | 4.25V ±25mV |
| Sous-décharge | 2.8V ±50mV |
| Surintensité | 25A |
| Court-circuit | < 100µs |
| Sur-température | 60°C |

---

## C1-B : Sécurité 5 Niveaux

### Architecture

```
+PACK → BMS → TCO 72°C → Relais K1 → Fusible F1 → D1+D2 → +22V_RAW
         N1      N2          N3          N4         N5
```

### Niveau 1 — BMS JBD

```
+BATT_PROT → Sortie P+ BMS
Protections: Surcharge, sous-décharge, surintensité, température
```

### Niveau 2 — TCO Thermique

```
+BATT_PROT → TCO Aupo A4-1A-F (72°C, 10A) → +BATT_TCO
Réarmable automatiquement sous 60°C
```

### Niveau 3 — Relais Coupure

```
+BATT_TCO → K1 contact NO (HF46F-G/12-HS1) → +BATT_RELAY
K1 Bobine+ → +BATT_TCO
K1 Bobine- → Q_RELAY Drain (Si2302)
Q_RELAY Source → GND
Q_RELAY Gate → RELAY_CTRL
```

**Driver opto-isolé :**
```
+3V3 → R_LED (1kΩ) → PC817 Anode
PC817 Cathode → GPIO42 (SAFE_EN)
PC817 Collecteur → +BATT_TCO via R_PULL (10kΩ)
PC817 Émetteur → RELAY_CTRL
RELAY_CTRL → R_GATE (10kΩ) → GND (pull-down)
```

### Niveau 4 — Fusible

```
+BATT_RELAY → F1 (5A Fast-blow ATO Littelfuse) → +BATT_FUSE
```

**Specs F1 :**
- Courant nominal : 5A
- Type : Fast-blow (fusion rapide)
- I²t : 0.15 A²s
- Temps @ 10A : 1.5ms

### Niveau 5 — Protection Surtension/Inversion

```
+BATT_FUSE → D1 anode (SS54 Schottky)
D1 cathode → +22V_RAW
+22V_RAW → D2 (SMBJ24CA TVS bidirectionnel) → GND
```

**Specs D1 SS54 :**
- VF @ 1A : 0.5V
- VRRM : 40V
- IF : 5A

**Specs D2 SMBJ24CA :**
- VRWM : 24V
- VBR : 26.7V min
- VC @ 38A : 38.9V

---

## C1-C : Protection PVDD Ampli

### Composant [MODIFIÉ V1.7]

**D3 = 1N5822** (diode standard, remplace SS54 Schottky)

### Connexions

```
+22V_RAW → D3 anode (1N5822)
D3 cathode → +PVDD_SAFE
```

### Calcul Marge V1.7

```
Batterie pleine : 25.2V
Après D1 (0.5V) : 24.7V
Après D3 (0.9V) : 23.8V nominal

Cas pire (batterie + back-EMF 1.35V) :
PVDD_max = 24.3V + 1.35V = 25.65V < 26V limite MA12070 ✅

Marge : 26V - 25.65V = 0.35V sécurité
```

### Specs D3 1N5822

| Paramètre | Valeur |
|-----------|--------|
| VF @ 1A | 0.9V |
| VF @ 3A | 1.1V |
| VRRM | 40V |
| IF | 3A |
| Package | DO-201AD |

---

## C1-D : Alimentations

### D1 — Buck DC-DC (22V → 5V)

**Module :** MP1584EN 3A

```
+22V_RAW → C_IN_BUCK (100µF 35V + 10µF ceramic) → MP1584 VIN
MP1584 VOUT → L_FILT (10µH) → +5V
+5V → C_OUT_BUCK (100µF + 10µF) → GND
```

### D2 — LDO Numérique (5V → 3.3V)

**Composant :** AMS1117-3.3 (SOT-223)

```
+5V → C_IN_LDO (10µF) → AMS1117 VIN
AMS1117 VOUT → +3V3
+3V3 → C_OUT_LDO (22µF + 100nF) → GND
```

### D3 — Rail Audio Analogique [NOUVEAU V1.7]

**Architecture corrigée :** Double régulation LM7812 + MCP1703A

```
+22V_RAW → C_7812_IN (100nF) → LM7812 VIN
LM7812 GND → GND
LM7812 VOUT → +12V_PRE
+12V_PRE → C_7812_OUT (100nF + 10µF) → GND

+12V_PRE → C_1703_IN (1µF) → MCP1703A VIN
MCP1703A GND → GND
MCP1703A VOUT → +5V_ANALOG
+5V_ANALOG → C_1703_OUT (1µF + 100nF) → GND
```

### Calcul Thermique V1.7

**LM7812 :**
```
VIN = 25.2V - 0.5V (D1) = 24.7V max
VOUT = 12V
I_charge = 20mA (section audio)

P_LM7812 = (24.7V - 12V) × 20mA = 0.254W

TO-220 sans radiateur : Rth_ja = 50°C/W
ΔT = 0.254W × 50°C/W = 12.7°C
Tj = 40°C + 12.7°C = 52.7°C << 125°C max ✅
```

**MCP1703A :**
```
VIN = 12V (garanti par LM7812)
VOUT = 5V
I_charge = 20mA

P_MCP1703 = (12V - 5V) × 20mA = 0.14W

TO-92 : Rth_ja = 180°C/W
ΔT = 0.14W × 180°C/W = 25.2°C
Tj = 40°C + 25.2°C = 65.2°C << 150°C max ✅
```

### Preuve VIN MCP1703A

```
VIN_MCP1703 = 12V (sortie LM7812 régulée)
VIN_max operating = 16V
VIN_absolu = 18V

Marge operating : (16V - 12V) / 16V = 25% ✅
Marge absolu : (18V - 12V) / 18V = 33% ✅
```

### D4 — Rail TDA7439 (22V → 9V)

**Composant :** LM7809 (TO-220)

```
+22V_RAW → C_7809_IN (100nF) → LM7809 VIN
LM7809 GND → GND
LM7809 VOUT → +9V_TDA
+9V_TDA → C_7809_OUT (100nF + 10µF) → GND
```

---

## C1-E : Amplificateur MA12070

### Composant

Infineon MA12070 (QFN-48)

### Connexions Alimentation

```
+PVDD_SAFE → C_BULK (220µF 35V) → GND_PWR
+PVDD_SAFE → MA12070 pins 1-4 (PVDD)
MA12070 pins 44-47 (PGND) → GND_PWR
```

### Connexions Signal

```
AUDIO_L (nappe pin 7) → C_IN_L (2.2µF film) → MA12070 pin 32 (IN_L)
AUDIO_R (nappe pin 9) → C_IN_R (2.2µF film) → MA12070 pin 17 (IN_R)
```

### Connexions Contrôle

```
SDA (nappe pin 11) → MA12070 pin 12 (SDA)
SCL (nappe pin 12) → MA12070 pin 13 (SCL)
AMP_EN (nappe pin 13) → MA12070 pin 31 (/EN)
AMP_MUTE (nappe pin 14) → MA12070 pin 30 (/MUTE)
MA12070 pin 29 (/ERR) → AMP_ERR (nappe pin 15)
```

### Sorties HP

```
MA12070 pin 25 (OUT_L+) → L_OUT_L (10µH) → HP_L+
MA12070 pin 24 (OUT_L-) → L_OUT_L- (10µH) → HP_L-
MA12070 pin 26 (OUT_R+) → L_OUT_R (10µH) → HP_R+
MA12070 pin 23 (OUT_R-) → L_OUT_R- (10µH) → HP_R-
```

---

## C1-F : Star Ground [V1.6+]

### Point Étoile

**Localisation :** Borne négative de C_BULK (220µF)

```
⭐ STAR GROUND (C_BULK négatif)
    │
    ├── GND_PWR (nappe pin 4)
    ├── GND_SIG (nappe pin 5)
    ├── MA12070 PGND (pins 44-47)
    ├── Buck MP1584 GND
    ├── LDO AMS1117 GND
    ├── LM7812 GND
    ├── LM7809 GND
    ├── MCP1703A GND
    ├── D2 TVS cathode
    └── Connecteur batterie GND
```

### Règle Critique

**Tous les retours de courant convergent vers ce point unique.**  
**Aucune boucle de masse autorisée.**

---

# CARTE 2 — SIGNAL/CONTRÔLE (80 × 120 mm)

## C2-A : Module Bluetooth BTM525

### Composant

BTM525 (Qualcomm QCC5125)

### Connexions

```
+3V3 → BTM525 pins 1, 8 (VCC)
GND → BTM525 pins 7, 20 (GND)
BTM525 pin 4 (I2S_BCLK) → PCM5102A BCK
BTM525 pin 5 (I2S_LRCK) → PCM5102A LCK
BTM525 pin 6 (I2S_DATA) → PCM5102A DIN
BTM525 pin 19 (STATUS) → GPIO4
GPIO7 → BTM525 pin 3 (RESET)
```

---

## C2-B : DAC PCM5102A

### Composant

TI PCM5102A (TSSOP-20)

### Connexions

```
+3V3 → PCM5102A pin 1 (VCC)
GND → PCM5102A pins 2, 15, 16, 17 (GND)
BTM525 BCLK → PCM5102A pin 12 (BCK)
BTM525 LRCK → PCM5102A pin 11 (LCK)
BTM525 DATA → PCM5102A pin 13 (DIN)
GND → PCM5102A pin 18 (FMT) — Format I2S standard
PCM5102A pin 6 (OUTL) → BT_AUDIO_L
PCM5102A pin 8 (OUTR) → BT_AUDIO_R
```

---

## C2-C : Préampli Phono RIAA

### Composant

OPA2134PA (DIP-8) — Op-Amp audio faible bruit

### Schéma

```
J_PHONO_L → C_IN_L (0.1µF FILM) → R_IN_L (1kΩ) → OPA_A pin 3 (IN+)
OPA_A pin 1 (OUT) → PHONO_L
OPA_A pin 1 → R1_L (75kΩ 1%) → C1_L (100nF FILM) → OPA_A pin 2 (IN-)
OPA_A pin 2 → R2_L (750Ω 1%) → C2_L (3.3nF FILM) → GND

+5V_ANALOG → R_BIAS_L (100kΩ) → OPA_A pin 3
OPA_A pin 3 → R_BIAS_L2 (100kΩ) → GND

+5V_ANALOG → OPA pin 8 (V+)
GND → OPA pin 4 (V-)
+5V_ANALOG → C_DEC_OPA (100nF) → GND
```

*Canal droit identique sur OPA_B (pins 5, 6, 7)*

### ⚠️ IMPORTANT

**Tous condensateurs signal audio = FILM (polypropylène)**  
Céramique X7R interdit (effet piézoélectrique → THD 0.1-1%)

---

## C2-D : Sélecteur Source CD4053

### Composant

CD4053BE (DIP-16)

### Connexions

```
+5V_ANALOG → CD4053 pin 16 (VDD)
GND → CD4053 pins 7, 8 (VSS, VEE)

BT_AUDIO_L → CD4053 pin 12 (A0)
AUX_L → CD4053 pin 13 (A1)
CD4053 pin 14 (A_COM) → MUX_OUT_L

BT_AUDIO_R → CD4053 pin 1 (B0)
AUX_R → CD4053 pin 2 (B1)
CD4053 pin 15 (B_COM) → MUX_OUT_R

GPIO5 (SRC_SEL0) → CD4053 pin 11 (A_SEL)
GPIO5 → CD4053 pin 10 (B_SEL)
GPIO6 (SRC_SEL1) → Non utilisé (Phono via TDA IN2)
```

---

## C2-E : Processeur Audio TDA7439

### Composant

ST TDA7439 (DIP-30)

### Connexions Alimentation

```
+9V_TDA → TDA7439 pin 15 (VCC)
GND → TDA7439 pin 14 (GND)
+9V_TDA → C_TDA (100nF) → GND
```

### Connexions Audio

```
MUX_OUT_L → C_TDA_IN1_L (1µF FILM) → TDA7439 pin 1 (IN1_L)
MUX_OUT_R → C_TDA_IN1_R (1µF FILM) → TDA7439 pin 2 (IN1_R)
PHONO_L → C_TDA_IN2_L (1µF FILM) → TDA7439 pin 3 (IN2_L)
PHONO_R → C_TDA_IN2_R (1µF FILM) → TDA7439 pin 4 (IN2_R)

TDA7439 pin 11 (OUT_L) → C_TDA_OUT_L (1µF FILM) → BUFFER_IN_L
TDA7439 pin 12 (OUT_R) → C_TDA_OUT_R (1µF FILM) → BUFFER_IN_R
```

### Filtres Externes (FILM obligatoire)

```
Bass filter (3× 100nF par canal) — pins 19-24
Mid filter (3× 22nF par canal) — pins 25-30
Treble filter (1× 5.6nF par canal) — pins 17-18
```

### Connexions I2C

```
SDA → TDA7439 pin 9 (SDA)
SCL → TDA7439 pin 10 (SCL)
Adresse I2C : 0x44
```

---

## C2-F : Buffer Sortie

### Composant

OPA2134PA (DIP-8) — second exemplaire

### Schéma

```
BUFFER_IN_L → C_BUF_IN_L (1µF FILM) → R_BUF_IN_L (10kΩ) → OPA_A pin 3
OPA_A pin 1 (OUT) → AUDIO_L (nappe pin 7)
OPA_A pin 2 (IN-) → OPA_A pin 1 (suiveur)

+5V_ANALOG → OPA pin 8 (V+)
GND → OPA pin 4 (V-)
```

*Canal droit identique*

---

## C2-G : Microcontrôleur ESP32-S3

### Module

ESP32-S3-WROOM-1-N8R8

### GPIO Assignation

| GPIO | Fonction | Direction |
|------|----------|-----------|
| 1 | I2C_SDA | Bidir |
| 2 | I2C_SCL | Sortie |
| 4 | BT_STATUS | Entrée |
| 5 | SRC_SEL0 | Sortie |
| 6 | SRC_SEL1 | Sortie |
| 7 | BT_RESET | Sortie |
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
| 48 | LED_STATUS | Sortie |

### Découplage

```
+3V3 → ESP32 3V3
GND → ESP32 GND
+3V3 → C_ESP_1 (100nF) → GND (près pins alim)
+3V3 → C_ESP_2 (10µF) → GND
```

---

## C2-H : Interface Utilisateur

### OLED 128×64

```
+3V3 → OLED VCC
GND → OLED GND
SDA → OLED SDA
SCL → OLED SCL
Adresse I2C : 0x3C
```

### Encodeur Rotatif

```
ENC_A (GPIO18) → Encodeur A
ENC_B (GPIO19) → Encodeur B
ENC_SW (GPIO20) → Encodeur SW
GND → Encodeur GND
Pull-up internes ESP32 activés
```

### Récepteur IR

```
+3V3 → IR VCC
GND → IR GND
IR OUT → GPIO21
```

---

## C2-I : Monitoring

### Diviseur Batterie

```
+22V_RAW → R_DIV_H (100kΩ 1%) → ADC_BATT
ADC_BATT → R_DIV_L (47kΩ 1%) → GND
ADC_BATT → C_FILT_BATT (100nF) → GND
```

**Calcul :**
```
V_ADC = V_BATT × 47k / (100k + 47k) = V_BATT × 0.32
V_ADC_max = 25.2V × 0.32 = 8.06V → Hors plage !

Correction:
R_DIV_H = 220kΩ, R_DIV_L = 33kΩ
V_ADC = 25.2V × 33k / (220k + 33k) = 3.29V < 3.3V ✅
```

### Diviseur NTC

```
+3V3 → R_NTC_PULL (10kΩ) → ADC_NTC
ADC_NTC → NTC (10kΩ @ 25°C) → GND
ADC_NTC → C_FILT_NTC (100nF) → GND
```

---

# NAPPE INTER-CARTES

## Connecteur

JST XH 16 pins, câble 100mm AWG24

## Pinout

| Pin | Signal | Direction | Note |
|-----|--------|-----------|------|
| 1 | 22V_SENSE | C1→C2 | Via diviseur |
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

---

# RÈGLES PLACEMENT PCB

## Carte 1 — Puissance

1. **Star Ground** sur C_BULK négatif
2. Retours HP séparés des masses signal
3. D3 proche MA12070 PVDD
4. LM7812 + MCP1703A groupés, zone fraîche

## Carte 2 — Signal

1. **Minimum 3mm** entre I2C et audio analogique
2. **Plan GND** entre zones numérique et audio
3. I2C face TOP, audio face BOTTOM si 2 couches
4. Condensateurs RIAA éloignés des vias I2C
5. **Plan GND sous I2S** (BTM525 → PCM5102A)

---

# BOM COMPLÈTE V1.7

## Semiconducteurs

| Réf | Composant | Valeur/Type | Package | Qté | Note |
|-----|-----------|-------------|---------|-----|------|
| U1 | MA12070 | Ampli Class-D | QFN-48 | 1 | |
| U2 | OPA2134PA | Op-Amp audio | DIP-8 | 2 | RIAA + Buffer |
| U3 | TDA7439 | Processeur audio | DIP-30 | 1 | |
| U4 | CD4053BE | MUX analogique | DIP-16 | 1 | |
| U5 | AMS1117-3.3 | LDO 3.3V | SOT-223 | 1 | |
| U6 | **LM7812** | Régulateur 12V | TO-220 | 1 | **[NOUVEAU V1.7]** |
| U7 | MCP1703A-5002 | LDO 5V low-noise | TO-92 | 1 | |
| U8 | LM7809 | Régulateur 9V | TO-220 | 1 | |
| U9 | ESP32-S3-WROOM | MCU | Module | 1 | |
| U10 | BTM525 | Bluetooth LDAC | Module | 1 | |
| U11 | PCM5102A | DAC I2S | TSSOP-20 | 1 | |
| D1 | SS54 | Schottky 40V 5A | SMA | 1 | Anti-inversion |
| D2 | SMBJ24CA | TVS bidirectionnel | SMB | 1 | Protection surtension |
| D3 | **1N5822** | Diode 40V 3A | DO-201AD | 1 | **[MODIFIÉ V1.7]** |
| Q1 | Si2302 | N-MOSFET | SOT-23 | 1 | Driver relais |
| Q2 | PC817 | Optocoupleur | DIP-4 | 1 | Isolation |

## Passifs — Résistances

| Valeur | Tolérance | Puissance | Qté | Usage |
|--------|-----------|-----------|-----|-------|
| 750Ω | 1% | 0.25W | 2 | RIAA |
| 1kΩ | 5% | 0.25W | 5 | LED, pull-up |
| 4.7kΩ | 5% | 0.25W | 2 | I2C pull-up |
| 10kΩ | 5% | 0.25W | 10 | Pull-up/down, NTC |
| 33kΩ | 1% | 0.25W | 1 | Diviseur ADC |
| 75kΩ | 1% | 0.25W | 2 | RIAA |
| 100kΩ | 5% | 0.25W | 5 | Bias, diviseur |
| 220kΩ | 1% | 0.25W | 1 | Diviseur ADC |

**NOTE V1.7 :** R_DROP 47Ω 3W **SUPPRIMÉE**

## Passifs — Condensateurs

| Type | Valeur | Tension | Qté | Usage |
|------|--------|---------|-----|-------|
| Céramique | 100nF | 50V | 20 | Découplage |
| Céramique | 10µF | 25V | 10 | Découplage |
| Céramique | 1µF | 25V | 4 | LDO |
| Électrolytique | 220µF | 35V | 1 | C_BULK PVDD |
| Électrolytique | 100µF | 35V | 4 | Filtrage buck |
| Électrolytique | 22µF | 10V | 2 | LDO sortie |
| **FILM** | 0.1µF | 50V | 4 | Entrées RIAA |
| **FILM** | 1µF | 50V | 12 | Couplage TDA |
| **FILM** | 2.2µF | 50V | 4 | Entrées MA12070 |
| **FILM** | 100nF | 50V | 12 | Bass filter TDA |
| **FILM** | 22nF | 50V | 12 | Mid filter TDA |
| **FILM** | 5.6nF | 50V | 2 | Treble filter TDA |
| **FILM** | 3.3nF | 50V | 2 | RIAA |

## Inductances

| Valeur | Courant | Qté | Usage |
|--------|---------|-----|-------|
| 10µH | 3A | 5 | Filtres sortie HP, buck |

## Connecteurs

| Type | Qté | Usage |
|------|-----|-------|
| JST XH-16 | 2 | Nappe inter-cartes |
| JST XH-7P | 1 | Balance BMS |
| JST PH-2P | 1 | NTC BMS |
| Bornier 2P 5mm | 4 | HP L+/-, R+/- |
| Jack 3.5mm | 1 | AUX entrée |
| RCA femelle | 2 | Phono L/R |
| USB-C | 1 | Charge (via BMS) |

## Modules

| Module | Qté | Usage |
|--------|-----|-------|
| MP1584EN buck | 1 | 22V→5V |
| OLED 0.96" I2C | 1 | Affichage |
| Encodeur rotatif | 1 | Navigation |
| Récepteur IR | 1 | Télécommande |
| BMS JBD 6S 20A | 1 | Protection batterie |

## Divers

| Composant | Qté | Usage |
|-----------|-----|-------|
| Relais HF46F-G/12 | 1 | Coupure sécurité |
| Fusible 5A ATO | 1 | Protection surintensité |
| TCO 72°C | 1 | Protection thermique |
| LED 3mm | 1 | Status |

---

# WCCA — ANALYSE PIRE CAS V1.7

## Tableau Récapitulatif

| Composant | Usage Normal | Pire Cas | Rating | Marge | Status |
|-----------|--------------|----------|--------|-------|--------|
| LM7812 | 0.25W | 0.32W | 1W+ | >68% | ✅ |
| MCP1703A | 0.14W | 0.18W | 0.5W | >64% | ✅ |
| LM7809 | 0.32W | 0.81W | 1W+ | >19% | ✅ |
| D3 1N5822 | 90mW | 1.8W | 3W | >40% | ✅ |
| D1 SS54 | 50mW | 2.5W | 5W | >50% | ✅ |
| F1 5A | 2A typ | 5A | 5A | 0% | ✅ |

## Températures Jonction (Ta = 40°C)

| Composant | P_diss | Rth_ja | Tj | Tj_max | Status |
|-----------|--------|--------|----|----|--------|
| MA12070 | 2W | 25°C/W | 90°C | 150°C | ✅ |
| LM7812 | 0.25W | 50°C/W | 52.5°C | 125°C | ✅ |
| MCP1703A | 0.14W | 180°C/W | 65.2°C | 150°C | ✅ |
| LM7809 | 0.32W | 50°C/W | 56°C | 125°C | ✅ |
| D3 1N5822 | 0.09W | 60°C/W | 45.4°C | 150°C | ✅ |

---

# SCHÉMA BLOC V1.7

## Chaîne Alimentation

```
Batterie 6S (18-25.2V)
     │
     ▼
   BMS JBD ──► TCO 72°C ──► K1 Relais ──► F1 5A ──► D1+D2 ──► +22V_RAW
                                                                 │
          ┌──────────────────────────────────────────────────────┼────────────────┐
          │                                                      │                │
          ▼                                                      ▼                ▼
    D3 (1N5822)                                             LM7812           MP1584EN
    Vf = 0.9V                                                  │                 │
          │                                                    ▼                 ▼
          ▼                                               +12V_PRE              +5V
    +PVDD_SAFE                                                 │                 │
    (24.3V max)                                                ▼                 ▼
          │                                             MCP1703A            AMS1117
          ▼                                                    │                 │
      MA12070                                                  ▼                 ▼
                                                        +5V_ANALOG            +3V3
                                                              │
                                                              ▼
                                                   OPA2134, CD4053
```

## Chaîne Audio

```
BTM525 ──I2S──► PCM5102A ─────────────────────────────┐
                                                      │
AUX Jack ─────────────────────────────────────────────┼──► CD4053 ──► TDA7439 ──► Buffer ──► Nappe ──► MA12070
                                                      │      MUX       EQ 3-bd    OPA2134             Class-D
Phono RCA ──► OPA2134 RIAA ───────────────────────────┘
```

---

# HISTORIQUE VERSIONS

| Version | Date | Modifications |
|---------|------|---------------|
| **V1.7** | 13/12/2025 | **Audit ChatGPT : LM7812 ajouté, R_DROP supprimée, D3→1N5822** |
| V1.6 | 13/12/2025 | Audit exhaustif : R_DROP 3W, Star Ground, règles PCB |
| V1.5 | 13/12/2025 | Audit Gemini : D3 PVDD, TVS, nappe blindée |
| V1.4 | 13/12/2025 | TDA7439 EQ 3 bandes |
| V1.3 | 12/12/2025 | Préampli phono OPA2134 |
| V1.2 | 12/12/2025 | Pinouts explicites |
| V1.1 | 11/12/2025 | Sécurité 5 niveaux |
| V1.0 | 11/12/2025 | Architecture initiale |

---

# FIN DOCUMENT V1.7
