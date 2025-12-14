# Hardware

<p>
  <img src="https://img.shields.io/badge/PCB-Dual_Board-purple?style=flat-square" />
  <img src="https://img.shields.io/badge/Amp-MA12070_Class--D-red?style=flat-square" />
  <img src="https://img.shields.io/badge/Battery-6S_LiPo-yellow?style=flat-square" />
  <img src="https://img.shields.io/badge/Protection-5_Levels-green?style=flat-square" />
</p>

Dual-PCB design separating power and signal paths. Star grounding, proper decoupling, shielded interconnects.

---

## PCB Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     CARD 2 - SIGNAL                             │
│                     80 x 120 mm                                 │
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │
│  │ ESP32-S3│  │ BTM525  │  │PCM5102A │  │ TDA7439 │  │OPA2134│ │
│  │   MCU   │  │BT LDAC  │  │  DAC    │  │   EQ    │  │Buffer │ │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └───────┘ │
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌──────────────────────────────────┐ │
│  │ CD4053  │  │ OPA2134 │  │  OLED  │  Encoder  │  IR RX     │ │
│  │  MUX    │  │  RIAA   │  │ 128x64 │   + SW    │  TSOP4838  │ │
│  └─────────┘  └─────────┘  └──────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────┬───────────────────────────────────┘
                              │
                        JST XH-16
                     100mm ribbon AWG24
                   (shielded, PTC protected)
                              │
┌─────────────────────────────┴───────────────────────────────────┐
│                     CARD 1 - POWER                              │
│                     80 x 100 mm                                 │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  BMS   │  TCO  │  RELAY  │  NTC   │  FUSE  │  D1   │ D2 │   │
│  │JBD 6S  │ 72C   │  HF46F  │ 5ohm   │  5A    │ SS54  │TVS │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │  LM7812  │  │  LM7809  │  │  MP1584  │  │ AMS1117  │        │
│  │   +12V   │  │   +9V    │  │   +5V    │  │  +3V3    │        │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘        │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                       MA12070                           │   │
│  │                      Class-D                            │   │
│  │                   2 x 20W @ 8ohm                        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  [ HP L+ ]  [ HP L- ]  [ HP R+ ]  [ HP R- ]   Screw terminals  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Protection Chain

Each level is independent. If one fails, the next catches it.

```
         N1              N2              N3            N3bis           N4              N5
     ┌────────┐      ┌────────┐      ┌────────┐      ┌────────┐      ┌────────┐      ┌────────┐
     │  BMS   │      │  TCO   │      │ RELAY  │      │  NTC   │      │ FUSE   │      │  TVS   │
PACK─┤JBD 6S  ├──────┤ 72C    ├──────┤ HF46F  ├──────┤ 5 ohm  ├──────┤  5A    ├──────┤SMBJ24CA├──> +22V
     │ 20A    │      │ 10A    │      │  NO    │      │inrush  │      │fast-bl │      │ 24V    │
     └────────┘      └────────┘      └────────┘      └────────┘      └────────┘      └────────┘
         │               │               │               │               │               │
         v               v               v               v               v               v
     Cell V/I        PCB temp        Software       500A → 5A        Overcurr.       Surge
     Balance         > 72C           SAFE_EN        limiting         > 5A            > 24V
     Temp prot      (auto-reset)    (opto-iso)    (auto-reset)      (replace)       (clamp)
```

### N1 - BMS

```
Pack B-  ───────────────────────────> BMS B- (blue wire)
Pack B1 ─┐
Pack B2 ─┤
Pack B3 ─┼─ JST XH-7P ──────────────> BMS Balance
Pack B4 ─┤
Pack B5 ─┤
Pack B6 ─┘
NTC 10k ─── JST PH-2P ──────────────> BMS T

BMS C- (black) ─────────────────────> System GND
BMS P+ (red) ───────────────────────> +BATT_PROT
```

Thresholds: OVP 4.25V/cell, UVP 2.8V/cell, OCP 25A, SCP <100us, OTP 60C

### N3 - Relay Driver (Opto-isolated)

```
+3V3 ──── R (1k) ──── PC817 LED+ 
                          │
GPIO42 (SAFE_EN) ─────────┘ LED-

PC817 Collector ──── R (10k) ──── +BATT_TCO
         │
         └─── RELAY_CTRL ──── R (10k) ──── GND
                   │
                   └─── Si2302 Gate

Si2302 Drain ──── K1 Coil-
Si2302 Source ─── GND
+BATT_TCO ─────── K1 Coil+
```

### N3bis - NTC Inrush Limiter (V1.9)

```
Problem: C_BULK (220uF) charging → I_peak = V/ESR = 25V/0.05Ω = 500A!

Solution: NTC thermistor in series
          - Cold: 5 ohm → I_peak = 25V/5Ω = 5A
          - Hot:  0.5 ohm → minimal loss

Component: Ametherm SL10 5R005
           5 ohm @ 25C
           Max 5A continuous
           Warm-up time ~200ms
```

---

## Power Rails

```
+BATT (18-25.2V)
    │
    └──> [Protection Chain] ──> +22V_RAW
                                    │
                    ┌───────────────┼───────────────┬───────────────┐
                    │               │               │               │
                    v               v               v               v
               ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
               │ LM7812  │    │ LM7809  │    │ MP1584  │    │ 1N5822  │
               │ TO-220  │    │ TO-220  │    │  Buck   │    │  D3     │
               └────┬────┘    └────┬────┘    └────┬────┘    └────┬────┘
                    │              │              │              │
                    v              v              v              v
                +12V_PRE       +9V_BUFFER      +5V          +PVDD_SAFE
                    │              │              │          (MA12070)
                    v              │              v
               ┌─────────┐         │         ┌─────────┐
               │MCP1703A │         │         │AMS1117  │
               │ LDO 5V  │         │         │LDO 3V3  │
               └────┬────┘         │         └────┬────┘
                    │              │              │
                    v              v              v
               +5V_ANALOG     +9V_BUFFER       +3V3
              (RIAA, TDA)    (OPA2134 buf)   (ESP32, logic)
```

### Why +9V Buffer? (V1.9)

```
Problem:  OPA2134 powered from +5V
          Output swing = 5V - 1.5V = 3.5V peak
          TDA7439 output = 2Vrms = 2.83V peak
          Headroom = 3.5 - 2.83 = 0.67V → CLIPPING RISK!

Solution: LM7809 → +9V_BUFFER
          Output swing = 9V - 1.5V = 7.5V peak
          Headroom = 7.5 - 2.83 = 4.67V → Safe
```

---

## Ribbon Cable

16-pin JST XH, 100mm, AWG24, shielded.

```
Pin  Signal       Direction   Note
───────────────────────────────────────────────
1    22V_SENSE    C1 → C2     Via divider 220k/33k
2    +5V          C1 → C2     Through PTC1 (750mA)
3    +3V3         C1 → C2     Through PTC2 (500mA)
4    GND_PWR      —           Power ground
5    GND_SIG      —           Signal ground
6    GND_SHIELD   —           Cable shield
7    AUDIO_L      C2 → C1     Left channel
8    GND_SHIELD   —           Shield between L/R
9    AUDIO_R      C2 → C1     Right channel
10   GND_SHIELD   —           Shield after R
11   SDA          ↔           I2C data
12   SCL          C2 → C1     I2C clock
13   AMP_EN       C2 → C1     Amplifier enable
14   AMP_MUTE     C2 → C1     Mute control
15   AMP_ERR      C1 → C2     Error flag
16   SAFE_EN      C2 → C1     Relay control
```

### PTC Protection (V1.9)

```
+5V  ──── PTC1 (MF-R075) ──── NAPPE_5V
          I_hold = 750mA
          I_trip = 1.5A
          R = 0.3 ohm

+3V3 ──── PTC2 (MF-R050) ──── NAPPE_3V3
          I_hold = 500mA
          I_trip = 1.0A
          R = 0.5 ohm

Normal load Card 2: ~230mA (ESP32 + BT + DAC + EQ + OLED)
Trip current: Short on Card 2 → PTC opens → Card 1 protected
Auto-resettable after cooling.
```

---

## Audio Signal Path

```
                                                 ┌──────────────┐
BTM525 I2S ──────────────────────────> PCM5102A ─┤              │
                                          DAC    │              │
                                                 │    CD4053    │──> TDA7439 ──> OPA2134 ──> AUDIO OUT
AUX IN ──────────────────────────────────────────┤     MUX      │       EQ        Buffer
  3.5mm                                          │              │
                                                 │              │
PHONO IN ──> OPA2134 RIAA ───────────────────────┤              │
  RCA         Preamp                             └──────────────┘
               40dB

RIAA Network:
R1 = 750 ohm ─── C1 = 3.3nF   (pole @ 2122Hz)
R2 = 75k                       (zero @ 50Hz)
C2 = 100pF // R2               (pole @ 21.2kHz)

VREF = +5V_ANALOG / 2 = 2.5V
VREF decoupling: C_REF 10uF film (V1.9)
```

---

## MA12070 Class-D

```
+PVDD_SAFE ──┬── C (220uF 35V electro) ──┬── MA12070 PVDD
             │                           │
             └── C (10uF ceramic) ───────┘

MA12070 OUT_L+ ──── L (10uH) ──── HP_L+
MA12070 OUT_L- ──── L (10uH) ──── HP_L-
MA12070 OUT_R+ ──── L (10uH) ──── HP_R+
MA12070 OUT_R- ──── L (10uH) ──── HP_R-

I2C Address: 0x20
Max PVDD: 26V (protected by D3 1N5822, Vf = 0.9V)
```

---

## Grounding

Star ground topology. All grounds meet at one point on C_BULK.

```
                         ┌─────────────────┐
                         │     C_BULK      │
                         │    220uF 35V    │
                         └────────┬────────┘
                                  │
                    ══════════════╪══════════════  STAR POINT
                    │      │      │      │      │
                    v      v      v      v      v
                  PVDD   +22V   +5V    +3V3   Signal
                  GND    GND    GND    GND    GND
                   │      │      │      │      │
                   └──────┴──────┴──────┴──────┘
                              │
                              v
                           CHASSIS
```

---

## Thermal

| Component | P_max | Rth_ja | Tj @ 40C amb | Dissipator |
|-----------|-------|--------|--------------|------------|
| MA12070 | 2W | 25 C/W | 90C | PCB copper pour |
| LM7812 | 0.32W | 50 C/W | 56C | TO-220 bare OK |
| LM7809 | 0.20W | 50 C/W | 50C | TO-220 bare OK |
| MCP1703 | 0.18W | 180 C/W | 72C | TO-92 bare OK |

---

## BOM Highlights

| Component | Part Number | Note |
|-----------|-------------|------|
| Amp IC | MA12070P | QFN-48, Class-D |
| MCU | ESP32-S3-WROOM-1-N8R8 | 8MB Flash, 8MB PSRAM |
| Bluetooth | BTM525 | LDAC support |
| DAC | PCM5102A | 32-bit, 112dB SNR |
| EQ | TDA7439 | 3-band, analog |
| Op-Amp | OPA2134PA | Audio-grade, DIP-8 |
| Schottky | SS54 | 40V 5A, anti-reverse |
| TVS | SMBJ24CA | 24V bidirectional |
| Inrush NTC | SL10 5R005 | 5 ohm @ 25C |
| PTC | MF-R075 / MF-R050 | Ribbon protection |
| BMS | JBD SP22S003B | 6S 20A |

Full BOM: [`Ampli_Audiophile_Portable_V1_9.md`](./Ampli_Audiophile_Portable_V1_9.md)

---

## Layout Tips

1. **Star ground** on C_BULK negative terminal
2. **Power traces** minimum 2mm width (3A capability)
3. **Shield audio traces** with ground pours
4. **Separate analog/digital** ground planes, join at star
5. **100nF ceramic** on every IC, close to VCC/GND pins
6. **Keep I2C short** and away from power switching

---

## Test Jig

Breakout box with banana jacks and LED indicators for all test points.

See [`Breakout_Box_Test_V1_2.md`](./Breakout_Box_Test_V1_2.md)

**WARNING**: Ground loop risk when testing!
- OK: Amp on battery + scope on mains
- DANGER: Amp on charger + scope on mains + USB

---

## Changelog

| Version | Changes |
|---------|---------|
| **1.9** | PTC ribbon protection, NTC inrush, +9V buffer rail, C_REF decoupling |
| 1.8 | NTC temperature monitoring |
| 1.7 | LM7812 pre-regulator, 1N5822 PVDD diode |
| 1.6 | Star ground implementation |
| 1.5 | TVS protection, shielded ribbon |

---

<p align="center">
  <sub>Designed for veroboard or custom PCB. Works on both.</sub>
</p>
