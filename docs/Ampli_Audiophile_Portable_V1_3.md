# AMPLIFICATEUR AUDIOPHILE PORTABLE V1.3

## DOCUMENT TECHNIQUE COMPLET — PINOUTS EXPLICITES

**Version :** 1.3  
**Date :** 12 décembre 2025  
**Auteur :** Mehdi + Claude  

---

## CHANGELOG

| Version | Date | Modifications |
|---------|------|---------------|
| V1.0 | 11/12/2025 | Architecture initiale, choix composants |
| V1.1 | 11/12/2025 | Sécurité 5 niveaux, optimisation budget |
| V1.2 | 12/12/2025 | Pinouts explicites BMS, BT, DAC, Ampli, Nappe |
| V1.3 | 12/12/2025 | Préampli Phono, Volume MCP4261, ESP32-S3, OLED, Encodeur, Headers test, Façade |

---

## SPECIFICATIONS CIBLES

| Paramètre | Valeur |
|-----------|--------|
| Puissance | 2 × 20W RMS @ 8Ω |
| THD+N | < 0,01% @ 1W |
| SNR | > 110dB (ampli), > 65dB (phono) |
| Impédance HP | 4-8Ω |
| Sources | Bluetooth LDAC, AUX 3.5mm, Phono MM |
| Batterie | LiPo 6S (18-25,2V), 3000-5000mAh |
| Autonomie | 4-6h @ volume moyen |
| Dimensions | Carte 1: 80×100mm, Carte 2: 80×120mm |

---

## ARCHITECTURE BI-CARTE

### Justification séparation

- **Isolation EMI** : PWM 400kHz (ampli) loin du préampli phono 5mV
- **Thermique** : MA12070 chauffe, séparé des circuits sensibles
- **Debug** : Test indépendant puissance vs signal
- **Maintenance** : Remplacement carte défaillante sans tout refaire

### Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────────────┐
│                         FAÇADE                                      │
│  [OLED] [Encodeur] [LED BT] [LED Charge] [IR] [Jack AUX] [RCA Phono]│
└─────────────────────────────────────────────────────────────────────┘
         │              │                            │
         └──────────────┼────────────────────────────┘
                        │
┌───────────────────────┴───────────────────────────────────────────┐
│                     CARTE 2 — SIGNAL/CONTRÔLE                      │
│  ESP32-S3 | OLED | Encodeur | BTM525 | PCM5102A | OPA2134 | MCP4261│
│                         80 × 120 mm                                │
└───────────────────────────┬───────────────────────────────────────┘
                            │ Nappe J_INTER (14 pins)
┌───────────────────────────┴───────────────────────────────────────┐
│                     CARTE 1 — PUISSANCE                            │
│  BMS | Sécurité 5 niv | Buck | LDO | MA12070 | Connecteurs HP      │
│                         80 × 100 mm                                │
└───────────────────────────────────────────────────────────────────┘
         │                                              │
    [Batterie 6S]                              [HP Gauche] [HP Droit]
```

---

# ═══════════════════════════════════════════════════════════════════
# CARTE 1 — PUISSANCE (80 × 100 mm)
# ═══════════════════════════════════════════════════════════════════

---

## C1-A — MODULE BMS (JBD SP22S003B 6S 20A)

### Identification module

- **Référence :** JBD SP22S003B (ou équivalent "6S 20A BMS")
- **Fournisseur :** AliExpress, Banggood (~8-15€)
- **Dimensions :** ~60×45×4mm

### Connecteurs physiques du BMS

**Fils puissance (AWG12-14) :**
- **C-** (noir) : Négatif commun charge/décharge → GND_BATT
- **B-** (bleu) : Négatif pack batterie (Cell1-) → Pack négatif
- **P+** (rouge) : Positif pack (Cell6+) traverse le BMS → +BATT_BMS

**Connecteur balance JST XH-7P (B7B-XH-A) :**
```
Pin B0 (noir)   → Cell1 négatif (0V référence)
Pin B1 (rouge)  → Jonction Cell1/Cell2 (4,2V max)
Pin B2 (orange) → Jonction Cell2/Cell3 (8,4V max)
Pin B3 (jaune)  → Jonction Cell3/Cell4 (12,6V max)
Pin B4 (vert)   → Jonction Cell4/Cell5 (16,8V max)
Pin B5 (bleu)   → Jonction Cell5/Cell6 (21,0V max)
Pin B6 (violet) → Cell6 positif (25,2V max)
```

**Connecteur NTC JST PH-2P (S2B-PH-K-S) :**
- Pin 1 → Sonde NTC 10kΩ (collée sur cellule centrale)
- Pin 2 → Sonde NTC 10kΩ (autre fil)

### Câblage BMS sur carte

```
Pack Cell6+ ─────────────► BMS P+ ─────────────► +BATT_BMS (sortie vers circuit)
Pack Cell1- ─────────────► BMS B-
GND_BATT ◄───────────────── BMS C-

Balance JST ◄──────────────── Nappe 7 fils vers pack
NTC JST ◄──────────────────── 2 fils vers sonde température
```

### Specs critiques (vérifier avant achat)

| Paramètre | Valeur requise |
|-----------|----------------|
| Surcharge cellule | 4,25V ±25mV coupure |
| Sur-décharge | 2,8V ±50mV coupure |
| Balance actif | @ 4,15V, courant 50-80mA |
| Surintensité | 25A coupure |
| Court-circuit | < 100µs coupure |
| OTP (sur-température) | 60°C coupure |
| UTP (sous-température) | -20°C coupure |

---

## C1-B — SÉCURITÉ BATTERIE 5 NIVEAUX

### Vue d'ensemble chaîne sécurité

```
+PACK ──► BMS ──► TCO ──► Relais K1 ──► Fusible F1 ──► D1+D2 ──► +22V_RAW
          N1      N2         N3            N4           N5
```

### NIVEAU 1 — BMS (Protection primaire)

Voir section C1-A ci-dessus.

### NIVEAU 2 — TCO (Thermal Cut-Off)

**Composant :** Aupo A4-1A-F (72°C, 10A, réarmable)  
**Fournisseur :** TME, AliExpress (~1€)  
**Package :** Radial, fils AWG18

**Câblage :**
```
+BATT_BMS (depuis BMS P+) → TCO fil 1
TCO fil 2 → +BATT_TCO (vers relais)
```

**Installation physique :**
- Corps TCO collé thermiquement (colle thermique ou kapton) sur cellule centrale du pack
- Si T_cellule > 72°C → TCO ouvre → coupe alimentation

### NIVEAU 3 — Relais de sécurité K1

**Composant :** HF46F-G/12-HS1T (12V, 10A, SPST-NO)  
**Fournisseur :** TME, LCSC (~2€)  
**Package :** Through-hole, pas 5mm

**Pinout relais (vue dessus) :**
```
        ┌─────────┐
   1 ───┤  Bobine │─── 2
        │   K1    │
   3 ───┤   COM   │─── 4 (NO)
        └─────────┘

Pin 1 (Bobine+) ← +BATT_TCO via R_K1 (100Ω 1W)
Pin 2 (Bobine-) ← Q_SAFE drain (Si2302)
Pin 3 (COM)     ← +BATT_TCO (entrée puissance)
Pin 4 (NO)      → +BATT_PROT (sortie si relais fermé)
```

**Driver relais via opto-coupleur PC817 :**

**PC817 pinout (DIP-4) :**
```
        ┌───∪───┐
   1 ───┤●      │─── 4
  Anode │ PC817 │ Collecteur
   2 ───┤       │─── 3
 Cathode│       │ Émetteur
        └───────┘
```

**Câblage driver :**
```
+3V3 (nappe) → R_LED (1kΩ) → PC817 pin1 (Anode)
PC817 pin2 (Cathode) ← ESP32 GPIO42 (MCU_SAFE_EN via nappe)

+BATT_TCO → R_PULL (10kΩ) → PC817 pin4 (Collecteur)
PC817 pin3 (Émetteur) → Q_SAFE gate (Si2302)
Q_SAFE source → GND_PWR
Q_SAFE drain → K1 pin2 (Bobine-)
```

**Logique :**
- MCU_SAFE_EN = LOW → LED PC817 ON → Phototransistor ON → Q_SAFE ON → K1 excité → Contact fermé → Batterie connectée
- MCU_SAFE_EN = HIGH → LED OFF → Q_SAFE OFF → K1 ouvert → Batterie déconnectée

**Composants driver :**
- PC817 : Opto-coupleur DIP-4
- Q_SAFE : Si2302 (N-MOS SOT-23, Vgs_th 1,4V, Rds_on 50mΩ)
- R_LED : 1kΩ 0,25W
- R_PULL : 10kΩ 0,25W
- R_K1 : 100Ω 1W (limite courant bobine ~120mA)

### NIVEAU 4 — Fusible F1

**Composant :** Littelfuse 0297005.WXNV (5A, 32V, Fast-blow, ATO)  
**Support :** Keystone 3557-2 (porte-fusible ATO pour PCB)  
**Fournisseur :** TME, Mouser (~0,50€ + 1€ support)

**Câblage :**
```
+BATT_PROT (depuis K1 NO) → F1 entrée
F1 sortie → +BATT_FUSE
```

### NIVEAU 5 — Protection inversion + TVS

**Composants :**
- D1 : SS54 (Schottky 40V 5A, SMA) — anti-inversion
- D2 : SMBJ26CA (TVS 26V 600W bidirectionnel, SMB) — surtensions

**Câblage :**
```
+BATT_FUSE → D1 anode
D1 cathode (SS54) → +22V_RAW

+22V_RAW → D2 pin1 (SMBJ26CA)
D2 pin2 → GND_PWR
```

**Fonction :**
- D1 : Bloque courant inverse si batterie branchée à l'envers (Vf ~0,4V)
- D2 : Clampe surtensions >26V (load dump, transitoires)

---

## C1-C — ALIMENTATION (Buck + LDO)

### Convertisseur Buck 22V → 5V

**Composant :** MP1584EN (module) ou XL4015 (module)  
**Specs :** Vin 4,5-28V, Vout ajustable, 3A  
**Fournisseur :** AliExpress (~2€ module prêt)

**Câblage module buck :**
```
+22V_RAW → VIN+ module buck
GND_PWR → VIN- module buck
VOUT+ module buck → +5V_BUCK
VOUT- module buck → GND_PWR
```

**Réglage :** Ajuster potentiomètre pour Vout = 5,0V ±0,1V

**Découplage sortie :**
```
+5V_BUCK → C_BUCK (100µF/16V électro + 100nF céram) → GND_PWR
+5V_BUCK → +5V (rail distribué)
```

### LDO 5V → 3,3V

**Composant :** AMS1117-3.3 (SOT-223)  
**Specs :** Vin 4,5-12V, Vout 3,3V, 1A, dropout 1,2V

**Pinout AMS1117 (SOT-223, vue dessus) :**
```
        ┌─────────┐
   GND ─┤1   TAB  │─ VOUT (tab = pin2)
  VOUT ─┤2       │
   VIN ─┤3       │
        └─────────┘
```

**Câblage :**
```
+5V → AMS1117 pin3 (VIN)
AMS1117 pin1 (GND) → GND_PWR
AMS1117 pin2/TAB (VOUT) → +3V3

Découplage entrée : +5V → C (10µF céram) → GND_PWR
Découplage sortie : +3V3 → C (10µF céram + 100nF) → GND_PWR
```

---

## C1-D — AMPLIFICATEUR MA12070 (QFN-48)

### Identification

**Composant :** Infineon MA12070  
**Package :** QFN-48 (7×7mm, pas 0,5mm)  
**Fournisseur :** Mouser, LCSC (~8€)

### Pinout QFN-48 (vue dessus, pin1 = dot coin)

```
         Pin 37-48 (haut)
        ┌─────────────────┐
Pin 25- │●                │ -Pin 36
   36   │    MA12070      │
        │    QFN-48       │
Pin 13- │                 │ -Pin 24
   24   │    (PAD=GND)    │
        └─────────────────┘
         Pin 1-12 (bas)
```

### Connexions par fonction

**ALIMENTATION PVDD (22V) :**
```
+22V_FILT → MA12070 pin2 (PVDD)
+22V_FILT → MA12070 pin3 (PVDD)
+22V_FILT → MA12070 pin45 (PVDD)
+22V_FILT → MA12070 pin46 (PVDD)

GND_PWR → MA12070 pin1 (PGND)
GND_PWR → MA12070 pin4 (PGND)
GND_PWR → MA12070 pin44 (PGND)
GND_PWR → MA12070 pin47 (PGND)
GND_PWR → MA12070 PAD (thermal)
```

**Découplage PVDD (au plus près des pins) :**
```
+22V_FILT → C_PVDD1 (100µF/35V électro low-ESR) → GND_PWR
+22V_FILT → C_PVDD2 (100nF/50V X7R) → GND_PWR (près pin2-3)
+22V_FILT → C_PVDD3 (100nF/50V X7R) → GND_PWR (près pin45-46)
```

**ALIMENTATION AVDD/DVDD (5V) :**
```
+5V → MA12070 pin24 (AVDD)
+5V → MA12070 pin27 (DVDD)
+5V → MA12070 pin28 (CPVDD)

GND_SIG → MA12070 pin23 (AGND)
GND_SIG → MA12070 pin26 (DGND)
```

**Découplage AVDD/DVDD :**
```
+5V → C_AVDD (10µF + 100nF) → GND_SIG (près pin24)
+5V → C_DVDD (10µF + 100nF) → GND_SIG (près pin27)
+5V → C_CPVDD (1µF) → GND_PWR (près pin28)
```

**FLYING CAPACITORS (obligatoire, multi-level) :**
```
MA12070 pin5 (CF0P) ↔ C_FLY0 (1µF/50V X7R) ↔ MA12070 pin6 (CF0N)
MA12070 pin42 (CF1P) ↔ C_FLY1 (1µF/50V X7R) ↔ MA12070 pin43 (CF1N)
```

**GATE DRIVER CAPACITORS :**
```
MA12070 pin7 (CGD0P) ↔ C_GD0 (100nF) ↔ MA12070 pin8 (CGD0N)
MA12070 pin40 (CGD1P) ↔ C_GD1 (100nF) ↔ MA12070 pin41 (CGD1N)
```

**ENTRÉES AUDIO (depuis nappe J_INTER) :**
```
J_INTER pin6 (AUDIO_L) → C_INL (2,2µF film) → MA12070 pin18 (IN0P)
MA12070 pin17 (IN0N) → C_INL_N (2,2µF film) → GND_SIG

J_INTER pin7 (AUDIO_R) → C_INR (2,2µF film) → MA12070 pin19 (IN1P)
MA12070 pin20 (IN1N) → C_INR_N (2,2µF film) → GND_SIG
```

**SORTIES HP (BTL différentiel) :**
```
MA12070 pin9 (OUT0P) → Filtre LC → SPK_L+ → J_SPK_L pin1
MA12070 pin10 (OUT0N) → Filtre LC → SPK_L- → J_SPK_L pin2

MA12070 pin38 (OUT1P) → Filtre LC → SPK_R+ → J_SPK_R pin1
MA12070 pin39 (OUT1N) → Filtre LC → SPK_R- → J_SPK_R pin2
```

**Filtre LC sortie (par canal) :**
```
OUTxP → L1 (10µH) → SPK_x+
OUTxP → C_F1 (1µF/50V film) → OUTxN
OUTxN → L2 (10µH) → SPK_x-
```

**CONTRÔLE I2C :**
```
J_INTER pin8 (SDA) → MA12070 pin30 (SDA)
J_INTER pin9 (SCL) → MA12070 pin29 (SCL)

MA12070 pin31 (ADDR0) → GND_SIG (adresse I2C = 0x20)
MA12070 pin32 (ADDR1) → GND_SIG
```

**CONTRÔLE ENABLE/MUTE :**
```
J_INTER pin10 (AMP_EN) → MA12070 pin34 (/ENABLE) — actif LOW
J_INTER pin11 (AMP_MUTE) → MA12070 pin33 (/MUTE) — actif LOW

MA12070 pin35 (/ERROR) → R_ERR (10kΩ) → +3V3 (pull-up)
MA12070 pin35 (/ERROR) → J_INTER pin12 (AMP_ERR)
```

**PINS NON CONNECTÉS :**
```
MA12070 pin11-16, pin21-22, pin25, pin36-37, pin48 → NC
```

---

## C1-E — CONNECTEURS HP

### Bornes à vis J_SPK

**Composant :** Bornier à vis 2 positions, pas 5,08mm  
**Référence :** Phoenix Contact 1935161 (ou générique)  
**Quantité :** 2 (gauche + droit)

**Câblage :**
```
J_SPK_L pin1 ← SPK_L+ (depuis filtre LC)
J_SPK_L pin2 ← SPK_L-

J_SPK_R pin1 ← SPK_R+
J_SPK_R pin2 ← SPK_R-
```

---

## C1-F — HEADER TEST J_TEST_C1 (2×8 pins)

### Connecteur

**Type :** Header mâle 2×8, pas 2,54mm, shrouded (avec détrompeur)  
**Référence :** Amphenol 10129382-916001BLF ou générique  
**Fournisseur :** TME, Mouser (~0,50€)

### Assignation pins

```
        ┌─────────────┐
   Pin1 │● ○│ Pin2
   Pin3 │○ ○│ Pin4
   Pin5 │○ ○│ Pin6
   Pin7 │○ ○│ Pin8
   Pin9 │○ ○│ Pin10
  Pin11 │○ ○│ Pin12
  Pin13 │○ ○│ Pin14
  Pin15 │○ ○│ Pin16
        └─────────────┘
         (détrompeur)
```

| Pin | Signal | Type | Valeur attendue |
|-----|--------|------|-----------------|
| 1 | +BATT_BMS | Alim | 18-25,2V |
| 2 | +BATT_TCO | Alim | = Pin1 si TCO OK |
| 3 | +BATT_PROT | Alim | = Pin1 si K1 fermé |
| 4 | +BATT_FUSE | Alim | = Pin3 si F1 OK |
| 5 | +22V_RAW | Alim | Pin4 - 0,4V (Vf D1) |
| 6 | +22V_FILT | Alim | = Pin5, ripple <50mV |
| 7 | +5V | Alim | 4,9-5,1V |
| 8 | +3V3 | Alim | 3,2-3,4V |
| 9 | GND_PWR | Masse | 0V (référence) |
| 10 | GND_SIG | Masse | 0V |
| 11 | AMP_IN_L | Signal | Audio gauche |
| 12 | AMP_IN_R | Signal | Audio droit |
| 13 | SPK_L+ | Sortie | PWM filtré |
| 14 | SPK_L- | Sortie | PWM filtré |
| 15 | AMP_ERR | Logic | HIGH = OK |
| 16 | SAFE_EN | Logic | LOW = batterie ON |

### Câblage header vers points de mesure

```
J_TEST_C1 pin1 ← +BATT_BMS (après BMS)
J_TEST_C1 pin2 ← +BATT_TCO (après TCO)
J_TEST_C1 pin3 ← +BATT_PROT (après K1)
J_TEST_C1 pin4 ← +BATT_FUSE (après F1)
J_TEST_C1 pin5 ← +22V_RAW (après D1)
J_TEST_C1 pin6 ← +22V_FILT (après découplage)
J_TEST_C1 pin7 ← +5V (rail)
J_TEST_C1 pin8 ← +3V3 (rail)
J_TEST_C1 pin9 ← GND_PWR
J_TEST_C1 pin10 ← GND_SIG
J_TEST_C1 pin11 ← MA12070 pin18 (avant condensateur)
J_TEST_C1 pin12 ← MA12070 pin19 (avant condensateur)
J_TEST_C1 pin13 ← SPK_L+ (après filtre)
J_TEST_C1 pin14 ← SPK_L- (après filtre)
J_TEST_C1 pin15 ← MA12070 pin35 (/ERROR)
J_TEST_C1 pin16 ← PC817 pin2 (signal SAFE_EN)
```

---

## C1-G — CONNECTEUR NAPPE J_INTER (côté Carte 1)

### Connecteur

**Type :** JST XH 14 pins  
**Embase PCB :** B14B-XH-A (LF)(SN)  
**Fournisseur :** TME, LCSC (~0,50€)

### Câblage côté Carte 1

```
J_INTER pin1 (22V_SENSE) ← +22V_FILT via diviseur R (100kΩ/10kΩ)
J_INTER pin2 (+5V) ← Rail +5V
J_INTER pin3 (+3V3) ← Rail +3V3
J_INTER pin4 (GND_PWR) ← GND_PWR
J_INTER pin5 (GND_SIG) ← GND_SIG
J_INTER pin6 (AUDIO_L) → C_INL → MA12070 pin18
J_INTER pin7 (AUDIO_R) → C_INR → MA12070 pin19
J_INTER pin8 (SDA) → MA12070 pin30
J_INTER pin9 (SCL) → MA12070 pin29
J_INTER pin10 (AMP_EN) → MA12070 pin34
J_INTER pin11 (AMP_MUTE) → MA12070 pin33
J_INTER pin12 (AMP_ERR) ← MA12070 pin35
J_INTER pin13 (NTC_ADC) ← Jonction NTC/R_NTC diviseur
J_INTER pin14 (SAFE_EN) → PC817 pin1 via R_LED
```

---

# ═══════════════════════════════════════════════════════════════════
# CARTE 2 — SIGNAL/CONTRÔLE (80 × 120 mm)
# ═══════════════════════════════════════════════════════════════════

---

## C2-A — MODULE BLUETOOTH BTM525 (QCC5125)

### Identification

**Module :** SJR-BTM525  
**Chipset :** Qualcomm QCC5125  
**Dimensions :** 13×18×2,8mm, 54 pads  
**Fournisseur :** Audiophonics, AliExpress (~15-25€)  
**Codecs :** LDAC, aptX Adaptive, aptX HD, AAC, SBC

### Pinout 54 pads (vue dessus, antenne en haut)

```
        Côté antenne (haut)
  48-54 │                    │ 37-47
        │   SJR-BTM525       │
        │    QCC5125         │
   1-10 │                    │ 11-36
        Côté bas (pins 1-10, 11-36)
```

### Connexions utilisées

**ALIMENTATION :**
```
+3V3 → BTM525 pin14 (VBAT)
+3V3 → C_BT1 (10µF) → GND_SIG (découplage)
+3V3 → C_BT2 (100nF) → GND_SIG (découplage HF)
+3V3 → BTM525 pin23 (VDD_PADS1)
+3V3 → BTM525 pin24 (VDD_PADS3_7)

GND_SIG → BTM525 pin15 (GND)
GND_SIG → BTM525 pin21 (GND)
GND_SIG → BTM525 pin38 (GND)
GND_SIG → BTM525 pin44 (GND)
GND_SIG → BTM525 pin45 (GND)
GND_SIG → BTM525 pin47 (GND)

BTM525 pin22 (1V8) → C_1V8 (100nF) → GND_SIG (régulateur interne)
```

**SORTIE I2S :**
```
BTM525 pin5 (PIO16/PCM_CLK) → I2S_BCLK → PCM5102A pin4 (BCK)
BTM525 pin4 (PIO17/PCM_SYNC) → I2S_LRCK → PCM5102A pin3 (LRCK)
BTM525 pin3 (PIO18/PCM_DOUT) → I2S_DATA → PCM5102A pin2 (DIN)
```

**CONTRÔLE :**
```
+3V3 → R_SYSCTRL (10kΩ) → BTM525 pin13 (SYS_CTRL) — toujours ON
+3V3 → R_RST (10kΩ) → BTM525 pin32 (PIO1/RESET) — pull-up reset

BTM525 pin16 (LED0) → R_LED (1kΩ) → LED_BT (→ GND_SIG) — status optionnel
BTM525 pin17 (LED1) → BT_CONNECTED → ESP32 GPIO4 (status connecté)
```

**ANTENNE :**
```
BTM525 pin46 (BT_RF) → Antenne PCB intégrée ou connecteur U.FL
```

---

## C2-B — DAC PCM5102A (TSSOP-20)

### Identification

**Composant :** Texas Instruments PCM5102A  
**Package :** TSSOP-20  
**Fournisseur :** LCSC, Mouser (~3€)  
**Specs :** 32-bit, 384kHz, SNR 112dB, -100dB THD

### Pinout TSSOP-20 (vue dessus)

```
        ┌────∪────┐
  XSMT ─┤1     20├─ AVDD
   DIN ─┤2     19├─ AGND
  LRCK ─┤3     18├─ CAPP
   BCK ─┤4     17├─ CPGND
   SCK ─┤5     16├─ VNEG
   FLT ─┤6     15├─ CAPM
  DEMP ─┤7     14├─ DGND
   FMT ─┤8     13├─ DVDD
 VOUTL ─┤9     12├─ LDOO
 VOUTR ─┤10    11├─ NC
        └─────────┘
```

### Connexions complètes

**CONFIGURATION :**
```
+3V3 → PCM5102A pin1 (XSMT) — soft mute désactivé
GND_SIG → PCM5102A pin5 (SCK) — mode PLL auto, pas de SCK externe
GND_SIG → PCM5102A pin6 (FLT) — filtre normal
GND_SIG → PCM5102A pin7 (DEMP) — de-emphasis OFF
+3V3 → PCM5102A pin8 (FMT) — format I2S standard
```

**ENTRÉES I2S (depuis BTM525) :**
```
I2S_DATA → PCM5102A pin2 (DIN)
I2S_LRCK → PCM5102A pin3 (LRCK)
I2S_BCLK → PCM5102A pin4 (BCK)
```

**SORTIES AUDIO :**
```
PCM5102A pin9 (VOUTL) → C_DACL (1µF film) → DAC_OUT_L
PCM5102A pin10 (VOUTR) → C_DACR (1µF film) → DAC_OUT_R
```

**ALIMENTATION :**
```
+5V → R_DVDD (10Ω) → PCM5102A pin13 (DVDD)
PCM5102A pin13 → C_DVDD (10µF) → GND_SIG

+5V → R_AVDD (10Ω) → PCM5102A pin20 (AVDD)
PCM5102A pin20 → C_AVDD (10µF) → GND_SIG

GND_SIG → PCM5102A pin14 (DGND)
GND_SIG → PCM5102A pin19 (AGND)

PCM5102A pin12 (LDOO) → C_LDO (100nF) → GND_SIG — LDO interne
```

**CHARGE PUMP (alim négative interne) :**
```
PCM5102A pin18 (CAPP) ↔ C_CP1 (2,2µF) ↔ PCM5102A pin15 (CAPM)
PCM5102A pin16 (VNEG) → C_VNEG (2,2µF) → GND_SIG
PCM5102A pin17 (CPGND) → GND_SIG
```

**PIN NON CONNECTÉ :**
```
PCM5102A pin11 (NC) → Non connecté
```

---

## C2-C — PRÉAMPLI PHONO RIAA (OPA2134)

### Fonction

Amplifier et égaliser le signal d'une cellule phono MM (Moving Magnet) :
- Niveau entrée : ~5mV typique
- Niveau sortie : ~500mV (gain ~40dB = ×100)
- Courbe RIAA : Compensation fréquentielle normalisée

### Architecture

```
PHONO_IN_L ──► [Étage 1: Gain + RIAA] ──► [Étage 2: Buffer] ──► PHONO_OUT_L
                    OPA2134 (1/2)              OPA2134 (2/2)
```

### Composant

**Ampli-op :** Texas Instruments OPA2134PA (DIP-8) ou OPA2134UA (SOIC-8)  
**Fournisseur :** Mouser, TME (~4€)  
**Specs :** Dual, FET input, faible bruit 8nV/√Hz, THD 0,00008%

### Pinout OPA2134 (DIP-8, vue dessus)

```
        ┌────∪────┐
 OUT_A ─┤1      8├─ V+
  IN-A ─┤2      7├─ OUT_B
  IN+A ─┤3      6├─ IN-B
    V- ─┤4      5├─ IN+B
        └─────────┘
```

### Schéma préampli RIAA (canal gauche, idem droit)

**ÉTAGE 1 — Gain + RIAA (OPA2134 section A) :**

```
                    C_RIAA1
                   ┌──┤├──┐
                   │ 100nF │
           R_RIAA1 │       │ R_RIAA2
          ┌─[75kΩ]─┴───────┴─[750Ω]─┐
          │                          │
          │    ┌─────────────────────┤
          │    │                     │
          │   ─┴─ C_RIAA2           │
          │   ─┬─ 3,3nF             │
          │    │                     │
          │    └─────────┬───────────┘
          │              │
PHONO_IN_L ──► R_IN ─────┼──► OPA2134 pin3 (IN+A)
               1kΩ       │
                         │
              ┌──────────┘
              │
              └──────────────► OPA2134 pin2 (IN-A)
                              
OPA2134 pin1 (OUT_A) ──► RIAA_OUT_L
```

**Composants réseau RIAA :**
```
R_IN : 1kΩ (impédance entrée, charge cellule)
R_RIAA1 : 75kΩ ±1% (définit gain + pôle 50Hz)
R_RIAA2 : 750Ω ±1% (zéro 500Hz)
C_RIAA1 : 100nF ±5% film (pôle 2122Hz)
C_RIAA2 : 3,3nF ±5% film (zéro 500Hz)
```

**Calculs RIAA :**
- Gain DC : 1 + R_RIAA1/R_IN = 1 + 75k/1k = 76 (~38dB)
- Pôle 1 (50Hz) : 1/(2π × R_RIAA1 × C_RIAA1) = 21Hz
- Zéro (500Hz) : 1/(2π × R_RIAA2 × C_RIAA2) = 64Hz
- Pôle 2 (2122Hz) : 1/(2π × R_RIAA2 × C_RIAA1) = 2122Hz

**ÉTAGE 2 — Buffer (OPA2134 section B) :**

```
RIAA_OUT_L → C_COUPLE (1µF film) → OPA2134 pin5 (IN+B)
OPA2134 pin6 (IN-B) → OPA2134 pin7 (OUT_B) — feedback 100%
OPA2134 pin7 (OUT_B) → PHONO_OUT_L
```

**ALIMENTATION OPA2134 :**
```
+5V → R_VD (100Ω) → V_MID → C_MID (100µF électro) → GND_SIG
V_MID = +2,5V (point milieu virtuel, rail simple)

V_MID → OPA2134 pin3 (IN+A) via R_BIAS (100kΩ)
+5V → OPA2134 pin8 (V+)
GND_SIG → OPA2134 pin4 (V-)
```

Note : Fonctionnement sur rail simple +5V avec point milieu virtuel à 2,5V.

### Câblage complet canal gauche

```
J_PHONO pin1 (PHONO_L) → C_IN_L (100nF) → R_IN_L (1kΩ) → IN+A
IN+A → R_BIAS_L (100kΩ) → V_MID

Réseau RIAA entre IN-A et OUT_A (voir schéma ci-dessus)

OUT_A → C_COUPLE_L (1µF) → IN+B
IN-B → OUT_B (buffer)
OUT_B → PHONO_OUT_L
```

### Câblage complet canal droit (identique, utilise U3)

Un second OPA2134 (U3) pour le canal droit, câblage symétrique.

---

## C2-D — SÉLECTEUR DE SOURCE

### Fonction

Commuter entre 3 sources :
1. Bluetooth (DAC_OUT)
2. AUX (jack 3.5mm)
3. Phono (préampli RIAA)

### Solution : Multiplexeur analogique CD4053

**Composant :** CD4053BE (DIP-16) ou 74HC4053 (SOIC-16)  
**Fournisseur :** TME, LCSC (~0,30€)

### Pinout CD4053 (DIP-16)

```
        ┌────∪────┐
  IN_BY ─┤1     16├─ VDD
  IN_BX ─┤2     15├─ IN_B2
  OUT_B ─┤3     14├─ IN_AY
   OUT_C─┤4     13├─ IN_AX
  IN_CX ─┤5     12├─ OUT_A
  IN_CY ─┤6     11├─ SEL_A
    VSS ─┤7     10├─ SEL_B
    VEE ─┤8      9├─ SEL_C
        └─────────┘
```

### Utilisation pour 3 sources stéréo

On utilise 2× les sections A et B pour gauche/droit.

**Câblage CD4053 (U4) :**
```
+5V → CD4053 pin16 (VDD)
GND_SIG → CD4053 pin7 (VSS)
GND_SIG → CD4053 pin8 (VEE) — pas de tension négative

Source BT gauche : DAC_OUT_L → CD4053 pin13 (IN_AX)
Source AUX gauche : AUX_IN_L → CD4053 pin14 (IN_AY)
Sortie gauche : CD4053 pin12 (OUT_A) → SW_OUT_L

Source BT droit : DAC_OUT_R → CD4053 pin2 (IN_BX)
Source AUX droit : AUX_IN_R → CD4053 pin1 (IN_BY)
Sortie droit : CD4053 pin3 (OUT_B) → SW_OUT_R

Sélection : ESP32 GPIO5 → CD4053 pin11 (SEL_A)
Sélection : ESP32 GPIO6 → CD4053 pin10 (SEL_B)
CD4053 pin9 (SEL_C) → GND_SIG (non utilisé)
```

**Logique sélection :**
| SEL_A | SEL_B | Source |
|-------|-------|--------|
| 0 | 0 | Bluetooth (X) |
| 1 | 0 | AUX (Y) |

Pour le Phono, on utilise un second switch ou on le câble sur une entrée libre.

**Alternative simplifiée : Phono sur IN_B2/IN_A2**

En fait, CD4053 a 3 switches SPDT, on peut faire :
- Switch A : BT_L / AUX_L → OUT_A (gauche)
- Switch B : BT_R / AUX_R → OUT_B (droit)
- Switch C : PHONO_L / SW_OUT → Mix final

Pour simplifier, utilisons 2× CD4053 ou un CD4052 (4 entrées par canal).

**Solution retenue : 2 signaux de contrôle GPIO**
```
GPIO5 (SRC_SEL0) + GPIO6 (SRC_SEL1) → Décodage 3 sources
```

---

## C2-E — VOLUME NUMÉRIQUE MCP4261

### Fonction

Potentiomètre numérique double (stéréo) contrôlé par SPI :
- Entrée : Signal audio source sélectionnée
- Sortie : Signal atténué selon position (0-255)
- Contrôle : ESP32 via SPI

### Composant

**Référence :** Microchip MCP4261-103E/P (10kΩ, DIP-14)  
**Alternative :** MCP4261-103E/ST (TSSOP-14)  
**Fournisseur :** Mouser, TME (~1,50€)  
**Specs :** Dual 10kΩ, 256 pas, SPI, 5V

### Pinout MCP4261 (DIP-14)

```
        ┌────∪────┐
    CS ─┤1     14├─ VDD
   SCK ─┤2     13├─ SDO
   SDI ─┤3     12├─ SHDN
   VSS ─┤4     11├─ WP
  P1B  ─┤5     10├─ P0B
  P1W  ─┤6      9├─ P0W
  P1A  ─┤7      8├─ P0A
        └─────────┘
```

**Fonctions pins :**
- P0A, P0W, P0B : Potentiomètre 0 (canal gauche) — A=High, W=Wiper, B=Low
- P1A, P1W, P1B : Potentiomètre 1 (canal droit)
- CS, SCK, SDI, SDO : Interface SPI
- SHDN : Shutdown (actif LOW)
- WP : Write Protect

### Câblage MCP4261 (U5)

**ALIMENTATION :**
```
+5V → MCP4261 pin14 (VDD)
GND_SIG → MCP4261 pin4 (VSS)
```

**INTERFACE SPI :**
```
ESP32 GPIO10 (SPI_CS_VOL) → MCP4261 pin1 (CS)
ESP32 GPIO12 (SPI_CLK) → MCP4261 pin2 (SCK)
ESP32 GPIO11 (SPI_MOSI) → MCP4261 pin3 (SDI)
MCP4261 pin13 (SDO) → ESP32 GPIO13 (SPI_MISO)
```

**CONTRÔLE :**
```
+5V → MCP4261 pin12 (SHDN) — toujours actif
+5V → MCP4261 pin11 (WP) — write protect désactivé
```

**CANAL GAUCHE (POT0) :**
```
SW_OUT_L (depuis sélecteur) → MCP4261 pin8 (P0A)
MCP4261 pin9 (P0W) → VOL_OUT_L (vers buffer sortie)
MCP4261 pin10 (P0B) → GND_SIG
```

**CANAL DROIT (POT1) :**
```
SW_OUT_R (depuis sélecteur) → MCP4261 pin7 (P1A)
MCP4261 pin6 (P1W) → VOL_OUT_R (vers buffer sortie)
MCP4261 pin5 (P1B) → GND_SIG
```

### Commande SPI

Format commande 16-bit :
```
[Addr 4-bit][Cmd 2-bit][Data 10-bit]
```

Exemples :
- Pot0 écriture : Addr=0x00, Cmd=00, Data=position
- Pot1 écriture : Addr=0x01, Cmd=00, Data=position

Code ESP32 (exemple) :
```cpp
void setVolume(uint8_t left, uint8_t right) {
  // Pot0 (gauche)
  digitalWrite(CS_VOL, LOW);
  SPI.transfer16(0x0000 | left);
  digitalWrite(CS_VOL, HIGH);
  
  // Pot1 (droit)
  digitalWrite(CS_VOL, LOW);
  SPI.transfer16(0x1000 | right);
  digitalWrite(CS_VOL, HIGH);
}
```

---

## C2-F — BUFFER SORTIE (OPA2134)

### Fonction

Buffer basse impédance avant envoi vers nappe (évite pertes dans câble).

### Composant

**Référence :** OPA2134PA (DIP-8) — même que préampli phono  
**Désignation :** U6

### Câblage buffer (configuration suiveur)

```
VOL_OUT_L → C_BUF_L (1µF film) → OPA2134 pin3 (IN+A)
OPA2134 pin2 (IN-A) → OPA2134 pin1 (OUT_A) — feedback 100%
OPA2134 pin1 (OUT_A) → AUDIO_L (vers nappe J_INTER pin6)

VOL_OUT_R → C_BUF_R (1µF film) → OPA2134 pin5 (IN+B)
OPA2134 pin6 (IN-B) → OPA2134 pin7 (OUT_B)
OPA2134 pin7 (OUT_B) → AUDIO_R (vers nappe J_INTER pin7)

+5V → OPA2134 pin8 (V+)
GND_SIG → OPA2134 pin4 (V-)
```

---

## C2-G — MICROCONTRÔLEUR ESP32-S3

### Module sélectionné

**Référence :** ESP32-S3-WROOM-1-N8R8 (8MB Flash, 8MB PSRAM)  
**Alternative dev :** ESP32-S3-DevKitC-1 (pour prototypage)  
**Fournisseur :** LCSC, Mouser (~5€ module, ~15€ devkit)

### Pinout ESP32-S3-WROOM-1 (38 pins, vue dessus)

```
                    ┌─────────────┐
              GND ─┤1          38├─ GND
             3V3 ─┤2          37├─ IO36
              EN ─┤3          36├─ IO35
             IO4 ─┤4          35├─ IO34 (pas dispo)
             IO5 ─┤5          34├─ IO33 (pas dispo)
             IO6 ─┤6          33├─ IO26 (pas dispo)
             IO7 ─┤7          32├─ IO21
            IO15 ─┤8          31├─ IO20
            IO16 ─┤9          30├─ IO19
            IO17 ─┤10         29├─ IO18
            IO18 ─┤11         28├─ USB_D-
            IO8  ─┤12         27├─ USB_D+
             IO3 ─┤13         26├─ IO17
            IO46 ─┤14         25├─ IO16
             IO9 ─┤15         24├─ IO15
            IO10 ─┤16         23├─ IO14
            IO11 ─┤17         22├─ IO13
            IO12 ─┤18         21├─ IO12
            IO13 ─┤19         20├─ IO11
             GND ─┤20         19├─ IO10
                    └─────────────┘
```

### Assignation GPIO

| GPIO | Fonction | Direction | Connecté à |
|------|----------|-----------|------------|
| IO1 | I2C_SDA | Bidir | MA12070, OLED |
| IO2 | I2C_SCL | Out | MA12070, OLED |
| IO4 | BT_CONNECTED | In | BTM525 pin17 |
| IO5 | SRC_SEL0 | Out | CD4053 SEL_A |
| IO6 | SRC_SEL1 | Out | CD4053 SEL_B |
| IO10 | SPI_CS_VOL | Out | MCP4261 CS |
| IO11 | SPI_MOSI | Out | MCP4261 SDI |
| IO12 | SPI_CLK | Out | MCP4261 SCK |
| IO13 | SPI_MISO | In | MCP4261 SDO |
| IO15 | AMP_EN | Out | Nappe → MA12070 /EN |
| IO16 | AMP_MUTE | Out | Nappe → MA12070 /MUTE |
| IO17 | AMP_ERR | In | Nappe ← MA12070 /ERR |
| IO18 | ENC_A | In | Encodeur A |
| IO19 | ENC_B | In | Encodeur B |
| IO20 | ENC_SW | In | Encodeur Switch |
| IO21 | IR_RX | In | Récepteur IR (option) |
| IO38 | ADC_BATT | In | Nappe ← Diviseur 22V |
| IO39 | ADC_NTC | In | Nappe ← Diviseur NTC |
| IO42 | SAFE_EN | Out | Nappe → PC817 LED |

### Câblage ESP32-S3

**ALIMENTATION :**
```
+3V3 → ESP32 pin2 (3V3)
GND_SIG → ESP32 pin1 (GND)
GND_SIG → ESP32 pin20 (GND)

+3V3 → R_EN (10kΩ) → ESP32 pin3 (EN) — enable pull-up
ESP32 pin3 (EN) → C_EN (100nF) → GND_SIG — anti-rebond reset
```

**I2C (MA12070 + OLED) :**
```
ESP32 IO1 → SDA_BUS
ESP32 IO2 → SCL_BUS

SDA_BUS → R_SDA (4,7kΩ) → +3V3 (pull-up)
SCL_BUS → R_SCL (4,7kΩ) → +3V3 (pull-up)

SDA_BUS → J_INTER pin8 (vers MA12070)
SCL_BUS → J_INTER pin9 (vers MA12070)
SDA_BUS → OLED pin3 (SDA)
SCL_BUS → OLED pin4 (SCL)
```

**SPI (MCP4261) :**
```
ESP32 IO10 → MCP4261 pin1 (CS)
ESP32 IO11 → MCP4261 pin3 (SDI)
ESP32 IO12 → MCP4261 pin2 (SCK)
ESP32 IO13 ← MCP4261 pin13 (SDO)
```

**CONTRÔLE AMPLI :**
```
ESP32 IO15 → J_INTER pin10 (AMP_EN)
ESP32 IO16 → J_INTER pin11 (AMP_MUTE)
ESP32 IO17 ← J_INTER pin12 (AMP_ERR)
```

**ADC MONITORING :**
```
J_INTER pin1 (22V_SENSE) → Diviseur 100k/10k → ESP32 IO38 (ADC)
J_INTER pin13 (NTC_ADC) → ESP32 IO39 (ADC)
```

**SÉCURITÉ BATTERIE :**
```
ESP32 IO42 → J_INTER pin14 (SAFE_EN)
```

**BLUETOOTH STATUS :**
```
BTM525 pin17 (LED1) → ESP32 IO4
```

**SÉLECTION SOURCE :**
```
ESP32 IO5 → CD4053 pin11 (SEL_A)
ESP32 IO6 → CD4053 pin10 (SEL_B)
```

**ENCODEUR :**
```
ESP32 IO18 ← ENC_A (via R 10kΩ pull-up)
ESP32 IO19 ← ENC_B (via R 10kΩ pull-up)
ESP32 IO20 ← ENC_SW (via R 10kΩ pull-up)
```

**IR RÉCEPTEUR (option) :**
```
ESP32 IO21 ← IR_OUT (module TSOP38238)
```

---

## C2-H — ÉCRAN OLED

### Module sélectionné

**Référence :** SSD1306 128×64 I2C (module 4 pins)  
**Dimensions :** 27×27mm (écran 0,96")  
**Fournisseur :** AliExpress (~3€)

### Pinout module OLED (4 pins)

```
┌─────────────────┐
│   ┌─────────┐   │
│   │  OLED   │   │
│   │ 128×64  │   │
│   └─────────┘   │
│ [GND][VCC][SCL][SDA]
│   1    2    3    4
└─────────────────┘
```

### Câblage OLED

```
GND_SIG → OLED pin1 (GND)
+3V3 → OLED pin2 (VCC)
SCL_BUS → OLED pin3 (SCL)
SDA_BUS → OLED pin4 (SDA)
```

**Adresse I2C :** 0x3C (par défaut)

---

## C2-I — ENCODEUR ROTATIF

### Composant

**Référence :** Bourns PEC11R-4215F-S0024 ou générique EC11  
**Fournisseur :** TME, AliExpress (~1€)  
**Type :** 24 impulsions/tour, avec switch

### Pinout encodeur EC11 (5 pins)

```
      ┌───────┐
      │   ●   │  ← Axe rotatif
      │ ┌───┐ │
      │ │   │ │
      │ └───┘ │
      └───────┘
     [A][C][B] [1][2]
      │  │  │   │  │
      │  │  │   │  └─ Switch COM
      │  │  │   └──── Switch NO
      │  │  └──────── Signal B
      │  └─────────── Common (GND)
      └────────────── Signal A
```

### Câblage encodeur

```
Encodeur pin A → R_ENCA (10kΩ) → +3V3 (pull-up)
Encodeur pin A → ESP32 IO18 (ENC_A)

Encodeur pin C → GND_SIG (common)

Encodeur pin B → R_ENCB (10kΩ) → +3V3 (pull-up)
Encodeur pin B → ESP32 IO19 (ENC_B)

Encodeur pin 1 → R_ENCSW (10kΩ) → +3V3 (pull-up)
Encodeur pin 1 → ESP32 IO20 (ENC_SW)

Encodeur pin 2 → GND_SIG (switch common)
```

**Logique :**
- Rotation droite : A puis B passe LOW
- Rotation gauche : B puis A passe LOW
- Appui : SW passe LOW

---

## C2-J — CONNECTEURS ENTRÉE AUDIO

### Jack AUX 3.5mm (J_AUX)

**Composant :** PJ-307 ou SJ1-3523N (jack 3.5mm stéréo, 5 pins)  
**Fournisseur :** LCSC, TME (~0,30€)

**Pinout jack 3.5mm (5 pins) :**
```
Pin 1 (Tip) : Left
Pin 2 (Ring) : Right  
Pin 3 (Sleeve) : GND
Pin 4 : Tip switch (NC quand inséré)
Pin 5 : Ring switch
```

**Câblage :**
```
J_AUX pin1 (Tip) → C_AUXL (100nF) → AUX_IN_L
J_AUX pin2 (Ring) → C_AUXR (100nF) → AUX_IN_R
J_AUX pin3 (Sleeve) → GND_SIG
```

### RCA Phono (J_PHONO)

**Composant :** Embase RCA double PCB  
**Fournisseur :** TME, AliExpress (~1€)

**Câblage :**
```
J_PHONO_L (centre) → C_PHONOL (100nF) → PHONO_IN_L → Préampli
J_PHONO_L (masse) → GND_SIG

J_PHONO_R (centre) → C_PHONOR (100nF) → PHONO_IN_R → Préampli
J_PHONO_R (masse) → GND_SIG
```

---

## C2-K — HEADER TEST J_TEST_C2 (2×10 pins)

### Connecteur

**Type :** Header mâle 2×10, pas 2,54mm, shrouded  
**Référence :** Amphenol 10129382-920001BLF ou générique  
**Fournisseur :** TME, Mouser (~0,60€)

### Assignation pins

```
        ┌─────────────┐
   Pin1 │● ○│ Pin2
   Pin3 │○ ○│ Pin4
   Pin5 │○ ○│ Pin6
   Pin7 │○ ○│ Pin8
   Pin9 │○ ○│ Pin10
  Pin11 │○ ○│ Pin12
  Pin13 │○ ○│ Pin14
  Pin15 │○ ○│ Pin16
  Pin17 │○ ○│ Pin18
  Pin19 │○ ○│ Pin20
        └─────────────┘
```

| Pin | Signal | Type | Valeur attendue |
|-----|--------|------|-----------------|
| 1 | +5V | Alim | 4,9-5,1V |
| 2 | +3V3 | Alim | 3,2-3,4V |
| 3 | GND_SIG | Masse | 0V |
| 4 | I2S_BCLK | Digital | 3,072MHz @ 48kHz |
| 5 | I2S_LRCK | Digital | 48kHz |
| 6 | I2S_DATA | Digital | Signal I2S |
| 7 | DAC_OUT_L | Analog | 0-2,1Vrms |
| 8 | DAC_OUT_R | Analog | 0-2,1Vrms |
| 9 | PHONO_IN_L | Analog | ~5mV (très faible) |
| 10 | PHONO_OUT_L | Analog | ~500mV |
| 11 | SW_OUT_L | Analog | Source sélectionnée |
| 12 | SW_OUT_R | Analog | Source sélectionnée |
| 13 | VOL_OUT_L | Analog | Volume ajusté |
| 14 | VOL_OUT_R | Analog | Volume ajusté |
| 15 | SDA | I2C | Data |
| 16 | SCL | I2C | Clock |
| 17 | BT_STATUS | Logic | HIGH = connecté |
| 18 | AMP_EN | Logic | Enable ampli |
| 19 | AMP_MUTE | Logic | Mute ampli |
| 20 | ENC_SW | Logic | Bouton encodeur |

### Câblage header vers points de mesure

```
J_TEST_C2 pin1 ← +5V (rail)
J_TEST_C2 pin2 ← +3V3 (rail)
J_TEST_C2 pin3 ← GND_SIG
J_TEST_C2 pin4 ← I2S_BCLK (entre BTM525 et PCM5102A)
J_TEST_C2 pin5 ← I2S_LRCK
J_TEST_C2 pin6 ← I2S_DATA
J_TEST_C2 pin7 ← DAC_OUT_L (sortie PCM5102A)
J_TEST_C2 pin8 ← DAC_OUT_R
J_TEST_C2 pin9 ← PHONO_IN_L (entrée préampli)
J_TEST_C2 pin10 ← PHONO_OUT_L (sortie préampli)
J_TEST_C2 pin11 ← SW_OUT_L (sortie sélecteur)
J_TEST_C2 pin12 ← SW_OUT_R
J_TEST_C2 pin13 ← VOL_OUT_L (sortie MCP4261)
J_TEST_C2 pin14 ← VOL_OUT_R
J_TEST_C2 pin15 ← SDA_BUS
J_TEST_C2 pin16 ← SCL_BUS
J_TEST_C2 pin17 ← ESP32 IO4 (BT_STATUS)
J_TEST_C2 pin18 ← ESP32 IO15 (AMP_EN)
J_TEST_C2 pin19 ← ESP32 IO16 (AMP_MUTE)
J_TEST_C2 pin20 ← ESP32 IO20 (ENC_SW)
```

---

## C2-L — CONNECTEUR NAPPE J_INTER (côté Carte 2)

### Câblage côté Carte 2

```
J_INTER pin1 (22V_SENSE) → Diviseur 100k/10k → ESP32 IO38
J_INTER pin2 (+5V) → Rail +5V Carte 2
J_INTER pin3 (+3V3) → Rail +3V3 Carte 2
J_INTER pin4 (GND_PWR) → GND_PWR
J_INTER pin5 (GND_SIG) → GND_SIG
J_INTER pin6 (AUDIO_L) ← Buffer OPA2134 OUT_A
J_INTER pin7 (AUDIO_R) ← Buffer OPA2134 OUT_B
J_INTER pin8 (SDA) ← SDA_BUS (ESP32 IO1)
J_INTER pin9 (SCL) ← SCL_BUS (ESP32 IO2)
J_INTER pin10 (AMP_EN) ← ESP32 IO15
J_INTER pin11 (AMP_MUTE) ← ESP32 IO16
J_INTER pin12 (AMP_ERR) → ESP32 IO17
J_INTER pin13 (NTC_ADC) → Diviseur NTC → ESP32 IO39
J_INTER pin14 (SAFE_EN) ← ESP32 IO42
```

---

# ═══════════════════════════════════════════════════════════════════
# NAPPE INTER-CARTES J_INTER (14 pins)
# ═══════════════════════════════════════════════════════════════════

### Connecteur et câble

**Type :** JST XH 14 pins  
**Embase PCB :** B14B-XH-A (LF)(SN) — 1 par carte = 2 total  
**Boîtier câble :** XHP-14  
**Contacts :** SXH-001T-P0.6 (×14)  
**Câble :** 100mm, AWG24-26  
**Fournisseur :** TME, LCSC (~0,50€/embase + 2€ câble)

### Assignation pins complète

| Pin | Signal | Dir | Carte 1 | Carte 2 | Couleur fil |
|-----|--------|-----|---------|---------|-------------|
| 1 | 22V_SENSE | C1→C2 | Diviseur 22V | ADC ESP32 | Orange |
| 2 | +5V | C1→C2 | Buck out | Rail +5V | Rouge |
| 3 | +3V3 | C1→C2 | LDO out | Rail +3V3 | Rose |
| 4 | GND_PWR | — | GND_PWR | GND_PWR | Noir |
| 5 | GND_SIG | — | GND_SIG | GND_SIG | Noir |
| 6 | AUDIO_L | C2→C1 | MA12070 IN | Buffer out | Blanc |
| 7 | AUDIO_R | C2→C1 | MA12070 IN | Buffer out | Gris |
| 8 | SDA | ↔ | MA12070 | ESP32 | Bleu |
| 9 | SCL | C2→C1 | MA12070 | ESP32 | Jaune |
| 10 | AMP_EN | C2→C1 | MA12070 /EN | ESP32 | Vert |
| 11 | AMP_MUTE | C2→C1 | MA12070 /MUTE | ESP32 | Violet |
| 12 | AMP_ERR | C1→C2 | MA12070 /ERR | ESP32 | Marron |
| 13 | NTC_ADC | C1→C2 | Diviseur NTC | ADC ESP32 | Blanc/Noir |
| 14 | SAFE_EN | C2→C1 | PC817 LED | ESP32 | Vert/Blanc |

---

# ═══════════════════════════════════════════════════════════════════
# ÉLÉMENTS FAÇADE
# ═══════════════════════════════════════════════════════════════════

## Éléments montés en façade (accessibles utilisateur)

| Élément | Fixation | Câble vers carte | Longueur |
|---------|----------|------------------|----------|
| Écran OLED 0,96" | Vis M2 ou collé | Nappe 4 fils vers C2 | 100mm |
| Encodeur EC11 | Écrou façade | Nappe 5 fils vers C2 | 100mm |
| LED BT (bleue 3mm) | Support LED | 2 fils vers C2 | 100mm |
| LED Charge (rouge/vert) | Support LED | 2 fils vers C1 (BMS) | 150mm |
| Jack AUX 3.5mm | Écrou façade | 3 fils vers C2 | 100mm |
| RCA Phono (×2) | Vis façade | 4 fils vers C2 | 100mm |
| Interrupteur ON/OFF | Perçage façade | 2 fils vers C1 | 150mm |
| Récepteur IR (option) | Collé façade | 3 fils vers C2 | 100mm |

## Éléments restant sur les cartes (internes)

**Carte 1 :**
- BMS + connecteurs batterie
- Sécurité 5 niveaux (TCO, relais, fusible, diodes)
- Alimentation (buck, LDO)
- Amplificateur MA12070
- Borniers HP (accessibles arrière boîtier)
- Header test J_TEST_C1
- Connecteur nappe J_INTER

**Carte 2 :**
- Module Bluetooth BTM525
- DAC PCM5102A
- Préampli Phono OPA2134
- Sélecteur CD4053
- Volume MCP4261
- Buffer OPA2134
- ESP32-S3
- Header test J_TEST_C2
- Connecteur nappe J_INTER

## Perçages façade recommandés

```
┌────────────────────────────────────────────────────────────────┐
│                         FAÇADE                                  │
│                                                                │
│   ┌──────────┐                              ○ ○                │
│   │  OLED    │     [ENC]                   BT CHG              │
│   │ 128×64   │       ○                     LED LED             │
│   └──────────┘                                                 │
│    27×20mm      Ø7mm                       Ø3mm Ø3mm           │
│                                                                │
│                                                                │
│   (○)AUX     (○)PHONO_L   (○)PHONO_R       □ IR               │
│   Ø6mm         Ø8mm         Ø8mm          5×5mm               │
│                                                                │
│                                      [ON/OFF]                  │
│                                        Ø12mm                   │
└────────────────────────────────────────────────────────────────┘
```

---

# ═══════════════════════════════════════════════════════════════════
# BOM COMPLÈTE
# ═══════════════════════════════════════════════════════════════════

## Semiconducteurs

| Réf | Composant | Valeur/Fonction | Package | Qté | Prix unit |
|-----|-----------|-----------------|---------|-----|-----------|
| U1 | MA12070 | Ampli Class-D 2×20W | QFN-48 | 1 | 8€ |
| U2 | OPA2134PA | Dual Op-Amp Audio | DIP-8 | 1 | 4€ |
| U3 | OPA2134PA | Dual Op-Amp Phono R | DIP-8 | 1 | 4€ |
| U4 | CD4053BE | Triple MUX analogique | DIP-16 | 1 | 0,30€ |
| U5 | MCP4261-103 | Dual Pot Numérique 10k | DIP-14 | 1 | 1,50€ |
| U6 | OPA2134PA | Dual Op-Amp Buffer | DIP-8 | 1 | 4€ |
| U7 | AMS1117-3.3 | LDO 3,3V 1A | SOT-223 | 1 | 0,30€ |
| U8 | ESP32-S3-WROOM | MCU WiFi/BT | Module | 1 | 5€ |
| U9 | BTM525 | Module BT QCC5125 | Module | 1 | 20€ |
| U10 | PCM5102A | DAC I2S 32-bit | TSSOP-20 | 1 | 3€ |
| U11 | PC817 | Opto-coupleur | DIP-4 | 1 | 0,20€ |
| Q1 | Si2302 | N-MOS driver relais | SOT-23 | 1 | 0,15€ |
| D1 | SS54 | Schottky 40V 5A | SMA | 1 | 0,30€ |
| D2 | SMBJ26CA | TVS 26V 600W | SMB | 1 | 0,50€ |
| K1 | HF46F-G/12 | Relais 12V 10A | TH | 1 | 2€ |

## Passifs - Résistances

| Réf | Valeur | Tolérance | Puissance | Qté | Usage |
|-----|--------|-----------|-----------|-----|-------|
| R diverses | 1kΩ | 5% | 0,25W | 5 | Pull-up, LED |
| R diverses | 4,7kΩ | 5% | 0,25W | 2 | I2C pull-up |
| R diverses | 10kΩ | 5% | 0,25W | 10 | Pull-up/down |
| R diverses | 100kΩ | 5% | 0,25W | 3 | Diviseur, bias |
| R_K1 | 100Ω | 5% | 1W | 1 | Limite bobine |
| R_RIAA | 75kΩ | 1% | 0,25W | 2 | Préampli RIAA |
| R_RIAA | 750Ω | 1% | 0,25W | 2 | Préampli RIAA |
| R_RIAA | 1kΩ | 1% | 0,25W | 2 | Entrée phono |
| R_DIV | 100kΩ | 1% | 0,25W | 1 | Diviseur 22V |
| R_DIV | 10kΩ | 1% | 0,25W | 1 | Diviseur 22V |

## Passifs - Condensateurs

| Réf | Valeur | Type | Tension | Qté | Usage |
|-----|--------|------|---------|-----|-------|
| C diverses | 100nF | Céramique X7R | 50V | 20 | Découplage |
| C diverses | 10µF | Céramique X5R | 16V | 10 | Découplage |
| C diverses | 100µF | Électro low-ESR | 35V | 3 | Filtrage PVDD |
| C diverses | 1µF | Film polypropylène | 50V | 10 | Couplage audio |
| C diverses | 2,2µF | Film | 50V | 6 | Couplage ampli |
| C_RIAA | 100nF | Film 5% | 50V | 2 | Réseau RIAA |
| C_RIAA | 3,3nF | Film 5% | 50V | 2 | Réseau RIAA |
| C_FLY | 1µF | X7R | 50V | 2 | Flying cap MA12070 |
| C_CP | 2,2µF | Céramique | 16V | 3 | Charge pump DAC |

## Passifs - Inductances

| Réf | Valeur | Courant | Qté | Usage |
|-----|--------|---------|-----|-------|
| L1-L4 | 10µH | 3A | 4 | Filtre LC sortie ampli |

## Connecteurs

| Réf | Type | Qté | Usage |
|-----|------|-----|-------|
| J_INTER | JST XH 14P | 2 | Nappe inter-cartes |
| J_TEST_C1 | Header 2×8 shrouded | 1 | Test Carte 1 |
| J_TEST_C2 | Header 2×10 shrouded | 1 | Test Carte 2 |
| J_SPK | Bornier 2P pas 5,08mm | 2 | Sorties HP |
| J_AUX | Jack 3.5mm stéréo | 1 | Entrée AUX |
| J_PHONO | Embase RCA double | 1 | Entrée Phono |
| J_BAL | JST XH 7P | 1 | Balance BMS |
| J_NTC | JST PH 2P | 1 | Sonde température |

## Modules

| Réf | Description | Qté | Prix |
|-----|-------------|-----|------|
| BMS | JBD SP22S003B 6S 20A | 1 | 12€ |
| Buck | MP1584EN module | 1 | 2€ |
| OLED | SSD1306 128×64 I2C | 1 | 3€ |

## Divers

| Réf | Description | Qté | Prix |
|-----|-------------|-----|------|
| TCO | Aupo A4-1A-F 72°C | 1 | 1€ |
| F1 | Fusible 5A ATO + support | 1 | 1,50€ |
| Encodeur | EC11 24 imp/tour | 1 | 1€ |
| LED | 3mm bleue + rouge + verte | 3 | 0,30€ |
| Nappe | IDC 14 fils 100mm | 1 | 2€ |

## TOTAL ESTIMÉ

| Catégorie | Sous-total |
|-----------|------------|
| Semiconducteurs | ~55€ |
| Passifs | ~10€ |
| Connecteurs | ~8€ |
| Modules | ~17€ |
| Divers | ~6€ |
| **TOTAL** | **~96€** |

(hors PCB, boîtier, batterie, HP)

---

# ═══════════════════════════════════════════════════════════════════
# NOTES DE CONCEPTION
# ═══════════════════════════════════════════════════════════════════

## Masses séparées

- **GND_PWR** : Masse puissance (batterie, ampli, buck)
- **GND_SIG** : Masse signal (audio, logique, MCU)
- **Jonction unique** : Point étoile près de J_INTER

## Ordre de mise sous tension

1. Batterie connectée → BMS actif
2. ESP32 démarre → SAFE_EN = LOW → Relais fermé
3. +22V disponible → Buck démarre → +5V → LDO → +3V3
4. ESP32 initialise I2C → Configure MA12070
5. AMP_EN = LOW → Ampli actif
6. AMP_MUTE = HIGH → Unmute après 500ms (anti-pop)

## Protection thermique logicielle

```cpp
if (readNTC() > 60) {
  setMute(true);
  if (readNTC() > 70) {
    setSafeEN(false);  // Coupe batterie
  }
}
```

---

# ═══════════════════════════════════════════════════════════════════
# FIN DOCUMENT V1.3
# ═══════════════════════════════════════════════════════════════════
