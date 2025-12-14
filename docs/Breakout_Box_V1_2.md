# BREAKOUT BOX TEST V1.2

## AMPLIFICATEUR AUDIOPHILE PORTABLE - OUTIL DE TEST

**Version :** 1.2
**Date :** 14 decembre 2025
**Auteur :** Mehdi + Claude
**Compatible avec :** Ampli Audiophile V1.9

---

## CHANGELOG V1.2

| Modification | Raison |
|--------------|--------|
| **AVERTISSEMENT BOUCLE DE MASSE** | Risque destruction composants si oscillo secteur + USB PC + chargeur ampli |
| Sticker securite obligatoire | Rappel visible sur boitier |
| Procedure test securisee | Checklist anti-boucle de masse |

### Changelog V1.1

| Modification | Raison |
|--------------|--------|
| Protection ESD position 7 (PHONO) | Signal 5mV ultra-sensible expose sur selecteur metallique |
| R serie 100 ohm ajoutee | Limite courant ESD sans attenuer signal |
| TVS PESD5V0S1BA ajoutee | Clamp ESD 9V, capacite 0.5pF (transparent audio) |

---

## !!! AVERTISSEMENT SECURITE V1.2 !!!

### Risque Boucle de Masse

```
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!                                                           !!
!!          DANGER - BOUCLE DE MASSE                         !!
!!                                                           !!
!!  NE JAMAIS CONNECTER SIMULTANEMENT:                       !!
!!                                                           !!
!!    Oscillo SECTEUR + Ampli CHARGEUR + PC USB              !!
!!                                                           !!
!!  RISQUE: Courant parasite -> DESTRUCTION COMPOSANTS       !!
!!                                                           !!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
```

### Configurations DANGEREUSES (INTERDIT)

- Oscillo secteur + Ampli chargeur branche + PC USB
- Oscillo secteur + PC USB (meme sans ampli en charge)

### Configurations SURES (OK)

- Ampli sur BATTERIE SEULE + Oscillo secteur
- Ampli sur BATTERIE SEULE + Oscillo batterie
- Oscillo avec sonde differentielle isolee
- Oscillo USB isole galvaniquement

### Sticker Obligatoire

**A coller sur le boitier Breakout Box (80 x 30 mm):**

```
+-------------------------------------------------------------+
|                                                             |
|   !!!  AVANT TEST - VERIFIER:                               |
|                                                             |
|   [ ] Ampli sur BATTERIE (chargeur debranche)               |
|   [ ] OU oscillo isole/batterie                             |
|   [ ] PC USB debranche si oscillo secteur                   |
|                                                             |
|   INTERDIT: Oscillo secteur + Chargeur + USB                |
|                                                             |
+-------------------------------------------------------------+
```

---

## OBJECTIF

Reduire le temps de test de **2 heures a 15 minutes** en :
- Centralisant tous les points de mesure sur des bornes banane accessibles
- Affichant l'etat des alimentations par LEDs
- Permettant de brancher multimetre/oscillo une seule fois

---

## CONCEPT

```
+-----------------+     Nappe 20cm      +---------------------+
|   CARTE 1       | =================== |                     |
|   (Puissance)   |    16 fils          |   BREAKOUT BOX      |
|  J_TEST_C1      |                     |                     |
+-----------------+                     |  - Bornes banane    |
                                        |  - LEDs status      |
+-----------------+     Nappe 20cm      |  - Selecteur probe  |
|   CARTE 2       | =================== |  - Protection ESD   |
|   (Signal)      |    20 fils          |  - Sticker securite |
|  J_TEST_C2      |                     +---------------------+
+-----------------+                            |
                                    Multimetre/Oscillo branches UNE FOIS
```

---

## SPECIFICATIONS BOITIER

### Boitier

**Reference :** Hammond 1591XXCBK (ou equivalent)
**Dimensions :** 150 x 100 x 50 mm
**Materiau :** ABS noir
**Fournisseur :** TME, Mouser (~8 EUR)

### Face avant

```
+--------------------------------------------------------------------------+
|                                                                          |
|                     BREAKOUT BOX TEST V1.2                               |
|                     Ampli Audiophile                                     |
|                                                                          |
+--------------------------------------------------------------------------+
|                                                                          |
|  === ALIMENTATIONS ===          === SIGNAUX AUDIO ===                    |
|                                                                          |
|   [O]    [O]    [O]    [O]       [O]    [O]    [O]    [O]                 |
|   BATT   +22V   +5V   +3V3       DAC_L  DAC_R  VOL_L  VOL_R               |
|   Rouge  Orange Rouge Orange     Blanc  Gris   Blanc  Gris               |
|                                                                          |
|  === STATUS LEDs ===            === LOGIQUE/I2C ===                      |
|                                                                          |
|   o BATT OK    (vert)            [O]    [O]    [O]                        |
|   o +22V OK    (vert)             SDA    SCL    ERR                       |
|   o +5V OK     (vert)            Jaune  Jaune  Rouge                      |
|   o +3V3 OK    (vert)                                                    |
|   o AMP OK     (vert)           === ACTIVE PROBE ===                     |
|   o AMP ERR    (rouge)                                                   |
|   o BT CONN    (bleu)            +---------------+                       |
|                                  |   O   BNC/4mm |                       |
|   [O]  GND                       +---------------+                       |
|        Noir                       Selecteur 12 pos                       |
|                                                                          |
+--------------------------------------------------------------------------+
|                                                                          |
|   NAPPE C1 ===============        NAPPE C2 ===================           |
|            [Connecteur IDC]                [Connecteur IDC]              |
|                                                                          |
+--------------------------------------------------------------------------+
```

---

## SCHEMA INTERNE

### Vue d'ensemble

```
J_C1 (Carte 1)                    J_C2 (Carte 2)
    |                                  |
    +- BATT -------+---> Borne BATT    +- DAC_L ---+---> Pos 5
    |              +---> LED1          |           |
    +- +22V -------+---> Borne +22V    +- DAC_R ---+---> Pos 6
    |              +---> LED2          |           |
    +- +5V --------+---> Borne +5V     +- PHONO_L -+---> Pos 7 (+ESD V1.1)
    |              +---> LED3          |           |
    +- +3V3 -------+---> Borne +3V3    +- PHONO_R -+---> Pos 8 (+ESD V1.1)
    |              +---> LED4          |           |
    +- SPK_L+ ----------> Pos 1        +- SW_OUT_L-+---> Pos 9
    +- SPK_L- ----------> Pos 2        +- SW_OUT_R-+---> Pos 10
    +- SPK_R+ ----------> Pos 3        |           |
    +- SPK_R- ----------> Pos 4        +- SDA -----+---> Borne SDA
    |                                  +- SCL -----+---> Borne SCL
    +- AMP_EN ----------> LED5         |           |
    +- AMP_ERR ---------> LED6         +- GND -----+---> GND commun
    +- BT_CONN ---------> LED7
    |
    +- GND -------------> GND commun <-----------------+
```

---

## SELECTEUR 12 POSITIONS

### Table de routage

| Position | Signal | Source | Usage |
|----------|--------|--------|-------|
| 1 | SPK_L+ | C1 | Sortie HP gauche + |
| 2 | SPK_L- | C1 | Sortie HP gauche - |
| 3 | SPK_R+ | C1 | Sortie HP droit + |
| 4 | SPK_R- | C1 | Sortie HP droit - |
| 5 | DAC_L | C2 | Sortie DAC gauche |
| 6 | DAC_R | C2 | Sortie DAC droit |
| 7 | PHONO_L | C2 | Entree phono gauche (+ESD) |
| 8 | PHONO_R | C2 | Entree phono droit (+ESD) |
| 9 | SW_OUT_L | C2 | Sortie switch audio L |
| 10 | SW_OUT_R | C2 | Sortie switch audio R |
| 11 | AUX_L | C2 | Entree AUX gauche |
| 12 | AUX_R | C2 | Entree AUX droit |

---

## PROTECTION ESD POSITION 7-8 (V1.1)

### Schema protection PHONO

```
PHONO_L/R (nappe) --> R_ESD (100 ohm) --> Selecteur pos 7/8
                           |
                           +---> TVS (PESD5V0S1BA) --> GND
```

### Justification

- Signal PHONO = 5mV typique, tres sensible
- Selecteur metallique = antenne ESD
- R 100 ohm limite courant sans attenuer signal audio
- TVS clamp a 9V, capacite 0.5pF (transparent 20kHz)

### Calcul attenuation

```
R_ESD = 100 ohm
Z_entree TDA7439 = 47k ohm
Attenuation = 100 / (100 + 47000) = 0.2% = -0.02dB (negligeable)
```

---

## INDICATEURS LED

### Configuration

| LED | Fonction | Couleur | R serie | Seuil |
|-----|----------|---------|---------|-------|
| LED1 | BATT OK | Rouge | 1k | >15V |
| LED2 | +22V OK | Vert | 1k | >15V |
| LED3 | +5V OK | Vert | 680 ohm | >4V |
| LED4 | +3V3 OK | Vert | 470 ohm | >2.5V |
| LED5 | AMP_EN | Vert | 1k | Logic HIGH |
| LED6 | AMP_ERR | Rouge | 1k | Logic HIGH (erreur!) |
| LED7 | BT_CONN | Bleu | 1k | Logic HIGH |

### Schema LEDs

```
LED1-2 (haute tension):
+BATT/+22V --> R (1k 0.5W) --> LED --> Zener 12V --> GND
                                  (protection LED)

LED3-7 (basse tension):
Signal --> R --> LED --> GND
```

---

## BOM BREAKOUT BOX V1.2

### Connecteurs

| Composant | Qte | Usage |
|-----------|-----|-------|
| IDC-16 femelle | 1 | Nappe Carte 1 |
| IDC-20 femelle | 1 | Nappe Carte 2 |
| Borne banane 4mm rouge | 3 | BATT, +5V, +22V |
| Borne banane 4mm orange | 2 | +22V, +3V3 |
| Borne banane 4mm noir | 1 | GND |
| Borne banane 4mm jaune | 2 | SDA, SCL |
| BNC femelle | 1 | Sortie selecteur |
| Selecteur SP12T | 1 | Selection signal |

### LEDs et resistances

| Composant | Qte | Usage |
|-----------|-----|-------|
| LED 3mm verte | 5 | Status OK |
| LED 3mm rouge | 2 | BATT, ERR |
| LED 3mm bleue | 1 | BT |
| Resistance 470 ohm | 1 | LED 3V3 |
| Resistance 680 ohm | 1 | LED 5V |
| Resistance 1k | 7 | Autres LEDs |
| Zener 12V 500mW | 2 | Protection LED HT |

### Protection V1.1

| Composant | Qte | Usage |
|-----------|-----|-------|
| Resistance 100 ohm | 2 | Serie PHONO |
| TVS PESD5V0S1BA | 2 | ESD PHONO |

### Divers

| Composant | Qte | Usage |
|-----------|-----|-------|
| Boitier Hammond 1591XX | 1 | Enclosure |
| Nappe IDC 16 fils 20cm | 1 | Carte 1 |
| Nappe IDC 20 fils 20cm | 1 | Carte 2 |
| Entretoise M3x10 | 4 | Fixation PCB |
| Vis M3x6 | 8 | Assemblage |
| **Sticker avertissement** | **1** | **Securite boucle masse V1.2** |

---

## PROCEDURE TEST SECURISEE V1.2

### !!! Checklist Pre-Test OBLIGATOIRE !!!

```
AVANT CHAQUE SESSION DE TEST:

[ ] 1. VERIFIER CONFIGURATION ALIMENTATION
     [ ] Ampli sur batterie (chargeur DEBRANCHE)
     [ ] OU oscillo sur batterie/isole
     
[ ] 2. VERIFIER CONNEXIONS PC
     [ ] Si oscillo secteur: USB PC DEBRANCHE de l'ampli
     [ ] OU utiliser hub USB isole
     
[ ] 3. VERIFIER OSCILLO
     [ ] Sonde attenuee x10 pour signaux >5V
     [ ] Couplage AC pour audio
     
[ ] 4. STICKER VISIBLE
     [ ] Relire avertissement sur boitier

!!! SI UN SEUL [ ] NON COCHE --> NE PAS TESTER !!!
```

### Test 1 - Alimentations (2 min)

1. **Verifier configuration sure** (checklist ci-dessus)
2. Brancher nappes C1 et C2
3. Alimenter ampli (BATTERIE SEULE)
4. Verifier LEDs :
   - LED1 (BATT) : OK Rouge
   - LED2 (+22V) : OK Vert
   - LED3 (+5V) : OK Vert
   - LED4 (+3V3) : OK Vert

### Test 2 - Tensions (3 min)

Multimetre sur bornes banane :

| Borne | Attendu | Tolerance |
|-------|---------|-----------|
| BATT | 18-25.2V | Selon charge |
| +22V | 18-25.2V | -Vf_D1 |
| +5V | 5.0V | +/-5% |
| +3V3 | 3.3V | +/-5% |

### Test 3 - Signaux audio (5 min)

1. Injecter signal test 1kHz sur source
2. Oscillo sur BNC, selecteur position 9 (SW_OUT_L)
3. Verifier signal propre, niveau correct
4. Basculer sur position 7 (PHONO) : signal ~5mV

### Test 4 - I2C (2 min)

1. Oscillo sur SDA (borne jaune)
2. Verifier activite I2C (bursts periodiques)
3. Frequence : 100kHz ou 400kHz

### Test 5 - Ampli (3 min)

1. LED5 (AMP OK) : OK Vert quand ampli enabled
2. LED6 (AMP ERR) : Eteinte (pas d'erreur)
3. Oscillo position 1 (SPK_L+) : PWM ~400kHz

---

## DEPANNAGE

| Symptome | Cause probable | Action |
|----------|----------------|--------|
| Toutes LEDs eteintes | Pas d'alimentation | Verifier nappe C1 |
| LED BATT OK mais +22V eteinte | Fusible F1 grille | Remplacer fusible |
| LED +5V eteinte | Buck defaillant | Verifier MP1584 |
| Pas de signal position 7 | Protection ESD mal soudee | Verifier R 100 ohm |
| Signal attenue position 7 | R serie trop haute | Verifier valeur 100 ohm |
| Composants chauds inexpliques | **BOUCLE DE MASSE** | **Debrancher chargeur!** |
| Bruit 50Hz sur oscillo | Boucle de masse | Utiliser sonde differentielle |
| ESP32 reset aleatoire | Courant boucle masse | Isoler ou batterie seule |

---

## HISTORIQUE VERSIONS

| Version | Date | Modifications |
|---------|------|---------------|
| **V1.2** | 14/12/2025 | **Avertissement boucle de masse, sticker securite, checklist** |
| V1.1 | 13/12/2025 | Protection ESD position 7 (PHONO) |
| V1.0 | 12/12/2025 | Version initiale |

---

## RESUME SECURITE V1.2

### Configurations de Test

| Configuration | Securite | Note |
|---------------|----------|------|
| Ampli batterie + Oscillo secteur | OK SUR | Recommandee |
| Ampli batterie + Oscillo USB | OK SUR | OK |
| Ampli chargeur + Oscillo isole | OK SUR | Sonde differentielle |
| Ampli chargeur + Oscillo secteur | DANGER | **INTERDIT** |
| Ampli + PC USB + Oscillo secteur | DANGER | **INTERDIT** |

### Actions si Boucle de Masse Suspectee

1. **ARRETER IMMEDIATEMENT** le test
2. Debrancher chargeur ampli
3. Debrancher USB PC (si applicable)
4. Verifier temperature composants
5. Attendre 5 min avant reprise
6. Reprendre avec configuration sure

---

# FIN DOCUMENT BREAKOUT BOX V1.2
