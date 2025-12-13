# AMPLIFICATEUR AUDIOPHILE PORTABLE V1.4

## DOCUMENT TECHNIQUE COMPLET — PINOUTS EXPLICITES

**Version :** 1.4  
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
| **V1.4** | **13/12/2025** | **TDA7439 3-band EQ, Loudness, Spatial, architecture audio revisee** |

---

## NOUVEAUTES V1.4

- **TDA7439 DIP-30** : Processeur audio 3 bandes (Bass/Mid/Treble)
- **Egaliseur** : +/-14dB par bande, pas de 2dB
- **Loudness** : Boost bass automatique a faible volume
- **Spatial** : Effet stereo elargi via attenuation differentielle
- **8 presets** : Flat, Bass+, Vocal, Rock, Jazz, Cinema, Live, Custom
- **Volume integre** : TDA7439 remplace fonction volume MCP4261
- **Gain entree** : Ajustable 0-30dB par logiciel

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
| Autonomie | 4-6h @ volume moyen |
| Dimensions | Carte 1: 80x100mm, Carte 2: 80x120mm |

---

## ARCHITECTURE BI-CARTE V1.4

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
                             | Nappe J_INTER (14 pins)
+----------------------------+----------------------------------+
|                 CARTE 1 - PUISSANCE                            |
|  BMS | Securite 5 niv | Buck | LDO | MA12070 | Connecteurs HP |
|                         80 x 100 mm                            |
+---------------------------------------------------------------+
         |                                              |
    [Batterie 6S]                              [HP Gauche] [HP Droit]
```

### Chaine audio V1.4

```
SOURCES:
  BT (BTM525 I2S) --> PCM5102A DAC --> +
  AUX (Jack 3.5mm) --------------------> CD4053 MUX --> TDA7439 --> Buffer --> NAPPE --> MA12070
  Phono (OPA2134 RIAA) ---------------> +              (EQ 3 bandes)  OPA2134

Signal flow TDA7439:
  IN --> Gain 0-30dB --> Volume -47dB --> Bass --> Mid --> Treble --> Speaker Att --> OUT
```

---

# ===========================================================================
# CARTE 1 - PUISSANCE (80 x 100 mm) - INCHANGEE V1.3
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

### NIVEAU 5 - Protection inversion + TVS

**Composants :**
- D1 : SS54 (Schottky 40V 5A, SMA) - anti-inversion
- D2 : SMBJ26CA (TVS 26V 600W bidirectionnel, SMB) - surtensions

**Cablage :**
```
+BATT_FUSE --> D1 anode
D1 cathode (SS54) --> +22V_RAW

+22V_RAW --> D2 pin1 (SMBJ26CA)
D2 pin2 --> GND_PWR
```

---

## C1-C - ALIMENTATION

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

---

## C1-D - AMPLIFICATEUR MA12070

### Identification

**Composant :** Infineon MA12070  
**Package :** QFN-48 (7x7mm)  
**Alimentation :** 4-26V  
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

### Cablage MA12070

```
ALIMENTATION:
+22V_RAW --> C_PVDD (100uF + 100nF) --> MA12070 PVDD (pins 1,2,3,4)
GND_PWR --> MA12070 PGND (pins 47,46,45,44)
+3V3 --> MA12070 DVDD (pin 5)

ENTREES AUDIO (depuis nappe J_INTER):
J_INTER pin6 (AUDIO_L) --> C_IN_L (2,2uF film) --> MA12070 IN_L (pin 32)
J_INTER pin7 (AUDIO_R) --> C_IN_R (2,2uF film) --> MA12070 IN_R (pin 17)

I2C:
J_INTER pin8 (SDA) --> MA12070 I2C_SDA (pin 12)
J_INTER pin9 (SCL) --> MA12070 I2C_SCL (pin 13)
MA12070 I2C_ADDR (pin 14) --> GND (adresse 0x20)

CONTROLE:
J_INTER pin10 (AMP_EN) --> MA12070 /EN (pin 31)
J_INTER pin11 (AMP_MUTE) --> MA12070 /MUTE (pin 30)
MA12070 /ERR (pin 19) --> J_INTER pin12 (AMP_ERR)

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
# CARTE 2 - SIGNAL/CONTROLE (80 x 120 mm) - MISE A JOUR V1.4
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

I2S vers PCM5102A:
BTM525 I2S_MCLK (pin 3) --> PCM5102A BCK
BTM525 I2S_BCLK (pin 4) --> PCM5102A BCK
BTM525 I2S_LRCK (pin 5) --> PCM5102A LRCK
BTM525 I2S_DATA (pin 6) --> PCM5102A DIN

CONTROLE:
BTM525 EN (pin 2) --> +3V3 via 10k (toujours actif)
BTM525 STATUS (pin 19) --> ESP32 GPIO4 (BT_STATUS)
```

---

## C2-B - DAC PCM5102A

### Identification

**Composant :** TI PCM5102A  
**Resolution :** 32 bits, 384kHz  
**THD+N :** -93dB typ  
**Package :** TSSOP-20  

### Pinout PCM5102A

```
            PCM5102A TSSOP-20
        +--------------------+
  CPGND |1                 20| VCC3V3
    CPL |2                 19| VCCR
    CPR |3                 18| VNEG
  CPVSS |4                 17| XSMT (mute)
   AGND |5                 16| LRCK
  OUTL |6                  15| DIN
  OUTR |7                  14| BCK
   AGND |8                 13| SCK (system clock)
  DEMP |9                  12| FMT (format)
  DGND |10                 11| FLT (filter)
        +--------------------+
```

### Cablage PCM5102A

```
ALIMENTATION:
+3V3 --> VCC3V3 (pin 20), VCCR (pin 18)
+3V3 --> CPL (pin 2) via 10nF
+3V3 --> CPR (pin 3) via 10nF
GND --> CPGND (pin 1), CPVSS (pin 4), AGND (pins 5,8), DGND (pin 10)

I2S depuis BTM525:
BTM525 I2S_BCK --> PCM5102A BCK (pin 14)
BTM525 I2S_LRCK --> PCM5102A LRCK (pin 15)
BTM525 I2S_DATA --> PCM5102A DIN (pin 16)

CONFIGURATION:
PCM5102A FMT (pin 12) --> GND (I2S standard)
PCM5102A FLT (pin 11) --> GND (normal latency)
PCM5102A DEMP (pin 9) --> GND (no de-emphasis)
PCM5102A XSMT (pin 17) --> +3V3 (unmute)
PCM5102A SCK (pin 13) --> GND (internal PLL)

SORTIES AUDIO:
PCM5102A OUTL (pin 6) --> C_DAC_L (1uF film) --> BT_AUDIO_L
PCM5102A OUTR (pin 7) --> C_DAC_R (1uF film) --> BT_AUDIO_R
```

---

## C2-C - PREAMPLI PHONO RIAA (OPA2134)

### Schema preampli (inchange V1.3)

```
                       C_RIAA1 100nF
                    +------||------+
                    |              |
PHONO_L --+-- R_IN1 --+-- R_RIAA1 --+--+
          |  1k       |   75k        |
          |           +-- C_RIAA2 ---+ 
          |               3.3nF      |
          |                          |
        GND            +-------------+
                       |
                    +--+--+
                    |  -  |
         R_RIAA2 ---+ OPA |---- PHONO_PREAMP_L
            750     |  +  |
                    +--+--+
                       |
                     VREF (2.5V)
```

### Composants RIAA

| Ref | Valeur | Tolerance | Role |
|-----|--------|-----------|------|
| R_IN | 1k Ohms | 1% | Impedance entree (47k total avec RCA) |
| R_RIAA1 | 75k Ohms | 1% | Pole 75us |
| R_RIAA2 | 750 Ohms | 1% | Gain DC |
| C_RIAA1 | 100nF | 5% film | Zero 318us |
| C_RIAA2 | 3,3nF | 5% film | Pole 3180us |

### Calculs RIAA

- Gain @ 1kHz : ~38dB
- f1 = 50Hz (pole)
- f2 = 500Hz (zero)  
- f3 = 2122Hz (pole)

---

## C2-D - SELECTEUR SOURCE CD4053

### Identification

**Composant :** CD4053BE  
**Package :** DIP-16  
**Nombre de canaux :** 3 x SPDT

### Pinout CD4053

```
         CD4053BE DIP-16
        +------U------+
   1Y --|1          16|-- VDD
   1Z --|2          15|-- 2Z
   2Y --|3          14|-- 2Y-1
   2Y-1-|4          13|-- 2Z-1
   2Z-1-|5          12|-- 3Y
    A --|6          11|-- 3Z
    B --|7          10|-- 3Y-1
    C --|8           9|-- 3Z-1
  VSS --|0           0|-- VEE (=VSS pour unipolar)
        +-------------+
```

### Assignation canaux

| Canal | Entree A (SEL=0) | Entree B (SEL=1) | Sortie |
|-------|------------------|------------------|--------|
| X (1) | BT_AUDIO_L | AUX_L | MUX_OUT_L |
| Y (2) | BT_AUDIO_R | AUX_R | MUX_OUT_R |
| Z (3) | Inutilise (Phono) | - | - |

Note: Phono va directement au TDA7439 IN2 (pas via mux)

### Cablage CD4053

```
ALIMENTATION:
+5V --> CD4053 VDD (pin 16)
GND --> CD4053 VSS (pin 8), VEE (pin 7)

ENTREES:
BT_AUDIO_L --> CD4053 1Y (pin 1)
AUX_L --> CD4053 1Z (pin 2)
BT_AUDIO_R --> CD4053 2Y (pin 3)
AUX_R --> CD4053 2Z (pin 5)

SORTIES:
CD4053 1Y-1 (pin 4) --> C_MUX_L (1uF) --> TDA7439_IN1_L
CD4053 2Y-1 (pin 14) --> C_MUX_R (1uF) --> TDA7439_IN1_R

CONTROLE:
ESP32 GPIO5 --> CD4053 A (pin 6) : Sel BT/AUX
ESP32 GPIO6 --> CD4053 B (pin 7) : Sel Phono (IN2 TDA7439)
CD4053 INH (pin 9) --> GND (toujours actif)
```

---

## C2-E - PROCESSEUR AUDIO TDA7439 [NOUVEAU V1.4]

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

### Schema bloc interne TDA7439

```
IN1/IN2/IN3/IN4 --> MUX --> Input Gain --> Volume --> Bass --> Mid --> Treble --> Spkr Att --> OUT
                           (0-30dB)      (-47dB)    (+/-14dB chaque)           (L/R -79dB)
```

### Cablage TDA7439

```
ALIMENTATION:
+9V (depuis buck) --> TDA7439 VCC (pin 30)
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
TDA7439 LOUT (pin 7) --> C_OUTL (2,2uF film) --> AUDIO_TO_MA12070_L
TDA7439 ROUT (pin 8) --> C_OUTR (2,2uF film) --> AUDIO_TO_MA12070_R

REFERENCE:
TDA7439 CREF (pin 19) --> C_REF (2,2uF) --> GND
```

### Filtres externes TDA7439

Les frequences de coupure Bass/Mid/Treble sont definies par condensateurs externes.

**BASS filter (T-filter bandpass):**
```
                    C_B1
      BOUT ----+----||----+---- BIN
               |          |
             R_B1       R_B2
               |          |
              GND        GND

Composants recommandes:
C_B1, C_B2 : 100nF film (fc ~ 100Hz)
R_B (interne) : 44k Ohms
```

**Cablage Bass:**
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
Meme structure que Bass.

Composants recommandes:
C_M1, C_M2 : 22nF film (fc ~ 1kHz)
R_M (interne) : 25k Ohms
```

**Cablage Mid:**
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
          C_T
TREBLE ---||--- GND

Composant:
C_T : 5,6nF film (fc ~ 10kHz)
R_T (interne) : 25k Ohms
```

**Cablage Treble:**
```
TDA7439 TREBLE_L (pin 5) --> C_TL (5,6nF) --> GND
TDA7439 TREBLE_R (pin 4) --> C_TR (5,6nF) --> GND
```

### Registres I2C TDA7439

| Sub-Addr | Fonction | Plage | Notes |
|----------|----------|-------|-------|
| 0x00 | Input selector | 0-3 | 0=IN4, 1=IN3, 2=IN2, 3=IN1 |
| 0x01 | Input gain | 0-15 | 0=0dB, 15=+30dB (pas 2dB) |
| 0x02 | Volume | 0-48 | 0=0dB, 47=-47dB, 48=mute |
| 0x03 | Bass | 0-14 | 0=+14dB, 7=0dB, 14=-14dB |
| 0x04 | Mid-range | 0-14 | Idem bass |
| 0x05 | Treble | 0-14 | Idem bass |
| 0x06 | Speaker att R | 0-79 | 0=0dB, 79=-79dB |
| 0x07 | Speaker att L | 0-79 | Idem R |

Note: Adresse I2C = 0x44 (7-bit) ou 0x88 (8-bit avec R/W)

---

## C2-F - BUFFER SORTIE (OPA2134)

### Schema buffer (simplifie sans MCP4261)

Le TDA7439 gere le volume, le buffer OPA2134 fournit juste le gain unite vers la nappe.

```
                    R_FB 10k
                 +----/\/\/----+
                 |             |
AUDIO_L_FROM_TDA -+-- R_IN ----+--+
                    10k        |
                            +--+--+
                            |  -  |
                            | OPA |---- AUDIO_L_TO_NAPPE
                            |  +  |
                            +--+--+
                               |
                              GND
```

### Cablage buffer

```
ALIMENTATION:
+9V --> OPA2134 V+ (pin 8) via C_VPLUS (100nF)
GND --> OPA2134 V- (pin 4)

ENTREES (depuis TDA7439):
TDA7439 LOUT --> C_BUF_INL (1uF) --> OPA2134 IN- A (pin 2) + R_IN_L (10k)
TDA7439 ROUT --> C_BUF_INR (1uF) --> OPA2134 IN- B (pin 6) + R_IN_R (10k)

FEEDBACK (gain = 1):
OPA2134 OUT A (pin 1) --> R_FB_L (10k) --> OPA2134 IN- A (pin 2)
OPA2134 OUT B (pin 7) --> R_FB_R (10k) --> OPA2134 IN- B (pin 6)

REFERENCE:
OPA2134 IN+ A (pin 3) --> GND
OPA2134 IN+ B (pin 5) --> GND

SORTIES (vers nappe):
OPA2134 OUT A (pin 1) --> C_OUT_L (2,2uF) --> J_INTER AUDIO_L (pin 6)
OPA2134 OUT B (pin 7) --> C_OUT_R (2,2uF) --> J_INTER AUDIO_R (pin 7)
```

---

## C2-G - MICROCONTROLEUR ESP32-S3

### Identification

**Module :** ESP32-S3-WROOM-1-N8R8  
**Flash :** 8MB  
**PSRAM :** 8MB  
**Fournisseur :** AliExpress, Mouser (~5 EUR)

### Assignation GPIO complete V1.4

| GPIO | Fonction | Direction | Notes |
|------|----------|-----------|-------|
| 1 | I2C SDA | Bidirectionnel | MA12070, OLED, TDA7439 |
| 2 | I2C SCL | Output | MA12070, OLED, TDA7439 |
| 4 | BT_STATUS | Input | BTM525 status LED |
| 5 | SRC_SEL0 | Output | CD4053 A (BT/AUX) |
| 6 | SRC_SEL1 | Output | CD4053 B (Phono select) |
| 7 | BT_RESET | Output | Reset BTM525 (optionnel) |
| 15 | AMP_EN | Output | MA12070 /EN |
| 16 | AMP_MUTE | Output | MA12070 /MUTE |
| 17 | AMP_ERR | Input | MA12070 /ERR |
| 18 | ENC_A | Input pullup | Encodeur phase A |
| 19 | ENC_B | Input pullup | Encodeur phase B |
| 20 | ENC_SW | Input pullup | Encodeur bouton |
| 21 | IR_RX | Input | Recepteur IR |
| 38 | ADC_BATT | ADC | Tension batterie (diviseur) |
| 39 | ADC_NTC | ADC | Temperature NTC |
| 40 | ADC_AUDIO_L | ADC | VU-metre gauche |
| 41 | ADC_AUDIO_R | ADC | VU-metre droite |
| 42 | SAFE_EN | Output | PC817 -> Relais K1 |
| 48 | LED_STATUS | Output | LED status generale |

### Bus I2C partage (adresses)

| Device | Adresse 7-bit | Adresse 8-bit |
|--------|---------------|---------------|
| OLED SSD1306 | 0x3C | 0x78 |
| MA12070 | 0x20 | 0x40 |
| TDA7439 | 0x44 | 0x88 |

---

## C2-H - AFFICHAGE OLED

### Identification

**Module :** SSD1306 128x64  
**Interface :** I2C  
**Tension :** 3,3V  
**Adresse :** 0x3C

### Cablage OLED

```
OLED VCC --> +3V3
OLED GND --> GND
OLED SDA --> ESP32 GPIO1
OLED SCL --> ESP32 GPIO2
```

---

## C2-I - ENCODEUR ROTATIF

### Identification

**Composant :** EC11 (24 impulsions/tour)

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
# NAPPE INTER-CARTES J_INTER (14 pins)
# ===========================================================================

### Connecteur et cable

**Type :** JST XH 14 pins  
**Embase PCB :** B14B-XH-A (LF)(SN) - 1 par carte = 2 total  
**Boitier cable :** XHP-14  
**Contacts :** SXH-001T-P0.6 (x14)  
**Cable :** 100mm, AWG24-26  
**Fournisseur :** TME, LCSC (~0,50 EUR/embase + 2 EUR cable)

### Assignation pins complete

| Pin | Signal | Dir | Carte 1 | Carte 2 | Couleur fil |
|-----|--------|-----|---------|---------|-------------|
| 1 | 22V_SENSE | C1->C2 | Diviseur 22V | ADC ESP32 | Orange |
| 2 | +5V | C1->C2 | Buck out | Rail +5V | Rouge |
| 3 | +3V3 | C1->C2 | LDO out | Rail +3V3 | Rose |
| 4 | GND_PWR | - | GND_PWR | GND_PWR | Noir |
| 5 | GND_SIG | - | GND_SIG | GND_SIG | Noir |
| 6 | AUDIO_L | C2->C1 | MA12070 IN | Buffer out | Blanc |
| 7 | AUDIO_R | C2->C1 | MA12070 IN | Buffer out | Gris |
| 8 | SDA | <-> | MA12070 | ESP32 | Bleu |
| 9 | SCL | C2->C1 | MA12070 | ESP32 | Jaune |
| 10 | AMP_EN | C2->C1 | MA12070 /EN | ESP32 | Vert |
| 11 | AMP_MUTE | C2->C1 | MA12070 /MUTE | ESP32 | Violet |
| 12 | AMP_ERR | C1->C2 | MA12070 /ERR | ESP32 | Marron |
| 13 | NTC_ADC | C1->C2 | Diviseur NTC | ADC ESP32 | Blanc/Noir |
| 14 | SAFE_EN | C2->C1 | PC817 LED | ESP32 | Vert/Blanc |

---

# ===========================================================================
# BOM COMPLETE V1.4
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
| U7 | ESP32-S3-WROOM | MCU WiFi/BT | Module | 1 | 5 EUR |
| U8 | BTM525 | Module BT QCC5125 | Module | 1 | 20 EUR |
| U9 | PCM5102A | DAC I2S 32-bit | TSSOP-20 | 1 | 3 EUR |
| U10 | PC817 | Opto-coupleur | DIP-4 | 1 | 0,20 EUR |
| Q1 | Si2302 | N-MOS driver relais | SOT-23 | 1 | 0,15 EUR |
| D1 | SS54 | Schottky 40V 5A | SMA | 1 | 0,30 EUR |
| D2 | SMBJ26CA | TVS 26V 600W | SMB | 1 | 0,50 EUR |
| K1 | HF46F-G/12 | Relais 12V 10A | TH | 1 | 2 EUR |

## Passifs - Resistances

| Ref | Valeur | Tolerance | Puissance | Qte | Usage |
|-----|--------|-----------|-----------|-----|-------|
| R diverses | 1k Ohms | 5% | 0,25W | 5 | Pull-up, LED |
| R diverses | 4,7k Ohms | 5% | 0,25W | 2 | I2C pull-up |
| R diverses | 10k Ohms | 5% | 0,25W | 15 | Pull-up/down, buffer |
| R diverses | 100k Ohms | 5% | 0,25W | 3 | Diviseur, bias |
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
| C diverses | 10uF | Ceramique X5R | 16V | 10 | Decouplage |
| C diverses | 100uF | Electro low-ESR | 35V | 3 | Filtrage PVDD |
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

## Passifs - Inductances

| Ref | Valeur | Courant | Qte | Usage |
|-----|--------|---------|-----|-------|
| L1-L4 | 10uH | 3A | 4 | Filtre LC sortie ampli |

## Connecteurs

| Ref | Type | Qte | Usage |
|-----|------|-----|-------|
| J_INTER | JST XH 14P | 2 | Nappe inter-cartes |
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
| Nappe | IDC 14 fils 100mm | 1 | 2 EUR |

## TOTAL ESTIME V1.4

| Categorie | Sous-total |
|-----------|------------|
| Semiconducteurs | ~52 EUR |
| Passifs | ~15 EUR |
| Connecteurs | ~8 EUR |
| Modules | ~17 EUR |
| Divers | ~6 EUR |
| **TOTAL** | **~98 EUR** |

(hors PCB, boitier, batterie, HP)

Note V1.4: +3 EUR (TDA7439) -1.5 EUR (suppression MCP4261) = +1.5 EUR net

---

# ===========================================================================
# NOTES DE CONCEPTION V1.4
# ===========================================================================

## Chaine audio V1.4

```
Source BT/AUX --> CD4053 MUX --> TDA7439 IN1 --> EQ 3 bandes --> Buffer OPA2134 --> Nappe --> MA12070
Source Phono --> OPA2134 RIAA --> TDA7439 IN2 ------^
```

## Avantages architecture V1.4

1. **Volume + EQ integres** : TDA7439 gere tout, moins de composants
2. **Loudness automatique** : Compensation Fletcher-Munson en firmware
3. **Spatial** : Effet stereo sans DSP externe (via speaker att)
4. **THD inchange** : TDA7439 < 0.01% ne degrade pas la chaine
5. **4 entrees disponibles** : Evolutivite future (IN3, IN4 libres)

## Alimentation TDA7439

Le TDA7439 necessite 6-10V (typ 9V). Options:

**Option A - Buck dedie:**
```
+22V_RAW --> LM2596 (regle 9V) --> +9V_TDA --> TDA7439 VCC
```

**Option B - Rail 5V + boost (deconseille):**
Moins efficace, eviter.

**Option C - Depuis +22V via regulateur lineaire (simple):**
```
+22V_RAW --> LM7809 (TO-220) --> +9V_TDA --> TDA7439 VCC
P_diss = (22-9) x 0.05A = 0.65W (acceptable sans radiateur)
```

**Recommandation:** Option C pour simplicite (ajout LM7809)

## Ordre de mise sous tension V1.4

1. Batterie connectee --> BMS actif
2. ESP32 demarre --> SAFE_EN = LOW --> Relais ferme
3. +22V disponible --> Buck demarre --> +5V --> LDO --> +3V3
4. +22V --> LM7809 --> +9V_TDA (TDA7439 alimenté)
5. ESP32 initialise I2C --> Configure TDA7439 (volume mute)
6. ESP32 configure MA12070
7. AMP_EN = LOW --> Ampli actif
8. TDA7439 unmute --> AMP_MUTE = HIGH (anti-pop sequentiel)

## Commandes I2C TDA7439

Exemple initialisation:
```cpp
// Adresse TDA7439 = 0x44
Wire.beginTransmission(0x44);
Wire.write(0x00);  // Sub-addr: Input selector
Wire.write(0x03);  // Data: IN1 selectionne
Wire.endTransmission();

Wire.beginTransmission(0x44);
Wire.write(0x02);  // Sub-addr: Volume
Wire.write(20);    // Data: -20dB
Wire.endTransmission();

Wire.beginTransmission(0x44);
Wire.write(0x03);  // Sub-addr: Bass
Wire.write(7);     // Data: 0dB (flat)
Wire.endTransmission();
```

---

# ===========================================================================
# SCHEMA BLOC COMPLET V1.4
# ===========================================================================

```
                                    CARTE 2 - SIGNAL
+--------+    +--------+    +-------+    +---------+    +--------+
|        |    |        |    |       |    |         |    |        |
| BTM525 |--->| PCM5102|--+ | CD4053|--->| TDA7439 |--->| Buffer |--+
|  BT    |    |  DAC   |  | | MUX   |    | EQ 3-bd |    | OPA2134|  |
+--------+    +--------+  | +-------+    +---------+    +--------+  |
                          |     ^                                   |
+--------+                |     |                                   |
| AUX    |----------------+-----+                                   |
| Jack   |                |                                         |
+--------+                |                                         |
                          |                                         |
+--------+    +--------+  |                                         |
| PHONO  |--->| OPA2134|--+                                         |
| RCA    |    |  RIAA  |                                            |
+--------+    +--------+                                            |
                                                                    |
+-----------+    +------+                                           |
| ESP32-S3  |<-->| OLED |                                           |
|           |    +------+                                           |
|   I2C --->|---------------------------------------------> TDA7439 |
|           |---------------------------------------------> MA12070 |
|           |<-- Encodeur                                           |
|           |<-- IR                                                 |
|           |--> GPIO42 --> PC817 --> Relais K1                     |
+-----------+                                                       |
      |                                                             |
      |            NAPPE J_INTER 14 pins                           |
      +--------------------+------------------------------------+---+
                           |                                    |
                           v                                    v
+------------------------------------------------------------------------------+
|                           CARTE 1 - PUISSANCE                                |
|                                                                              |
|  +------+    +-----+    +-------+    +------+    +-------+    +---------+   |
|  | BMS  |--->| TCO |--->| Relais|--->| Fuse |--->| D1+D2 |--->| +22V_RAW|   |
|  | 6S   |    | 72C |    | K1    |    | 5A   |    | TVS   |    |         |   |
|  +------+    +-----+    +-------+    +------+    +-------+    +---------+   |
|                                                                    |        |
|                         +--------+    +--------+                   |        |
|                         | MP1584 |--->| AMS1117|---> +3V3          |        |
|                         | Buck   |    | LDO    |                   |        |
|                         | +5V    |    +--------+                   |        |
|                         +--------+                                 |        |
|                                                                    v        |
|  AUDIO_L/R (depuis nappe) --> +--------+                                    |
|                               | MA12070|---> HP_L+/-, HP_R+/-              |
|                               | Ampli  |                                    |
|                               +--------+                                    |
+------------------------------------------------------------------------------+
```

---

# ===========================================================================
# FIN DOCUMENT V1.4
# ===========================================================================
