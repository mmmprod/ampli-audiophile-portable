/*
 * ===================================================================
 * AMPLIFICATEUR AUDIOPHILE PORTABLE - FIRMWARE ESP32-S3
 * ===================================================================
 * 
 * Version  : 1.4
 * Date     : 13 decembre 2025
 * Auteur   : Mehdi + Claude
 * Board    : ESP32-S3-WROOM-1-N8R8
 * Framework: Arduino (ESP32 Core 2.0+)
 * 
 * CHANGELOG V1.4:
 *   - [P0] Filtre median ADC batterie/temperature (anti-spike)
 *   - [P1] Section critique encodeur (atomicite ESP32)
 *   - [P2] Verification code retour I2C avec retry
 *   - [P3] VU-metre calcul explicite (sans ambiguite)
 *   - [P4] Watchdog reduit 10s -> 5s
 *   - [P5] Compteur erreurs I2C avec seuil alarme
 *   - Corrections suite audit Copilot
 * 
[...continue with the FULL firmware code from the user's message...]
*/