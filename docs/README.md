# 🔧 Hardware Documentation V1.10

Technical hardware documentation for the Portable Audiophile Amplifier.

[![Hardware](https://img.shields.io/badge/Hardware-v1.10-blue)](docs/Hardware_V1_10.md)
[![Cards](https://img.shields.io/badge/Cards-2--PCB-orange)]()
[![Protection](https://img.shields.io/badge/Protection-5--Level-red)]()

---

## 🏗️ Dual-PCB Architecture

### Card 1 — Power (80×100mm)

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────────────┐  │
│  │   BMS   │  │   TCO   │  │  RELAY  │  │    MA12070    │  │
│  │  6S 20A │  │   72°C  │  │  HF46F  │  │    Class-D    │  │
│  └────┬────┘  └────┬────┘  └────┬────┘  └───────┬───────┘  │
│       │            │            │               │          │
│  ┌────┴────────────┴────────────┴───────────────┴──────┐   │
│  │                   ⭐ STAR GROUND                     │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ LM7812  │  │ LM7809  │  │ MP1584  │  │MCP1703A │        │
│  │  +12V   │  │  +9V    │  │  +5V    │  │+5V_ANA  │        │
│  │[COPPER] │  │[Buffer] │  │         │  │         │        │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘        │
│                                                             │
│              [NTC Inrush 5Ω]  [PTC 750mA/500mA]            │
└──────────────────────────┬──────────────────────────────────┘
                           │
                    🔗 Molex Micro-Fit 16P
                           │
┌──────────────────────────┴──────────────────────────────────┐
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ ESP32-S3 │  │  BTM525  │  │ PCM5102A │  │  CD4053  │    │
│  │   MCU    │  │ Bluetooth│  │   DAC    │  │   MUX    │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌────────────────────────┐    │
│  │  TDA7439 │  │ OPA2134  │  │    Level Shifter       │    │
│  │    EQ    │  │  Buffer  │  │   BSS138 x2 [V1.10]    │    │
│  └──────────┘  └──────────┘  └────────────────────────┘    │
│                                                             │
│                 Card 2 — Signal (80×120mm)                  │
└─────────────────────────────────────────────────────────────┘
```

---

## 🛡️ 5-Level Protection Chain

```
+PACK ──┬─────────────────────────────────────────────────────> +22V_RAW
        │
        ▼
  ┌───────────┐
  │ 🔋 N1:BMS │  JBD SP22S003B
  │   6S 20A  │  • OVP: 4.25V/cell
  │           │  • UVP: 2.8V/cell
  │           │  • OCP: 25A, SCP: <100µs
  └─────┬─────┘
        ▼
  ┌───────────┐
  │ 🌡️ N2:TCO │  Aupo A4-1A-F
  │    72°C   │  • Auto-reset thermal cutoff
  └─────┬─────┘
        ▼
  ┌───────────┐
  │ 🔌 N3:RLY │  HF46F-G/12-HS1
  │    10A    │  • Software controlled
  └─────┬─────┘
        ▼
  ┌───────────┐
  │ 🔥 N3b:NTC│  Ametherm SL10 5R005 [V1.9]
  │  Inrush   │  • R_cold: 5Ω → I_peak: 5A
  │  Limiter  │  • R_hot: 0.2Ω
  └─────┬─────┘
        ▼
  ┌───────────┐
  │ ⚡ N4:FUSE│  5A ATO Fast-blow
  └─────┬─────┘
        ▼
  ┌───────────┐
  │ 🛡️ N5:TVS │  SMBJ24CA 600W
  │           │  • Clamp: 38V @ 15A
  └─────┬─────┘
        ▼
     +22V_RAW
```

### 🔥 NTC Inrush Limiter [V1.9]

| Without NTC | With NTC |
|-------------|----------|
| C_BULK = 220µF, ESR = 40mΩ | NTC 5Ω @ 25°C in series |
| I_peak = 25V / 0.04Ω = **625A** 💀 | I_peak = 25V / 5Ω = **5A** ✅ |
| Welded relay contacts, melted traces | Safe progressive charge |

---

## 🔌 I2C Level Shifter [V1.10]

### ❌ The Problem

```
TDA7439 @ 9V:  V_IH = 0.7 × 9V = 6.3V minimum for HIGH
ESP32 @ 3.3V:  V_OH = 3.3V maximum

3.3V < 6.3V → TDA7439 NEVER sees HIGH!
             → I2C completely dead! 💀
```

### ✅ The Fix: BSS138 Bidirectional Level Shifter

```
+3V3                                   +9V
  │                                      │
[10k]                                  [10k]
  │                                      │
  ├────────[Source]    [Drain]───────────┤
  │            │          │              │
  │          BSS138       │              │
  │            │          │              │
SDA_3V3 ──────[Gate]──────┘         SDA_9V ───▶ TDA7439
  │                                      │       MA12070
  ▼                                      │
ESP32                                    │
OLED                                     │
```

### 🔄 How It Works

| ESP32 Output | BSS138 State | SDA_9V | TDA7439 Sees |
|--------------|--------------|--------|--------------|
| LOW (0V) | ON (V_GS=3.3V) | 0V | ✅ LOW |
| HIGH (3.3V) | OFF (V_GS=0V) | 9V (pull-up) | ✅ HIGH |
| INPUT | OFF | 9V (pull-up) | ✅ HIGH |

**Reverse direction:** TDA pulls SDA_9V low → Body diode conducts → ESP32 sees ~0.7V = LOW ✅

---

## 🔗 Ribbon Connector [V1.10]

### ❌ JST XH Problem

| Issue | Consequence |
|-------|-------------|
| Friction lock only (0.5N) | Vibrations → progressive disconnect |
| GND disconnects before +22V | Current seeks return via ESP32 GPIO |
| Result | **Fried MCU** 💀 |

### ✅ Molex Micro-Fit 3.0 Solution

| Feature | Value |
|---------|-------|
| Lock type | Positive latch (audible click) |
| Extraction force | 12N (24× stronger!) |
| Vibration rated | Automotive grade |
| Part numbers | Header: 43045-1600, Plug: 43025-1600 |

### 📍 Pinout

| Pin | Signal | Direction | Protection |
|-----|--------|-----------|------------|
| 1 | 22V_SENSE | C1→C2 | Voltage divider |
| 2 | +5V | C1→C2 | PTC 750mA |
| 3 | +3V3 | C1→C2 | PTC 500mA |
| 4-6 | GND_PWR/SIG/SHIELD | — | — |
| 7,9 | AUDIO_L/R | C2→C1 | — |
| 8,10 | GND_SHIELD | — | — |
| 11 | SDA_9V | ↔ | Level shifter |
| 12 | SCL_9V | C2→C1 | Level shifter |
| 13-16 | AMP_EN/MUTE/ERR/SAFE | ↔ | — |

---

## ⚡ Power Rails

```
+22V_RAW (18-25.2V)
    │
    ├──▶ LM7812 ──────▶ +12V_PRE
    │    [15×15mm         │
    │     copper pour]    ├──▶ LM7809 ──▶ +9V_BUFFER
    │                     │                  │
    │                     │                  ├── TDA7439
    │                     │                  └── OPA2134
    │                     │
    │                     └──▶ MCP1703A ──▶ +5V_ANALOG
    │                                           │
    │                                           ├── PCM5102A
    │                                           └── CD4053
    │
    ├──▶ MP1584 ──────▶ +5V_DIGITAL
    │    (Buck 96%)       │
    │                     └──▶ AMS1117 ──▶ +3V3
    │                                        │
    │                                        ├── ESP32-S3
    │                                        └── OLED
    │
    └──▶ Direct ──────▶ +PVDD (MA12070)
         via SS54
```

### 🌡️ Thermal Calculations

| Component | P_diss | Rth | Tj @ 40°C ambient |
|-----------|--------|-----|-------------------|
| LM7812 | 0.66W | 40°C/W (copper) | 66°C ✅ |
| LM7809 | 0.12W | 65°C/W | 48°C ✅ |
| MA12070 | 2W @ 10W out | 25°C/W | 90°C ✅ |
| OPA2134 | 50mW | 100°C/W | 45°C ✅ |

---

## 🎵 Audio Signal Path

```
┌─────────┐     ┌──────────┐     ┌─────────┐     ┌────────┐
│ 📶 BT   │────▶│ 🎵 DAC   │────▶│         │────▶│        │
│ BTM525  │ I2S │ PCM5102A │     │         │     │        │
└─────────┘     └──────────┘     │         │     │        │
                                 │ 🎛️ MUX  │     │🎚️TDA7439│
┌─────────┐                      │ CD4053  │────▶│   EQ   │
│ 🎧 AUX  │─────────────────────▶│         │     │        │
└─────────┘                      │         │     │        │
                                 │         │     │        │
┌─────────┐     ┌──────────┐     │         │     │        │
│ 💿 PHONO│────▶│  RIAA    │────▶│         │     │        │
└─────────┘     │ OPA2134  │     └─────────┘     └────┬───┘
                └──────────┘                         │
                                                     ▼
                ┌──────────┐     ┌──────────┐   ┌────────┐
                │ 🔊 Buffer│◀────│  Volume  │◀──┘        │
                │ OPA2134  │     │ (TDA int)│            │
                │   @9V    │     └──────────┘            │
                └────┬─────┘                             │
                     ▼                                   │
              ┌────────────┐                             │
              │ 🔊 MA12070 │                             │
              │  Class-D   │                             │
              │    BTL     │                             │
              └──────┬─────┘                             │
                     ▼                                   │
              ┌─────────────┐                            │
              │  🔈 HP 8Ω   │                            │
              │  2 × 20W    │                            │
              └─────────────┘                            │
```

---

## ⚠️ BTL Warning

```
┌────────────────────────────────────────────────────────────┐
│                                                            │
│   🔴 HP_L- and HP_R- are ACTIVE OUTPUTS!                  │
│                                                            │
│   They oscillate in anti-phase with HP_L+/HP_R+           │
│   They are NOT ground!                                     │
│                                                            │
│   ❌ FORBIDDEN:                                            │
│      • Standard scope probe (ground clip)                  │
│      • Measurement between HP and chassis                  │
│      • USB connected during HP measurement                 │
│                                                            │
│   ✅ REQUIRED:                                             │
│      • Differential probes only                            │
│      • OR disconnect USB first                             │
│      • OR USB galvanic isolator                            │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## 📦 Key BOM Items

### Semiconductors

| Ref | Part | Package | Note |
|-----|------|---------|------|
| U1 | MA12070 | QFN-48 | Class-D amp |
| U2 | OPA2134PA | DIP-8 | Audio op-amp ×2 |
| U3 | TDA7439 | DIP-30 | EQ processor |
| Q_SDA | BSS138 | SOT-23 | Level shifter |
| Q_SCL | BSS138 | SOT-23 | Level shifter |

### Protection

| Ref | Part | Value | Note |
|-----|------|-------|------|
| NTC1 | Ametherm SL10 | 5Ω | Inrush limiter |
| PTC1 | Bourns MF-R075 | 750mA | 5V protection |
| PTC2 | Bourns MF-R050 | 500mA | 3V3 protection |
| F1 | ATO Fuse | 5A Fast | Main protection |
| D2 | SMBJ24CA | 600W | TVS transient |

### Connectors

| Ref | Part | Note |
|-----|------|------|
| J_INTER | Molex 43045-1600 | PCB header |
| J_PLUG | Molex 43025-1600 | Cable plug |

---

## 📝 Changelog

### V1.10

| Fix | Before | After |
|-----|--------|-------|
| TDA7439 supply | 5V (out of spec!) | 9V + level shifter |
| I2C levels | 3.3V direct | BSS138 3.3V ↔ 9V |
| Ribbon connector | JST XH (friction) | Molex Micro-Fit (latch) |
| LM7812 thermal | Free air | 15×15mm copper pour |
| C_REF RIAA | 10µF | 47µF (-37dB @ 50Hz) |

### V1.9

| Fix | Description |
|-----|-------------|
| NTC inrush | 5Ω limits I_peak to 5A |
| PTC ribbon | 750mA/500mA protection |
| Buffer 9V | 7.5V headroom instead of 3.5V |
| VREF decoupling | 10µF for PSRR |

---

**🔧 Happy building!**
