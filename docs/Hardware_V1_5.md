# AMPLIFICATEUR AUDIOPHILE PORTABLE V1.5

## DOCUMENT TECHNIQUE COMPLET — 

**Version :** 1.5  
**Date :** 13 decembre 2025  
**Auteur :** Mehdi + Claude  

---

## CHANGELOG

| Version | Date | Modifications |
|---------|------|---------------|
| V1.0 | 11/12/2025 | Architecture initiale, choix composants |
| V1.1 | 11/12/2025 | Securite 5 niveaux, optimisation budget |
| V1.2 | 12/12/2025 | Pinouts explicites BMS, BT, DAC, Ampli, Nappe |
| V1.3 | 12/12/2025 | Preampli Phono, Volume MCP4261, ESP32-S3, OLED, Encodeur |
| V1.4 | 13/12/2025 | TDA7439 3-band EQ, Loudness, Spatial |
| **V1.5** | **13/12/2025** | **CORRECTIONS AUDIT GEMINI: Protection PVDD, TVS adaptee, Nappe blindee** |

---

## CORRECTIONS V1.5 

### 🔴 CRITIQUES (obligatoires)

| # | Probleme identifie | Correction V1.5 |
|---|-------------------|-----------------|
| 1 | **MA12070 PVDD 26V max** vs 6S 25.2V = marge 0.8V dangereuse | **Diode Schottky D3** en serie PVDD → -0.5V → 24.7V max |
| 2 | **TVS SMBJ26CA** Vbr=28.9V trop haut | **SMBJ24CA** Vbr=26.7V → clamp AVANT destruction ampli |
| 3 | **Crosstalk nappe** Audio adjacent I2C | **Nappe 16 pins** avec GND blindage entre Audio et I2C |

### 🟡 RECOMMANDES (integres)

| # | Amelioration | Implementation |
|---|--------------|----------------|
| 4 | I2C timeout | `Wire.setTimeOut(10)` dans firmware |
| 5 | Alimentation OPA2134 | Rail +5V_ANALOG dedie via LDO faible bruit |
| 6 | Vibrations nappe | Note: Securiser mecaniquement connecteurs |

---

## SPECIFICATIONS CIBLES

| Parametre | Valeur |
|-----------|--------|
| Puissance | 2 x 20W RMS @ 8 Ohms |
| THD+N | < 0,01% @ 1W |
| SNR | > 110dB (ampli), > 65dB (phono) |
| Impedance HP | 4-8 Ohms |
| Sources | Bluetooth LDAC, AUX 3.5mm, Phono MM |
| Egaliseur | 3 bandes +/-14dB (TDA7439) |
| Batterie | LiPo 6S (18-25,2V), 3000-5000mAh |
| **PVDD ampli** | **24.7V max (protege par D3)** |
| Autonomie | 4-6h @ volume moyen |
| Dimensions | Carte 1: 80x100mm, Carte 2: 80x120mm |

---

## ARCHITECTURE BI-CARTE V1.5

### Vue d'ensemble

```
                          FACADE
  [OLED] [Encodeur] [LED BT] [LED Charge] [IR] [Jack AUX] [RCA Phono]
         |              |                            |
         +--------------+----------------------------+
                        |
+-----------------------+---------------------------------------+
|                 CARTE 2 - SIGNAL/CONTROLE                      |
|  ESP32-S3 | OLED | Encodeur | BTM525 | PCM5102A | OPA2134     |
|  CD4053 (Selecteur) | TDA7439 (EQ 3 bandes) | Buffer sortie   |
|                         80 x 120 mm                            |
+----------------------------+----------------------------------+
                             | Nappe J_INTER (16 pins) [V1.5]
+----------------------------+----------------------------------+
|                 CARTE 1 - PUISSANCE                            |
|  BMS | Securite 5 niv | D3 PVDD | Buck | LDO | MA12070 | HP   |
|                         80 x 100 mm                            |
+---------------------------------------------------------------+
         |                                              |
    [Batterie 6S]                              [HP Gauche] [HP Droit]
```

### Chaine audio V1.5

```
SOURCES:
  BT (BTM525 I2S) --> PCM5102A DAC --> +
  AUX (Jack 3.5mm) --------------------> CD4053 MUX --> TDA7439 --> Buffer --> NAPPE --> MA12070
  Phono (OPA2134 RIAA) ---------------> +              (EQ 3 bandes)  OPA2134

Signal flow TDA7439:
  IN --> Gain 0-30dB --> Volume -47dB --> Bass --> Mid --> Treble --> Speaker Att --> OUT

Protection PVDD V1.5:
  +22V_RAW --> D3 (SS54) --> +PVDD_SAFE (24.7V max) --> MA12070
```

---

# ===========================================================================
# CARTE 1 - PUISSANCE (80 x 100 mm) - MISE A JOUR V1.5
# ===========================================================================

---

## C1-A - MODULE BMS (JBD SP22S003B 6S 20A)

### Identification module

- **Reference :** JBD SP22S003B (ou equivalent "6S 20A BMS")
- **Fournisseur :** AliExpress, Banggood (~8-15 EUR)
- **Dimensions :** ~60x45x4mm

### Connecteurs physiques du BMS

**Fils puissance (AWG12-14) :**
- **C-** (noir) : Negatif commun charge/decharge --> GND_BATT
- **B-** (bleu) : Negatif pack batterie (Cell1-) --> Pack negatif
- **P+** (rouge) : Positif pack (Cell6+) traverse le BMS --> +BATT_BMS

**Connecteur balance JST XH-7P (B7B-XH-A) :**
```
Pin B0 (noir)   --> Cell1 negatif (0V reference)
Pin B1 (rouge)  --> Jonction Cell1/Cell2 (4,2V max)
Pin B2 (orange) --> Jonction Cell2/Cell3 (8,4V max)
Pin B3 (jaune)  --> Jonction Cell3/Cell4 (12,6V max)
Pin B4 (vert)   --> Jonction Cell4/Cell5 (16,8V max)
Pin B5 (bleu)   --> Jonction Cell5/Cell6 (21,0V max)
Pin B6 (violet) --> Cell6 positif (25,2V max)
```

**Connecteur NTC JST PH-2P (S2B-PH-K-S) :**
- Pin 1 --> Sonde NTC 10k Ohms (collee sur cellule centrale)
- Pin 2 --> Sonde NTC 10k Ohms (autre fil)

### Cablage BMS sur carte

```
Pack Cell6+ -------------> BMS P+ -------------> +BATT_BMS (sortie vers circuit)
Pack Cell1- -------------> BMS B-
GND_BATT <----------------- BMS C-

Balance JST <-------------- Nappe 7 fils vers pack
NTC JST <------------------ 2 fils vers sonde temperature
```

### Specs critiques (verifier avant achat)

| Parametre | Valeur requise |
|-----------|----------------|
| Surcharge cellule | 4,25V +/-25mV coupure |
| Sur-decharge | 2,8V +/-50mV coupure |
| Balance actif | @ 4,15V, courant 50-80mA |
| Surintensite | 25A coupure |
| Court-circuit | < 100us coupure |
| OTP (sur-temperature) | 60C coupure |
| UTP (sous-temperature) | -20C coupure |

---

## C1-B - SECURITE BATTERIE 5 NIVEAUX

### Vue d'ensemble chaine securite

```
+PACK --> BMS --> TCO --> Relais K1 --> Fusible F1 --> D1+D2 --> +22V_RAW
          N1      N2         N3            N4           N5
```

### NIVEAU 1 - BMS (Protection primaire)

Voir section C1-A ci-dessus.

### NIVEAU 2 - TCO (Thermal Cut-Off)

**Composant :** Aupo A4-1A-F (72C, 10A, rearmable)  
**Fournisseur :** TME, AliExpress (~1 EUR)  
**Package :** Radial, fils AWG18

**Cablage :**
```
+BATT_BMS (depuis BMS P+) --> TCO fil 1
TCO fil 2 --> +BATT_TCO (vers relais)
```

**Installation physique :**
- Corps TCO colle thermiquement (colle thermique ou kapton) sur cellule centrale du pack
- Si T_cellule > 72C --> TCO ouvre --> coupe alimentation

### NIVEAU 3 - Relais de securite K1

**Composant :** HF46F-G/12-HS1T (12V, 10A, SPST-NO)  
**Fournisseur :** TME, LCSC (~2 EUR)  
**Package :** Through-hole, pas 5mm

**Pinout relais (vue dessus) :**
```
        +--------+
   1 ---|  Bobine|--- 2
        |   K1   |
   3 ---|   COM  |--- 4 (NO)
        +--------+

Pin 1 (Bobine+) <-- +BATT_TCO via R_K1 (100 Ohms 1W)
Pin 2 (Bobine-) <-- Q_SAFE drain (Si2302)
Pin 3 (COM)     <-- +BATT_TCO (entree puissance)
Pin 4 (NO)      --> +BATT_PROT (sortie si relais ferme)
```

**Driver relais via opto-coupleur PC817 :**

**PC817 pinout (DIP-4) :**
```
        +---U---+
   1 ---|o      |--- 4
  Anode | PC817 | Collecteur
   2 ---|       |--- 3
 Cathode|       | Emetteur
        +-------+
```

**Cablage driver :**
```
+3V3 (nappe) --> R_LED (1k Ohms) --> PC817 pin1 (Anode)
PC817 pin2 (Cathode) <-- ESP32 GPIO42 (MCU_SAFE_EN via nappe)

+BATT_TCO --> R_PULL (10k Ohms) --> PC817 pin4 (Collecteur)
PC817 pin3 (Emetteur) --> Q_SAFE gate (Si2302)
Q_SAFE source --> GND_PWR
Q_SAFE drain --> K1 pin2 (Bobine-)
```

**Logique :**
- MCU_SAFE_EN = LOW --> LED PC817 ON --> Phototransistor ON --> Q_SAFE ON --> K1 excite --> Contact ferme --> Batterie connectee
- MCU_SAFE_EN = HIGH --> LED OFF --> Q_SAFE OFF --> K1 ouvert --> Batterie deconnectee

**Composants driver :**
- PC817 : Opto-coupleur DIP-4
- Q_SAFE : Si2302 (N-MOS SOT-23, Vgs_th 1,4V, Rds_on 50m Ohms)
- R_LED : 1k Ohms 0,25W
- R_PULL : 10k Ohms 0,25W
- R_K1 : 100 Ohms 1W (limite courant bobine ~120mA)

### NIVEAU 4 - Fusible F1

**Composant :** Littelfuse 0297005.WXNV (5A, 32V, Fast-blow, ATO)  
**Support :** Keystone 3557-2 (porte-fusible ATO pour PCB)  
**Fournisseur :** TME, Mouser (~0,50 EUR + 1 EUR support)

**Cablage :**
```
+BATT_PROT (depuis K1 NO) --> F1 entree
F1 sortie --> +BATT_FUSE
```

### NIVEAU 5 - Protection inversion + TVS [MODIFIE V1.5]

**Composants :**
- D1 : SS54 (Schottky 40V 5A, SMA) - anti-inversion
- **D2 : SMBJ24CA** (TVS **24V** 600W bidirectionnel, SMB) - surtensions [CHANGE V1.5]

**Cablage :**
```
+BATT_FUSE --> D1 anode
D1 cathode (SS54) --> +22V_RAW

+22V_RAW --> D2 pin1 (SMBJ24CA)
D2 pin2 --> GND_PWR
```

**Justification changement TVS V1.5 :**
```
AVANT (SMBJ26CA):
  Vwm = 26V (stand-off)
  Vbr = 28.9V min (breakdown)
  → Clamp APRES destruction MA12070 (26V max)

APRES (SMBJ24CA):
  Vwm = 24V (stand-off)
  Vbr = 26.7V min (breakdown)  
  → Clamp AVANT 26V = protection MA12070
```

---

## C1-C - PROTECTION PVDD AMPLI [NOUVEAU V1.5]

### Probleme identifie (Audit Gemini)

**MA12070 Absolute Maximum PVDD = 26.0V**

Batterie 6S pleine charge = 25.2V  
Back EMF / Pumping effect Class-D = +0.5V a +1V possible  
→ Risque depassement 26V → **DESTRUCTION AMPLI**

### Solution V1.5 : Diode Schottky D3 serie

**Composant :** D3 = SS54 (Schottky 40V 5A, SMA)  
**Vf @ 2A** : 0.5V typique  
**Package :** SMA  
**Prix :** ~0.30 EUR

**Cablage :**
```
+22V_RAW --> D3 anode (SS54)
D3 cathode --> +PVDD_SAFE

+PVDD_SAFE --> C_PVDD (100uF + 100nF) --> MA12070 PVDD (pins 1,2,3,4)
```

**Calcul protection :**
```
Batterie pleine : 25.2V
Apres D3 : 25.2V - 0.5V = 24.7V
Marge vs 26V max : 1.3V (vs 0.8V avant)

Back EMF +0.8V : 24.7V + 0.8V = 25.5V < 26V ✅
Pire cas +1V : 24.7V + 1.0V = 25.7V < 26V ✅
```

**Dissipation D3 :**
```
P_D3 = Vf × I_ampli = 0.5V × 1A (crête) = 0.5W
SS54 rating : 5A, Pd = 2W (avec dissipation PCB)
→ Marge confortable ✅
```

**Decouplage PVDD renforce :**
```
+PVDD_SAFE --> C_BULK (220uF 35V electro low-ESR) --> GND
+PVDD_SAFE --> C_HF (100nF ceramic X7R) --> GND
+PVDD_SAFE --> C_UHF (100nF ceramic X7R) --> GND (pres pins MA12070)
```

---

## C1-D - ALIMENTATION

### Buck DC-DC (22V --> 5V)

**Module :** MP1584EN (3A)  
**Fournisseur :** AliExpress (~2 EUR)

```
+22V_RAW --> MP1584 VIN
MP1584 GND --> GND_PWR
MP1584 VOUT --> +5V_RAW

Filtrage sortie:
+5V_RAW --> L_FILT (10uH) --> +5V
+5V --> C_FILT (100uF + 100nF) --> GND_PWR
```

### LDO (5V --> 3,3V)

**Composant :** AMS1117-3.3  
**Package :** SOT-223

```
+5V --> AMS1117 VIN (pin 3)
AMS1117 GND (pin 1) --> GND_PWR
AMS1117 VOUT (pin 2) --> +3V3

Decouplage:
+5V --> C_IN (10uF) --> GND
+3V3 --> C_OUT (22uF + 100nF) --> GND
```

### LDO Audio [NOUVEAU V1.5]

**Composant :** MCP1703A-5002E/TO (5V, 250mA, ultra-low noise)  
**Package :** TO-92  
**Fournisseur :** TME, Mouser (~0.60 EUR)

**Justification :**
Le Buck MP1584EN a un ripple HF (~1MHz) qui peut polluer les OPA2134.  
LDO dedie pour la section audio = SNR optimal.

```
+22V_RAW --> R_DROP (47 Ohms 1W) --> +12V_PRE
+12V_PRE --> MCP1703 VIN (pin 3)
MCP1703 GND (pin 2) --> GND_SIG
MCP1703 VOUT (pin 1) --> +5V_ANALOG

Decouplage:
+12V_PRE --> C_IN (10uF) --> GND_SIG
+5V_ANALOG --> C_OUT (10uF tantalum + 100nF) --> GND_SIG
```

**Note :** R_DROP (47Ω) dissipe (22-12)×0.05A = 0.5W (limite acceptable 1W)

---

## C1-E - AMPLIFICATEUR MA12070 [MODIFIE V1.5]

### Identification

**Composant :** Infineon MA12070  
**Package :** QFN-48 (7x7mm)  
**Alimentation :** 4-26V (**PVDD_SAFE < 24.7V via D3**)  
**Puissance :** 2x20W @ 8 Ohms THD 1%

### Pinout simplifie MA12070

```
                    MA12070 QFN-48
              +----------------------+
      PVDD ---|1                  48|--- PVDD
      PVDD ---|2                  47|--- PGND
       ...    |        ...         |    ...
    I2C_SDA --|12                 37|--- MCLK (NC)
    I2C_SCL --|13                 36|--- LRCK (NC)
    I2C_ADDR -|14                 35|--- SCLK (NC)
       ...    |                    |    ...
      OUT_L+ -|25                 24|--- OUT_L-
      OUT_R+ -|26                 23|--- OUT_R-
       ...    |                    |
      /MUTE --|30                 19|--- /ERR
       /EN ---|31                 18|--- CLIP
      IN_L ---|32                 17|--- IN_R
              +----------------------+
```

### Cablage MA12070 V1.5

```
ALIMENTATION [MODIFIE V1.5]:
+PVDD_SAFE (depuis D3) --> C_PVDD (220uF + 100nF) --> MA12070 PVDD (pins 1,2,3,4)
GND_PWR --> MA12070 PGND (pins 47,46,45,44)
+3V3 --> MA12070 DVDD (pin 5)

Note: +PVDD_SAFE = 24.7V max (vs +22V_RAW = 25.2V max)

ENTREES AUDIO (depuis nappe J_INTER V1.5):
J_INTER pin7 (AUDIO_L) --> C_IN_L (2,2uF film) --> MA12070 IN_L (pin 32)
J_INTER pin9 (AUDIO_R) --> C_IN_R (2,2uF film) --> MA12070 IN_R (pin 17)

I2C:
J_INTER pin11 (SDA) --> MA12070 I2C_SDA (pin 12)
J_INTER pin12 (SCL) --> MA12070 I2C_SCL (pin 13)
MA12070 I2C_ADDR (pin 14) --> GND (adresse 0x20)

CONTROLE:
J_INTER pin13 (AMP_EN) --> MA12070 /EN (pin 31)
J_INTER pin14 (AMP_MUTE) --> MA12070 /MUTE (pin 30)
MA12070 /ERR (pin 19) --> J_INTER pin15 (AMP_ERR)

SORTIES HP:
MA12070 OUT_L+ (pin 25) --> L_OUT_L (10uH) --> J_SPK_L+
MA12070 OUT_L- (pin 24) --> L_OUT_L- (10uH) --> J_SPK_L-
MA12070 OUT_R+ (pin 26) --> L_OUT_R (10uH) --> J_SPK_R+
MA12070 OUT_R- (pin 23) --> L_OUT_R- (10uH) --> J_SPK_R-

FLYING CAPACITORS:
MA12070 FLY1 --> C_FLY1 (1uF) --> MA12070 FLY2
```

---

# ===========================================================================
# CARTE 2 - SIGNAL/CONTROLE (80 x 120 mm)
# ===========================================================================

---

## C2-A - MODULE BLUETOOTH BTM525

### Identification

**Module :** BTM525 (base QCC5125)  
**Codecs :** LDAC, aptX HD, aptX, AAC, SBC  
**Sortie :** I2S stereo  
**Fournisseur :** AliExpress (~20 EUR)

### Pinout BTM525 (Header 2x10)

```
       +------------------+
  VCC -|1              20|- GND
   EN -|2              19|- STATUS (LED)
 I2S_MCLK -|3          18|- UART_TX
I2S_BCLK -|4           17|- UART_RX
I2S_LRCK -|5           16|- KEY1
I2S_DATA -|6           15|- KEY2
  GND -|7              14|- KEY3
 VCC -|8               13|- AUX_DET
 I2C_SDA -|9           12|- AUX_L
 I2C_SCL -|10          11|- AUX_R
       +------------------+
```

### Cablage BTM525

```
ALIMENTATION:
+3V3 --> BTM525 VCC (pins 1, 8)
GND --> BTM525 GND (pins 7, 20)

I2S SORTIE:
BTM525 I2S_DATA (pin 6) --> PCM5102A DIN
BTM525 I2S_BCLK (pin 4) --> PCM5102A BCK
BTM525 I2S_LRCK (pin 5) --> PCM5102A LCK

STATUS:
BTM525 STATUS (pin 19) --> ESP32 GPIO4
```

---

## C2-B - DAC PCM5102A

### Identification

**Composant :** TI PCM5102A  
**Interface :** I2S  
**Resolution :** 32-bit, 384kHz  
**Package :** TSSOP-20

### Cablage PCM5102A

```
ALIMENTATION:
+3V3 --> PCM5102A VCC (pin 1)
+3V3 --> PCM5102A CPVDD (pin 3) via filtre LC
GND --> PCM5102A GND

I2S ENTREE:
BTM525 I2S_BCLK --> PCM5102A BCK (pin 12)
BTM525 I2S_LRCK --> PCM5102A LCK (pin 11)
BTM525 I2S_DATA --> PCM5102A DIN (pin 13)

CONFIGURATION:
PCM5102A FMT (pin 18) --> GND (I2S standard)
PCM5102A DEMP (pin 17) --> GND (pas de de-emphasis)
PCM5102A XSMT (pin 16) --> +3V3 (soft-mute off)

SORTIES ANALOGIQUES:
PCM5102A OUTL (pin 6) --> BT_AUDIO_L
PCM5102A OUTR (pin 8) --> BT_AUDIO_R
```

---

## C2-C - PREAMPLI PHONO RIAA (OPA2134)

### Identification

**Composant :** TI OPA2134PA  
**Package :** DIP-8  
**Gain phono :** 38dB @ 1kHz  
**RIAA :** Reseau passif RC
**Alimentation :** +5V_ANALOG (V1.5 - rail dedie)

### Cablage preampli phono

```
ALIMENTATION [MODIFIE V1.5]:
+5V_ANALOG (depuis LDO MCP1703) --> OPA2134 V+ (pin 8)
GND_SIG --> OPA2134 V- (pin 4)

Note: Alimentation sur +5V_ANALOG dedie (pas Buck 5V) pour SNR optimal

ENTREES (canal L):
J_PHONO RCA_L --> C_IN_L (0,1uF) --> R_IN_L (1k) --> OPA2134 IN+ (pin 3)
OPA2134 IN+ (pin 3) --> R_BIAS_L (100k) --> +2,5V_REF

RESEAU RIAA (canal L):
OPA2134 OUT (pin 1) --> R1 (75k) --> C1 (100nF) --> OPA2134 IN- (pin 2)
OPA2134 IN- (pin 2) --> R2 (750 Ohms) --> C2 (3,3nF) --> GND

SORTIE:
OPA2134 OUT_L (pin 1) --> PHONO_PREAMP_L

Meme schema pour canal R (pins 5,6,7)
```

---

## C2-D - SELECTEUR SOURCE CD4053

### Identification

**Composant :** CD4053BE  
**Package :** DIP-16  
**Fonction :** Triple MUX 2:1 analogique

### Cablage CD4053

```
ALIMENTATION:
+5V_ANALOG --> CD4053 VDD (pin 16)
GND --> CD4053 VSS (pin 8)
GND --> CD4053 VEE (pin 7)

ENTREES CANAL A (Bluetooth/AUX gauche):
BT_AUDIO_L --> CD4053 A0-0 (pin 1)
AUX_L --> CD4053 A0-1 (pin 2)

ENTREES CANAL B (Bluetooth/AUX droit):
BT_AUDIO_R --> CD4053 B0-0 (pin 5)
AUX_R --> CD4053 B0-1 (pin 3)

SORTIES:
CD4053 1Y-1 (pin 4) --> C_MUX_L (1uF) --> TDA7439_IN1_L
CD4053 2Y-1 (pin 14) --> C_MUX_R (1uF) --> TDA7439_IN1_R

CONTROLE:
ESP32 GPIO5 --> CD4053 A (pin 6) : Sel BT/AUX
ESP32 GPIO6 --> CD4053 B (pin 7) : Sel Phono (IN2 TDA7439)
CD4053 INH (pin 9) --> GND (toujours actif)
```

---

## C2-E - PROCESSEUR AUDIO TDA7439

### Identification

**Composant :** ST TDA7439  
**Package :** DIP-30  
**Fonction :** Volume + EQ 3 bandes + Balance  
**Interface :** I2C (adresse 0x44)  
**Alimentation :** 6-10V (typ 9V)  
**Fournisseur :** AliExpress (~3 EUR)

### Caracteristiques audio

| Parametre | Valeur |
|-----------|--------|
| THD+N | < 0,01% @ 1kHz |
| SNR | > 90dB |
| Diaphonie | < -80dB |
| Gain entree | 0 a +30dB (pas 2dB) |
| Volume | 0 a -47dB (pas 1dB) + mute |
| Bass/Mid/Treble | +/-14dB (pas 2dB) |
| Attenuation HP | 0 a -79dB (pas 1dB) |

### Pinout TDA7439 DIP-30

```
                   TDA7439 DIP-30
              +----------------------+
      BOUT_R -|1                   30|- VCC
      BIN_R --|2                   29|- MUXOUT_R
       NC ----|3                   28|- MOUT_R
    TREBLE_R -|4                   27|- MIN_R
    TREBLE_L -|5                   26|- RB (44k internal)
       NC ----|6                   25|- RM (25k internal)
      LOUT ---|7                   24|- BOUT_L
      ROUT ---|8                   23|- BIN_L
       GND ---|9                   22|- MOUT_L
     IN4_R ---|10                  21|- MIN_L
     IN4_L ---|11                  20|- MUXOUT_L
     IN3_R ---|12                  19|- CREF
     IN3_L ---|13                  18|- SDA
     IN2_R ---|14                  17|- SCL
     IN2_L ---|15                  16|- IN1_R
       GND ---|0                    0|- IN1_L
              +----------------------+
```

### Cablage TDA7439

```
ALIMENTATION:
+9V (depuis LM7809) --> TDA7439 VCC (pin 30)
GND --> TDA7439 GND (pins 9, 0)

ENTREES AUDIO (4 stereo):
IN1 - Source principale (BT/AUX via CD4053):
  MUX_OUT_L --> C_IN1L (0,47uF film) --> TDA7439 IN1_L (pin 16)
  MUX_OUT_R --> C_IN1R (0,47uF film) --> TDA7439 IN1_R (pin 15)

IN2 - Phono preamplifie:
  PHONO_PREAMP_L --> C_IN2L (0,47uF film) --> TDA7439 IN2_L (pin 15)
  PHONO_PREAMP_R --> C_IN2R (0,47uF film) --> TDA7439 IN2_R (pin 14)

IN3, IN4 - Non utilises (connecter a GND via 10k)
  TDA7439 IN3_L (pin 13), IN3_R (pin 12) --> 10k --> GND
  TDA7439 IN4_L (pin 11), IN4_R (pin 10) --> 10k --> GND

I2C:
TDA7439 SDA (pin 18) --> ESP32 GPIO1 (SDA commun)
TDA7439 SCL (pin 17) --> ESP32 GPIO2 (SCL commun)

SORTIES AUDIO:
TDA7439 LOUT (pin 7) --> C_OUTL (2,2uF film) --> Buffer OPA2134
TDA7439 ROUT (pin 8) --> C_OUTR (2,2uF film) --> Buffer OPA2134

REFERENCE:
TDA7439 CREF (pin 19) --> C_REF (2,2uF) --> GND
```

### Filtres externes TDA7439

**BASS filter (T-filter bandpass):**
```
TDA7439 BOUT_L (pin 24) --> C_BL1 (100nF) --> TDA7439 BIN_L (pin 23)
TDA7439 BOUT_L (pin 24) --> C_BL2 (100nF) --> GND
TDA7439 BIN_L (pin 23) --> C_BL3 (100nF) --> GND

TDA7439 BOUT_R (pin 1) --> C_BR1 (100nF) --> TDA7439 BIN_R (pin 2)
TDA7439 BOUT_R (pin 1) --> C_BR2 (100nF) --> GND
TDA7439 BIN_R (pin 2) --> C_BR3 (100nF) --> GND
```

**MID filter (T-filter bandpass):**
```
TDA7439 MOUT_L (pin 22) --> C_ML1 (22nF) --> TDA7439 MIN_L (pin 21)
TDA7439 MOUT_L (pin 22) --> C_ML2 (22nF) --> GND
TDA7439 MIN_L (pin 21) --> C_ML3 (22nF) --> GND

TDA7439 MOUT_R (pin 28) --> C_MR1 (22nF) --> TDA7439 MIN_R (pin 27)
TDA7439 MOUT_R (pin 28) --> C_MR2 (22nF) --> GND
TDA7439 MIN_R (pin 27) --> C_MR3 (22nF) --> GND
```

**TREBLE filter (high-pass):**
```
TDA7439 TREBLE_L (pin 5) --> C_TL (5,6nF) --> GND
TDA7439 TREBLE_R (pin 4) --> C_TR (5,6nF) --> GND
```

---

## C2-F - BUFFER SORTIE OPA2134

### Cablage buffer

```
ALIMENTATION [MODIFIE V1.5]:
+5V_ANALOG (depuis LDO MCP1703) --> OPA2134 V+ (pin 8)
GND_SIG --> OPA2134 V- (pin 4)

ENTREE (depuis TDA7439):
TDA7439 LOUT --> C_BUF_IN_L (1uF) --> OPA2134 IN+ (pin 3)
OPA2134 IN+ (pin 3) --> R_BIAS (100k) --> +2,5V_REF

FEEDBACK (gain=1):
OPA2134 OUT (pin 1) --> OPA2134 IN- (pin 2)

SORTIE:
OPA2134 OUT_L (pin 1) --> R_OUT_L (100 Ohms) --> J_INTER AUDIO_L

Meme schema pour canal R
```

---

## C2-G - MICROCONTROLEUR ESP32-S3

### Identification

**Module :** ESP32-S3-WROOM-1-N8R8  
**Flash :** 8MB  
**PSRAM :** 8MB  
**Interfaces :** I2C, SPI, ADC, GPIO

### Assignation GPIO complete

| GPIO | Fonction | Direction | Peripherique |
|------|----------|-----------|--------------|
| 1 | I2C_SDA | Bidirectionnel | MA12070, OLED, TDA7439 |
| 2 | I2C_SCL | Sortie | MA12070, OLED, TDA7439 |
| 4 | BT_STATUS | Entree | BTM525 |
| 5 | SRC_SEL0 | Sortie | CD4053 |
| 6 | SRC_SEL1 | Sortie | CD4053 |
| 7 | BT_RESET | Sortie | BTM525 |
| 15 | AMP_EN | Sortie | MA12070 /EN |
| 16 | AMP_MUTE | Sortie | MA12070 /MUTE |
| 17 | AMP_ERR | Entree | MA12070 /ERR |
| 18 | ENC_A | Entree | Encodeur |
| 19 | ENC_B | Entree | Encodeur |
| 20 | ENC_SW | Entree | Encodeur bouton |
| 21 | IR_RX | Entree | Recepteur IR |
| 38 | ADC_BATT | Entree ADC | Diviseur 22V |
| 39 | ADC_NTC | Entree ADC | Diviseur NTC |
| 40 | ADC_AUDIO_L | Entree ADC | VU-metre L |
| 41 | ADC_AUDIO_R | Entree ADC | VU-metre R |
| 42 | SAFE_EN | Sortie | PC817 LED |
| 48 | LED_STATUS | Sortie | LED facade |

### Bus I2C partage (adresses)

| Device | Adresse 7-bit | Adresse 8-bit |
|--------|---------------|---------------|
| OLED SSD1306 | 0x3C | 0x78 |
| MA12070 | 0x20 | 0x40 |
| TDA7439 | 0x44 | 0x88 |

---

## C2-H - AFFICHAGE OLED

### Cablage OLED

```
OLED VCC --> +3V3
OLED GND --> GND
OLED SDA --> ESP32 GPIO1
OLED SCL --> ESP32 GPIO2
```

---

## C2-I - ENCODEUR ROTATIF

### Cablage encodeur

```
ENC_A --> ESP32 GPIO18 (avec 10k pullup)
ENC_B --> ESP32 GPIO19 (avec 10k pullup)
ENC_SW --> ESP32 GPIO20 (avec 10k pullup)
ENC_COM --> GND
```

---

## C2-J - RECEPTEUR IR

### Cablage IR

```
+3V3 --> IR VCC
GND --> IR GND
IR OUT --> ESP32 GPIO21
```

---

# ===========================================================================
# NAPPE INTER-CARTES J_INTER (16 pins) [MODIFIE V1.5]
# ===========================================================================

### Changement V1.5 : Nappe 14 pins → 16 pins

**Justification :**
L'audit Gemini a identifie un risque de **crosstalk** entre les signaux I2C (signaux carres rapides) et les lignes Audio analogiques adjacentes dans la nappe V1.4.

**Solution V1.5 :**
- Passer de 14 pins a 16 pins
- Intercaler des lignes GND (blindage) entre Audio et I2C
- Isoler Audio_L et Audio_R par GND de chaque cote

### Connecteur et cable

**Type :** JST XH 16 pins  
**Embase PCB :** B16B-XH-A (LF)(SN) - 1 par carte = 2 total  
**Boitier cable :** XHP-16  
**Contacts :** SXH-001T-P0.6 (x16)  
**Cable :** 100mm, AWG24-26  
**Fournisseur :** TME, LCSC (~0,60 EUR/embase + 2.50 EUR cable)

**⚠️ IMPORTANT V1.5 :** Securiser mecaniquement les connecteurs avec colle chaude ou maintien mecanique pour eviter vibrations/deconnexions.

### Assignation pins V1.5 (avec blindage GND)

| Pin | Signal | Dir | Carte 1 | Carte 2 | Couleur fil |
|-----|--------|-----|---------|---------|-------------|
| 1 | 22V_SENSE | C1->C2 | Diviseur 22V | ADC ESP32 | Orange |
| 2 | +5V | C1->C2 | Buck out | Rail +5V | Rouge |
| 3 | +3V3 | C1->C2 | LDO out | Rail +3V3 | Rose |
| 4 | GND_PWR | - | GND_PWR | GND_PWR | Noir |
| 5 | GND_SIG | - | GND_SIG | GND_SIG | Noir |
| 6 | **GND_SHIELD** | - | GND | GND | **Noir** |
| 7 | **AUDIO_L** | C2->C1 | MA12070 IN | Buffer out | Blanc |
| 8 | **GND_SHIELD** | - | GND | GND | **Noir** |
| 9 | **AUDIO_R** | C2->C1 | MA12070 IN | Buffer out | Gris |
| 10 | **GND_SHIELD** | - | GND | GND | **Noir** |
| 11 | **SDA** | <-> | MA12070 | ESP32 | Bleu |
| 12 | **SCL** | C2->C1 | MA12070 | ESP32 | Jaune |
| 13 | AMP_EN | C2->C1 | MA12070 /EN | ESP32 | Vert |
| 14 | AMP_MUTE | C2->C1 | MA12070 /MUTE | ESP32 | Violet |
| 15 | AMP_ERR | C1->C2 | MA12070 /ERR | ESP32 | Marron |
| 16 | SAFE_EN | C2->C1 | PC817 LED | ESP32 | Vert/Blanc |

### Schema blindage V1.5

```
AVANT V1.4 (probleme crosstalk):
... | Pin6=AUDIO_L | Pin7=AUDIO_R | Pin8=SDA | Pin9=SCL | ...
                                    ↑
                              Signaux carres I2C
                              adjacent Audio = CROSSTALK

APRES V1.5 (blinde):
... | Pin6=GND | Pin7=AUDIO_L | Pin8=GND | Pin9=AUDIO_R | Pin10=GND | Pin11=SDA | Pin12=SCL | ...
                     ↑                          ↑              ↑
                Audio isole par GND          Separation       I2C isole
```

### Notes supplementaires nappe V1.5

- **NTC_ADC supprime de la nappe** : Mesure NTC locale sur Carte 1 (pas besoin de transmettre)
- **Pins GND multiples** : Reduisent impedance retour et crosstalk
- **AWG24 recommande** : Meilleure tenue mecanique vs AWG26

---

# ===========================================================================
# BOM COMPLETE V1.5
# ===========================================================================

## Semiconducteurs

| Ref | Composant | Valeur/Fonction | Package | Qte | Prix unit |
|-----|-----------|-----------------|---------|-----|-----------|
| U1 | MA12070 | Ampli Class-D 2x20W | QFN-48 | 1 | 8 EUR |
| U2 | OPA2134PA | Dual Op-Amp Phono RIAA | DIP-8 | 1 | 4 EUR |
| U3 | TDA7439 | Audio Processor 3-band EQ | DIP-30 | 1 | 3 EUR |
| U4 | CD4053BE | Triple MUX analogique | DIP-16 | 1 | 0,30 EUR |
| U5 | OPA2134PA | Dual Op-Amp Buffer | DIP-8 | 1 | 4 EUR |
| U6 | AMS1117-3.3 | LDO 3,3V 1A | SOT-223 | 1 | 0,30 EUR |
| **U7** | **MCP1703A-5002** | **LDO 5V audio low-noise** | **TO-92** | **1** | **0,60 EUR** |
| U8 | ESP32-S3-WROOM | MCU WiFi/BT | Module | 1 | 5 EUR |
| U9 | BTM525 | Module BT QCC5125 | Module | 1 | 20 EUR |
| U10 | PCM5102A | DAC I2S 32-bit | TSSOP-20 | 1 | 3 EUR |
| U11 | PC817 | Opto-coupleur | DIP-4 | 1 | 0,20 EUR |
| U12 | LM7809 | Regulateur 9V | TO-220 | 1 | 0,50 EUR |
| Q1 | Si2302 | N-MOS driver relais | SOT-23 | 1 | 0,15 EUR |
| D1 | SS54 | Schottky 40V 5A (anti-inversion) | SMA | 1 | 0,30 EUR |
| **D2** | **SMBJ24CA** | **TVS 24V 600W** | **SMB** | **1** | **0,50 EUR** |
| **D3** | **SS54** | **Schottky 40V 5A (PVDD)** | **SMA** | **1** | **0,30 EUR** |
| K1 | HF46F-G/12 | Relais 12V 10A | TH | 1 | 2 EUR |

## Passifs - Resistances

| Ref | Valeur | Tolerance | Puissance | Qte | Usage |
|-----|--------|-----------|-----------|-----|-------|
| R diverses | 1k Ohms | 5% | 0,25W | 5 | Pull-up, LED |
| R diverses | 4,7k Ohms | 5% | 0,25W | 2 | I2C pull-up |
| R diverses | 10k Ohms | 5% | 0,25W | 15 | Pull-up/down, buffer |
| R diverses | 100k Ohms | 5% | 0,25W | 3 | Diviseur, bias |
| **R_DROP** | **47 Ohms** | **5%** | **1W** | **1** | **Pre-LDO audio** |
| R_K1 | 100 Ohms | 5% | 1W | 1 | Limite bobine |
| R_RIAA | 75k Ohms | 1% | 0,25W | 2 | Preampli RIAA |
| R_RIAA | 750 Ohms | 1% | 0,25W | 2 | Preampli RIAA |
| R_RIAA | 1k Ohms | 1% | 0,25W | 2 | Entree phono |
| R_DIV | 100k Ohms | 1% | 0,25W | 1 | Diviseur 22V |
| R_DIV | 10k Ohms | 1% | 0,25W | 1 | Diviseur 22V |

## Passifs - Condensateurs

| Ref | Valeur | Type | Tension | Qte | Usage |
|-----|--------|------|---------|-----|-------|
| C diverses | 100nF | Ceramique X7R | 50V | 25 | Decouplage, Bass filter |
| C diverses | 10uF | Ceramique X5R | 16V | 12 | Decouplage |
| **C_PVDD** | **220uF** | **Electro low-ESR** | **35V** | **1** | **Filtrage PVDD_SAFE** |
| C diverses | 100uF | Electro low-ESR | 35V | 3 | Filtrage |
| C diverses | 1uF | Film polypropylene | 50V | 10 | Couplage audio |
| C diverses | 2,2uF | Film | 50V | 10 | Couplage ampli, CREF |
| C diverses | 0,47uF | Film | 50V | 8 | Entrees TDA7439 |
| C_RIAA | 100nF | Film 5% | 50V | 2 | Reseau RIAA |
| C_RIAA | 3,3nF | Film 5% | 50V | 2 | Reseau RIAA |
| C_TDA_B | 100nF | Film | 50V | 12 | Bass filter TDA7439 |
| C_TDA_M | 22nF | Film | 50V | 12 | Mid filter TDA7439 |
| C_TDA_T | 5,6nF | Film | 50V | 2 | Treble filter TDA7439 |
| C_FLY | 1uF | X7R | 50V | 2 | Flying cap MA12070 |
| C_CP | 2,2uF | Ceramique | 16V | 3 | Charge pump DAC |
| **C_AUDIO_LDO** | **10uF** | **Tantalum** | **16V** | **1** | **Decouplage LDO audio** |

## Passifs - Inductances

| Ref | Valeur | Courant | Qte | Usage |
|-----|--------|---------|-----|-------|
| L1-L4 | 10uH | 3A | 4 | Filtre LC sortie ampli |

## Connecteurs

| Ref | Type | Qte | Usage |
|-----|------|-----|-------|
| **J_INTER** | **JST XH 16P** | **2** | **Nappe inter-cartes V1.5** |
| J_TEST_C1 | Header 2x8 shrouded | 1 | Test Carte 1 |
| J_TEST_C2 | Header 2x10 shrouded | 1 | Test Carte 2 |
| J_SPK | Bornier 2P pas 5,08mm | 2 | Sorties HP |
| J_AUX | Jack 3.5mm stereo | 1 | Entree AUX |
| J_PHONO | Embase RCA double | 1 | Entree Phono |
| J_BAL | JST XH 7P | 1 | Balance BMS |
| J_NTC | JST PH 2P | 1 | Sonde temperature |

## Modules

| Ref | Description | Qte | Prix |
|-----|-------------|-----|------|
| BMS | JBD SP22S003B 6S 20A | 1 | 12 EUR |
| Buck | MP1584EN module | 1 | 2 EUR |
| OLED | SSD1306 128x64 I2C | 1 | 3 EUR |

## Divers

| Ref | Description | Qte | Prix |
|-----|-------------|-----|------|
| TCO | Aupo A4-1A-F 72C | 1 | 1 EUR |
| F1 | Fusible 5A ATO + support | 1 | 1,50 EUR |
| Encodeur | EC11 24 imp/tour | 1 | 1 EUR |
| LED | 3mm bleue + rouge + verte | 3 | 0,30 EUR |
| Nappe | IDC 16 fils 100mm | 1 | 2,50 EUR |

## TOTAL ESTIME V1.5

| Categorie | Sous-total |
|-----------|------------|
| Semiconducteurs | ~53 EUR |
| Passifs | ~16 EUR |
| Connecteurs | ~9 EUR |
| Modules | ~17 EUR |
| Divers | ~6,50 EUR |
| **TOTAL** | **~101,50 EUR** |

(hors PCB, boitier, batterie, HP)

**Note V1.5:** +3.50 EUR vs V1.4 (D3, MCP1703, nappe 16P, C220uF)

---

# ===========================================================================
# NOTES DE CONCEPTION V1.5
# ===========================================================================

## Justification corrections Gemini

### 1. Protection PVDD MA12070

**Probleme :**
MA12070 Abs Max PVDD = 26.0V  
Batterie 6S = 25.2V → marge 0.8V seulement  
Back EMF amplificateur Class-D = +0.5V a +1V transitoires  
→ Risque destruction ampli

**Solution :**
D3 (SS54 Schottky) en serie → perte 0.5V → PVDD_SAFE = 24.7V max  
Nouvelle marge = 1.3V (suffisant pour Back EMF)

### 2. TVS adaptee

**Probleme :**
SMBJ26CA : Vbr = 28.9V min → clamp APRES destruction MA12070

**Solution :**
SMBJ24CA : Vbr = 26.7V min → clamp AVANT 26V  
Stand-off 24V > PVDD_SAFE 24.7V max → OK, pas de conduction normale

### 3. Nappe blindee

**Probleme :**
I2C (signaux carres 400kHz) adjacent Audio analogique → crosstalk audible ("Bzzt-Bzzt")

**Solution :**
GND entre chaque groupe de signaux :
- GND | AUDIO_L | GND | AUDIO_R | GND | SDA | SCL
- Blindage capacitif par plans de masse

### 4. Alimentation audio dediee

**Probleme :**
Buck MP1584EN ripple HF (~1MHz) peut polluer OPA2134 malgre PSRR

**Solution :**
LDO MCP1703A ultra-low-noise dedie pour section audio analogique :
- Preampli phono OPA2134
- Buffer sortie OPA2134  
- CD4053 MUX

## Ordre de mise sous tension V1.5

1. Batterie connectee --> BMS actif
2. ESP32 demarre --> SAFE_EN = LOW --> Relais ferme
3. +22V disponible --> D3 --> +PVDD_SAFE (24.7V)
4. +22V --> Buck --> +5V --> LDO --> +3V3
5. +22V --> LM7809 --> +9V_TDA (TDA7439 alimente)
6. +22V --> R_DROP --> MCP1703 --> +5V_ANALOG (section audio)
7. ESP32 initialise I2C (avec timeout 10ms) --> Configure TDA7439 (volume mute)
8. ESP32 configure MA12070
9. AMP_EN = LOW --> Ampli actif
10. TDA7439 unmute --> AMP_MUTE = HIGH (anti-pop sequentiel)

## Notes firmware V1.5

**Changements requis :**
```cpp
// Dans setup():
Wire.begin(PIN_SDA, PIN_SCL);
Wire.setClock(400000);
Wire.setTimeOut(10);  // [V1.5] Timeout 10ms anti-blocage I2C
```

**Autres ameliorations V1.4 deja presentes :**
- Filtre median ADC (anti-spike)
- Section critique encodeur (atomicite)
- Verification retour I2C avec retry
- Watchdog 5s

---

# ===========================================================================
# SCHEMA BLOC COMPLET V1.5
# ===========================================================================

```
                                    CARTE 2 - SIGNAL
+--------+    +--------+    +-------+    +---------+    +--------+
|        |    |        |    |       |    |         |    |        |
| BTM525 |--->| PCM5102|--+ | CD4053|--->| TDA7439 |--->| Buffer |--+
|  BT    |    |  DAC   |  | | MUX   |    | EQ 3-bd |    | OPA2134|  |
+--------+    +--------+  | +-------+    +---------+    +--------+  |
                          |     ^            |                     |
+--------+                |     |            |                     |
| AUX    |----------------+-----+            |                     |
| Jack   |                |         +5V_ANALOG (MCP1703)           |
+--------+                |                                        |
                          |                                        |
+--------+    +--------+  |                                        |
| PHONO  |--->| OPA2134|--+                                        |
| RCA    |    |  RIAA  |                                           |
+--------+    +--------+                                           |
                                                                   |
+-----------+    +------+                                          |
| ESP32-S3  |<-->| OLED |                                          |
|           |    +------+                                          |
|   I2C --->|--------------------------------------------> TDA7439 |
|           |--------------------------------------------> MA12070 |
|           |<-- Encodeur                                          |
|           |<-- IR                                                |
|           |--> GPIO42 --> PC817 --> Relais K1                    |
+-----------+                                                      |
      |                                                            |
      |            NAPPE J_INTER 16 pins (V1.5 blinde)            |
      +--------------------+-----------------------------------+---+
                           |                                   |
                           v                                   v
+-----------------------------------------------------------------------------+
|                           CARTE 1 - PUISSANCE                               |
|                                                                             |
|  +------+    +-----+    +-------+    +------+    +-------+    +---------+  |
|  | BMS  |--->| TCO |--->| Relais|--->| Fuse |--->| D1+D2 |--->| +22V_RAW|  |
|  | 6S   |    | 72C |    | K1    |    | 5A   |    |TVS 24V|    |         |  |
|  +------+    +-----+    +-------+    +------+    +-------+    +---------+  |
|                                                                    |       |
|                                                    +-------+       |       |
|                                                    |  D3   |<------+       |
|                                                    | SS54  |               |
|                                                    +---+---+               |
|                                                        | +PVDD_SAFE       |
|                         +--------+    +--------+       v                   |
|                         | MP1584 |--->| AMS1117|---> +3V3                  |
|                         | Buck   |    | LDO    |                           |
|                         | +5V    |    +--------+                           |
|                         +--------+                                         |
|                                                                            |
|  AUDIO_L/R (depuis nappe) --> +--------+                                   |
|                               | MA12070|---> HP_L+/-, HP_R+/-             |
|                               | Ampli  |<--- +PVDD_SAFE (24.7V)           |
|                               +--------+                                   |
+-----------------------------------------------------------------------------+
```

---

# ===========================================================================
# FIN DOCUMENT V1.5
# ===========================================================================
