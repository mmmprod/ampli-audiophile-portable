# AMPLI AUDIOPHILE PORTABLE V1.10

## INFORMATIONS DOCUMENT

**Version:** 1.10
**Date:** 14 decembre 2025
**Auteur:** Mehdi
**Status:** Corrections audit externe - Level Shifter I2C, Molex, Anti-Plop

---

## CHANGELOG V1.10

### CORRECTIONS CRITIQUES (Audit Externe #2)

| # | Bug | Gravite | Correction |
|---|-----|---------|------------|
| A1 | TDA7439 @ 5V hors specs (Vcc_min=7V) | FATAL | Alimentation 9V + Level Shifter BSS138 |
| A2 | I2C 3.3V vs TDA7439 9V (V_IH=6.3V) | LOGIQUE | Level Shifter BSS138 bidirectionnel |
| A3 | JST XH vibrations = deconnexion | MECA | Molex Micro-Fit 3.0 (verrouillage positif) |
| A4 | Plop extinction (DC dans HP) | UX | Sequence firmware MUTE -> EN -> RELAY |
| A5 | LM7812 sans dissipation | THERMIQUE | Plan cuivre 15x15mm sous tab |
| A6 | VREF RIAA bruit residuel | SIGNAL | C_REF 10uF -> 47uF |
| A7 | Oscillo BTL = court-circuit | DANGER | Isolateur USB + sticker geant |

### Rappel Corrections V1.9

| # | Bug | Correction |
|---|-----|------------|
| S1 | I2C recovery OUTPUT+HIGH | pinMode(INPUT) open-drain |
| S2 | Nappe sans protection | PTC 750mA/500mA |
| S3 | Inrush relais 500A | NTC 5ohm |
| S4 | Buffer headroom 5V | Alimentation 9V |
| S5 | VREF instable | C_REF decouplage |

---

## SPECIFICATIONS

| Parametre | Valeur |
|-----------|--------|
| Puissance | 2 x 20W RMS @ 8ohm |
| THD+N | < 0.01% @ 1W |
| SNR | > 110dB (ampli) / > 65dB (phono) |
| Bluetooth | LDAC, aptX HD, aptX, AAC, SBC |
| Entrees | Bluetooth, AUX 3.5mm, Phono MM |
| Egaliseur | 3 bandes +/-14dB (Bass/Mid/Treble) |
| Batterie | LiPo 6S 22.2V nominal (18-25.2V) |
| Autonomie | 4-6h @ volume moyen |

---

## ARCHITECTURE BI-CARTE

```
+-------------------------------------------------------------+
|                    CARTE 2 - SIGNAL                         |
|  ESP32-S3 | BTM525 BT | PCM5102A DAC | TDA7439 EQ | OPA2134 |
|  [V1.10: Level Shifter BSS138 pour I2C 9V]                  |
|                      80 x 120 mm                            |
+-------------------------+-----------------------------------+
                          | Molex Micro-Fit 3.0 16 pins
                          | [V1.10: Verrouillage positif]
+-------------------------+-----------------------------------+
|                    CARTE 1 - PUISSANCE                      |
|  BMS 6S | Securite 5 niv | MA12070 Class-D | Sorties HP     |
|  [V1.10: Plan cuivre LM7812]                                |
|                      80 x 100 mm                            |
+-------------------------------------------------------------+
```

---

# CARTE 1 - PUISSANCE (80 x 100 mm)

## C1-A : Module BMS

### Composant
JBD SP22S003B (6S 20A)

### Connexions

```
Pack 6S B- --> BMS B- (fil bleu)
Pack 6S B1-B6 --> JST XH-7P --> BMS Balance
BMS C- (noir) --> GND_STAR (C_BULK negatif)
BMS P+ (rouge) --> +BATT_PROT
NTC 10k pack --> JST PH-2P --> BMS T
```

### Protections integrees

| Protection | Seuil |
|------------|-------|
| Surcharge cellule | 4.25V +/-25mV |
| Sous-decharge | 2.8V +/-50mV |
| Surintensite | 25A |
| Court-circuit | < 100us |
| Sur-temperature | 60C |

---

## C1-B : Securite 5 Niveaux

### Architecture V1.10

```
+PACK --> BMS --> TCO 72C --> K1 --> NTC1 --> F1 --> D1+D2 --> +22V_RAW
          N1       N2         N3     N3bis    N4      N5
```

### Niveau 1 - BMS JBD

```
+BATT_PROT --> Sortie P+ BMS
```

### Niveau 2 - TCO Thermique

```
+BATT_PROT --> TCO Aupo A4-1A-F (72C, 10A) --> +BATT_TCO
```

### Niveau 3 - Relais Coupure

```
+BATT_TCO --> K1 contact NO (HF46F-G/12-HS1) --> +BATT_K1
K1 Bobine+ --> +BATT_TCO
K1 Bobine- --> Q_RELAY Drain (Si2302)
Q_RELAY Source --> GND
Q_RELAY Gate --> RELAY_CTRL
```

**Driver opto-isole :**

```
+3V3 --> R_LED (1k) --> PC817 Anode
PC817 Cathode --> GPIO42 (SAFE_EN)
PC817 Collecteur --> R_PULL (10k) --> +BATT_TCO
PC817 Emetteur --> RELAY_CTRL
RELAY_CTRL --> R_GATE (10k) --> GND
```

### Niveau 3bis - NTC Inrush Limiter [V1.9]

```
+BATT_K1 --> NTC1 (Ametherm SL10 5R005, 5ohm@25C) --> +BATT_RELAY
```

| Parametre | Valeur |
|-----------|--------|
| R @ 25C | 5 ohm |
| R @ regime | 0.1-0.5 ohm |
| I_max | 5A continu |
| I_peak limite | 25V / 5ohm = 5A |

### Niveau 4 - Fusible

```
+BATT_RELAY --> F1 (5A ATO Fast-blow) --> +BATT_FUSED
```

### Niveau 5 - Protection Surtension

```
+BATT_FUSED --> D1 anode (SS54 Schottky)
D1 cathode --> +22V_RAW
+22V_RAW --> D2 (SMBJ24CA TVS) --> GND
```

---

## C1-C : Regulateurs

### LM7812 (+12V Pre-regulateur) [V1.10: Plan cuivre]

```
+22V_RAW --> C_IN (100uF 35V) --> LM7812 IN
LM7812 GND --> GND_STAR
LM7812 OUT --> C_OUT (10uF) --> +12V_PRE
```

**[V1.10] Plan cuivre 15x15mm sous tab LM7812:**
```
Calcul thermique:
P_max = (25.2V - 12V) x 50mA = 0.66W
Rth_ja (avec cuivre) = 40C/W
Tj = 40C + 0.66W x 40C/W = 66C < 125C OK
```

### LM7809 (+9V Buffer et TDA7439) [V1.10: Alimente aussi TDA7439]

```
+12V_PRE --> LM7809 IN
LM7809 GND --> GND_STAR
LM7809 OUT --> C_OUT (10uF) --> +9V_BUFFER
```

**Charge +9V_BUFFER V1.10:**
```
- OPA2134 buffer: 5mA
- TDA7439: 35mA
- Total: 40mA
- P_diss = (12V - 9V) x 40mA = 0.12W OK
```

### MP1584 Buck (+5V Digital)

```
+22V_RAW --> MP1584 VIN
MP1584 GND --> GND_STAR
MP1584 VOUT --> +5V
```

### MCP1703A (+5V Analog)

```
+12V_PRE --> MCP1703A VIN
MCP1703A GND --> GND_STAR
MCP1703A VOUT --> +5V_ANALOG
```

### AMS1117-3.3 (+3V3)

```
+5V --> AMS1117 VIN
AMS1117 GND --> GND_STAR
AMS1117 VOUT --> +3V3
```

---

## C1-D : Protection PVDD MA12070

```
+22V_RAW --> D3 anode (1N5822 Schottky)
D3 cathode --> +PVDD_SAFE
+PVDD_SAFE --> C_PVDD (220uF 35V) --> GND
```

---

## C1-E : PTC Nappe [V1.9]

```
+5V --> PTC1 (MF-R075, 750mA) --> NAPPE_5V
+3V3 --> PTC2 (MF-R050, 500mA) --> NAPPE_3V3
```

| PTC | I_hold | I_trip | R_typ |
|-----|--------|--------|-------|
| PTC1 | 750mA | 1.5A | 0.3 ohm |
| PTC2 | 500mA | 1.0A | 0.5 ohm |

---

## C1-F : Amplificateur MA12070

```
+PVDD_SAFE --> C_PVDD (220uF 35V + 10uF ceramic) --> MA12070 PVDD
GND_STAR --> MA12070 GND
MA12070 OUT_L+ --> L_OUT_L (10uH) --> HP_L+
MA12070 OUT_L- --> L_OUT_L- (10uH) --> HP_L-
MA12070 OUT_R+ --> L_OUT_R (10uH) --> HP_R+
MA12070 OUT_R- --> L_OUT_R- (10uH) --> HP_R-
```

**ATTENTION BTL:**
```
HP_L- et HP_R- sont des SORTIES ACTIVES!
NE PAS RELIER A LA MASSE!
Mesure oscillo: SONDES DIFFERENTIELLES UNIQUEMENT!
```

**I2C Control :**

```
SDA_9V (nappe) --> MA12070 SDA
SCL_9V (nappe) --> MA12070 SCL
AMP_EN (nappe) --> MA12070 ENABLE
AMP_MUTE (nappe) --> MA12070 MUTE
MA12070 ERROR --> AMP_ERR (nappe)
Adresse I2C : 0x20
```

---

## C1-G : Star Ground

```
Tous les GND convergent vers C_BULK negatif:
- GND_BMS
- GND_REG (regulateurs)
- GND_AMP (MA12070)
- GND_NAPPE_PWR
- GND_NAPPE_SIG

JAMAIS de connexion GND ailleurs que star point!
```

---

# CARTE 2 - SIGNAL (80 x 120 mm)

## C2-A : ESP32-S3 MCU

**Module :** ESP32-S3-WROOM-1-N8R8

| GPIO | Fonction | Direction | Note |
|------|----------|-----------|------|
| 1 | SDA_3V3 | I/O | Via level shifter |
| 2 | SCL_3V3 | OUT | Via level shifter |
| 3 | I2S_BCK | OUT | |
| 4 | I2S_WS | OUT | |
| 5 | I2S_DATA | IN | |
| 18 | ENC_A | IN | |
| 19 | ENC_B | IN | |
| 20 | ENC_SW | IN | |
| 21 | IR_RX | IN | |
| 38 | AMP_EN | OUT | |
| 39 | AMP_MUTE | OUT | |
| 40 | AMP_ERR | IN | |
| 41 | MUX_S0 | OUT | |
| 42 | SAFE_EN | OUT | |
| 6 | ADC_BATT | ADC | |
| 7 | ADC_NTC | ADC | |
| 8 | POWER_FAIL | IN | [V1.10] Detection coupure |

---

## C2-B : Level Shifter I2C [NOUVEAU V1.10]

**Probleme resolu:**
```
TDA7439 Vcc = 9V --> V_IH = 0.7 x 9V = 6.3V minimum
ESP32 V_OH = 3.3V
3.3V < 6.3V --> TDA7439 ne voit pas les "1"!
```

**Solution: BSS138 bidirectionnel**

### SDA Level Shifter

```
+3V3 --> R_SDA_LV (10k) --> SDA_3V3
SDA_3V3 --> Q_SDA Source (BSS138)
Q_SDA Gate --> +3V3
Q_SDA Drain --> SDA_9V
SDA_9V --> R_SDA_HV (10k) --> +9V_BUFFER
```

### SCL Level Shifter

```
+3V3 --> R_SCL_LV (10k) --> SCL_3V3
SCL_3V3 --> Q_SCL Source (BSS138)
Q_SCL Gate --> +3V3
Q_SCL Drain --> SCL_9V
SCL_9V --> R_SCL_HV (10k) --> +9V_BUFFER
```

**Fonctionnement:**
```
ESP32 LOW (0V):
  - V_GS = 3.3V > V_th --> BSS138 ON
  - SDA_9V tire a GND via BSS138
  - TDA7439 voit LOW OK

ESP32 HIGH (3.3V) ou Input:
  - V_GS = 0V --> BSS138 OFF
  - R_SDA_HV tire SDA_9V a 9V
  - TDA7439 voit HIGH (9V > 6.3V) OK

TDA7439 tire LOW:
  - SDA_9V = 0V
  - Diode body BSS138 conduit
  - SDA_3V3 tire a ~0.7V
  - ESP32 voit LOW OK
```

**Composants:**
```
Q_SDA, Q_SCL: BSS138 (SOT-23)
R_SDA_LV, R_SCL_LV: 10k 0603
R_SDA_HV, R_SCL_HV: 10k 0603
```

---

## C2-C : Bus I2C V1.10

**Trois domaines de tension:**

```
Domaine 3V3 (ESP32, OLED):
  SDA_3V3 --> ESP32 GPIO1
  SDA_3V3 --> OLED SDA
  SCL_3V3 --> ESP32 GPIO2
  SCL_3V3 --> OLED SCL

Domaine 9V (TDA7439):
  SDA_9V --> Level Shifter --> SDA_3V3
  SCL_9V --> Level Shifter --> SCL_3V3
  SDA_9V --> TDA7439 SDA (pin 21)
  SCL_9V --> TDA7439 SCL (pin 20)

Domaine Nappe (MA12070 sur C1):
  SDA_9V --> Nappe pin 11
  SCL_9V --> Nappe pin 12
```

**Adresses I2C:**

| Device | Adresse | Domaine |
|--------|---------|---------|
| OLED SSD1306 | 0x3C | 3V3 |
| TDA7439 | 0x44 | 9V |
| MA12070 | 0x20 | 9V (via nappe) |

---

## C2-D : Bluetooth BTM525

```
+5V --> BTM525 VCC
GND --> BTM525 GND
BTM525 I2S_BCK --> GPIO3
BTM525 I2S_WS --> GPIO4
BTM525 I2S_DATA --> GPIO5
```

---

## C2-E : DAC PCM5102A

```
+3V3 --> PCM5102A VCC
GND --> PCM5102A GND
GPIO3 (I2S_BCK) --> PCM5102A BCK
GPIO4 (I2S_WS) --> PCM5102A LRCK
GPIO5 (I2S_DATA) --> PCM5102A DIN
PCM5102A OUT_L --> DAC_L
PCM5102A OUT_R --> DAC_R
PCM5102A FMT --> GND (I2S standard)
PCM5102A XSMT --> +3V3 (soft mute off)
```

---

## C2-F : Preampli Phono RIAA

```
PHONO_L --> C_IN_L (100nF film) --> OPA2134_RIAA IN+
OPA2134_RIAA IN- --> Reseau RIAA
OPA2134_RIAA OUT --> PHONO_OUT_L
```

**Reseau RIAA :**

```
R1 = 750 ohm --> C1 = 3.3nF (serie, pole 2122Hz)
R2 = 75k --> parallele avec R1+C1 (zero 50Hz)
C2 = 100pF // R2 (pole 21.2kHz)
```

**Alimentation OPA2134 RIAA:**

```
+5V_ANALOG --> OPA2134_RIAA V+ (pin 8)
GND --> OPA2134_RIAA V- (pin 4)
```

**Masse virtuelle VREF [V1.10: 47uF]:**

```
+5V_ANALOG --> R_VREF_H (10k) --> VREF
VREF --> R_VREF_L (10k) --> GND
VREF --> C_REF (47uF film) --> GND

Calcul filtre:
fc = 1 / (2 x pi x 5k x 47uF) = 0.68Hz
Attenuation @ 50Hz = -37dB
Bruit residuel < 1mVrms OK
```

---

## C2-G : Selecteur Source CD4053

```
+5V_ANALOG --> CD4053 VCC
GND --> CD4053 VEE, VSS
GPIO41 (MUX_S0) --> CD4053 A, B, C

CD4053 X0/Y0 --> DAC_L / DAC_R (Bluetooth)
CD4053 X1/Y1 --> AUX_L / AUX_R
CD4053 X2/Y2 --> PHONO_OUT_L / PHONO_OUT_R
CD4053 X/Y --> TDA7439 IN
```

---

## C2-H : Processeur Audio TDA7439 [V1.10: Alim 9V]

```
+9V_BUFFER --> TDA7439 VCC (pin 18)
GND --> TDA7439 GND (pin 15)
SDA_9V --> TDA7439 SDA (pin 21)
SCL_9V --> TDA7439 SCL (pin 20)
Adresse I2C : 0x44
```

**Datasheet TDA7439:**
```
Vcc min = 7V, Vcc typ = 9V, Vcc max = 12V
V_IH = 0.7 x Vcc = 6.3V @ 9V
V_IL = 0.3 x Vcc = 2.7V @ 9V
```

**Condensateurs Frequence Coupure :**

| Bande | Pin | Valeur | fc |
|-------|-----|--------|-----|
| Bass | 4,25 | 100nF | 159Hz |
| Mid | 5,24 | 22nF | 723Hz |
| Treble | 6,23 | 5.6nF | 2.84kHz |

---

## C2-I : Buffer Sortie OPA2134 [V1.9]

```
TDA7439 OUT_L --> R_IN_L (10k) --> OPA2134_BUF IN+
OPA2134_BUF IN- --> OPA2134_BUF OUT (suiveur)
OPA2134_BUF OUT --> C_OUT_L (2.2uF film) --> AUDIO_L (nappe)
```

**Alimentation 9V :**

```
+9V_BUFFER --> OPA2134_BUF V+ (pin 8)
GND --> OPA2134_BUF V- (pin 4)
+9V_BUFFER --> C_DEC (100nF ceramic) --> GND
```

**Calcul headroom:**
```
Signal TDA7439 = 2Vrms = 2.83V peak
V_swing OPA2134 @ 9V = 9V - 1.5V = 7.5V
Headroom = 7.5V - 2.83V = 4.67V OK
```

---

## C2-J : Interface Utilisateur

**OLED 0.96" :**

```
+3V3 --> OLED VCC
GND --> OLED GND
SDA_3V3 --> OLED SDA
SCL_3V3 --> OLED SCL
Adresse I2C : 0x3C
```

**Encodeur Rotatif :**

```
ENC_A (GPIO18) --> Encodeur A --> R (10k) --> +3V3
ENC_B (GPIO19) --> Encodeur B --> R (10k) --> +3V3
ENC_SW (GPIO20) --> Encodeur SW --> R (10k) --> +3V3
Encodeur COM --> GND
```

---

## C2-K : Monitoring

**Diviseur Batterie :**

```
+22V_SENSE (nappe) --> R_DIV_H (220k 1%) --> ADC_BATT
ADC_BATT --> R_DIV_L (33k 1%) --> GND
ADC_BATT --> C_FILT_BATT (100nF) --> GND

V_ADC = 25.2V x 33k / (220k + 33k) = 3.29V < 3.3V OK
```

**Detecteur Coupure Alimentation [NOUVEAU V1.10]:**

```
+22V_SENSE --> R_PF1 (100k) --> COMP_IN+
COMP_IN+ --> R_PF2 (33k) --> GND
VREF_2V5 (TL431) --> COMP_IN-
LM393 OUT --> GPIO8 (POWER_FAIL)

Seuil detection:
V_seuil = 2.5V x (100k + 33k) / 33k = 10V sur +22V
Marge avant effondrement rails OK
```

---

# CONNECTEUR INTER-CARTES V1.10

## Molex Micro-Fit 3.0 [NOUVEAU V1.10]

**Justification:**
```
JST XH = friction lock seulement
Vibrations (sac, voiture) --> deconnexion possible
Si GND deconnecte avant +22V --> destruction ESP32!

Molex Micro-Fit 3.0:
- Verrouillage positif (click audible)
- Rated vibrations automobile
- Deconnexion impossible sans action volontaire
```

**References:**
```
Embase PCB C1: Molex 43045-1600 (16 pins, vertical)
Embase PCB C2: Molex 43045-1600 (16 pins, vertical)
Connecteur cable: Molex 43025-1600 (2x)
Contacts: Molex 43030-0007 (16x par connecteur)
```

**Pinout:**

| Pin | Signal | Direction | Note |
|-----|--------|-----------|------|
| 1 | 22V_SENSE | C1-->C2 | Via diviseur 220k/33k |
| 2 | +5V | C1-->C2 | Via PTC1 750mA |
| 3 | +3V3 | C1-->C2 | Via PTC2 500mA |
| 4 | GND_PWR | - | Masse puissance SEULE |
| 5 | GND_SIG | - | Masse signal SEULE |
| 6 | GND_SHIELD | - | Blindage cable |
| 7 | AUDIO_L | C2-->C1 | Audio gauche |
| 8 | GND_SHIELD | - | Blindage entre canaux |
| 9 | AUDIO_R | C2-->C1 | Audio droit |
| 10 | GND_SHIELD | - | Blindage |
| 11 | SDA_9V | <--> | I2C data niveau 9V |
| 12 | SCL_9V | C2-->C1 | I2C clock niveau 9V |
| 13 | AMP_EN | C2-->C1 | Enable ampli |
| 14 | AMP_MUTE | C2-->C1 | Mute ampli |
| 15 | AMP_ERR | C1-->C2 | Erreur ampli |
| 16 | SAFE_EN | C2-->C1 | Controle relais |

**Separation masses:**
```
GND_PWR (pin 4): ESP32, BT, digital uniquement
GND_SIG (pin 5): DAC, TDA7439, OPA2134, RIAA uniquement
Jonction UNIQUE sur C1 au star point (C_BULK negatif)
```

---

# BOM COMPLETE V1.10

## Semiconducteurs

| Ref | Composant | Valeur/Type | Package | Qte |
|-----|-----------|-------------|---------|-----|
| U1 | MA12070 | Ampli Class-D | QFN-48 | 1 |
| U2 | OPA2134PA | Op-Amp audio | DIP-8 | 2 |
| U3 | TDA7439 | Processeur audio | DIP-30 | 1 |
| U4 | CD4053BE | MUX analogique | DIP-16 | 1 |
| U5 | AMS1117-3.3 | LDO 3.3V | SOT-223 | 1 |
| U6 | LM7812 | Regulateur 12V | TO-220 | 1 |
| U7 | MCP1703A-5002 | LDO 5V low-noise | TO-92 | 1 |
| U8 | LM7809 | Regulateur 9V | TO-220 | 1 |
| U9 | ESP32-S3-WROOM | MCU | Module | 1 |
| U10 | BTM525 | Bluetooth LDAC | Module | 1 |
| U11 | PCM5102A | DAC I2S | TSSOP-20 | 1 |
| U12 | LM393 | Comparateur | DIP-8 | 1 |
| U13 | TL431 | Reference 2.5V | TO-92 | 1 |
| Q_SDA | BSS138 | N-MOS Level Shift | SOT-23 | 1 |
| Q_SCL | BSS138 | N-MOS Level Shift | SOT-23 | 1 |
| Q_RELAY | Si2302 | N-MOS driver | SOT-23 | 1 |
| D1 | SS54 | Schottky 40V 5A | SMA | 1 |
| D2 | SMBJ24CA | TVS 24V 600W | SMB | 1 |
| D3 | 1N5822 | Schottky 40V 3A | DO-201 | 1 |

## Passifs Critiques

| Ref | Valeur | Tolerance | Type | Qte |
|-----|--------|-----------|------|-----|
| C_BULK | 220uF 35V | 20% | Electro low-ESR | 1 |
| C_PVDD | 220uF 35V | 20% | Electro low-ESR | 1 |
| C_REF | 47uF | 10% | Film | 1 |
| C_OUT_L/R | 2.2uF | 5% | Film | 2 |
| R_SDA_LV | 10k | 1% | 0603 | 1 |
| R_SDA_HV | 10k | 1% | 0603 | 1 |
| R_SCL_LV | 10k | 1% | 0603 | 1 |
| R_SCL_HV | 10k | 1% | 0603 | 1 |

## Protection

| Ref | Composant | Valeur | Qte |
|-----|-----------|--------|-----|
| NTC1 | Ametherm SL10 5R005 | 5ohm @ 25C | 1 |
| PTC1 | Bourns MF-R075 | 750mA | 1 |
| PTC2 | Bourns MF-R050 | 500mA | 1 |
| F1 | Fusible ATO | 5A Fast-blow | 1 |
| TCO | Aupo A4-1A-F | 72C 10A | 1 |

## Connecteurs

| Ref | Composant | Qte |
|-----|-----------|-----|
| J_INTER | Molex 43045-1600 | 2 |
| J_INTER_PLUG | Molex 43025-1600 | 2 |
| J_INTER_CONTACT | Molex 43030-0007 | 32 |
| K1 | HF46F-G/12-HS1 | 1 |

---

# NOTES CONCEPTION V1.10

## Separation Masses (Anti-Motorboating)

```
CARTE 2:
- Plan GND_PWR: ESP32, BTM525, MP1584
- Plan GND_SIG: PCM5102A, TDA7439, OPA2134, CD4053
- AUCUNE connexion entre plans sur C2!

NAPPE:
- Pin 4 (GND_PWR) separe de Pin 5 (GND_SIG)

CARTE 1:
- GND_PWR et GND_SIG arrivent separes
- Jonction UNIQUE au star point (C_BULK negatif)
```

## Layout LM7812 [V1.10]

```
Plan cuivre 15x15mm minimum sous tab TO-220
Relie au GND par vias multiples
Dissipation thermique gratuite
```

## Sequence Extinction (voir Firmware)

```
1. MUTE MA12070 (AMP_MUTE = HIGH)
2. Attendre 50ms
3. DISABLE MA12070 (AMP_EN = LOW)
4. Attendre 100ms
5. RELAY OFF (SAFE_EN = LOW)
```

---

# WARNINGS CRITIQUES

## Mesure Oscilloscope

```
!!! ATTENTION BTL !!!

HP_L- et HP_R- sont des SORTIES ACTIVES!
Elles oscillent en opposition de phase avec HP_L+ et HP_R+.

Si sonde oscillo (terre secteur) touche HP_L-:
--> Court-circuit via terre PC (USB debug)
--> Destruction MA12070 possible!

SOLUTIONS:
1. Sondes differentielles UNIQUEMENT sur HP
2. OU debrancher USB avant mesure HP
3. OU isolateur USB galvanique
```

## Vibrations Transport

```
Molex Micro-Fit 3.0 = verrouillage positif
Verifier click audible a chaque connexion
Ne JAMAIS transporter avec connecteur mal enfiche
```
