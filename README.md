<p align="center">
  <h1 align="center">🎵 Portable Audiophile Amp</h1>
  <p align="center">
    <strong>Battery-powered Class-D amp with LDAC, phono preamp, and 5-level protection.</strong>
  </p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.9-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/ESP32--S3-orange?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Bluetooth-LDAC-blue?style=for-the-badge&logo=bluetooth" />
  <img src="https://img.shields.io/badge/Class--D-2x20W-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/license-MIT-green?style=for-the-badge" />
</p>

---

## What Is This?

A serious portable amp for vintage passive speakers. Not a toy Bluetooth speaker - a real audiophile-grade system with:

- **MA12070 Class-D** pushing 2x20W RMS into 8 ohms
- **Bluetooth 5.0 LDAC** for actual hi-res streaming (990 kbps)
- **Phono MM preamp** with proper RIAA EQ (OPA2134-based)
- **6S LiPo** for 4-6 hours of real listening
- **5-level protection** because lithium doesn't forgive

---

## Block Diagram

```
                            ┌─────────────────────────────────────────────────────┐
                            │              CARD 2 - SIGNAL                        │
 ┌──────────┐               │  ┌─────────┐   ┌─────────┐   ┌─────────┐            │
 │ BTM525   │──I2S─────────────>│PCM5102A │──>│         │──>│         │            │
 │ BT LDAC  │               │  │  DAC    │   │         │   │ OPA2134 │──┐         │
 └──────────┘               │  └─────────┘   │ TDA7439 │   │ Buffer  │  │         │
                            │                │   EQ    │   │  +9V    │  │         │
 ┌──────────┐               │  ┌─────────┐   │         │   └─────────┘  │         │
 │ AUX IN   │───────────────────>│ CD4053 │──>│ Bass   │                │         │
 │  3.5mm   │               │  │  MUX    │   │ Mid    │                │         │
 └──────────┘               │  └─────────┘   │ Treble │                │         │
                            │                └─────────┘                │         │
 ┌──────────┐  ┌─────────┐  │                                           │         │
 │ PHONO IN │─>│ OPA2134 │─────────────────────────────────────────────>│         │
 │   MM     │  │  RIAA   │  │                                           │         │
 └──────────┘  └─────────┘  │  ┌─────────────────────────────────────┐  │         │
                            │  │ ESP32-S3  │ OLED │ Encoder │ IR RX  │  │         │
                            │  └─────────────────────────────────────┘  │         │
                            └───────────────────────┬───────────────────┘         │
                                                    │ 16-pin ribbon               │
                                                    │ (PTC protected)             │
                            ┌───────────────────────┴───────────────────┐         │
                            │              CARD 1 - POWER               │         │
 ┌──────────┐               │                                           │         │
 │  6S LiPo │──>[ BMS ]──>[ TCO ]──>[ K1 ]──>[ NTC ]──>[ F1 ]──>[ TVS ]│         │
 │ 18-25.2V │    6S 20A    72C     Relay    Inrush   5A       24V     │         │
 └──────────┘               │         │                                 │         │
                            │         v                                 │<────────┘
                            │  ┌─────────────────────────────────────┐  │  Audio L/R
                            │  │ LM7812 │ LM7809 │ MP1584 │ AMS1117  │  │
                            │  │  +12V  │  +9V   │  +5V   │  +3V3    │  │
                            │  └─────────────────────────────────────┘  │
                            │         │                                 │
                            │         v                                 │
                            │  ┌─────────────┐      ┌─────────────┐     │
                            │  │   MA12070   │─────>│  HP OUT     │     │
                            │  │   Class-D   │      │  2x20W 8ohm │     │
                            │  └─────────────┘      └─────────────┘     │
                            └───────────────────────────────────────────┘
```

---

## Specs

| Parameter | Value |
|-----------|-------|
| Output Power | 2 x 20W RMS @ 8 ohms, 2 x 35W @ 4 ohms |
| THD+N | < 0.01% @ 1W |
| SNR | > 110 dB (amp), > 65 dB (phono) |
| Frequency Response | 20 Hz - 20 kHz (+/- 0.5 dB) |
| Bluetooth | 5.0 - LDAC, aptX HD, aptX, AAC, SBC |
| EQ | 3-band +/- 14 dB @ 160Hz / 720Hz / 2.8kHz |
| Battery | 6S LiPo 22.2V nominal (18V - 25.2V) |
| Runtime | 4-6 hours @ typical listening levels |
| Quiescent Current | < 50 mA |
| Max Current | 3A |

---

## Protection System

Five independent protection layers. Hardware-based, not software-dependent.

```
Level   Component           Trigger              Response
─────────────────────────────────────────────────────────────────
N1      BMS JBD 6S 20A      Cell V, I, Temp      Disconnect pack
N2      TCO Aupo 72C        PCB temp > 72C       Open circuit (auto-reset)
N3      Relay HF46F         Software SAFE_EN     Isolate load
N3bis   NTC 5ohm            Inrush > 5A          Limit to 5A (was 500A!)
N4      Fuse 5A ATO         Overcurrent          Blow (replace)
N5      TVS SMBJ24CA        Surge > 24V          Clamp to 26V
```

---

## I2C Bus

```
┌─────────────┐
│  ESP32-S3   │
│             │
│  GPIO1 SDA ─┼───┬─────┬─────┬───── 4.7k ─── +3V3
│  GPIO2 SCL ─┼───┼──┬──┼──┬──┼───── 4.7k ─── +3V3
└─────────────┘   │  │  │  │  │
                  │  │  │  │  │
            ┌─────┴──┴┐ │  │  │
            │ MA12070 │ │  │  │     Address: 0x20
            │ Class-D │ │  │  │     Amp control, volume
            └─────────┘ │  │  │
                  ┌─────┴──┴┐ │
                  │ TDA7439 │ │     Address: 0x44
                  │   EQ    │ │     Source, EQ, volume
                  └─────────┘ │
                        ┌─────┴┐
                        │ OLED │    Address: 0x3C
                        │128x64│    SSD1306
                        └──────┘
```

---

## Power Rails

```
+BATT (18-25.2V)
    │
    ├──> D1 SS54 ──> +22V_RAW
    │                    │
    │                    ├──> LM7812 ──> +12V_PRE ──> MCP1703 ──> +5V_ANALOG
    │                    │
    │                    ├──> LM7809 ──> +9V_BUFFER (OPA2134 headroom)
    │                    │
    │                    ├──> MP1584 buck ──> +5V ──> AMS1117 ──> +3V3
    │                    │
    │                    └──> D3 1N5822 ──> +PVDD (MA12070, max 26V)
    │
    └──> GND (star ground on C_BULK 220uF)
```

---

## Directory Structure

```
/
├── README.md                           # You are here
├── hardware/
│   ├── README.md                       # Hardware overview
│   ├── Ampli_Audiophile_Portable_V1_9.md   # Full schematic
│   └── Breakout_Box_Test_V1_2.md       # Test jig
├── firmware/
│   ├── README.md                       # Firmware overview
│   └── Firmware_Ampli_V1_9.ino         # ESP32-S3 code
└── docs/
    └── ...                             # Additional documentation
```

---

## Getting Started

```bash
git clone https://github.com/mehdi/ampli-audiophile.git
cd ampli-audiophile
```

1. **Hardware**: Check `/hardware` for schematics and BOM
2. **Firmware**: Flash `/firmware/Firmware_Ampli_V1_9.ino` to ESP32-S3
3. **Build**: Dual-PCB design, veroboard or custom PCB
4. **Test**: Use the breakout box for systematic validation

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| **1.9** | Dec 2025 | PTC ribbon protection, NTC inrush limiter, +9V buffer rail, I2C open-drain fix |
| 1.8 | Dec 2025 | NTC fail-safe (50% volume limit), I2C bus recovery |
| 1.7 | Dec 2025 | LM7812 pre-regulator, 1N5822 PVDD protection |
| 1.6 | Dec 2025 | Star ground implementation, PCB layout rules |
| 1.5 | Dec 2025 | TVS protection, shielded ribbon cable |

---

## License

MIT - Use it, modify it, sell it, whatever. Just don't blame me if you fry something.

---

<p align="center">
  <sub>Designed for vintage speakers that deserve better than Bluetooth junk.</sub>
  <br>
  <sub>Made by Mehdi - 2025</sub>
</p>
