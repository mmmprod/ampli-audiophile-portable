# AMPLI AUDIOPHILE PORTABLE V1.9

## INFORMATIONS DOCUMENT

**Version:** 1.9
**Date:** 14 decembre 2025
**Auteur:** Mehdi
**Status:** Corrections critiques audit expert securite

---

## CHANGELOG V1.9

### CORRECTIONS CRITIQUES (Audit Expert Securite)

| # | Bug | Gravite | Correction |
|---|-----|---------|------------|
| S1 | I2C recovery OUTPUT+HIGH = court-circuit | DESTRUCTEUR | Firmware: pinMode(INPUT) pour relacher SDA |
| S2 | Nappe C2 sans protection | CRITIQUE | PTC 750mA sur alimentation nappe |
| S3 | Inrush relais K1 | CRITIQUE | NTC 5ohm inrush limiter |
| S4 | Buffer OPA2134 headroom insuffisant | CRITIQUE | Alimentation 9V (LM7809) |
| S5 | Masse virtuelle RIAA instable | IMPORTANT | C_REF 10uF decouplage |

### Detail Corrections V1.9

**[S1] I2C Recovery Open-Drain (FIRMWARE)**

```
AVANT V1.8 (BUG):
pinMode(SDA, OUTPUT);
digitalWrite(SDA, HIGH);  // DANGER: Force 3.3V sur bus
                          // Si esclave tire LOW = court-circuit!

APRES V1.9 (CORRIGE):
pinMode(SDA, INPUT);      // Relache ligne (haute impedance)
                          // Pull-up 4.7k tire naturellement a HIGH
                          // Conforme I2C open-drain
```

**[S2] PTC Protection Nappe Carte 2**

```
AVANT V1.8:
+5V (C1) --> Nappe pin2 --> +5V (C2)
Pas de protection court-circuit sur C2

APRES V1.9:
+5V (C1) --> PTC1 (750mA Bourns MF-R075) --> Nappe pin2 --> +5V (C2)
+3V3 (C1) --> PTC2 (500mA Bourns MF-R050) --> Nappe pin3 --> +3V3 (C2)
```

**[S3] NTC Inrush Limiter Relais K1**

```
AVANT V1.8:
K1 contact --> +BATT_RELAY (connexion directe)
Appel courant C_BULK: I_peak = V/ESR = 25V/0.05ohm = 500A instantane!

APRES V1.9:
K1 contact --> NTC1 (5ohm @ 25C, Ametherm SL10 5R005) --> +BATT_RELAY
I_peak = 25V/5ohm = 5A (acceptable)
```

**[S4] Buffer OPA2134 Alimentation 9V**

```
AVANT V1.8:
Buffer OPA2134 alimente en +5V_ANALOG
Headroom = 5V - 1.5V = 3.5V peak --> CLIPPING possible!

APRES V1.9:
Buffer OPA2134 alimente en +9V (LM7809)
Headroom = 9V - 1.5V = 7.5V peak --> Marge confortable
```

**[S5] Masse Virtuelle RIAA Decouplage**

```
APRES V1.9:
VREF --> C_REF (10uF film) --> GND
PSRR ameliore > 40dB @ 50Hz
```

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
|                      80 x 120 mm                            |
+-------------------------+-----------------------------------+
                          | Nappe 16 pins (blindee GND)
                          | [V1.9: PTC 750mA/500mA]
+-------------------------+-----------------------------------+
|                    CARTE 1 - PUISSANCE                      |
|  BMS 6S | Securite 5 niv | MA12070 Class-D | Sorties HP     |
|  Star Ground sur C_BULK | [V1.9: NTC inrush K1]             |
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
Pack 6S --> B- (bleu) --> BMS entree negative
Pack 6S --> B1-B6 (JST XH-7P) --> Balance cellules
BMS C- (noir) --> GND commun
BMS P+ (rouge) --> +BATT_PROT
NTC 10k (JST PH-2P) --> Sonde temperature pack
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

### Architecture V1.9

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
PC817 Collecteur --> +BATT_TCO via R_PULL (10k)
PC817 Emetteur --> RELAY_CTRL
RELAY_CTRL --> R_GATE (10k) --> GND (pull-down)
```

### Niveau 3bis - NTC Inrush Limiter [NOUVEAU V1.9]

```
+BATT_K1 --> NTC1 (Ametherm SL10 5R005, 5ohm@25C) --> +BATT_RELAY
```

**Specs NTC1 :**

| Parametre | Valeur |
|-----------|--------|
| R @ 25C | 5 ohm |
| R @ regime | < 0.5 ohm |
| I_max | 5A |
| Temps chauffe | ~200ms |

**Calcul inrush :**

```
Sans NTC: I_peak = 25V / 0.05ohm (ESR C_BULK) = 500A (destructeur!)
Avec NTC: I_peak = 25V / 5ohm = 5A (acceptable)
```

### Niveau 4 - Fusible

```
+BATT_RELAY --> F1 (5A Fast-blow ATO Littelfuse) --> +BATT_FUSE
```

### Niveau 5 - Protection Surtension/Inversion

```
+BATT_FUSE --> D1 anode (SS54 Schottky)
D1 cathode --> +22V_RAW
+22V_RAW --> D2 (SMBJ24CA TVS bidirectionnel) --> GND
```

---

## C1-C : Protection PVDD Ampli

**D3 = 1N5822** (Vf=0.9V)

```
+22V_RAW --> D3 anode (1N5822)
D3 cathode --> +PVDD_SAFE
```

**Calcul Marge :**

```
Batterie pleine : 25.2V
Apres D1 (0.5V) : 24.7V
Apres D3 (0.9V) : 23.8V nominal
Cas pire + back-EMF : 25.65V < 26V limite MA12070 OK
```

---

## C1-D : Alimentations

### D1 - Buck DC-DC (22V --> 5V)

**Module :** MP1584EN 3A

```
+22V_RAW --> C_IN_BUCK (100uF 35V + 10uF) --> MP1584 VIN
MP1584 VOUT --> L_FILT (10uH) --> +5V
+5V --> C_OUT_BUCK (100uF + 10uF) --> GND
```

### D2 - LDO Numerique (5V --> 3.3V)

**Composant :** AMS1117-3.3 (SOT-223)

```
+5V --> C_IN_LDO (10uF) --> AMS1117 VIN
AMS1117 VOUT --> +3V3
+3V3 --> C_OUT_LDO (22uF + 100nF) --> GND
```

### D3 - Rail Audio Analogique

```
+22V_RAW --> C_7812_IN (100nF) --> LM7812 VIN
LM7812 VOUT --> +12V_PRE
+12V_PRE --> C_7812_OUT (100nF + 10uF) --> GND

+12V_PRE --> C_1703_IN (1uF) --> MCP1703A VIN
MCP1703A VOUT --> +5V_ANALOG
+5V_ANALOG --> C_1703_OUT (1uF + 100nF) --> GND
```

### D4 - Rail Buffer 9V [NOUVEAU V1.9]

**Composant :** LM7809 (TO-220)

```
+22V_RAW --> C_7809_IN (100nF) --> LM7809 VIN
LM7809 VOUT --> +9V_BUFFER
+9V_BUFFER --> C_7809_OUT (100nF + 10uF) --> GND
```

---

## C1-E : Protection Nappe Carte 2 [NOUVEAU V1.9]

```
+5V --> PTC1 (Bourns MF-R075, 750mA) --> NAPPE_5V
+3V3 --> PTC2 (Bourns MF-R050, 500mA) --> NAPPE_3V3
```

**Specs PTC1 MF-R075 :**

| Parametre | Valeur |
|-----------|--------|
| I_hold | 750mA |
| I_trip | 1.5A |
| V_max | 30V |
| R @ 25C | 0.3 ohm |

---

## C1-F : Amplificateur MA12070

```
+PVDD_SAFE --> C_PVDD (220uF 35V + 10uF) --> MA12070 PVDD
GND --> MA12070 GND (Star Ground)
MA12070 OUT_L+ --> L_OUT_L (10uH) --> HP_L+
MA12070 OUT_L- --> L_OUT_L- (10uH) --> HP_L-
MA12070 OUT_R+ --> L_OUT_R (10uH) --> HP_R+
MA12070 OUT_R- --> L_OUT_R- (10uH) --> HP_R-
```

**I2C Control :**

```
SDA --> MA12070 SDA (via nappe)
SCL --> MA12070 SCL (via nappe)
AMP_EN --> MA12070 ENABLE
AMP_MUTE --> MA12070 MUTE
MA12070 ERROR --> AMP_ERR
Adresse I2C : 0x20
```

---

# CARTE 2 - SIGNAL (80 x 120 mm)

## C2-A : ESP32-S3 MCU

**Module :** ESP32-S3-WROOM-1-N8R8

| GPIO | Fonction | Direction |
|------|----------|-----------|
| 1 | SDA | I/O |
| 2 | SCL | OUT |
| 3 | I2S_BCK | OUT |
| 4 | I2S_WS | OUT |
| 5 | I2S_DATA | IN |
| 18 | ENC_A | IN |
| 19 | ENC_B | IN |
| 20 | ENC_SW | IN |
| 21 | IR_RX | IN |
| 38 | AMP_EN | OUT |
| 39 | AMP_MUTE | OUT |
| 40 | AMP_ERR | IN |
| 41 | MUX_S0 | OUT |
| 42 | SAFE_EN | OUT |
| 6 | ADC_BATT | ADC |
| 7 | ADC_NTC | ADC |

---

## C2-B : Bluetooth BTM525

```
+5V --> BTM525 VCC
GND --> BTM525 GND
BTM525 I2S_BCK --> GPIO3
BTM525 I2S_WS --> GPIO4
BTM525 I2S_DATA --> GPIO5
```

---

## C2-C : DAC PCM5102A

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

## C2-D : Preampli Phono RIAA

```
PHONO_L --> C_IN_L (100nF film) --> OPA2134 IN+
OPA2134 IN- --> Reseau RIAA
OPA2134 OUT --> PHONO_OUT_L
```

**Reseau RIAA :**

```
R1 = 750 ohm --> C1 = 3.3nF (en serie, pole 2122Hz)
R2 = 75k --> parallele avec R1+C1 (zero 50Hz)
C2 = 100pF // R2 (pole 21.2kHz)
```

**Alimentation avec VREF [V1.9] :**

```
+5V_ANALOG --> R_VREF_H (10k) --> VREF
VREF --> R_VREF_L (10k) --> GND
VREF --> C_REF (10uF film) --> GND  [NOUVEAU V1.9]
```

---

## C2-E : Selecteur Source CD4053

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

## C2-F : Processeur Audio TDA7439

```
+5V_ANALOG --> TDA7439 VCC (pin 18)
GND --> TDA7439 GND (pin 15)
SDA --> TDA7439 SDA (pin 21)
SCL --> TDA7439 SCL (pin 20)
Adresse I2C : 0x44
```

**Condensateurs Frequence Coupure :**

| Bande | Pin | Valeur | fc |
|-------|-----|--------|-----|
| Bass | 4,25 | 100nF | 159Hz |
| Mid | 5,24 | 22nF | 723Hz |
| Treble | 6,23 | 5.6nF | 2.84kHz |

---

## C2-G : Buffer Sortie OPA2134 [V1.9]

```
BUFFER_IN_L --> R_IN_L (10k) --> OPA2134 IN+
OPA2134 IN- --> OPA2134 OUT (suiveur)
OPA2134 OUT --> C_OUT_L (2.2uF film) --> AUDIO_L (nappe)
```

**Alimentation V1.9 (9V) :**

```
+9V_BUFFER --> OPA2134 V+ (pin 8)
GND --> OPA2134 V- (pin 4)
+9V_BUFFER --> C_DEC (100nF) --> GND
```

---

## C2-H : Interface Utilisateur

**OLED 0.96" :**

```
+3V3 --> OLED VCC
GND --> OLED GND
SDA --> OLED SDA
SCL --> OLED SCL
Adresse I2C : 0x3C
```

**Encodeur Rotatif :**

```
ENC_A (GPIO18) --> Encodeur A
ENC_B (GPIO19) --> Encodeur B
ENC_SW (GPIO20) --> Encodeur SW
GND --> Encodeur GND
```

---

## C2-I : Monitoring

**Diviseur Batterie :**

```
+22V_RAW --> R_DIV_H (220k 1%) --> ADC_BATT
ADC_BATT --> R_DIV_L (33k 1%) --> GND
ADC_BATT --> C_FILT_BATT (100nF) --> GND

V_ADC = 25.2V x 33k / (220k + 33k) = 3.29V < 3.3V OK
```

---

# NAPPE INTER-CARTES V1.9

**Connecteur :** JST XH 16 pins, cable 100mm AWG24

| Pin | Signal | Direction | Note |
|-----|--------|-----------|------|
| 1 | 22V_SENSE | C1-->C2 | Via diviseur |
| 2 | +5V | C1-->C2 | Via PTC1 750mA |
| 3 | +3V3 | C1-->C2 | Via PTC2 500mA |
| 4 | GND_PWR | - | Masse puissance |
| 5 | GND_SIG | - | Masse signal |
| 6 | GND_SHIELD | - | Blindage |
| 7 | AUDIO_L | C2-->C1 | Audio gauche |
| 8 | GND_SHIELD | - | Blindage |
| 9 | AUDIO_R | C2-->C1 | Audio droit |
| 10 | GND_SHIELD | - | Blindage |
| 11 | SDA | <--> | I2C data |
| 12 | SCL | C2-->C1 | I2C clock |
| 13 | AMP_EN | C2-->C1 | Enable ampli |
| 14 | AMP_MUTE | C2-->C1 | Mute ampli |
| 15 | AMP_ERR | C1-->C2 | Erreur ampli |
| 16 | SAFE_EN | C2-->C1 | Controle relais |

---

# BOM COMPLETE V1.9

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
| D1 | SS54 | Schottky 40V 5A | SMA | 1 |
| D2 | SMBJ24CA | TVS bidirectionnel | SMB | 1 |
| D3 | 1N5822 | Diode 40V 3A | DO-201AD | 1 |
| Q1 | Si2302 | N-MOSFET | SOT-23 | 1 |
| Q2 | PC817 | Optocoupleur | DIP-4 | 1 |

## Protections [NOUVEAU V1.9]

| Ref | Composant | Valeur | Qte | Note |
|-----|-----------|--------|-----|------|
| PTC1 | MF-R075 | 750mA | 1 | Protection +5V nappe |
| PTC2 | MF-R050 | 500mA | 1 | Protection +3V3 nappe |
| NTC1 | SL10 5R005 | 5ohm@25C | 1 | Inrush limiter K1 |

## Passifs - Resistances

| Valeur | Tolerance | Puissance | Qte | Usage |
|--------|-----------|-----------|-----|-------|
| 750 ohm | 1% | 0.25W | 2 | RIAA |
| 1k | 5% | 0.25W | 5 | LED, pull-up |
| 4.7k | 5% | 0.25W | 2 | I2C pull-up |
| 10k | 5% | 0.25W | 12 | Pull-up/down, NTC, VREF |
| 33k | 1% | 0.25W | 1 | Diviseur ADC |
| 75k | 1% | 0.25W | 2 | RIAA |
| 100k | 5% | 0.25W | 5 | Bias, diviseur |
| 220k | 1% | 0.25W | 1 | Diviseur ADC |

## Passifs - Condensateurs

| Type | Valeur | Tension | Qte | Usage |
|------|--------|---------|-----|-------|
| Ceramique | 100nF | 50V | 20 | Decouplage |
| Ceramique | 10uF | 25V | 10 | Decouplage |
| Ceramique | 1uF | 25V | 4 | LDO |
| Electrolytique | 220uF | 35V | 1 | C_BULK PVDD |
| Electrolytique | 100uF | 35V | 4 | Filtrage buck |
| Electrolytique | 22uF | 10V | 2 | LDO sortie |
| FILM | 0.1uF | 50V | 4 | Entrees RIAA |
| FILM | 1uF | 50V | 12 | Couplage TDA |
| FILM | 2.2uF | 50V | 4 | Entrees MA12070 |
| FILM | 10uF | 50V | 1 | C_REF VREF [V1.9] |

---

# WCCA - ANALYSE PIRE CAS V1.9

| Composant | Usage Normal | Pire Cas | Rating | Marge | Status |
|-----------|--------------|----------|--------|-------|--------|
| LM7812 | 0.25W | 0.32W | 1W+ | >68% | OK |
| MCP1703A | 0.14W | 0.18W | 0.5W | >64% | OK |
| LM7809 | 0.16W | 0.20W | 1W+ | >80% | OK |
| D3 1N5822 | 90mW | 1.8W | 3W | >40% | OK |
| PTC1 | 230mA | 400mA | 750mA | >47% | OK |
| NTC1 | 2A | 3A | 5A | >40% | OK |

---

# HISTORIQUE VERSIONS

| Version | Date | Modifications |
|---------|------|---------------|
| V1.9 | 14/12/2025 | Audit expert: PTC nappe, NTC inrush, Buffer 9V, C_REF, I2C open-drain |
| V1.8 | 14/12/2025 | Post-audit Breakout Box: NTC fail-safe, I2C recovery |
| V1.7 | 13/12/2025 | Audit ChatGPT: LM7812, R_DROP supprimee, D3 1N5822 |
| V1.6 | 13/12/2025 | Audit exhaustif: Star Ground, regles PCB |
| V1.5 | 13/12/2025 | Audit Gemini: D3 PVDD, TVS, nappe blindee |
| V1.4 | 13/12/2025 | TDA7439 EQ 3 bandes |
| V1.3 | 12/12/2025 | Preampli phono OPA2134 |
| V1.2 | 12/12/2025 | Pinouts explicites |
| V1.1 | 11/12/2025 | Securite 5 niveaux |
| V1.0 | 11/12/2025 | Architecture initiale |

---

# FIN DOCUMENT V1.9
