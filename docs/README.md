# HARDWARE - AMPLIFICATEUR AUDIOPHILE

## Informations

| Parametre | Valeur |
|-----------|--------|
| Version | 1.9 |
| Date | 14 decembre 2025 |
| PCB | 2 cartes (puissance + signal) |

## Specifications Electriques

| Parametre | Valeur |
|-----------|--------|
| Alimentation | LiPo 6S 18-25.2V |
| Puissance sortie | 2 x 20W RMS @ 8 ohms |
| Impedance charge | 4-8 ohms |
| Consommation repos | < 50mA |
| Consommation max | 3A |

## Architecture Dual-PCB

```
CARTE 2 - SIGNAL (80 x 120 mm)
+----------------------------------------------------------+
|                                                          |
|  [ESP32-S3]  [BTM525]  [PCM5102A]  [TDA7439]  [OPA2134]  |
|     MCU        BT        DAC         EQ       Buffer     |
|                                                          |
+------------------------------+---------------------------+
                               |
                         Nappe 16 pins
                    [PTC1 750mA] [PTC2 500mA]
                               |
+------------------------------+---------------------------+
|                                                          |
|  [BMS 6S]  [Securite]  [Alims]  [MA12070]  [Sorties HP] |
|    20A     5 niveaux   5V/3V3   Class-D     Borniers    |
|                        9V/12V                            |
+----------------------------------------------------------+
CARTE 1 - PUISSANCE (80 x 100 mm)
```

## CARTE 1 - Puissance

### C1-A : BMS 6S

**Composant:** JBD SP22S003B (6S 20A)

```
Pack B- --> BMS B- (fil bleu)
Pack B1-B6 --> BMS Balance (JST XH-7P)
BMS C- --> GND commun (fil noir)
BMS P+ --> +BATT_PROT (fil rouge)
NTC 10k --> BMS T (JST PH-2P)
```

### C1-B : Securite 5 Niveaux

```
+PACK --> [N1 BMS] --> [N2 TCO] --> [N3 K1] --> [N3bis NTC] --> [N4 F1] --> [N5 D1+D2] --> +22V_RAW
```

**Niveau 1 - BMS**
- Surcharge: 4.25V/cellule
- Sous-decharge: 2.8V/cellule
- Surintensite: 25A
- Court-circuit: <100us

**Niveau 2 - TCO Thermique**
```
+BATT_PROT --> TCO (Aupo A4-1A-F 72C 10A) --> +BATT_TCO
```

**Niveau 3 - Relais**
```
+BATT_TCO --> K1 NO (HF46F-G/12-HS1) --> +BATT_K1
K1 Bobine+ --> +BATT_TCO
K1 Bobine- --> Q1 Drain (Si2302)
Q1 Source --> GND
Q1 Gate --> RELAY_CTRL (via PC817)
```

**Niveau 3bis - NTC Inrush [V1.9]**
```
+BATT_K1 --> NTC1 (Ametherm SL10 5R005, 5ohm@25C) --> +BATT_RELAY
```
- Limite inrush de 500A a 5A
- Resistance regime permanent: <0.5 ohm

**Niveau 4 - Fusible**
```
+BATT_RELAY --> F1 (5A Fast-blow ATO) --> +BATT_FUSE
```

**Niveau 5 - Protection**
```
+BATT_FUSE --> D1 anode (SS54 Schottky)
D1 cathode --> +22V_RAW
+22V_RAW --> D2 (SMBJ24CA TVS) --> GND
```

### C1-C : Protection PVDD

```
+22V_RAW --> D3 anode (1N5822)
D3 cathode --> +PVDD_SAFE
```
- Protege MA12070 (max 26V)
- Chute 0.9V sous charge

### C1-D : Alimentations

**Buck 22V --> 5V**
```
+22V_RAW --> MP1584EN --> +5V
Decouplage: 100uF + 10uF entree/sortie
```

**LDO 5V --> 3.3V**
```
+5V --> AMS1117-3.3 --> +3V3
Decouplage: 10uF entree, 22uF + 100nF sortie
```

**Rail Audio 22V --> 12V --> 5V**
```
+22V_RAW --> LM7812 --> +12V_PRE
+12V_PRE --> MCP1703A-5002 --> +5V_ANALOG
```

**Rail Buffer 9V [V1.9]**
```
+22V_RAW --> LM7809 --> +9V_BUFFER
Pour OPA2134 buffer (headroom 7.5V)
```

### C1-E : Protection Nappe [V1.9]

```
+5V --> PTC1 (MF-R075 750mA) --> NAPPE_5V
+3V3 --> PTC2 (MF-R050 500mA) --> NAPPE_3V3
```
- Auto-rearmable apres court-circuit
- Protege Carte 1 si defaut Carte 2

### C1-F : Amplificateur MA12070

```
+PVDD_SAFE --> C_BULK (220uF 35V) --> MA12070 PVDD
GND (Star Ground sur C_BULK) --> MA12070 GND

MA12070 OUT_L+ --> L (10uH) --> HP_L+
MA12070 OUT_L- --> L (10uH) --> HP_L-
MA12070 OUT_R+ --> L (10uH) --> HP_R+
MA12070 OUT_R- --> L (10uH) --> HP_R-

I2C: SDA, SCL (via nappe)
Controle: AMP_EN, AMP_MUTE, AMP_ERR
```

## CARTE 2 - Signal

### C2-A : ESP32-S3

**Module:** ESP32-S3-WROOM-1-N8R8

Voir README_FIRMWARE.md pour pinout complet.

### C2-B : Bluetooth BTM525

```
+5V --> BTM525 VCC
GND --> BTM525 GND
I2S: BCK (GPIO3), WS (GPIO4), DATA (GPIO5)
```

### C2-C : DAC PCM5102A

```
+3V3 --> PCM5102A VCC
I2S: BCK, LRCK, DIN
FMT --> GND (I2S standard)
XSMT --> +3V3 (soft mute off)
OUT_L/R --> CD4053 entree
```

### C2-D : Preampli Phono RIAA

**OPA2134 + reseau RIAA**

```
PHONO_IN --> C (100nF film) --> OPA2134 IN+
OPA2134 configurer en ampli RIAA
Gain: ~40dB @ 1kHz
```

**Reseau RIAA:**
```
R1 = 750 ohm, C1 = 3.3nF (pole 2122Hz)
R2 = 75k (zero 50Hz)
C2 = 100pF (pole 21.2kHz)
```

**Masse virtuelle [V1.9]:**
```
+5V_ANALOG --> R (10k) --> VREF
VREF --> R (10k) --> GND
VREF --> C_REF (10uF film) --> GND
```

### C2-E : Selecteur CD4053

```
+5V_ANALOG --> CD4053 VCC
GPIO41 --> A, B, C (selection)

X0/Y0: DAC (Bluetooth)
X1/Y1: AUX
X2/Y2: PHONO
X/Y OUT --> TDA7439 IN
```

### C2-F : Processeur TDA7439

```
+5V_ANALOG --> TDA7439 VCC (pin 18)
I2C: SDA (pin 21), SCL (pin 20)
Adresse: 0x44
```

**Condensateurs EQ:**
| Bande | Pins | Valeur | fc |
|-------|------|--------|-----|
| Bass | 4,25 | 100nF | 159Hz |
| Mid | 5,24 | 22nF | 723Hz |
| Treble | 6,23 | 5.6nF | 2.8kHz |

### C2-G : Buffer OPA2134 [V1.9]

```
TDA7439 OUT --> R (10k) --> OPA2134 IN+
OPA2134 IN- --> OUT (suiveur)
OPA2134 OUT --> C (2.2uF film) --> AUDIO nappe

Alimentation: +9V_BUFFER (headroom 7.5V)
```

### C2-H : Interface Utilisateur

**OLED 0.96" I2C**
```
+3V3 --> VCC
SDA, SCL
Adresse: 0x3C
```

**Encodeur Rotatif**
```
GPIO18 --> A (pull-up interne)
GPIO19 --> B (pull-up interne)
GPIO20 --> SW
```

### C2-I : Monitoring ADC

**Batterie**
```
+22V_RAW --> R (220k 1%) --> ADC_BATT
ADC_BATT --> R (33k 1%) --> GND
ADC_BATT --> C (100nF) --> GND
```

**Temperature NTC**
```
+3V3 --> R (10k) --> ADC_NTC
ADC_NTC --> NTC (10k@25C) --> GND
```

## Nappe Inter-cartes

**Connecteur:** JST XH 16 pins, 100mm AWG24

| Pin | Signal | Dir | Note |
|-----|--------|-----|------|
| 1 | 22V_SENSE | C1->C2 | Via diviseur |
| 2 | +5V | C1->C2 | Via PTC1 |
| 3 | +3V3 | C1->C2 | Via PTC2 |
| 4-6 | GND | - | PWR/SIG/Shield |
| 7 | AUDIO_L | C2->C1 | Blindage pin 8 |
| 9 | AUDIO_R | C2->C1 | Blindage pin 10 |
| 11 | SDA | <-> | I2C |
| 12 | SCL | C2->C1 | I2C |
| 13 | AMP_EN | C2->C1 | |
| 14 | AMP_MUTE | C2->C1 | |
| 15 | AMP_ERR | C1->C2 | |
| 16 | SAFE_EN | C2->C1 | |

## BOM Principale

### Semiconducteurs

| Ref | Composant | Package | Qte |
|-----|-----------|---------|-----|
| U1 | MA12070 | QFN-48 | 1 |
| U2 | OPA2134PA | DIP-8 | 2 |
| U3 | TDA7439 | DIP-30 | 1 |
| U4 | CD4053BE | DIP-16 | 1 |
| U5 | AMS1117-3.3 | SOT-223 | 1 |
| U6 | LM7812 | TO-220 | 1 |
| U7 | MCP1703A-5002 | TO-92 | 1 |
| U8 | LM7809 | TO-220 | 1 |
| D1 | SS54 | SMA | 1 |
| D2 | SMBJ24CA | SMB | 1 |
| D3 | 1N5822 | DO-201AD | 1 |

### Protections V1.9

| Ref | Composant | Valeur | Qte |
|-----|-----------|--------|-----|
| PTC1 | MF-R075 | 750mA | 1 |
| PTC2 | MF-R050 | 500mA | 1 |
| NTC1 | SL10 5R005 | 5ohm | 1 |

### Modules

| Module | Qte |
|--------|-----|
| ESP32-S3-WROOM-1-N8R8 | 1 |
| BTM525 | 1 |
| PCM5102A | 1 |
| MP1584EN buck | 1 |
| JBD BMS 6S 20A | 1 |
| OLED 0.96" I2C | 1 |

## Regles PCB

### Carte 1 (Puissance)

- Star Ground sur C_BULK (220uF)
- Pistes puissance: 2mm minimum
- Dissipateur MA12070 via PCB
- Distance securite HT: 3mm

### Carte 2 (Signal)

- Plan de masse continu
- Separation analogique/numerique
- Traces audio blindees GND
- Decouplage 100nF chaque CI

### Thermique

| Composant | P_max | Dissipateur |
|-----------|-------|-------------|
| MA12070 | 2W | PCB copper pour |
| LM7812 | 0.3W | TO-220 nu OK |
| LM7809 | 0.2W | TO-220 nu OK |

## Tests

Voir Breakout_Box_Test_V1_2.md pour:
- Procedure de test complete
- Points de mesure
- Valeurs attendues
- Avertissement boucle de masse

## Fichiers

```
Ampli_Audiophile_Portable_V1_9.md   # Specs detaillees
Breakout_Box_Test_V1_2.md          # Outil de test
```

## Changelog Hardware

| Version | Modifications |
|---------|---------------|
| V1.9 | PTC nappe, NTC inrush, Buffer 9V, C_REF |
| V1.8 | NTC monitoring |
| V1.7 | LM7812 pre-regulator |
| V1.6 | Star Ground |
| V1.5 | TVS, nappe blindee |

---

*Schema concu pour montage DIY sur veroboard ou PCB custom*
