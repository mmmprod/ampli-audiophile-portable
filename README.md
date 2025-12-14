# AMPLIFICATEUR AUDIOPHILE PORTABLE

## Vue d'ensemble

Amplificateur stereo portable haut de gamme combinant technologie moderne et compatibilite avec equipements vintage.

```
+------------------+     +------------------+     +------------------+
|   SOURCES        |     |   AMPLIFICATEUR  |     |   SORTIES        |
|                  |     |                  |     |                  |
|  Bluetooth LDAC  | --> |  Preamp RIAA     | --> |  2 x 20W RMS     |
|  AUX 3.5mm       | --> |  EQ 3 bandes     | --> |  Enceintes 4-8   |
|  Phono MM        | --> |  Class-D MA12070 | --> |  ohms passives   |
+------------------+     +------------------+     +------------------+
                               |
                         Batterie 6S LiPo
                         4-6h autonomie
```

## Caracteristiques

| Parametre | Specification |
|-----------|---------------|
| Puissance | 2 x 20W RMS @ 8 ohms |
| THD+N | < 0.01% @ 1W |
| SNR | > 110 dB (ampli) |
| Bluetooth | LDAC, aptX HD, aptX, AAC, SBC |
| Entrees | Bluetooth, AUX 3.5mm, Phono MM |
| Egaliseur | Bass/Mid/Treble +/-14dB |
| Batterie | LiPo 6S 22.2V (18-25.2V) |
| Autonomie | 4-6 heures |

## Architecture

### Dual-PCB Design

```
CARTE 2 - SIGNAL (80 x 120 mm)
+-----------------------------------------------+
|  ESP32-S3  |  BTM525  |  PCM5102A  |  TDA7439 |
|    MCU     |    BT    |    DAC     |    EQ    |
+-----------------------------------------------+
                    |
              Nappe 16 pins
           (avec PTC protection)
                    |
+-----------------------------------------------+
|  BMS 6S  |  5-Level  |  MA12070  |  Sorties  |
|          |  Safety   |  Class-D  |    HP     |
+-----------------------------------------------+
CARTE 1 - PUISSANCE (80 x 100 mm)
```

### Securite 5 Niveaux

```
Pack LiPo --> [N1: BMS] --> [N2: TCO 72C] --> [N3: Relais K1]
                                                    |
          [N5: TVS + Schottky] <-- [N4: Fusible 5A] <-- [N3bis: NTC Inrush]
```

## Documentation

| Document | Description |
|----------|-------------|
| [README_HARDWARE.md](README_HARDWARE.md) | Schema complet, BOM, PCB |
| [README_FIRMWARE.md](README_FIRMWARE.md) | Code ESP32-S3, API |
| [Ampli_V1.9.md](Ampli_Audiophile_Portable_V1_9.md) | Specifications detaillees |
| [Breakout_Box_V1.2.md](Breakout_Box_Test_V1_2.md) | Outil de test |

## Quick Start

### 1. Assemblage

```
1. Assembler Carte 1 (puissance)
2. Assembler Carte 2 (signal)
3. Connecter nappe 16 pins
4. Connecter batterie 6S via BMS
5. Connecter enceintes 4-8 ohms
```

### 2. Premier demarrage

```
1. Verifier toutes connexions
2. Multimetre: pas de court-circuit
3. Alimenter (batterie chargee)
4. LED status doit s'allumer
5. Appairer Bluetooth "AMPLI-AUDIO"
```

### 3. Utilisation

```
Encodeur:
- Rotation = Volume
- Clic = Changer source (BT -> AUX -> PHONO)
- Double-clic = Menu EQ

Telecommande IR:
- VOL+/- = Volume
- 1/2/3 = Source BT/AUX/PHONO
- MUTE = Sourdine
```

## Changelog

| Version | Date | Modifications |
|---------|------|---------------|
| V1.9 | 14/12/2025 | PTC nappe, NTC inrush, Buffer 9V, I2C fix |
| V1.8 | 14/12/2025 | NTC fail-safe, I2C recovery |
| V1.7 | 13/12/2025 | LM7812 pre-regulator, D3 1N5822 |
| V1.6 | 13/12/2025 | Star Ground, regles PCB |
| V1.5 | 13/12/2025 | D3 PVDD, TVS, nappe blindee |

## Securite

### Avertissements

```
!!! HAUTE TENSION - Batterie 25V max !!!
!!! NE PAS court-circuiter la batterie !!!
!!! Utiliser uniquement chargeur 6S balance !!!
```

### Boucle de masse (tests)

```
INTERDIT: Oscillo secteur + Chargeur + USB PC
OK: Ampli sur batterie + Oscillo secteur
OK: Sonde differentielle isolee
```

## Licence

Projet open-source. Documentation et code libres.

## Auteur

Mehdi - Decembre 2025

---

*Amplificateur concu pour usage avec enceintes passives vintage.*
*Compatible 4-8 ohms. Ne pas utiliser sous 4 ohms.*
