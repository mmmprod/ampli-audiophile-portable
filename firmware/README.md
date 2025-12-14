# Firmware

<p>
  <img src="https://img.shields.io/badge/ESP32--S3-WROOM--1-orange?style=flat-square&logo=espressif" />
  <img src="https://img.shields.io/badge/Arduino-Framework-00979D?style=flat-square&logo=arduino" />
  <img src="https://img.shields.io/badge/I2C-400kHz-blue?style=flat-square" />
</p>

ESP32-S3 firmware for the audiophile amp. Handles everything: volume, EQ, sources, thermal protection, battery monitoring.

---

## Quick Start

```bash
# Arduino IDE or PlatformIO
Board: ESP32-S3 Dev Module
Flash: 8MB
PSRAM: OPI PSRAM
```

Upload `Firmware_Ampli_V1_9.ino` and you're done.

---

## I2C Devices

| Address | Device | Function |
|---------|--------|----------|
| `0x20` | MA12070 | Class-D amp |
| `0x3C` | SSD1306 | OLED display |
| `0x44` | TDA7439 | EQ + volume |

---

## Key Features

- **I2C bus recovery** - Auto-recovers from stuck bus (open-drain compliant)
- **NTC fail-safe** - Limits volume to 50% if temp sensor fails
- **5s watchdog** - Auto-reset on hang
- **Median ADC filter** - No more noisy readings

---

## Pin Summary

| Function | GPIO |
|----------|------|
| I2C SDA/SCL | 1, 2 |
| I2S BCK/WS/DATA | 3, 4, 5 |
| Encoder A/B/SW | 18, 19, 20 |
| Amp EN/MUTE/ERR | 38, 39, 40 |
| Battery ADC | 6 |
| NTC ADC | 7 |

Full pinout in the code header.

---

## API

```c
setVolume(uint8_t vol);           // 0-100
setSource(AudioSource_t src);     // BT, AUX, PHONO
setTDA7439Bass(int8_t db);        // -14 to +14
setTDA7439Mid(int8_t db);
setTDA7439Treble(int8_t db);
```

---

## Changelog

| Version | What |
|---------|------|
| **1.9** | I2C open-drain fix, NTC failsafe |
| 1.8 | I2C recovery, watchdog |
| 1.7 | TDA7439 integration |

---

Full docs: [`Firmware_Ampli_V1_9.ino`](./Firmware_Ampli_V1_9.ino)
