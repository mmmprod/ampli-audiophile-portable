# Ampli Audiophile Portable

![Version](https://img.shields.io/badge/version-1.10-blue)
![MCU](https://img.shields.io/badge/MCU-ESP32--S3-red)
![Bluetooth](https://img.shields.io/badge/Bluetooth-LDAC-purple)
![Amp](https://img.shields.io/badge/Amp-MA12070_Class--D-green)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

Amplificateur audiophile portable DIY avec Bluetooth LDAC, entree phono RIAA, et amplification Class-D. Concu pour alimenter des enceintes passives vintage depuis une batterie LiPo 6S.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         SIGNAL PATH                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────┐    ┌──────────┐    ┌─────────┐    ┌────────┐          │
│  │ BTM525  │───>│ PCM5102A │───>│ TDA7439 │───>│OPA2134 │          │
│  │  LDAC   │    │   DAC    │    │   EQ    │    │ Buffer │          │
│  └─────────┘    └──────────┘    └─────────┘    └────┬───┘          │
│       │                              ^              │               │
│       │              ┌───────────────┘              │               │
│  ┌────┴────┐    ┌────┴────┐                   ┌─────v─────┐        │
│  │  AUX    │    │  PHONO  │                   │  MA12070  │        │
│  │ 3.5mm   │    │  RIAA   │                   │  Class-D  │        │
│  └─────────┘    └─────────┘                   └─────┬─────┘        │
│                                                     │               │
│                                              ┌──────v──────┐        │
│                                              │   HP 8ohm   │        │
│                                              │  2x20W RMS  │        │
│                                              └─────────────┘        │
└─────────────────────────────────────────────────────────────────────┘
```

## Specifications

| Parametre | Valeur |
|-----------|--------|
| Puissance | 2 x 20W RMS @ 8ohm |
| THD+N | < 0.01% @ 1W |
| SNR | > 110dB (amp) / > 65dB (phono) |
| Bluetooth | LDAC, aptX HD, aptX, AAC, SBC |
| Entrees | Bluetooth, AUX 3.5mm, Phono MM |
| Egaliseur | 3 bandes +/-14dB |
| Batterie | LiPo 6S 22.2V (18-25.2V) |
| Autonomie | 4-6h |

## Architecture Bi-Carte

```
┌─────────────────────────────────────────────┐
│           CARTE 2 - SIGNAL                  │
│  ESP32-S3 | BT | DAC | EQ | Buffer         │
│  [V1.10: Level Shifter BSS138 I2C]         │
└──────────────────┬──────────────────────────┘
                   │ Molex Micro-Fit 16P
                   │ [V1.10: Verrouillage]
┌──────────────────┴──────────────────────────┐
│           CARTE 1 - PUISSANCE               │
│  BMS 6S | 5-Level Protection | MA12070     │
│  [V1.10: Sequence Anti-Plop]               │
└─────────────────────────────────────────────┘
```

## Protection 5 Niveaux

```
+PACK --> [N1 BMS] --> [N2 TCO] --> [N3 Relay] --> [N3bis NTC] --> [N4 Fuse] --> [N5 TVS] --> +22V
              |            |            |              |              |             |
              v            v            v              v              v             v
          OVP/UVP     Thermal 72C   Software      Inrush 5A       5A Fast      Transient
          25A OCP      Auto-Reset    Control        Limit          Blow         Suppress
```

## Bus I2C [V1.10]

```
                    Level Shifter BSS138
                    ┌─────────────────┐
ESP32 (3.3V) ──────>│ SDA_3V3  SDA_9V │──────> TDA7439 (9V)
GPIO1              │                 │         MA12070
                   │ SCL_3V3  SCL_9V │
ESP32 ────────────>│                 │──────>
GPIO2              └─────────────────┘

OLED SSD1306 @ 0x3C  (3.3V domain)
TDA7439      @ 0x44  (9V domain via level shifter)
MA12070      @ 0x20  (9V domain via ribbon)
```

## Power Rails

```
+BATT (18-25V)
    |
    +---> LM7812 ---> +12V_PRE
    |                    |
    |                    +---> LM7809 ---> +9V_BUFFER (TDA7439 + OPA2134)
    |                    |
    |                    +---> MCP1703A ---> +5V_ANALOG (DAC, MUX)
    |
    +---> MP1584 ---> +5V (Digital)
    |                    |
    |                    +---> AMS1117 ---> +3V3 (ESP32, OLED)
    |
    +---> Direct ---> +PVDD (MA12070 Class-D)
```

## Fichiers

```
ampli-audiophile-portable/
├── README.md                           # Ce fichier
├── README_HARDWARE.md                  # Documentation hardware
├── README_FIRMWARE.md                  # Documentation firmware
├── hardware/
│   └── Ampli_Audiophile_Portable_V1_10.md
├── firmware/
│   └── Firmware_Ampli_V1_10.ino
└── test/
    └── Breakout_Box_Test_V1_3.md
```

## Changelog

| Version | Date | Corrections |
|---------|------|-------------|
| V1.10 | Dec 2025 | Level shifter BSS138, sequence anti-plop, Molex |
| V1.9 | Dec 2025 | I2C open-drain fix, PTC nappe, NTC inrush, buffer 9V |
| V1.8 | Nov 2025 | NTC fail-safe, median filter, encoder mutex |

## Corrections Critiques V1.10

| Bug | Impact | Solution |
|-----|--------|----------|
| TDA7439 @ 5V | I2C mort (V_IH=6.3V > 3.3V) | Alim 9V + BSS138 level shifter |
| Plop extinction | Stress HP, destruction tweeter | Sequence MUTE -> EN -> RELAY |
| JST XH vibrations | Deconnexion = destruction MCU | Molex Micro-Fit 3.0 |

## Quick Start

1. **Assembler** les deux cartes selon documentation hardware
2. **Flasher** le firmware via USB-C
3. **Connecter** batterie 6S et enceintes 8ohm
4. **Appairer** en Bluetooth (nom: "Ampli Portable")

## Securite

```
!!! SORTIES BTL !!!

HP_L- et HP_R- sont des SORTIES ACTIVES, pas la masse!

INTERDIT:
  - Sonde oscillo standard sur HP_L- ou HP_R-
  - USB branche pendant mesure HP

OBLIGATOIRE:
  - Sondes differentielles uniquement
  - OU debrancher USB avant mesure HP
  - OU isolateur USB galvanique (Adafruit #2107)
```

## License

MIT License - Voir LICENSE pour details.

## Auteur

Mehdi - Projet DIY audiophile
