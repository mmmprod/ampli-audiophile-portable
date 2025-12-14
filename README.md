<p align="center">
  <h1 align="center">🎵 Portable Audiophile Amp</h1>
  <p align="center">
    <strong>Bring your vintage speakers back to life. Anywhere.</strong>
  </p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.9-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/ESP32--S3-orange?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Bluetooth-LDAC-blue?style=for-the-badge&logo=bluetooth" />
  <img src="https://img.shields.io/badge/Class--D-2x20W-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/license-MIT-green?style=for-the-badge" />
</p>

<p align="center">
  <img src="https://img.shields.io/github/stars/mehdi/ampli-audiophile?style=social" />
  <img src="https://img.shields.io/github/forks/mehdi/ampli-audiophile?style=social" />
</p>

---

## Why?

Because your grandpa's speakers deserve better than gathering dust.

This amp lets you:
- Stream **hi-res audio** (LDAC) to any passive speakers
- Spin **vinyl** with the built-in phono preamp
- Take it **anywhere** with 6h battery life
- Actually **hear the difference** with real Class-D power

No compromises. No cheap Bluetooth modules. No garbage.

---

## The Stack

```
Bluetooth LDAC ──┐
                 │
AUX 3.5mm ───────┼──> EQ 3-band ──> MA12070 Class-D ──> 2x20W @ 8Ω
                 │
Phono MM ────────┘
        (RIAA preamp)
```

**Dual-PCB design** for clean audio. Power and signal completely separated.

---

## Specs

| | |
|---|---|
| **Power** | 2 x 20W RMS @ 8 ohms |
| **THD+N** | < 0.01% |
| **SNR** | > 110 dB |
| **Bluetooth** | LDAC / aptX HD / AAC |
| **Battery** | 6S LiPo, 4-6h runtime |
| **Inputs** | BT, AUX, Phono MM |

---

## Get Started

```bash
git clone https://github.com/mehdi/ampli-audiophile.git
```

Then check:
- [`/hardware`](./hardware) - Schematics & BOM
- [`/firmware`](./firmware) - ESP32-S3 code
- [`/docs`](./docs) - Full documentation

---

## Safety First

5-level protection system. Because lithium batteries don't forgive mistakes.

```
LiPo ─> BMS ─> Thermal ─> Relay ─> Inrush ─> Fuse ─> TVS
```

---

## Contributing

Found a bug? Want to improve something? PRs welcome.

1. Fork it
2. Create your branch (`git checkout -b feature/cool-stuff`)
3. Commit (`git commit -m 'Add cool stuff'`)
4. Push (`git push origin feature/cool-stuff`)
5. Open a PR

---

## License

MIT - Do whatever you want with it.

---

<p align="center">
  <strong>Built with obsession for sound quality.</strong>
  <br>
  <sub>Made by Mehdi - 2025</sub>
</p>

<p align="center">
  <a href="#top">Back to top</a>
</p>
