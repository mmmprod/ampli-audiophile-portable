# 🎵 Portable Audiophile Amplifier

Battery-powered Class-D stereo amplifier with phono preamp, Bluetooth LDAC, 3-band EQ and digital control. Designed for vintage passive speakers.

[![Hardware](https://img.shields.io/badge/Hardware-v1.10-blue)](docs/Hardware_V1_10.md)
[![Firmware](https://img.shields.io/badge/Firmware-v1.10-green)](firmware/Firmware_Ampli_V1_10.ino)
[![Status](https://img.shields.io/badge/status-active-success)](https://github.com/mmmprod/ampli-audiophile-portable)
[![MCU](https://img.shields.io/badge/MCU-ESP32--S3-red)](https://www.espressif.com/)
[![Bluetooth](https://img.shields.io/badge/Bluetooth-LDAC-purple)](https://www.sony.com/electronics/ldac)
[![License](https://img.shields.io/badge/license-proprietary-orange)](LICENSE)

---

## ✅ Recommended Versions

| Hardware | Firmware | Status | Notes |
|----------|----------|--------|-------|
| **V1.10** | **V1.10** | ✅ Recommended | I2C level shifter, anti-plop, Molex |
| **V1.9** | **V1.9** | 🔧 Stable | I2C open-drain fix, PTC ribbon, NTC inrush |
| **V1.4** | **V1.4** | 📦 Archive | TDA7439, reliability fixes |

> ⚠️ **V1.10** fixes critical bugs found during external audit. Upgrade strongly recommended!

---

## ✨ Features

| Spec | Value |
|------|-------|
| 🔊 **Power** | 2 × 20W RMS @ 8Ω (MA12070 Class-D) |
| 📶 **Bluetooth** | LDAC, aptX HD, aptX, AAC, SBC (BTM525 QCC5125) |
| 🎚️ **Equalizer** | 3-band ±14dB (TDA7439) |
| 🎛️ **Inputs** | Bluetooth, AUX 3.5mm, Phono MM |
| 💿 **Phono Preamp** | RIAA OPA2134, 40dB gain |
| 🔋 **Battery** | LiPo 6S 22.2V, 4-6h runtime |
| 🛡️ **Protection** | 5-level chain (BMS→TCO→Relay→NTC→Fuse→TVS) |

---

## 🔥 What's New in V1.10

### 🐛 Critical Bugs Fixed

| Bug | Impact | Fix |
|-----|--------|-----|
| 🔴 TDA7439 @ 5V | I2C dead (V_IH=6.3V > 3.3V) | 9V supply + **BSS138 level shifter** |
| 🔴 Power-off pop | Speaker stress, blown tweeters | **MUTE→EN→RELAY** sequence |
| 🔴 JST XH vibrations | Disconnect = fried MCU | **Molex Micro-Fit 3.0** |
| 🟡 LM7812 overheating | High Tj | 15×15mm copper pour |
| 🟡 Noisy VREF | Audible 50Hz hum | 47µF C_REF (-37dB) |

### 🛡️ Enhanced Safety

| Feature | Description |
|---------|-------------|
| 🔌 USB Isolator | Protection against BTL scope short-circuit |
| ⚡ Power Fail ISR | Instant MUTE on power loss |
| 🔒 Locking connector | Molex = audible click, vibration-proof |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│            CARD 2 - SIGNAL (80×120mm)           │
│  🧠 ESP32-S3 │ 📶 BT │ 🎵 DAC │ 🎚️ EQ │ 🔊 Buffer │
│           [V1.10: BSS138 Level Shifter]         │
└──────────────────────┬──────────────────────────┘
                       │ 🔗 Molex Micro-Fit 16P
┌──────────────────────┴──────────────────────────┐
│            CARD 1 - POWER (80×100mm)            │
│  🔋 BMS 6S │ 🛡️ 5-Level Protection │ 🔊 MA12070  │
│           [V1.10: Anti-Plop Sequence]           │
└─────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start

### 1️⃣ Documentation

| Document | Description |
|----------|-------------|
| 📋 [Hardware_V1_10.md](docs/Hardware_V1_10.md) | Schematics, BOM, connections |
| 💻 [Firmware_V1_10.ino](firmware/Firmware_Ampli_V1_10.ino) | ESP32-S3 code |
| 🧪 [Breakout_Box_V1_3.md](docs/Breakout_Box_V1_3.md) | Test protocol |

### 2️⃣ Flash Firmware

```bash
# Install ESP32 Core 2.0+
# Required libraries:
# - Adafruit_GFX
# - Adafruit_SSD1306
# - IRremoteESP8266

# Board: ESP32S3 Dev Module
# Upload speed: 921600
```

### 3️⃣ Assembly

1. Solder Card 1 (power) — watch the **LM7812 copper pour**
2. Solder Card 2 (signal) — watch the **BSS138 level shifter**
3. Connect with **Molex Micro-Fit** (check the click!)
4. Connect 6S battery + 8Ω speakers
5. 🎵 **Enjoy!**

---

## 🧪 Testing & Diagnostics

### Serial Commands (115200 baud)

```
i2cscan   → Device detection (0x3C OLED, 0x44 TDA, 0x20 MA12070)
adctest   → ADC + median filter test
temptest  → NTC temperature reading
shutdown  → Anti-plop sequence test
stats     → Full statistics
```

### V1.10 Checklist

- [ ] I2C scan finds 3 devices
- [ ] Level shifter 3.3V ↔ 9V working
- [ ] Silent power-off (no pop)
- [ ] NTC fail-safe active
- [ ] Molex audible click

---

## ⚠️ Warnings

### 🔴 BTL OUTPUTS — DANGER!

```
┌────────────────────────────────────────────────────────────┐
│  ⚡ HP_L- and HP_R- are ACTIVE OUTPUTS!                   │
│     They are NOT ground!                                   │
│                                                            │
│  ❌ Standard scope probe = SHORT CIRCUIT                  │
│  ❌ USB connected + HP measurement = DESTRUCTION          │
│                                                            │
│  ✅ Differential probes only                              │
│  ✅ OR disconnect USB before HP measurement               │
│  ✅ OR use USB galvanic isolator (Adafruit #2107)         │
└────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
ampli-audiophile-portable/
├── 📄 README.md                    ← You are here
├── 📄 README_HARDWARE.md           ← Hardware technical doc
├── 📄 README_FIRMWARE.md           ← Firmware technical doc
├── 📁 docs/
│   ├── Hardware_V1_10.md
│   ├── Breakout_Box_V1_3.md
│   └── ...
├── 📁 firmware/
│   ├── Firmware_Ampli_V1_10.ino
│   └── ...
└── 📄 LICENSE
```

---

## 🤝 Contributing

Contributions welcome! Open issues, suggest improvements, submit PRs.

---

## 📜 License

Proprietary license for non-commercial use. Commercial licensing available on request. See [LICENSE](LICENSE).

---

**🎵 Enjoy high-fidelity audio!**
