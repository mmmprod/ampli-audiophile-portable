# BREAKOUT BOX TEST V1.0

## AMPLIFICATEUR AUDIOPHILE PORTABLE — OUTIL DE TEST

**Version :** 1.0  
**Date :** 12 décembre 2025  
**Auteur :** Mehdi + Claude  
**Compatible avec :** Ampli Audiophile V1.3+

---

## OBJECTIF

Réduire le temps de test de **2 heures à 15 minutes** en :
- Centralisant tous les points de mesure sur des bornes banane accessibles
- Affichant l'état des alimentations par LEDs
- Permettant de brancher multimètre/oscillo une seule fois

---

## CONCEPT

```
┌─────────────────┐     Nappe 20cm      ┌─────────────────────┐
│   CARTE 1       │ =================== │                     │
│   (Puissance)   │    16 fils          │   BREAKOUT BOX      │
│  J_TEST_C1      │                     │                     │
└─────────────────┘                     │  • Bornes banane    │
                                        │  • LEDs status      │
┌─────────────────┐     Nappe 20cm      │  • Sélecteur probe  │
│   CARTE 2       │ =================== │                     │
│   (Signal)      │    20 fils          │                     │
│  J_TEST_C2      │                     └─────────────────────┘
└─────────────────┘                            ↑
                                    Multimètre/Oscillo branchés UNE FOIS
```

---

## SPECIFICATIONS BOÎTIER

### Boîtier

**Référence :** Hammond 1591XXCBK (ou équivalent)  
**Dimensions :** 150 × 100 × 50 mm  
**Matériau :** ABS noir  
**Fournisseur :** TME, Mouser (~8€)

### Perçages face avant

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│                     BREAKOUT BOX TEST V1.0                               │
│                     Ampli Audiophile                                     │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ═══ ALIMENTATIONS ═══          ═══ SIGNAUX AUDIO ═══                   │
│                                                                          │
│   ┌───┐  ┌───┐  ┌───┐  ┌───┐     ┌───┐  ┌───┐  ┌───┐  ┌───┐           │
│   │ ● │  │ ● │  │ ● │  │ ● │     │ ● │  │ ● │  │ ● │  │ ● │           │
│   └───┘  └───┘  └───┘  └───┘     └───┘  └───┘  └───┘  └───┘           │
│   BATT   +22V   +5V   +3V3       DAC_L  DAC_R  VOL_L  VOL_R            │
│   Rouge  Orange Rouge Orange     Blanc  Gris   Blanc  Gris             │
│                                                                          │
│  ═══ STATUS LEDs ═══            ═══ LOGIQUE/I2C ═══                     │
│                                                                          │
│   ○ BATT OK    (vert)            ┌───┐  ┌───┐  ┌───┐                   │
│   ○ +22V OK    (vert)            │ ● │  │ ● │  │ ● │                   │
│   ○ +5V OK     (vert)            └───┘  └───┘  └───┘                   │
│   ○ +3V3 OK    (vert)             SDA    SCL    ERR                     │
│   ○ AMP OK     (vert)            Jaune  Jaune  Rouge                    │
│   ○ AMP ERR    (rouge)                                                   │
│   ○ BT CONN    (bleu)           ═══ ACTIVE PROBE ═══                    │
│                                                                          │
│   ┌───┐                          ┌─────────────────┐                    │
│   │ ● │  GND                     │   ○   BNC/4mm   │                    │
│   └───┘  Noir                    └─────────────────┘                    │
│                                   Sélecteur 12 pos                       │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   NAPPE C1 ═══════════════        NAPPE C2 ═══════════════════          │
│            [Connecteur IDC]                [Connecteur IDC]              │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### Dimensions perçages

| Élément | Diamètre | Quantité |
|---------|----------|----------|
| Borne banane 4mm | Ø6mm | 15 |
| LED 3mm | Ø3mm | 7 |
| BNC femelle (option) | Ø10mm | 1 |
| Sélecteur rotatif | Ø7mm + méplat | 1 |
| Connecteur IDC | Fente 25×8mm | 2 |

---

## SCHÉMA INTERNE COMPLET

### Alimentation breakout box

La breakout box n'a PAS besoin d'alimentation externe.  
Les LEDs sont alimentées par les tensions testées elles-mêmes.

### Câblage nappe C1 (16 pins) → Face avant

```
NAPPE C1 (J_TEST_C1)                              FACE AVANT
       │
       │
Pin 1 (+BATT_BMS) ────┬───────────────────► Borne banane ROUGE "BATT"
                      │
                      └──► R1 (10kΩ) ──► LED1 (verte) ──► GND_INT
                           (protection haute tension)
                           
Pin 2 (+BATT_TCO) ────────────────────────► (non sorti, test interne)

Pin 3 (+BATT_PROT) ───────────────────────► (non sorti, test interne)

Pin 4 (+BATT_FUSE) ───────────────────────► (non sorti, test interne)

Pin 5 (+22V_RAW) ─────┬───────────────────► Borne banane ORANGE "+22V"
                      │
                      └──► R2 (10kΩ) ──► LED2 (verte) ──► GND_INT

Pin 6 (+22V_FILT) ────────────────────────► (parallèle Pin 5, même borne)

Pin 7 (+5V) ──────────┬───────────────────► Borne banane ROUGE "+5V"
                      │
                      └──► R3 (1kΩ) ──► LED3 (verte) ──► GND_INT

Pin 8 (+3V3) ─────────┬───────────────────► Borne banane ORANGE "+3V3"
                      │
                      └──► R4 (470Ω) ──► LED4 (verte) ──► GND_INT

Pin 9 (GND_PWR) ──────┬───────────────────► Borne banane NOIR "GND"
                      │
                      └───────────────────► GND_INT (masse interne box)

Pin 10 (GND_SIG) ─────────────────────────► (parallèle Pin 9)

Pin 11 (AMP_IN_L) ────────────────────────► Borne banane BLANC "AMP_L"

Pin 12 (AMP_IN_R) ────────────────────────► Borne banane GRIS "AMP_R"

Pin 13 (SPK_L+) ──────────────────────────► Sélecteur position 1

Pin 14 (SPK_L-) ──────────────────────────► Sélecteur position 2

Pin 15 (AMP_ERR) ─────┬───────────────────► Borne banane ROUGE "ERR"
                      │
                      └──► Via inverseur:
                           AMP_ERR = HIGH (OK) → LED5 (verte) ON
                           AMP_ERR = LOW (erreur) → LED6 (rouge) ON

Pin 16 (SAFE_EN) ─────────────────────────► Sélecteur position 3
```

### Câblage nappe C2 (20 pins) → Face avant

```
NAPPE C2 (J_TEST_C2)                              FACE AVANT
       │
       │
Pin 1 (+5V) ──────────────────────────────► (parallèle C1 Pin 7)

Pin 2 (+3V3) ─────────────────────────────► (parallèle C1 Pin 8)

Pin 3 (GND_SIG) ──────────────────────────► (parallèle C1 Pin 9)

Pin 4 (I2S_BCLK) ─────────────────────────► Sélecteur position 4

Pin 5 (I2S_LRCK) ─────────────────────────► Sélecteur position 5

Pin 6 (I2S_DATA) ─────────────────────────► Sélecteur position 6

Pin 7 (DAC_OUT_L) ────────────────────────► Borne banane BLANC "DAC_L"

Pin 8 (DAC_OUT_R) ────────────────────────► Borne banane GRIS "DAC_R"

Pin 9 (PHONO_IN_L) ───────────────────────► Sélecteur position 7

Pin 10 (PHONO_OUT_L) ─────────────────────► Sélecteur position 8

Pin 11 (SW_OUT_L) ────────────────────────► Sélecteur position 9

Pin 12 (SW_OUT_R) ────────────────────────► Sélecteur position 10

Pin 13 (VOL_OUT_L) ───────────────────────► Borne banane BLANC "VOL_L"

Pin 14 (VOL_OUT_R) ───────────────────────► Borne banane GRIS "VOL_R"

Pin 15 (SDA) ─────────────────────────────► Borne banane JAUNE "SDA"

Pin 16 (SCL) ─────────────────────────────► Borne banane JAUNE "SCL"

Pin 17 (BT_STATUS) ───┬───────────────────► Sélecteur position 11
                      │
                      └──► R5 (1kΩ) ──► LED7 (bleue) ──► GND_INT

Pin 18 (AMP_EN) ──────────────────────────► Sélecteur position 12
                      │
                      └──► (intégré à LED5 "AMP OK")

Pin 19 (AMP_MUTE) ────────────────────────► (non sorti)

Pin 20 (ENC_SW) ──────────────────────────► (non sorti)
```

---

## CIRCUIT LEDs STATUS

### LED1 — BATT OK (verte)

Indique présence tension batterie (18-25V)

```
+BATT (18-25V) ──► R1 (10kΩ) ──► LED1 (verte) ──► GND_INT

I_LED = (18V - 2V) / 10kΩ = 1,6mA (faible mais visible)
I_LED = (25V - 2V) / 10kΩ = 2,3mA

Note: 10kΩ pour limiter le courant avec haute tension
```

### LED2 — +22V OK (verte)

```
+22V ──► R2 (10kΩ) ──► LED2 (verte) ──► GND_INT

I_LED = (22V - 2V) / 10kΩ = 2mA
```

### LED3 — +5V OK (verte)

```
+5V ──► R3 (1kΩ) ──► LED3 (verte) ──► GND_INT

I_LED = (5V - 2V) / 1kΩ = 3mA
```

### LED4 — +3V3 OK (verte)

```
+3V3 ──► R4 (470Ω) ──► LED4 (verte) ──► GND_INT

I_LED = (3,3V - 2V) / 470Ω = 2,8mA
```

### LED5 — AMP OK (verte) + LED6 — AMP ERR (rouge)

Circuit inverseur pour afficher état ampli :

```
                                    +5V
                                     │
                                    R6 (4,7kΩ)
                                     │
AMP_ERR ───┬─────────────────────────┼──────► LED5 (verte) via R (1kΩ)
           │                         │
           │                    Q1 (BC547)
           │                    Base ──► R7 (10kΩ) ──► AMP_ERR
           │                    Émetteur ──► GND_INT
           │                    Collecteur ──► LED6 (rouge) ──► +5V via R (1kΩ)
           │
           └──► R8 (10kΩ) ──► Base Q1

Logique:
- AMP_ERR = HIGH (3V3, OK) → Q1 OFF → LED6 OFF, LED5 ON (via pull-up)
- AMP_ERR = LOW (erreur) → Q1 ON → LED6 ON, LED5 OFF
```

**Version simplifiée (sans transistor) :**

```
AMP_ERR ──► R (1kΩ) ──► LED5 (verte) ──► GND_INT

Si AMP_ERR = HIGH (3,3V) → LED5 ON (OK)
Si AMP_ERR = LOW (0V) → LED5 OFF (pas d'erreur visible mais pas OK non plus)

Pour LED6 (erreur), il faut le transistor inverseur ou accepter
qu'on voit juste "LED5 éteinte = problème"
```

### LED7 — BT CONN (bleue)

```
BT_STATUS ──► R5 (1kΩ) ──► LED7 (bleue) ──► GND_INT

Si BT connecté → BT_STATUS = HIGH → LED7 ON
Si BT déconnecté → BT_STATUS = LOW → LED7 OFF
```

---

## SÉLECTEUR ACTIVE PROBE

### Fonction

Permet de router n'importe quel signal vers une seule sortie BNC/banane pour l'oscilloscope, sans changer de câble.

### Composant

**Type :** Sélecteur rotatif 1 pôle 12 positions  
**Référence :** Lorlin CK1049 ou générique  
**Fournisseur :** TME (~2€)

### Câblage sélecteur

```
Position commune (pôle) ──────────────► BNC femelle "PROBE"
                                        (ou borne banane)

Position 1  ◄──── SPK_L+ (PWM ampli gauche)
Position 2  ◄──── SPK_L- (PWM ampli gauche)
Position 3  ◄──── SAFE_EN (commande relais)
Position 4  ◄──── I2S_BCLK (3,072MHz)
Position 5  ◄──── I2S_LRCK (48kHz)
Position 6  ◄──── I2S_DATA (signal I2S)
Position 7  ◄──── PHONO_IN_L (~5mV)
Position 8  ◄──── PHONO_OUT_L (~500mV)
Position 9  ◄──── SW_OUT_L (source sélectionnée)
Position 10 ◄──── SW_OUT_R (source sélectionnée)
Position 11 ◄──── BT_STATUS
Position 12 ◄──── AMP_EN
```

### Sérigraphie sélecteur

```
         12  1
       11      2
      10        3
        9      4
          8  5
            6 7
            
1: SPK+    7: PHO_IN
2: SPK-    8: PHO_OUT
3: SAFE    9: SW_L
4: BCLK   10: SW_R
5: LRCK   11: BT
6: I2S    12: AMP
```

---

## BOM BREAKOUT BOX

### Boîtier et mécanique

| Composant | Quantité | Référence | Prix unit |
|-----------|----------|-----------|-----------|
| Boîtier ABS 150×100×50mm | 1 | Hammond 1591XXCBK | 8€ |
| Vis M3×10 + écrous | 8 | - | 0,50€ |
| Entretoises M3×10 | 4 | - | 0,50€ |

### Connecteurs

| Composant | Quantité | Couleur | Référence | Prix unit |
|-----------|----------|---------|-----------|-----------|
| Borne banane 4mm | 3 | Rouge | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 2 | Orange | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 4 | Blanc | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 2 | Gris | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 2 | Jaune | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 1 | Rouge | Hirschmann MBI1 | 1,50€ |
| Borne banane 4mm | 1 | Noir | Hirschmann MBI1 | 1,50€ |
| BNC femelle châssis | 1 | - | Amphenol 31-10 | 2€ |
| Embase IDC 2×8 femelle | 1 | - | Standard | 0,50€ |
| Embase IDC 2×10 femelle | 1 | - | Standard | 0,60€ |

**Total bornes : 15 bornes banane + 1 BNC**

### LEDs et résistances

| Composant | Quantité | Valeur | Prix unit |
|-----------|----------|--------|-----------|
| LED 3mm verte | 5 | 20mA, 2V | 0,10€ |
| LED 3mm rouge | 1 | 20mA, 2V | 0,10€ |
| LED 3mm bleue | 1 | 20mA, 3,2V | 0,15€ |
| Résistance 10kΩ | 3 | 0,25W | 0,02€ |
| Résistance 4,7kΩ | 1 | 0,25W | 0,02€ |
| Résistance 1kΩ | 4 | 0,25W | 0,02€ |
| Résistance 470Ω | 1 | 0,25W | 0,02€ |

### Sélecteur et transistor

| Composant | Quantité | Référence | Prix unit |
|-----------|----------|-----------|-----------|
| Sélecteur rotatif 1P12T | 1 | Lorlin CK1049 | 2€ |
| Bouton sélecteur | 1 | - | 0,50€ |
| BC547 (optionnel) | 1 | NPN | 0,10€ |

### Câbles

| Composant | Quantité | Spécification | Prix unit |
|-----------|----------|---------------|-----------|
| Nappe IDC 16 conducteurs | 1 | 200mm, pas 2,54mm | 1,50€ |
| Nappe IDC 20 conducteurs | 1 | 200mm, pas 2,54mm | 2€ |
| Fil câblage interne | 2m | AWG22 multicolore | 1€ |

### Récapitulatif coût

| Catégorie | Sous-total |
|-----------|------------|
| Boîtier | 9€ |
| Connecteurs | 25€ |
| LEDs + résistances | 2€ |
| Sélecteur | 2,50€ |
| Câbles | 4,50€ |
| **TOTAL** | **~43€** |

---

## INSTRUCTIONS DE FABRICATION

### Étape 1 — Préparation boîtier

1. Imprimer gabarit de perçage (ci-dessous)
2. Coller gabarit sur face avant du boîtier
3. Pointer les centres avec pointeau
4. Percer les trous pilotes Ø2mm
5. Agrandir aux diamètres finaux

### Étape 2 — Montage connecteurs face avant

1. Installer les 15 bornes banane avec écrous
2. Installer le connecteur BNC
3. Installer le sélecteur rotatif
4. Installer les 7 LEDs avec supports

### Étape 3 — Câblage interne

1. Souder les résistances aux LEDs (côté cathode)
2. Relier toutes les cathodes LEDs à un fil commun GND_INT
3. Câbler chaque borne banane à son fil correspondant
4. Câbler le sélecteur (12 positions + commun)
5. Connecter les nappes IDC aux embases

### Étape 4 — Fermeture et test

1. Vérifier continuité de chaque ligne
2. Vérifier isolation entre lignes adjacentes
3. Fermer le boîtier
4. Étiqueter la face avant

---

## GABARIT DE PERÇAGE

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│    15mm   30mm   45mm   60mm        90mm  105mm 120mm 135mm              │
│     │      │      │      │           │      │     │     │                │
│     ▼      ▼      ▼      ▼           ▼      ▼     ▼     ▼                │
│ ─ ─ ○ ─ ─ ○ ─ ─ ○ ─ ─ ○ ─ ─ ─ ─ ─ ○ ─ ─ ○ ─ ─ ○ ─ ─ ○ ─ ─   ◄─ 15mm    │
│   BATT   +22V   +5V   +3V3       DAC_L DAC_R VOL_L VOL_R                 │
│   Ø6mm   Ø6mm   Ø6mm  Ø6mm       Ø6mm  Ø6mm  Ø6mm  Ø6mm                  │
│                                                                          │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ○ ─ ─ ○ ─ ─ ○ ─ ─ ─ ─ ─ ─   ◄─ 30mm  │
│    LED1 (BATT)                   SDA   SCL   ERR                         │
│    Ø3mm                          Ø6mm  Ø6mm  Ø6mm                        │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 38mm  │
│    LED2 (+22V)                                                           │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 46mm  │
│    LED3 (+5V)                                                            │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ○ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 54mm  │
│    LED4 (+3V3)                  PROBE (BNC)                              │
│                                  Ø10mm                                   │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ◎ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 62mm  │
│    LED5 (AMP OK)              Sélecteur                                  │
│                                  Ø7mm                                    │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 70mm  │
│    LED6 (AMP ERR)                                                        │
│                                                                          │
│ ─ ─ ● ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 78mm  │
│    LED7 (BT)                                                             │
│                                                                          │
│ ─ ─ ○ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   ◄─ 88mm  │
│    GND                                                                   │
│    Ø6mm                                                                  │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  [═══════════════]                    [═══════════════════]    ◄─ 95mm  │
│   Fente IDC 16P                        Fente IDC 20P                     │
│   25×8mm                               30×8mm                            │
│   centre @ 35mm                        centre @ 115mm                    │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
         │                                              │
         0                                            150mm
```

---

## UTILISATION

### Branchement initial

1. **Connecter les nappes** aux cartes (circuit hors tension)
   - Nappe 16P → J_TEST_C1 (Carte 1)
   - Nappe 20P → J_TEST_C2 (Carte 2)

2. **Brancher instruments** sur la breakout box
   - Multimètre COM → Borne noire GND
   - Multimètre V+ → Libre (selon test)
   - Oscillo CH1 → BNC PROBE

### Procédure de test rapide

**Phase 1 — Vérification visuelle (10 secondes)**

Mettre sous tension et observer LEDs :

| LED | État attendu | Si éteinte |
|-----|--------------|------------|
| BATT OK | Verte ON | Batterie déconnectée ou BMS coupé |
| +22V OK | Verte ON | TCO/Relais/Fusible ouvert ou D1 HS |
| +5V OK | Verte ON | Buck défaillant |
| +3V3 OK | Verte ON | LDO défaillant |
| AMP OK | Verte ON | MA12070 erreur ou non initialisé |
| AMP ERR | Rouge OFF | Si ON → Erreur thermique/surcharge |
| BT CONN | Bleue | ON si appareil appairé |

**Phase 2 — Mesures alimentations (2 minutes)**

| Borne | Multimètre sur | Attendu | Tolérance |
|-------|----------------|---------|-----------|
| BATT | DC Volts | 18-25,2V | Selon charge |
| +22V | DC Volts | ~21,5V | ±0,5V |
| +5V | DC Volts | 5,00V | ±0,1V |
| +3V3 | DC Volts | 3,30V | ±0,05V |

**Phase 3 — Mesures signaux audio (5 minutes)**

Jouer une tonalité 1kHz depuis Bluetooth ou AUX.

| Borne | Oscillo sur | Attendu |
|-------|-------------|---------|
| DAC_L | Sinus | 0,5-2Vrms selon volume source |
| DAC_R | Sinus | Identique gauche |
| VOL_L | Sinus | Atténué selon position volume |
| VOL_R | Sinus | Identique gauche |

**Phase 4 — Signaux numériques via sélecteur (5 minutes)**

| Position | Signal | Oscillo attendu |
|----------|--------|-----------------|
| 4 (BCLK) | I2S Clock | Carré 3,072MHz |
| 5 (LRCK) | I2S Word | Carré 48kHz |
| 6 (I2S) | I2S Data | Train bits |
| 7 (PHO_IN) | Phono | ~5mV (très faible) |
| 8 (PHO_OUT) | Phono amplifié | ~500mV |

**Phase 5 — Vérification I2C (2 minutes)**

Avec analyseur logique ou oscillo sur SDA/SCL :
- Vérifier présence de trames I2C
- Adresses attendues : 0x20 (MA12070), 0x3C (OLED)

---

## COMPARAISON TEMPS DE TEST

### Sans breakout box (méthode manuelle)

| Étape | Temps |
|-------|-------|
| Localiser chaque pad sur PCB | 30s × 28 = 14 min |
| Positionner pointe de touche | 15s × 28 = 7 min |
| Mesurer et noter | 30s × 28 = 14 min |
| Changer point oscillo | 1 min × 10 = 10 min |
| Erreurs de repositionnement | ~15 min |
| **TOTAL** | **~60 min par carte = 2h total** |

### Avec breakout box

| Étape | Temps |
|-------|-------|
| Brancher 2 nappes | 30 sec |
| Vérifier 7 LEDs status | 10 sec |
| Mesurer 4 alimentations | 2 min |
| Mesurer 4 signaux audio | 4 min |
| Vérifier I2C | 2 min |
| Parcourir sélecteur (12 pos) | 6 min |
| **TOTAL** | **~15 min total** |

### Gain

**Réduction : 85-90% du temps de test**
**Réduction erreurs : Pas de recherche de pads, pas de glissement pointe**

---

## ÉVOLUTIONS POSSIBLES (V2)

- **Afficheur LCD** : Mesure tensions en direct (ADC Arduino)
- **Relais de test** : Injection signaux automatisée
- **USB** : Log des mesures sur PC
- **Auto-test** : Séquence GO/NO-GO automatique

---

## ANNEXE — CORRESPONDANCE PINS

### Nappe C1 (J_TEST_C1) — 16 pins

| Pin | Fil nappe | Signal | Borne/LED |
|-----|-----------|--------|-----------|
| 1 | Rouge | +BATT_BMS | Borne BATT + LED1 |
| 2 | Orange | +BATT_TCO | (interne) |
| 3 | Jaune | +BATT_PROT | (interne) |
| 4 | Vert | +BATT_FUSE | (interne) |
| 5 | Bleu | +22V_RAW | Borne +22V + LED2 |
| 6 | Violet | +22V_FILT | (// pin5) |
| 7 | Gris | +5V | Borne +5V + LED3 |
| 8 | Blanc | +3V3 | Borne +3V3 + LED4 |
| 9 | Noir | GND_PWR | Borne GND |
| 10 | Marron | GND_SIG | (// pin9) |
| 11 | Rouge/Blanc | AMP_IN_L | Borne AMP_L |
| 12 | Orange/Blanc | AMP_IN_R | Borne AMP_R |
| 13 | Jaune/Blanc | SPK_L+ | Sélecteur pos1 |
| 14 | Vert/Blanc | SPK_L- | Sélecteur pos2 |
| 15 | Bleu/Blanc | AMP_ERR | Borne ERR + LED5/6 |
| 16 | Violet/Blanc | SAFE_EN | Sélecteur pos3 |

### Nappe C2 (J_TEST_C2) — 20 pins

| Pin | Fil nappe | Signal | Borne/LED |
|-----|-----------|--------|-----------|
| 1 | Rouge | +5V | (// C1 pin7) |
| 2 | Orange | +3V3 | (// C1 pin8) |
| 3 | Jaune | GND_SIG | (// C1 pin9) |
| 4 | Vert | I2S_BCLK | Sélecteur pos4 |
| 5 | Bleu | I2S_LRCK | Sélecteur pos5 |
| 6 | Violet | I2S_DATA | Sélecteur pos6 |
| 7 | Gris | DAC_OUT_L | Borne DAC_L |
| 8 | Blanc | DAC_OUT_R | Borne DAC_R |
| 9 | Noir | PHONO_IN_L | Sélecteur pos7 |
| 10 | Marron | PHONO_OUT_L | Sélecteur pos8 |
| 11 | Rouge/Blanc | SW_OUT_L | Sélecteur pos9 |
| 12 | Orange/Blanc | SW_OUT_R | Sélecteur pos10 |
| 13 | Jaune/Blanc | VOL_OUT_L | Borne VOL_L |
| 14 | Vert/Blanc | VOL_OUT_R | Borne VOL_R |
| 15 | Bleu/Blanc | SDA | Borne SDA |
| 16 | Violet/Blanc | SCL | Borne SCL |
| 17 | Gris/Blanc | BT_STATUS | Sélecteur pos11 + LED7 |
| 18 | Blanc/Noir | AMP_EN | Sélecteur pos12 |
| 19 | Noir/Blanc | AMP_MUTE | (non sorti) |
| 20 | Marron/Blanc | ENC_SW | (non sorti) |

---

# ═══════════════════════════════════════════════════════════════════
# FIN DOCUMENT BREAKOUT BOX V1.0
# ═══════════════════════════════════════════════════════════════════
