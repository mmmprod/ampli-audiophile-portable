# Hardware Documentation V1.10

Documentation technique du hardware pour l'Ampli Audiophile Portable.

## Dual-PCB Architecture

### Carte 1 - Puissance (80 x 100 mm)

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────────┐   │
│  │   BMS   │  │  TCO    │  │  RELAY  │  │    MA12070      │   │
│  │  6S 20A │  │  72C    │  │  HF46F  │  │    Class-D      │   │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────────┬────────┘   │
│       │            │            │                 │            │
│  ┌────v────────────v────────────v─────────────────v────────┐  │
│  │                    STAR GROUND                           │  │
│  │                  (C_BULK negative)                       │  │
│  └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐          │
│  │ LM7812  │  │ LM7809  │  │ MP1584  │  │MCP1703A │          │
│  │  +12V   │  │  +9V    │  │  +5V    │  │+5V_ANA  │          │
│  │[CUIVRE] │  │[Buffer] │  │         │  │         │          │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘          │
│                                                                 │
│                  [NTC Inrush 5ohm]  [PTC 750mA/500mA]          │
│                                                                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                    Molex Micro-Fit 16P [V1.10]
                           │
┌──────────────────────────┴──────────────────────────────────────┐
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ ESP32-S3 │  │  BTM525  │  │ PCM5102A │  │  CD4053  │       │
│  │   MCU    │  │ Bluetooth│  │   DAC    │  │   MUX    │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────────┐     │
│  │  TDA7439 │  │ OPA2134  │  │      Level Shifter       │     │
│  │    EQ    │  │  Buffer  │  │   BSS138 x2 [V1.10]      │     │
│  │  [9V]    │  │  [9V]    │  │   3.3V <-> 9V I2C        │     │
│  └──────────┘  └──────────┘  └──────────────────────────┘     │
│                                                                 │
│                    CARTE 2 - SIGNAL (80 x 120 mm)              │
└─────────────────────────────────────────────────────────────────┘
```

## 5-Level Protection Chain

```
+PACK ────┬────────────────────────────────────────────────────> +22V_RAW
          │
          v
    ┌───────────┐
    │  N1: BMS  │  JBD SP22S003B
    │  6S 20A   │  - OVP: 4.25V/cell
    │           │  - UVP: 2.8V/cell
    │           │  - OCP: 25A
    │           │  - SCP: <100us
    └─────┬─────┘
          │
          v
    ┌───────────┐
    │  N2: TCO  │  Aupo A4-1A-F
    │   72C     │  - Auto-reset
    │   10A     │  - Trip @ 72+/-5C
    └─────┬─────┘
          │
          v
    ┌───────────┐
    │ N3: Relay │  HF46F-G/12-HS1
    │  10A NO   │  - Opto-isolated driver
    │           │  - Software control
    └─────┬─────┘
          │
          v
    ┌───────────┐
    │N3bis: NTC │  Ametherm SL10 5R005 [V1.9]
    │  Inrush   │  - R_cold: 5ohm
    │  Limiter  │  - R_hot: 0.2ohm
    │           │  - I_peak: 5A (vs 500A sans!)
    └─────┬─────┘
          │
          v
    ┌───────────┐
    │ N4: Fuse  │  5A ATO Fast-blow
    │   5A      │  - Break: 35A @ 32VDC
    └─────┬─────┘
          │
          v
    ┌───────────┐
    │ N5: TVS   │  SMBJ24CA
    │  600W     │  - Vbr: 24V
    │           │  - Clamp: 38V @ 15A
    └─────┬─────┘
          │
          v
       +22V_RAW
```

### NTC Inrush Limiter [V1.9]

```
PROBLEME:
C_BULK = 220uF, ESR = 40mohm
I_peak = V / ESR = 25V / 0.04ohm = 625A!
--> Contacts relay soudes, traces fondues

SOLUTION:
NTC 5ohm @ 25C en serie avec relay
I_peak = 25V / 5ohm = 5A (acceptable)

Sequence:
T=0:     K1 ferme, NTC froid (5ohm)
T=200ms: NTC chauffe, R descend a 0.2ohm
         C_BULK charge progressivement
         Pas d'arc destructeur
```

## Power Rails

```
+22V_RAW (18-25.2V)
    │
    ├──> LM7812 ──────> +12V_PRE
    │    Vin: 18-25V       │
    │    Dropout: 2V       │
    │    [V1.10: Cuivre    │
    │     15x15mm]         │
    │                      │
    │                      ├──> LM7809 ──> +9V_BUFFER
    │                      │    Vin: 12V      │
    │                      │    I: 40mA       │
    │                      │                  ├── TDA7439 (35mA)
    │                      │                  └── OPA2134 (5mA)
    │                      │
    │                      └──> MCP1703A ──> +5V_ANALOG
    │                           Vin: 12V         │
    │                           Low noise        ├── PCM5102A
    │                           16uVrms          └── CD4053
    │
    ├──> MP1584 ──────> +5V
    │    Buck             │
    │    Eff: 96%         │
    │                     └──> AMS1117 ──> +3V3
    │                          800mA           │
    │                                          ├── ESP32-S3
    │                                          └── OLED
    │
    └──> Direct ──────> +PVDD
         Via SS54           │
         Schottky           └── MA12070 (20V min for 8ohm)
```

### LM7812 Thermal [V1.10]

```
Calcul:
P_max = (25.2V - 12V) x 50mA = 0.66W
Rth_ja (air libre) = 65C/W --> Tj = 83C
Rth_ja (avec cuivre 15x15mm) = 40C/W --> Tj = 66C

Layout:
┌─────────────────────┐
│      LM7812         │
│    ┌─────────┐      │
│    │   TAB   │      │
│    └────┬────┘      │
│         │           │
│  ┌──────┴───────┐   │
│  │    CUIVRE    │   │  15 x 15 mm
│  │   15x15mm    │   │  Relie GND par vias
│  └──────────────┘   │
└─────────────────────┘
```

## Level Shifter I2C [V1.10]

### Probleme

```
TDA7439 @ 9V:
  V_IH = 0.7 x 9V = 6.3V minimum pour HIGH
  
ESP32 @ 3.3V:
  V_OH = 3.3V maximum

3.3V < 6.3V --> TDA7439 ne voit jamais HIGH!
               I2C completement mort!
```

### Solution: BSS138 Bidirectionnel

```
+3V3                                   +9V
  │                                      │
 [10k]                                 [10k]
  │                                      │
  ├──────────[Source]  [Drain]───────────┤
  │              │        │              │
  │            BSS138     │              │
  │              │        │              │
SDA_3V3 ────────[Gate]────┘         SDA_9V ───> TDA7439
  │              │                       │       MA12070
  │              │                       │
 to             3.3V                     │
ESP32                                    │
OLED                                     │

Meme circuit pour SCL
```

### Fonctionnement

| ESP32 | BSS138 | SDA_9V | TDA voit |
|-------|--------|--------|----------|
| LOW (0V) | ON (V_GS=3.3V) | 0V (via Rds) | LOW OK |
| HIGH (3.3V) | OFF (V_GS=0V) | 9V (via pull-up) | HIGH OK |
| INPUT | OFF | 9V (via pull-up) | HIGH OK |

**Sens inverse (TDA tire LOW):**
```
TDA7439 tire SDA_9V a 0V
--> Body diode BSS138 conduit
--> SDA_3V3 = ~0.7V
--> ESP32 voit LOW
```

### Composants

```
Q_SDA: BSS138 (SOT-23)
Q_SCL: BSS138 (SOT-23)
R_SDA_LV: 10k 0603
R_SDA_HV: 10k 0603
R_SCL_LV: 10k 0603
R_SCL_HV: 10k 0603
```

## Ribbon Cable (Molex Micro-Fit 3.0) [V1.10]

### Probleme JST XH

```
JST XH = friction lock seulement
Force extraction: ~0.5N

Vibrations (sac, voiture):
--> Connecteur recule progressivement
--> Si GND se deconnecte avant +22V
--> Courant cherche retour via GPIO ESP32
--> I = 22V / R_gpio --> DESTRUCTION MCU
```

### Solution: Molex Micro-Fit 3.0

```
Molex Micro-Fit 3.0:
- Verrouillage positif (click audible)
- Force extraction: 12N (24x plus!)
- Rated vibrations automobile
- Impossible de se deconnecter sans action volontaire

References:
- Embase: Molex 43045-1600
- Connecteur: Molex 43025-1600
- Contacts: Molex 43030-0007
```

### Pinout

| Pin | Signal | Dir | Protection | Note |
|-----|--------|-----|------------|------|
| 1 | 22V_SENSE | C1->C2 | Divider | Pour ADC |
| 2 | +5V | C1->C2 | PTC 750mA | Digital |
| 3 | +3V3 | C1->C2 | PTC 500mA | MCU |
| 4 | GND_PWR | - | - | Digital GND |
| 5 | GND_SIG | - | - | Audio GND |
| 6 | GND_SHIELD | - | - | Shield |
| 7 | AUDIO_L | C2->C1 | - | Left channel |
| 8 | GND_SHIELD | - | - | Shield |
| 9 | AUDIO_R | C2->C1 | - | Right channel |
| 10 | GND_SHIELD | - | - | Shield |
| 11 | SDA_9V | <-> | - | I2C Data |
| 12 | SCL_9V | C2->C1 | - | I2C Clock |
| 13 | AMP_EN | C2->C1 | - | MA12070 Enable |
| 14 | AMP_MUTE | C2->C1 | - | MA12070 Mute |
| 15 | AMP_ERR | C1->C2 | - | MA12070 Error |
| 16 | SAFE_EN | C2->C1 | - | Relay Control |

### PTC Protection [V1.9]

```
AVANT:
+5V (C1) --> Nappe --> +5V (C2)
Court-circuit C2 --> Nappe fond (I > 2A AWG24)

APRES:
+5V (C1) --> PTC1 (750mA) --> Nappe --> +5V (C2)
+3V3 (C1) --> PTC2 (500mA) --> Nappe --> +3V3 (C2)
Court-circuit C2 --> PTC trip --> Protection OK

PTC1: Bourns MF-R075 (I_hold=750mA, I_trip=1.5A)
PTC2: Bourns MF-R050 (I_hold=500mA, I_trip=1.0A)
```

## Audio Signal Path

```
┌─────────┐     ┌──────────┐     ┌─────────┐     ┌────────┐
│ BTM525  │────>│ PCM5102A │────>│         │────>│        │
│  LDAC   │ I2S │   DAC    │     │         │     │        │
└─────────┘     └──────────┘     │         │     │        │
                                 │ CD4053  │     │TDA7439 │
┌─────────┐                      │  MUX    │────>│  EQ    │
│  AUX    │─────────────────────>│         │     │  @9V   │
│ 3.5mm   │                      │         │     │        │
└─────────┘                      │         │     │        │
                                 │         │     │        │
┌─────────┐     ┌──────────┐     │         │     │        │
│ PHONO   │────>│  RIAA    │────>│         │     │        │
│   MM    │     │ OPA2134  │     └─────────┘     └────┬───┘
└─────────┘     └──────────┘                         │
                                                     │
                ┌──────────┐     ┌──────────┐        │
                │ OPA2134  │<────┤          │<───────┘
                │  Buffer  │     │  Volume  │
                │   @9V    │     │  Control │
                └────┬─────┘     └──────────┘
                     │
                     v
              ┌────────────┐
              │  MA12070   │
              │  Class-D   │
              │   BTL      │
              └──────┬─────┘
                     │
              ┌──────v──────┐
              │   HP 8ohm   │
              │  2 x 20W    │
              └─────────────┘
```

### RIAA Preamp

```
PHONO_IN --> C_IN (100nF film) --> OPA2134 IN+
                                       │
             ┌─────────────────────────┘
             │
        RIAA Network:
        R1=750ohm ──┬── C1=3.3nF (pole 2122Hz)
                    │
        R2=75k ─────┴── C2=100pF (pole 21.2kHz)
                    │
             ┌──────┘
             │
             v
      OPA2134 IN- ──── OPA2134 OUT ──> PHONO_OUT

Alimentation: +5V_ANALOG (single supply)
VREF: Diviseur 10k/10k + C_REF 47uF [V1.10]
```

### VREF Filtering [V1.10]

```
AVANT (V1.8):
VREF = diviseur nu
Bruit 5V --> VREF --> amplifie 40dB --> audible!

APRES (V1.10):
+5V_ANALOG --> R_H (10k) --> VREF
                              │
VREF --> R_L (10k) --> GND    │
                              v
VREF --> C_REF (47uF film) --> GND

fc = 1 / (2 x pi x 5k x 47uF) = 0.68Hz
Attenuation @ 50Hz: -37dB
Bruit residuel: < 1mVrms
```

## MA12070 Class-D

```
+PVDD (20-26V) ──> MA12070 PVDD
                       │
                  ┌────┴────┐
                  │ Class-D │
                  │  BTL    │
                  └────┬────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────v────┐    ┌────v────┐    ┌────v────┐
   │ OUT_L+  │    │ OUT_L-  │    │  etc.   │
   └────┬────┘    └────┬────┘    └─────────┘
        │              │
       [L]            [L]      (10uH LC filter)
        │              │
        v              v
      HP_L+          HP_L-

ATTENTION: HP_L- est une SORTIE ACTIVE!
           Elle oscille en opposition avec HP_L+
           CE N'EST PAS LA MASSE!
```

### BTL Warning

```
┌──────────────────────────────────────────────────────────┐
│                                                          │
│   !!! HP_L- et HP_R- SONT DES SORTIES ACTIVES !!!       │
│                                                          │
│   INTERDIT:                                              │
│   - Sonde oscillo standard (terre secteur)              │
│   - Mesure entre HP et chassis                          │
│   - USB branche pendant mesure HP                       │
│                                                          │
│   OBLIGATOIRE:                                           │
│   - Sondes differentielles uniquement                   │
│   - OU debrancher USB                                   │
│   - OU isolateur USB galvanique                         │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

## Star Ground Topology

```
                    C_BULK (220uF)
                         │
                    ┌────┴────┐
                    │  STAR   │
                    │  POINT  │
                    └────┬────┘
                         │
      ┌──────┬───────┬───┴───┬───────┬──────┐
      │      │       │       │       │      │
      v      v       v       v       v      v
   GND_BMS GND_REG GND_AMP GND_PWR GND_SIG GND_SHIELD
                            (nappe) (nappe) (nappe)

REGLE:
- Toutes les masses convergent vers C_BULK negatif
- AUCUNE connexion GND ailleurs
- GND_PWR et GND_SIG separes sur Carte 2
- Jonction UNIQUEMENT au star point
```

## Thermal Calculations

| Component | P_diss | Rth | Tj @ 40C amb |
|-----------|--------|-----|--------------|
| LM7812 | 0.66W | 40C/W (cuivre) | 66C |
| LM7809 | 0.12W | 65C/W | 48C |
| MA12070 | 2W @ 10W out | 25C/W | 90C |
| OPA2134 | 50mW | 100C/W | 45C |
| NTC inrush | 0.8W (regime) | - | OK |

## BOM Highlights

### Semiconducteurs Critiques

| Ref | Part | Package | Note |
|-----|------|---------|------|
| U1 | MA12070 | QFN-48 | Ampli Class-D |
| U2 | OPA2134PA | DIP-8 | Audio op-amp x2 |
| U3 | TDA7439 | DIP-30 | EQ processor |
| Q_SDA | BSS138 | SOT-23 | Level shifter |
| Q_SCL | BSS138 | SOT-23 | Level shifter |

### Protection

| Ref | Part | Value | Note |
|-----|------|-------|------|
| NTC1 | Ametherm SL10 | 5ohm | Inrush limiter |
| PTC1 | Bourns MF-R075 | 750mA | 5V protection |
| PTC2 | Bourns MF-R050 | 500mA | 3V3 protection |
| F1 | ATO Fuse | 5A Fast | Main protection |
| D2 | SMBJ24CA | 600W | TVS transient |

### Connecteurs

| Ref | Part | Note |
|-----|------|------|
| J_INTER | Molex 43045-1600 | Embase PCB |
| J_PLUG | Molex 43025-1600 | Connecteur cable |

## Layout Tips

1. **Star Ground**: Toutes masses vers C_BULK negatif
2. **Separation**: GND_PWR et GND_SIG separes sur C2
3. **Cuivre LM7812**: Plan 15x15mm sous tab
4. **I2C**: Traces courtes, pull-ups pres des devices
5. **Audio**: Blindage entre canaux L/R
6. **PVDD**: Traces larges (2A peak)

## Changelog V1.10

| Correction | Avant | Apres |
|------------|-------|-------|
| TDA7439 alim | 5V (hors specs!) | 9V + level shifter |
| I2C level | 3.3V direct | BSS138 3.3V <-> 9V |
| Connecteur nappe | JST XH (friction) | Molex Micro-Fit (lock) |
| LM7812 thermal | Air libre | Plan cuivre 15x15mm |
| C_REF RIAA | 10uF | 47uF (-37dB @ 50Hz) |

## Changelog V1.9

| Correction | Description |
|------------|-------------|
| NTC inrush | 5ohm limite I_peak a 5A |
| PTC nappe | Protection 750mA/500mA |
| Buffer 9V | Headroom 7.5V au lieu de 3.5V |
| VREF decoupling | 10uF pour PSRR |
