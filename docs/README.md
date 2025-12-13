# 📚 Documentation Hardware

Ce dossier contient la documentation technique complète du projet Amplificateur Audiophile Portable.

## 📋 Index des Documents

### Schémas Hardware

| Version | Fichier | Statut | Description |
|---------|---------|--------|-------------|
| **V1.5** | [Ampli_Audiophile_Portable_V1_5.md](Ampli_Audiophile_Portable_V1_5.md) | ✅ **Recommandé** | Corrections audit sécurité Gemini |
| V1.4 | [Hardware_V1_4.md](Hardware_V1_4.md) | 📦 Stable | TDA7439 EQ 3 bandes |
| V1.3 | [Ampli_Audiophile_Portable_V1_3.md](Ampli_Audiophile_Portable_V1_3.md) | 📦 Archive | PT2314 + MCP4261 |

### Outils & Accessoires

| Document | Description |
|----------|-------------|
| [Breakout_Box_V1.md](Breakout_Box_V1.md) | Boîtier de test avec LEDs et points de mesure |

## 🛡️ Pourquoi V1.5 ?

La V1.5 corrige des problèmes de sécurité critiques identifiés par audit externe :

| Problème V1.4 | Risque | Solution V1.5 |
|---------------|--------|---------------|
| PVDD MA12070 proche limite 26V | Destruction ampli | Schottky SS54 série → 24.7V max |
| TVS SMBJ26CA clamp à 28.9V | Protection inefficace | SMBJ24CA clamp à 26.7V |
| Crosstalk I2C/Audio nappe 14 pins | Bruit audible | Nappe 16 pins blindée GND |

## 📐 Architecture Bi-Carte

```
┌────────────────────────────┐     Nappe 16 pins    ┌────────────────────────────┐
│      CARTE 1 (80×100mm)    │◄═══════════════════►│      CARTE 2 (80×120mm)    │
│                            │                      │                            │
│  • BMS + Protections       │     Signaux:         │  • ESP32-S3 (contrôle)     │
│  • Relais sécurité         │     - Audio L/R      │  • BTM525 Bluetooth        │
│  • Régulateurs (Buck, LDO) │     - I2C (SDA/SCL)  │  • PCM5102A DAC            │
│  • MA12070 Class-D         │     - Contrôle AMP   │  • TDA7439 EQ 3 bandes     │
│  • Protection PVDD (V1.5)  │     - Alims 5V/3V3   │  • OPA2134 Préampli phono  │
│                            │     - 22V_SENSE      │  • OLED + Encodeur + IR    │
└────────────────────────────┘                      └────────────────────────────┘
         ↓                                                      ↓
    Haut-parleurs                                         Entrées audio
      8Ω 2×20W                                      (Phono, AUX, Bluetooth)
```

## 📖 Contenu Type d'un Document Hardware

Chaque fichier `.md` contient :

1. **En-tête** : Version, date, changelog
2. **Blocs fonctionnels** : Alimentation, régulation, signal, ampli
3. **Connexions** : Format flèche (`+5V → R1 → Noeud`)
4. **Calculs** : Puissance, seuils, thermique
5. **BOM** : Liste complète des composants
6. **Nappe inter-cartes** : Assignation des 16 pins
7. **Notes** : Précautions, alternatives

## ⚠️ Précautions

- **Batterie 6S LiPo** : Manipuler avec précaution (risque incendie)
- **Tensions** : 22-25V présents sur Carte 1
- **ESD** : Protéger les composants sensibles (ESP32, MA12070)
- **Polarité** : Vérifier orientation diodes et condensateurs électrolytiques

## 🔗 Liens Utiles

- [Datasheet MA12070](https://www.infineon.com/dgdl/Infineon-MA12070-DataSheet-v02_00-EN.pdf)
- [Datasheet TDA7439](https://www.st.com/resource/en/datasheet/tda7439.pdf)
- [Datasheet ESP32-S3](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Firmware correspondant](../firmware/)

---

*Dernière mise à jour : Décembre 2025 (V1.5)*
