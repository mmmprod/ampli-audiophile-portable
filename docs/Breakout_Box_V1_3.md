# BREAKOUT BOX TEST V1.3

## INFORMATIONS DOCUMENT

**Version:** 1.3
**Date:** 14 decembre 2025
**Auteur:** Mehdi
**Status:** Mise a jour pour Ampli V1.10

---

## CHANGELOG V1.3

| # | Modification | Raison |
|---|--------------|--------|
| B1 | Isolateur USB galvanique | Eviter court-circuit oscillo BTL |
| B2 | Sticker warning geant BTL | Securite operateur |
| B3 | Tests level shifter I2C | Validation BSS138 |
| B4 | Tests sequence extinction | Validation anti-plop |
| B5 | Test Molex verrouillage | Validation mecanique |

### Rappel V1.2
- LED status sur tous rails
- Points test organises par fonction
- Warning masses separees

---

## OBJECTIF

Banc de test portable pour valider l'Ampli Audiophile V1.10:
- Verification alimentations et protections
- Debug I2C avec level shifter
- Test sequence extinction anti-plop
- Mesures audio sans risque court-circuit

---

## ARCHITECTURE

```
+------------------------------------------------------------------+
|                    BREAKOUT BOX V1.3                             |
|                                                                  |
|  +------------------+  +------------------+  +----------------+  |
|  | ZONE ALIM        |  | ZONE I2C/DEBUG   |  | ZONE AUDIO     |  |
|  | LED + Points     |  | Level Shifter    |  | Connecteurs    |  |
|  +------------------+  +------------------+  +----------------+  |
|                                                                  |
|  [V1.3] ISOLATEUR USB         [V1.3] STICKER BTL GEANT          |
|  +------------------+         +----------------------------+     |
|  | Adafruit 2107    |         | !!! SONDES DIFF ONLY !!!  |     |
|  | Isolation 2.5kV  |         | HP_L- et HP_R- ACTIVES    |     |
|  +------------------+         +----------------------------+     |
|                                                                  |
+------------------------------------------------------------------+
```

---

# SECTION 1: ISOLATEUR USB [NOUVEAU V1.3]

## Probleme Resolu

```
DANGER SANS ISOLATEUR:

PC USB -----> ESP32 GND -----> Star Ground
                                    |
Oscillo GND -----> Prise Secteur ---+
                                    |
HP_L- (SORTIE ACTIVE MA12070) <-----+
                                    |
                              COURT-CIRCUIT!

MA12070 BTL: HP_L- oscille en opposition avec HP_L+
Si sonde oscillo (terre) touche HP_L- = court-circuit
```

## Solution

```
PC USB --> [ISOLATEUR USB] --> ESP32

Isolateur: Adafruit USB Isolator #2107
- Isolation galvanique 2.5kV
- USB 2.0 Full Speed
- Alimentation isolee incluse
```

## Composant

| Ref | Description | Fournisseur |
|-----|-------------|-------------|
| ISO1 | Adafruit USB Isolator #2107 | Adafruit / TME |

## Installation

```
Boitier Breakout:
PC USB-A --> ISO1 IN --> ISO1 OUT --> Cable USB-C --> ESP32

LED temoin:
ISO1 relie GND isole --> ne PAS relier a GND_BOX!
```

---

# SECTION 2: WARNINGS SECURITE

## Sticker BTL Geant [NOUVEAU V1.3]

**Taille:** 80 x 40 mm minimum, fond ROUGE

```
+----------------------------------------------------------+
|  ⚠️⚠️⚠️ ATTENTION - SORTIES BTL ⚠️⚠️⚠️                    |
|                                                          |
|  HP_L- et HP_R- sont des SORTIES ACTIVES!                |
|  Elles NE SONT PAS a la masse!                           |
|                                                          |
|  INTERDIT:                                               |
|  ❌ Sonde oscillo standard sur HP_L- ou HP_R-            |
|  ❌ Mesure entre HP et chassis                           |
|  ❌ USB branche pendant mesure HP                        |
|                                                          |
|  OBLIGATOIRE:                                            |
|  ✅ Sondes DIFFERENTIELLES uniquement                    |
|  ✅ OU debrancher USB avant mesure HP                    |
|  ✅ OU utiliser isolateur USB                            |
+----------------------------------------------------------+
```

## Sticker Masses Separees

```
+------------------------------------------+
|  MASSES SEPAREES - NE PAS PONTER!        |
|                                          |
|  GND_PWR (noir) = Digital, ESP32, BT     |
|  GND_SIG (vert) = Audio, DAC, Preamp     |
|                                          |
|  Jonction UNIQUEMENT sur Carte 1         |
|  au point etoile (C_BULK negatif)        |
+------------------------------------------+
```

---

# SECTION 3: CONNECTEURS ET POINTS TEST

## J1 - Entree Alimentation

```
+BATT_IN (rouge) --> Borne banane rouge
GND (noir) --> Borne banane noire

Limite alim labo: 2A max pour tests
```

## J2 - Nappe Molex Micro-Fit [V1.3]

```
Embase Molex 43045-1600 (16 pins)
Replicat exact de la nappe ampli

Permet:
- Test continuites
- Injection signaux
- Mesure tensions
```

| Pin | Signal | Point Test | LED |
|-----|--------|------------|-----|
| 1 | 22V_SENSE | TP1 | - |
| 2 | +5V | TP2 | LED verte |
| 3 | +3V3 | TP3 | LED verte |
| 4 | GND_PWR | TP4 | - |
| 5 | GND_SIG | TP5 | - |
| 6 | GND_SHIELD | - | - |
| 7 | AUDIO_L | TP7 | - |
| 8 | GND_SHIELD | - | - |
| 9 | AUDIO_R | TP9 | - |
| 10 | GND_SHIELD | - | - |
| 11 | SDA_9V | TP11 | - |
| 12 | SCL_9V | TP12 | - |
| 13 | AMP_EN | TP13 | LED jaune |
| 14 | AMP_MUTE | TP14 | LED rouge |
| 15 | AMP_ERR | TP15 | LED rouge |
| 16 | SAFE_EN | TP16 | LED verte |

## J3 - Debug I2C [V1.3]

```
Header 2x4 pins pour analyseur logique:

Pin 1: SDA_3V3 (cote ESP32)
Pin 2: SCL_3V3 (cote ESP32)
Pin 3: SDA_9V (cote TDA7439)
Pin 4: SCL_9V (cote TDA7439)
Pin 5: GND
Pin 6: +3V3
Pin 7: +9V
Pin 8: GND
```

## J4 - USB Isole [V1.3]

```
USB-A femelle --> Isolateur Adafruit --> USB-C male

LED bleue: Isolation active
```

## J5 - Sorties HP (ATTENTION BTL!)

```
Borniers a vis 4 positions:

HP_L+ (rouge)
HP_L- (noir) ⚠️ ACTIF!
HP_R+ (rouge)
HP_R- (noir) ⚠️ ACTIF!

STICKER WARNING a cote!
```

---

# SECTION 4: LED STATUS

## Schema LED

```
+22V_RAW --> R (10k) --> LED rouge --> GND     [Alim principale]
+12V_PRE --> R (4.7k) --> LED jaune --> GND    [Pre-regulateur]
+9V_BUFFER --> R (3.3k) --> LED jaune --> GND  [Buffer/TDA]
+5V --> R (1k) --> LED verte --> GND           [Digital]
+3V3 --> R (470) --> LED verte --> GND         [MCU]
SAFE_EN --> R (1k) --> LED verte --> GND       [Relay actif]
AMP_MUTE --> R (1k) --> LED rouge --> GND      [Mute actif]
AMP_EN --> R (1k) --> LED jaune --> GND        [Ampli actif]
```

## Interpretation

| LED | Couleur | Etat Normal | Signification |
|-----|---------|-------------|---------------|
| 22V | Rouge | ON | Batterie connectee |
| 12V | Jaune | ON | LM7812 OK |
| 9V | Jaune | ON | LM7809 OK |
| 5V | Verte | ON | MP1584 OK |
| 3V3 | Verte | ON | AMS1117 OK |
| RELAY | Verte | ON | Relay ferme |
| MUTE | Rouge | OFF | Audio actif |
| AMP_EN | Jaune | ON | MA12070 actif |

---

# SECTION 5: PROTOCOLE DE TEST V1.10

## Phase 1: Verification Hors Tension

### Test 1.1: Continuite Molex

```
Multimetre mode continuite:
- Chaque pin nappe --> point test correspondant
- GND_PWR (pin4) NON relie a GND_SIG (pin5) sur C2
- Verifier verrouillage Molex (click audible)
```

### Test 1.2: Isolation Masses

```
Multimetre mode resistance:
- GND_PWR <-> GND_SIG sur Carte 2: > 1Mohm
- GND_PWR <-> GND_SIG sur Carte 1: < 1ohm (star point)
```

### Test 1.3: Level Shifter I2C [V1.3]

```
Multimetre mode diode:
- SDA_3V3 <-> SDA_9V: ~0.5V (diode body BSS138)
- SCL_3V3 <-> SCL_9V: ~0.5V (diode body BSS138)
```

---

## Phase 2: Verification Alimentations

### Test 2.1: Montee Progressive

```
Alim labo: 0V --> 22V en 10 secondes
Limite courant: 500mA

Observer:
- LED 22V s'allume vers 18V
- Pas de fumee/odeur
- Courant repos < 50mA
```

### Test 2.2: Rails Regulateurs

```
Multimetre sur points test:

| Rail | TP | Min | Typ | Max |
|------|-----|-----|-----|-----|
| 22V_RAW | TP1 | 18V | 22V | 25.2V |
| +12V | TP12V | 11.4V | 12V | 12.6V |
| +9V | TP9V | 8.6V | 9V | 9.4V |
| +5V | TP2 | 4.9V | 5V | 5.1V |
| +3V3 | TP3 | 3.2V | 3.3V | 3.4V |
```

### Test 2.3: Protection Inrush NTC

```
Oscilloscope sur +22V_RAW:
- Temps de montee observe: > 50ms (NTC limite)
- Pas de pic > 5A (si shunt mesure disponible)
```

---

## Phase 3: Test I2C avec Level Shifter [V1.3]

### Test 3.1: Scan I2C

```
Via Serial Monitor (115200 baud):

Attendu:
"I2C: OLED trouve @ 0x3C"
"I2C: TDA7439 trouve @ 0x44"
"I2C: MA12070 trouve @ 0x20"
```

### Test 3.2: Niveaux Logiques

```
Oscilloscope sur J3 (header debug):

Canal 1: SDA_3V3 (jaune)
Canal 2: SDA_9V (bleu)

Envoyer commande I2C, observer:
- SDA_3V3 LOW = 0V, HIGH = 3.3V
- SDA_9V LOW = 0V, HIGH = 9V (level shifted!)

Transition 3V3 --> 9V via BSS138 OK
```

### Test 3.3: Timing I2C

```
Analyseur logique sur J3:

Verifier:
- Clock 400kHz (+/- 10%)
- Setup/Hold times respectes
- ACK present apres chaque byte
```

---

## Phase 4: Test Sequence Extinction [V1.3]

### Test 4.1: Extinction Normale

```
Oscilloscope 4 canaux:
- CH1: AMP_MUTE (TP14)
- CH2: AMP_EN (TP13)
- CH3: SAFE_EN (TP16)
- CH4: HP_L+ (sonde differentielle!)

Trigger sur appui bouton OFF

Sequence attendue:
1. T=0: MUTE passe HIGH
2. T=50ms: AMP_EN passe LOW
3. T=150ms: SAFE_EN passe LOW
4. HP_L+: Pas de transitoire > 100mV
```

### Test 4.2: Coupure Alimentation (Power Fail)

```
Setup:
- Ampli en fonctionnement
- Alim labo avec bouton ON/OFF

Test:
- Couper alimentation brutalement
- Observer GPIO8 (POWER_FAIL) --> doit passer LOW
- Observer AMP_MUTE --> doit passer HIGH immediatement (ISR)

Timing critique:
- Detection < 1ms
- MUTE actif AVANT effondrement rails
```

### Test 4.3: Plop Audio

```
HP 8 ohm connecte (petit volume)
Microphone ou oreille a 50cm

Test extinction:
- Aucun "plop" audible
- Si plop: verifier sequence firmware
```

---

## Phase 5: Test Thermique

### Test 5.1: Temperature NTC

```
Serial Monitor:
"Temp: 25.3C"

Chauffer NTC avec seche-cheveux:
- Temperature monte
- A 50C: "THERMAL WARNING"
- A 65C: "THERMAL CRITICAL" + volume reduit
- A 75C: "THERMAL SHUTDOWN"
```

### Test 5.2: NTC Fail-Safe [V1.9]

```
Deconnecter NTC:

Serial Monitor:
"NTC: Fail detected"
"Volume: Limite 50% (NTC HS)"

Volume bloque a 50% max
```

### Test 5.3: Dissipation LM7812 [V1.3]

```
Thermocouple sur tab LM7812:

Apres 30min fonctionnement:
- T_tab < 70C avec plan cuivre
- T_tab < 85C sans plan cuivre (a eviter)
```

---

## Phase 6: Test Audio

### Test 6.1: Signal Generator

```
Generateur: 1kHz sinus, 1Vrms
Entree: AUX (jack 3.5mm)

Oscilloscope differentiel sur HP:
- Signal propre, pas de distorsion visible
- Amplitude selon volume
```

### Test 6.2: THD (si analyseur disponible)

```
Charge: 8 ohm
Puissance: 1W (2.83Vrms)

THD+N attendu: < 0.1%
```

### Test 6.3: Bruit de Fond

```
Entree: Aucune (ou court-circuitee)
Volume: Maximum

Mesure sur HP (differentiel):
- Bruit < 1mVrms
- Pas de ronflement 50Hz
- Pas de sifflement HF
```

---

## Phase 7: Test Mecanique Molex [V1.3]

### Test 7.1: Verrouillage

```
Connecter nappe Molex:
- Click audible obligatoire
- Tirer doucement: ne doit PAS se deconnecter
```

### Test 7.2: Vibrations Simulees

```
Ampli en fonctionnement:
- Secouer moderement (simulation transport)
- Audio doit rester stable
- Pas de coupure
```

### Test 7.3: Cycles Connexion

```
10 cycles connexion/deconnexion:
- Contacts toujours OK
- Pas d'usure visible
```

---

# SECTION 6: CHECKLIST FINALE

## Avant Mise en Boitier

```
[ ] Tous rails tensions OK (+/- 5%)
[ ] I2C scan trouve 3 devices
[ ] Level shifter 3.3V <-> 9V OK
[ ] Sequence extinction sans plop
[ ] Detection power fail < 1ms
[ ] NTC fail-safe actif
[ ] Molex verrouillage OK
[ ] Sticker BTL visible
[ ] Isolateur USB installe
[ ] Temperature LM7812 < 70C
```

## GO / NO-GO

```
TOUS les points coches = GO pour terrain
UN SEUL point non coche = NO-GO, corriger d'abord
```

---

# SECTION 7: BOM BREAKOUT BOX V1.3

## Composants Specifiques

| Ref | Description | Qte |
|-----|-------------|-----|
| ISO1 | Adafruit USB Isolator #2107 | 1 |
| J2 | Molex 43045-1600 (embase) | 1 |
| J3 | Header 2x4 pins | 1 |
| J4 | USB-A femelle + USB-C male | 1 |
| J5 | Bornier a vis 4 positions | 1 |

## LED et Resistances

| Ref | Valeur | Qte |
|-----|--------|-----|
| LED 3mm rouge | - | 3 |
| LED 3mm jaune | - | 3 |
| LED 3mm verte | - | 4 |
| LED 3mm bleue | - | 1 |
| R 10k | 1/4W | 1 |
| R 4.7k | 1/4W | 1 |
| R 3.3k | 1/4W | 1 |
| R 1k | 1/4W | 5 |
| R 470 | 1/4W | 1 |

## Connecteurs

| Ref | Description | Qte |
|-----|-------------|-----|
| Borne banane 4mm rouge | - | 1 |
| Borne banane 4mm noire | - | 1 |
| Points test TP | - | 16 |

## Boitier

```
Hammond 1591XXCBK ou equivalent
Dimensions min: 120 x 80 x 40 mm
```

---

# SECTION 8: SCHEMA CABLAGE

## Vue Dessus

```
+------------------------------------------------------------------+
|  [STICKER BTL WARNING]                    [STICKER MASSES]       |
|                                                                  |
|  +--------+  +--------+  +--------+  +--------+  +--------+      |
|  | 22V    |  | 12V    |  | 9V     |  | 5V     |  | 3V3    |      |
|  | LED R  |  | LED J  |  | LED J  |  | LED V  |  | LED V  |      |
|  | TP     |  | TP     |  | TP     |  | TP     |  | TP     |      |
|  +--------+  +--------+  +--------+  +--------+  +--------+      |
|                                                                  |
|  +------------------+     +------------------+                   |
|  | J2 MOLEX 16P     |     | J3 DEBUG I2C    |                   |
|  | Nappe Ampli      |     | SDA/SCL 3V3/9V  |                   |
|  +------------------+     +------------------+                   |
|                                                                  |
|  +------------------+     +------------------+                   |
|  | J4 USB ISOLE     |     | J5 HP BTL       |                   |
|  | PC <-> ESP32     |     | L+/L-/R+/R-     |                   |
|  | LED bleue        |     | ⚠️ DIFF ONLY    |                   |
|  +------------------+     +------------------+                   |
|                                                                  |
|  (O) J1 +BATT        (O) J1 GND                                 |
|      Rouge               Noir                                    |
|                                                                  |
+------------------------------------------------------------------+
```

---

# SECTION 9: NOTES SECURITE

## Risques Identifies

| Risque | Cause | Mitigation |
|--------|-------|------------|
| Court-circuit BTL | Sonde oscillo terre | Isolateur USB + sticker |
| Destruction ESP32 | Molex deconnecte | Molex verrouillage |
| Plop HP | Extinction brutale | Sequence firmware |
| Surchauffe | NTC HS | Fail-safe 50% |

## Procedure Urgence

```
SI fumee ou odeur:
1. Couper alimentation IMMEDIATEMENT
2. Debrancher tous cables
3. Attendre refroidissement 5 min
4. Identifier composant en cause
5. Ne PAS rebrancher avant correction
```

## EPI Recommandes

```
- Lunettes de protection (condensateurs)
- Pas de bijoux metalliques
- Surface de travail isolante
```

---

# FIN BREAKOUT BOX TEST V1.3
