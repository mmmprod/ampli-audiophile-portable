/*
 * FIRMWARE AMPLIFICATEUR AUDIOPHILE PORTABLE V1.9
 * 
 * MCU: ESP32-S3-WROOM-1-N8R8
 * Date: 14 decembre 2025
 * Auteur: Mehdi + Claude
 * 
 * CHANGELOG V1.9:
 * [S1] CRITIQUE: I2C recovery utilise maintenant INPUT (open-drain correct)
 *      au lieu de OUTPUT+HIGH qui pouvait court-circuiter le bus
 * [D1] NTC fail-safe: Degrade a 50% volume si capteur temperature defaillant
 * [D2] Flag i2cHardwareFault si recovery echoue
 * [P0] Filtre median ADC anti-spike
 * [P1] Sections critiques encodeur avec portMUX
 * [P2] Retry I2C avec compteur erreurs
 * 
 * CHANGELOG V1.8:
 * - Ajout I2C bus recovery (9 clock pulses)
 * - NTC monitoring avec alertes
 * - Watchdog 5 secondes
 * 
 * CHANGELOG V1.7:
 * - Integration TDA7439 equalizer
 * - Gestion sources multiples
 * 
 * I2C BUS:
 * - 0x20: MA12070 (amplificateur)
 * - 0x3C: SSD1306 OLED
 * - 0x44: TDA7439 (equalizer)
 */

#include <Wire.h>
#include <driver/i2s.h>
#include <esp_task_wdt.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION PINS
// ============================================================================

// I2C
#define PIN_SDA           1
#define PIN_SCL           2

// I2S (Bluetooth BTM525)
#define PIN_I2S_BCK       3
#define PIN_I2S_WS        4
#define PIN_I2S_DATA      5

// ADC
#define PIN_ADC_BATT      6
#define PIN_ADC_NTC       7

// Encodeur rotatif
#define PIN_ENC_A         18
#define PIN_ENC_B         19
#define PIN_ENC_SW        20

// IR
#define PIN_IR_RX         21

// Controle ampli
#define PIN_AMP_EN        38
#define PIN_AMP_MUTE      39
#define PIN_AMP_ERR       40

// Controle source
#define PIN_MUX_S0        41

// Securite
#define PIN_SAFE_EN       42

// ============================================================================
// ADRESSES I2C
// ============================================================================

#define I2C_ADDR_MA12070  0x20
#define I2C_ADDR_OLED     0x3C
#define I2C_ADDR_TDA7439  0x44

// ============================================================================
// CONSTANTES SYSTEME
// ============================================================================

// Batterie 6S LiPo
#define BATT_CELL_COUNT       6
#define BATT_VOLT_MIN         18.0f   // 3.0V/cell
#define BATT_VOLT_LOW         19.8f   // 3.3V/cell - alerte
#define BATT_VOLT_MAX         25.2f   // 4.2V/cell
#define BATT_DIVIDER_RATIO    7.666f  // (220k + 33k) / 33k

// NTC 10k
#define NTC_NOMINAL_R         10000.0f
#define NTC_NOMINAL_TEMP      25.0f
#define NTC_BETA              3950.0f
#define NTC_PULLUP_R          10000.0f

// Seuils temperature (Celsius)
#define TEMP_WARNING          50.0f
#define TEMP_CRITICAL         65.0f
#define TEMP_SHUTDOWN         75.0f
#define TEMP_FAIL_LOW         -20.0f  // NTC deconnecte
#define TEMP_FAIL_HIGH        120.0f  // NTC court-circuit

// Volume
#define VOLUME_MIN            0
#define VOLUME_MAX            100
#define VOLUME_DEFAULT        30
#define VOLUME_FAILSAFE       50      // [D1] Limite si NTC defaillant

// I2C
#define I2C_TIMEOUT_MS        100
#define I2C_MAX_RETRIES       3
#define I2C_RECOVERY_CLOCKS   9
#define I2C_ERROR_THRESHOLD   10      // [P2] Seuil alarme erreurs

// Watchdog
#define WDT_TIMEOUT_SEC       5

// ADC
#define ADC_RESOLUTION        12
#define ADC_SAMPLES           16
#define ADC_MEDIAN_SIZE       5       // [P0] Taille filtre median

// ============================================================================
// STRUCTURES
// ============================================================================

typedef enum {
    SOURCE_BLUETOOTH = 0,
    SOURCE_AUX,
    SOURCE_PHONO,
    SOURCE_COUNT
} AudioSource_t;

typedef enum {
    AMP_STATE_OFF = 0,
    AMP_STATE_STARTING,
    AMP_STATE_RUNNING,
    AMP_STATE_MUTED,
    AMP_STATE_ERROR,
    AMP_STATE_THERMAL_WARN,
    AMP_STATE_THERMAL_SHUTDOWN
} AmpState_t;

typedef struct {
    int8_t bass;      // -14 to +14 dB
    int8_t mid;       // -14 to +14 dB
    int8_t treble;    // -14 to +14 dB
} Equalizer_t;

typedef struct {
    float voltage;
    uint8_t percent;
    bool isLow;
    bool isCritical;
} BatteryStatus_t;

typedef struct {
    float celsius;
    bool isValid;           // [D1] False si NTC defaillant
    bool isWarning;
    bool isCritical;
    bool isShutdown;
} TempStatus_t;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Etat systeme
volatile AmpState_t ampState = AMP_STATE_OFF;
volatile AudioSource_t currentSource = SOURCE_BLUETOOTH;
volatile uint8_t currentVolume = VOLUME_DEFAULT;
volatile bool systemEnabled = false;
volatile bool bluetoothConnected = false;

// Equalizer
Equalizer_t equalizer = {0, 0, 0};

// Monitoring
BatteryStatus_t batteryStatus;
TempStatus_t tempStatus;

// Encodeur (avec section critique)
volatile int32_t encoderPosition = 0;
volatile bool encoderButtonPressed = false;
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;  // [P1]

// I2C
volatile uint16_t i2cErrorCount = 0;    // [P2] Compteur erreurs
volatile bool i2cHardwareFault = false; // [D2] Flag defaut hardware

// NTC
volatile bool ntcFailsafeActive = false;  // [D1] Mode degrade actif

// Preferences
Preferences prefs;

// Buffers ADC pour filtre median [P0]
uint16_t battAdcBuffer[ADC_MEDIAN_SIZE];
uint16_t ntcAdcBuffer[ADC_MEDIAN_SIZE];
uint8_t adcBufferIndex = 0;

// ============================================================================
// PROTOTYPES
// ============================================================================

void initPins(void);
void initI2C(void);
void initI2S(void);
void initADC(void);
void initWatchdog(void);

bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t data);
bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* data);
bool i2cRecovery(void);

void initMA12070(void);
void setMA12070Volume(uint8_t volume);
void setMA12070Mute(bool mute);
void enableMA12070(bool enable);

void initTDA7439(void);
void setTDA7439Volume(uint8_t volume);
void setTDA7439Bass(int8_t db);
void setTDA7439Mid(int8_t db);
void setTDA7439Treble(int8_t db);
void setTDA7439Source(AudioSource_t source);

void initOLED(void);
void updateDisplay(void);
void displayBattery(void);
void displayVolume(void);
void displaySource(void);
void displayEqualizer(void);
void displayTemperature(void);
void displayError(const char* msg);

void readBattery(void);
void readTemperature(void);
uint16_t medianFilter(uint16_t* buffer, uint8_t size);  // [P0]

void IRAM_ATTR encoderISR(void);
void IRAM_ATTR buttonISR(void);
void processEncoder(void);

void setSource(AudioSource_t source);
void setVolume(uint8_t vol);
void setEqualizer(int8_t bass, int8_t mid, int8_t treble);

void saveSettings(void);
void loadSettings(void);

void handleThermalWarning(void);
void handleThermalShutdown(void);
void handleNTCFailure(void);       // [D1]
void handleI2CError(void);         // [P2]
void emergencyShutdown(void);

// ============================================================================
// INITIALISATION PINS
// ============================================================================

void initPins(void) {
    // Sorties
    pinMode(PIN_AMP_EN, OUTPUT);
    pinMode(PIN_AMP_MUTE, OUTPUT);
    pinMode(PIN_MUX_S0, OUTPUT);
    pinMode(PIN_SAFE_EN, OUTPUT);
    
    // Etats par defaut (securite)
    digitalWrite(PIN_AMP_EN, LOW);      // Ampli OFF
    digitalWrite(PIN_AMP_MUTE, HIGH);   // Mute actif
    digitalWrite(PIN_SAFE_EN, LOW);     // Relais OFF
    
    // Entrees
    pinMode(PIN_AMP_ERR, INPUT);
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    pinMode(PIN_IR_RX, INPUT);
    
    // ADC
    pinMode(PIN_ADC_BATT, INPUT);
    pinMode(PIN_ADC_NTC, INPUT);
}

// ============================================================================
// I2C AVEC RECOVERY [S1] CORRIGE V1.9
// ============================================================================

void initI2C(void) {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);  // 400kHz Fast mode
    
    // Test communication
    Wire.beginTransmission(I2C_ADDR_OLED);
    if (Wire.endTransmission() != 0) {
        Serial.println("I2C: OLED non detecte, tentative recovery...");
        if (!i2cRecovery()) {
            Serial.println("I2C: Recovery echoue!");
            i2cHardwareFault = true;  // [D2]
        }
    }
}

/*
 * I2C BUS RECOVERY - VERSION V1.9 CORRIGEE [S1]
 * 
 * PROBLEME V1.8:
 *   pinMode(SDA, OUTPUT);
 *   digitalWrite(SDA, HIGH);  // DANGER! Force 3.3V sur le bus
 *                             // Si un esclave tire SDA LOW = COURT-CIRCUIT!
 * 
 * SOLUTION V1.9:
 *   pinMode(SDA, INPUT);      // Haute impedance - RELACHE la ligne
 *                             // Le pull-up 4.7k tire naturellement a HIGH
 *                             // Conforme au protocole I2C open-drain
 * 
 * PRINCIPE I2C OPEN-DRAIN:
 *   - Les lignes SDA/SCL sont tirees a VCC par des resistances pull-up
 *   - Les devices peuvent seulement tirer la ligne a LOW (drain ouvert)
 *   - Pour "relacher" la ligne, on passe en haute impedance (INPUT)
 *   - Le pull-up fait remonter naturellement la tension a VCC
 * 
 * SEQUENCE RECOVERY:
 *   1. Generer 9 impulsions clock (libere esclaves bloques)
 *   2. Generer condition STOP (SDA LOW->HIGH pendant SCL HIGH)
 *   3. Reinitialiser le bus I2C
 */
bool i2cRecovery(void) {
    Serial.println("I2C Recovery: Debut sequence...");
    
    // Desactiver le peripheral I2C
    Wire.end();
    delay(10);
    
    // Configurer pins en mode GPIO
    pinMode(PIN_SCL, OUTPUT);
    pinMode(PIN_SDA, INPUT);  // [S1] V1.9: INPUT pas OUTPUT!
    
    // Generer 9 clock pulses pour liberer esclave bloque
    for (int i = 0; i < I2C_RECOVERY_CLOCKS; i++) {
        digitalWrite(PIN_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(PIN_SCL, HIGH);
        delayMicroseconds(5);
        
        // Verifier si SDA est relache
        if (digitalRead(PIN_SDA) == HIGH) {
            Serial.printf("I2C Recovery: SDA libre apres %d clocks\n", i + 1);
            break;
        }
    }
    
    // Generer condition STOP
    // STOP = SDA passe de LOW a HIGH pendant que SCL est HIGH
    
    // D'abord, on doit s'assurer que SDA est LOW pour faire un STOP valide
    // On utilise OUTPUT temporairement SEULEMENT pour forcer LOW
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(PIN_SDA, LOW);   // Force SDA LOW (OK car on tire vers GND)
    delayMicroseconds(5);
    
    digitalWrite(PIN_SCL, HIGH);  // SCL HIGH
    delayMicroseconds(5);
    
    // [S1] V1.9 CRITIQUE: Pour relacher SDA vers HIGH, on passe en INPUT
    // Le pull-up 4.7k externe tire la ligne a HIGH naturellement
    // NE PAS faire digitalWrite(HIGH) car ca forcerait 3.3V!
    pinMode(PIN_SDA, INPUT);      // Relache SDA - pull-up tire a HIGH
    delayMicroseconds(5);
    
    // Verifier que SDA est bien HIGH maintenant
    if (digitalRead(PIN_SDA) != HIGH) {
        Serial.println("I2C Recovery: ECHEC - SDA toujours LOW apres STOP");
        i2cHardwareFault = true;  // [D2] Marquer defaut hardware
        return false;
    }
    
    // Reinitialiser I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    
    // Test final
    Wire.beginTransmission(I2C_ADDR_OLED);
    uint8_t result = Wire.endTransmission();
    
    if (result == 0) {
        Serial.println("I2C Recovery: SUCCES");
        i2cErrorCount = 0;  // Reset compteur
        return true;
    } else {
        Serial.printf("I2C Recovery: ECHEC code %d\n", result);
        i2cHardwareFault = true;  // [D2]
        return false;
    }
}

bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t data) {
    for (int retry = 0; retry < I2C_MAX_RETRIES; retry++) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(data);
        
        if (Wire.endTransmission() == 0) {
            return true;
        }
        
        // [P2] Incrementer compteur erreurs
        i2cErrorCount++;
        if (i2cErrorCount >= I2C_ERROR_THRESHOLD) {
            handleI2CError();
        }
        
        delay(5);
        
        if (retry == I2C_MAX_RETRIES - 1) {
            Serial.printf("I2C Write echec addr 0x%02X reg 0x%02X\n", addr, reg);
            i2cRecovery();
        }
    }
    return false;
}

bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* data) {
    for (int retry = 0; retry < I2C_MAX_RETRIES; retry++) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        if (Wire.endTransmission() != 0) {
            i2cErrorCount++;
            continue;
        }
        
        if (Wire.requestFrom(addr, (uint8_t)1) == 1) {
            *data = Wire.read();
            return true;
        }
        
        i2cErrorCount++;
        delay(5);
    }
    
    if (i2cErrorCount >= I2C_ERROR_THRESHOLD) {
        handleI2CError();
    }
    
    return false;
}

// [P2] Gestion erreurs I2C
void handleI2CError(void) {
    Serial.printf("I2C: Seuil erreurs atteint (%d)\n", i2cErrorCount);
    
    // Tenter recovery
    if (!i2cRecovery()) {
        // Si recovery echoue, passer en mode degrade
        displayError("I2C ERROR");
        // Ne pas shutdown complet, continuer avec fonctionnalites limitees
    }
}

// ============================================================================
// MA12070 AMPLIFICATEUR
// ============================================================================

void initMA12070(void) {
    // Enable ampli
    digitalWrite(PIN_AMP_EN, HIGH);
    delay(50);
    
    // Configuration initiale
    // Registres MA12070 (voir datasheet)
    i2cWrite(I2C_ADDR_MA12070, 0x00, 0x00);  // Power mode normal
    i2cWrite(I2C_ADDR_MA12070, 0x01, 0x00);  // I2S mode
    i2cWrite(I2C_ADDR_MA12070, 0x02, 0x30);  // Volume initial -24dB
    
    // Mute OFF apres configuration
    digitalWrite(PIN_AMP_MUTE, LOW);
    
    ampState = AMP_STATE_RUNNING;
}

void setMA12070Volume(uint8_t volume) {
    // MA12070: 0x00 = 0dB, 0xFF = -127.5dB (pas de 0.5dB)
    // Mapper 0-100 vers plage utile 0dB a -60dB
    uint8_t regVal = (100 - volume) * 120 / 100;
    i2cWrite(I2C_ADDR_MA12070, 0x02, regVal);
}

void setMA12070Mute(bool mute) {
    digitalWrite(PIN_AMP_MUTE, mute ? HIGH : LOW);
}

void enableMA12070(bool enable) {
    if (enable) {
        digitalWrite(PIN_AMP_EN, HIGH);
        delay(50);
        digitalWrite(PIN_AMP_MUTE, LOW);
        ampState = AMP_STATE_RUNNING;
    } else {
        digitalWrite(PIN_AMP_MUTE, HIGH);
        delay(10);
        digitalWrite(PIN_AMP_EN, LOW);
        ampState = AMP_STATE_OFF;
    }
}

// ============================================================================
// TDA7439 EQUALIZER
// ============================================================================

void initTDA7439(void) {
    // TDA7439 - Processeur audio 3 bandes
    // Input selector + Volume + Bass/Mid/Treble
    
    // Input 1 (Bluetooth par defaut)
    i2cWrite(I2C_ADDR_TDA7439, 0x00, 0x00);
    
    // Input gain 0dB
    i2cWrite(I2C_ADDR_TDA7439, 0x01, 0x00);
    
    // Volume (attenuateur principal) - sera configure par setTDA7439Volume
    
    // Bass flat
    i2cWrite(I2C_ADDR_TDA7439, 0x03, 0x07);  // 0dB
    
    // Mid flat
    i2cWrite(I2C_ADDR_TDA7439, 0x04, 0x07);  // 0dB
    
    // Treble flat
    i2cWrite(I2C_ADDR_TDA7439, 0x05, 0x07);  // 0dB
    
    // Attenuateurs L/R a 0dB
    i2cWrite(I2C_ADDR_TDA7439, 0x06, 0x00);  // Right
    i2cWrite(I2C_ADDR_TDA7439, 0x07, 0x00);  // Left
}

void setTDA7439Volume(uint8_t volume) {
    // TDA7439: 0x00 = 0dB, 0x38 = -56dB (pas de 1dB)
    uint8_t regVal = (100 - volume) * 56 / 100;
    i2cWrite(I2C_ADDR_TDA7439, 0x02, regVal);
}

void setTDA7439Bass(int8_t db) {
    // -14dB a +14dB, pas de 2dB
    // Registre: 0x00 = -14dB, 0x07 = 0dB, 0x0E = +14dB
    int8_t clamped = constrain(db, -14, 14);
    uint8_t regVal = (clamped + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x03, regVal);
    equalizer.bass = clamped;
}

void setTDA7439Mid(int8_t db) {
    int8_t clamped = constrain(db, -14, 14);
    uint8_t regVal = (clamped + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x04, regVal);
    equalizer.mid = clamped;
}

void setTDA7439Treble(int8_t db) {
    int8_t clamped = constrain(db, -14, 14);
    uint8_t regVal = (clamped + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x05, regVal);
    equalizer.treble = clamped;
}

void setTDA7439Source(AudioSource_t source) {
    // Input selector: 0=IN1, 1=IN2, 2=IN3
    i2cWrite(I2C_ADDR_TDA7439, 0x00, (uint8_t)source);
}

// ============================================================================
// OLED DISPLAY (SSD1306 128x64)
// ============================================================================

void initOLED(void) {
    // Sequence init SSD1306
    const uint8_t initSeq[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex ratio (64)
        0xD3, 0x00, // Display offset
        0x40,       // Start line
        0x8D, 0x14, // Charge pump enable
        0x20, 0x00, // Memory mode horizontal
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins config
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge period
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // Resume RAM content
        0xA6,       // Normal display
        0xAF        // Display ON
    };
    
    Wire.beginTransmission(I2C_ADDR_OLED);
    Wire.write(0x00);  // Command stream
    for (size_t i = 0; i < sizeof(initSeq); i++) {
        Wire.write(initSeq[i]);
    }
    Wire.endTransmission();
}

void updateDisplay(void) {
    // Implementation simplifiee - a completer avec bibliotheque graphique
    displayBattery();
    displayVolume();
    displaySource();
    displayTemperature();
    
    // [D1] Afficher avertissement si mode degrade
    if (ntcFailsafeActive) {
        displayError("NTC FAIL");
    }
    
    // [D2] Afficher si defaut I2C
    if (i2cHardwareFault) {
        displayError("I2C FAULT");
    }
}

void displayBattery(void) {
    // Afficher icone batterie + pourcentage
}

void displayVolume(void) {
    // Afficher barre volume
}

void displaySource(void) {
    // Afficher source active (BT/AUX/PHONO)
    const char* sourceNames[] = {"BLUETOOTH", "AUX", "PHONO"};
    // Afficher sourceNames[currentSource]
}

void displayTemperature(void) {
    // Afficher si warning/critical
    if (tempStatus.isWarning) {
        // Afficher icone temperature
    }
}

void displayEqualizer(void) {
    // Afficher niveaux EQ
}

void displayError(const char* msg) {
    // Afficher message erreur
    Serial.printf("DISPLAY ERROR: %s\n", msg);
}

// ============================================================================
// MONITORING BATTERIE ET TEMPERATURE
// ============================================================================

// [P0] Filtre median pour rejeter spikes ADC
uint16_t medianFilter(uint16_t* buffer, uint8_t size) {
    // Copie pour tri (ne pas modifier buffer original)
    uint16_t sorted[ADC_MEDIAN_SIZE];
    memcpy(sorted, buffer, size * sizeof(uint16_t));
    
    // Tri a bulles simple (petit tableau)
    for (uint8_t i = 0; i < size - 1; i++) {
        for (uint8_t j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    
    return sorted[size / 2];  // Valeur mediane
}

void readBattery(void) {
    // Lecture ADC avec moyennage
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(PIN_ADC_BATT);
        delayMicroseconds(100);
    }
    uint16_t adcValue = sum / ADC_SAMPLES;
    
    // [P0] Ajouter au buffer median
    battAdcBuffer[adcBufferIndex % ADC_MEDIAN_SIZE] = adcValue;
    adcValue = medianFilter(battAdcBuffer, ADC_MEDIAN_SIZE);
    
    // Conversion en tension
    float adcVoltage = adcValue * 3.3f / 4095.0f;
    batteryStatus.voltage = adcVoltage * BATT_DIVIDER_RATIO;
    
    // Calcul pourcentage (approximation lineaire)
    if (batteryStatus.voltage >= BATT_VOLT_MAX) {
        batteryStatus.percent = 100;
    } else if (batteryStatus.voltage <= BATT_VOLT_MIN) {
        batteryStatus.percent = 0;
    } else {
        batteryStatus.percent = (uint8_t)((batteryStatus.voltage - BATT_VOLT_MIN) 
                                / (BATT_VOLT_MAX - BATT_VOLT_MIN) * 100.0f);
    }
    
    // Flags
    batteryStatus.isLow = (batteryStatus.voltage < BATT_VOLT_LOW);
    batteryStatus.isCritical = (batteryStatus.voltage < BATT_VOLT_MIN);
}

void readTemperature(void) {
    // Lecture ADC
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(PIN_ADC_NTC);
        delayMicroseconds(100);
    }
    uint16_t adcValue = sum / ADC_SAMPLES;
    
    // [P0] Filtre median
    ntcAdcBuffer[adcBufferIndex % ADC_MEDIAN_SIZE] = adcValue;
    adcValue = medianFilter(ntcAdcBuffer, ADC_MEDIAN_SIZE);
    
    // Calcul resistance NTC
    float adcVoltage = adcValue * 3.3f / 4095.0f;
    float ntcResistance = NTC_PULLUP_R * adcVoltage / (3.3f - adcVoltage);
    
    // [D1] Detection NTC defaillant
    if (ntcResistance < 100.0f || ntcResistance > 500000.0f) {
        // NTC probablement deconnecte ou en court-circuit
        tempStatus.isValid = false;
        handleNTCFailure();
        return;
    }
    
    // Equation Steinhart-Hart simplifiee (Beta)
    float steinhart = ntcResistance / NTC_NOMINAL_R;
    steinhart = log(steinhart);
    steinhart /= NTC_BETA;
    steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    steinhart -= 273.15f;
    
    tempStatus.celsius = steinhart;
    tempStatus.isValid = true;
    
    // [D1] Verification plausibilite
    if (tempStatus.celsius < TEMP_FAIL_LOW || tempStatus.celsius > TEMP_FAIL_HIGH) {
        tempStatus.isValid = false;
        handleNTCFailure();
        return;
    }
    
    // Si on arrive ici, NTC OK - desactiver failsafe si actif
    if (ntcFailsafeActive && tempStatus.isValid) {
        Serial.println("NTC: Capteur OK, sortie mode degrade");
        ntcFailsafeActive = false;
    }
    
    // Flags temperature
    tempStatus.isWarning = (tempStatus.celsius > TEMP_WARNING);
    tempStatus.isCritical = (tempStatus.celsius > TEMP_CRITICAL);
    tempStatus.isShutdown = (tempStatus.celsius > TEMP_SHUTDOWN);
    
    // Actions selon temperature
    if (tempStatus.isShutdown) {
        handleThermalShutdown();
    } else if (tempStatus.isCritical) {
        handleThermalWarning();
    }
}

// [D1] Gestion NTC defaillant
void handleNTCFailure(void) {
    if (!ntcFailsafeActive) {
        Serial.println("NTC: DEFAILLANCE DETECTEE - Mode degrade");
        ntcFailsafeActive = true;
        
        // Limiter volume a 50% pour securite thermique
        if (currentVolume > VOLUME_FAILSAFE) {
            setVolume(VOLUME_FAILSAFE);
            Serial.printf("NTC: Volume limite a %d%%\n", VOLUME_FAILSAFE);
        }
        
        displayError("NTC FAIL");
    }
}

// ============================================================================
// ENCODEUR ROTATIF [P1] Avec sections critiques
// ============================================================================

void IRAM_ATTR encoderISR(void) {
    portENTER_CRITICAL_ISR(&encoderMux);
    
    static uint8_t lastState = 0;
    uint8_t state = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
    
    // Table de transition Gray code
    static const int8_t transitions[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    
    uint8_t index = (lastState << 2) | state;
    encoderPosition += transitions[index];
    lastState = state;
    
    portEXIT_CRITICAL_ISR(&encoderMux);
}

void IRAM_ATTR buttonISR(void) {
    static uint32_t lastPress = 0;
    uint32_t now = millis();
    
    if (now - lastPress > 200) {  // Debounce 200ms
        encoderButtonPressed = true;
        lastPress = now;
    }
}

void processEncoder(void) {
    // [P1] Lecture atomique position
    portENTER_CRITICAL(&encoderMux);
    int32_t pos = encoderPosition;
    encoderPosition = 0;
    portEXIT_CRITICAL(&encoderMux);
    
    if (pos != 0) {
        // [D1] Verifier limite si NTC defaillant
        int newVol = currentVolume + pos;
        
        if (ntcFailsafeActive && newVol > VOLUME_FAILSAFE) {
            newVol = VOLUME_FAILSAFE;
            Serial.println("Volume limite (NTC failsafe)");
        }
        
        newVol = constrain(newVol, VOLUME_MIN, VOLUME_MAX);
        setVolume(newVol);
    }
    
    if (encoderButtonPressed) {
        encoderButtonPressed = false;
        // Changer de source
        currentSource = (AudioSource_t)((currentSource + 1) % SOURCE_COUNT);
        setSource(currentSource);
    }
}

// ============================================================================
// CONTROLE AUDIO
// ============================================================================

void setSource(AudioSource_t source) {
    currentSource = source;
    
    // Mute pendant changement
    setMA12070Mute(true);
    delay(50);
    
    // Changer MUX
    setTDA7439Source(source);
    
    // Configurer MUX hardware si necessaire
    switch (source) {
        case SOURCE_BLUETOOTH:
            digitalWrite(PIN_MUX_S0, LOW);
            break;
        case SOURCE_AUX:
            digitalWrite(PIN_MUX_S0, HIGH);
            break;
        case SOURCE_PHONO:
            digitalWrite(PIN_MUX_S0, HIGH);
            break;
    }
    
    delay(50);
    setMA12070Mute(false);
    
    updateDisplay();
    saveSettings();
}

void setVolume(uint8_t vol) {
    // [D1] Appliquer limite si NTC failsafe
    if (ntcFailsafeActive && vol > VOLUME_FAILSAFE) {
        vol = VOLUME_FAILSAFE;
    }
    
    currentVolume = constrain(vol, VOLUME_MIN, VOLUME_MAX);
    
    // Appliquer aux deux etages
    setTDA7439Volume(currentVolume);
    setMA12070Volume(currentVolume);
    
    updateDisplay();
}

void setEqualizer(int8_t bass, int8_t mid, int8_t treble) {
    setTDA7439Bass(bass);
    setTDA7439Mid(mid);
    setTDA7439Treble(treble);
    
    updateDisplay();
    saveSettings();
}

// ============================================================================
// GESTION THERMIQUE
// ============================================================================

void handleThermalWarning(void) {
    if (ampState != AMP_STATE_THERMAL_WARN) {
        Serial.printf("THERMAL WARNING: %.1fC\n", tempStatus.celsius);
        ampState = AMP_STATE_THERMAL_WARN;
        
        // Reduire volume de 20%
        uint8_t reducedVol = currentVolume * 80 / 100;
        setVolume(reducedVol);
        
        displayError("TEMP HIGH");
    }
}

void handleThermalShutdown(void) {
    Serial.printf("THERMAL SHUTDOWN: %.1fC\n", tempStatus.celsius);
    emergencyShutdown();
}

void emergencyShutdown(void) {
    Serial.println("!!! EMERGENCY SHUTDOWN !!!");
    
    // Couper ampli
    setMA12070Mute(true);
    enableMA12070(false);
    
    // Couper relais principal
    digitalWrite(PIN_SAFE_EN, LOW);
    
    ampState = AMP_STATE_THERMAL_SHUTDOWN;
    
    displayError("SHUTDOWN");
}

// ============================================================================
// PERSISTANCE SETTINGS
// ============================================================================

void saveSettings(void) {
    prefs.begin("ampli", false);
    prefs.putUChar("volume", currentVolume);
    prefs.putUChar("source", (uint8_t)currentSource);
    prefs.putChar("bass", equalizer.bass);
    prefs.putChar("mid", equalizer.mid);
    prefs.putChar("treble", equalizer.treble);
    prefs.end();
}

void loadSettings(void) {
    prefs.begin("ampli", true);
    currentVolume = prefs.getUChar("volume", VOLUME_DEFAULT);
    currentSource = (AudioSource_t)prefs.getUChar("source", SOURCE_BLUETOOTH);
    equalizer.bass = prefs.getChar("bass", 0);
    equalizer.mid = prefs.getChar("mid", 0);
    equalizer.treble = prefs.getChar("treble", 0);
    prefs.end();
    
    // [D1] Appliquer limite si NTC etait deja defaillant
    if (ntcFailsafeActive && currentVolume > VOLUME_FAILSAFE) {
        currentVolume = VOLUME_FAILSAFE;
    }
}

// ============================================================================
// WATCHDOG
// ============================================================================

void initWatchdog(void) {
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true);  // Panic on timeout
    esp_task_wdt_add(NULL);  // Add current task
}

// ============================================================================
// I2S (pour Bluetooth)
// ============================================================================

void initI2S(void) {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_I2S_BCK,
        .ws_io_num = PIN_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_I2S_DATA
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ============================================================================
// ADC
// ============================================================================

void initADC(void) {
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);  // Plage 0-3.3V
    
    // [P0] Initialiser buffers median
    for (int i = 0; i < ADC_MEDIAN_SIZE; i++) {
        battAdcBuffer[i] = 0;
        ntcAdcBuffer[i] = 0;
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== AMPLI AUDIOPHILE V1.9 ===");
    Serial.println("Corrections: I2C open-drain, NTC failsafe, PTC nappe");
    
    // Init hardware
    initPins();
    initADC();
    initI2C();
    initI2S();
    
    // Charger settings
    loadSettings();
    
    // Init peripheriques I2C
    if (!i2cHardwareFault) {
        initOLED();
        initTDA7439();
        initMA12070();
    } else {
        Serial.println("ERREUR: I2C defaillant, fonctionnalites limitees");
    }
    
    // Interruptions encodeur
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), buttonISR, FALLING);
    
    // Activer relais principal
    digitalWrite(PIN_SAFE_EN, HIGH);
    systemEnabled = true;
    
    // Appliquer settings
    setSource(currentSource);
    setVolume(currentVolume);
    setEqualizer(equalizer.bass, equalizer.mid, equalizer.treble);
    
    // Watchdog
    initWatchdog();
    
    Serial.println("Initialisation complete");
    updateDisplay();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    // Reset watchdog
    esp_task_wdt_reset();
    
    // Lecture capteurs (toutes les 500ms)
    static uint32_t lastSensorRead = 0;
    if (millis() - lastSensorRead > 500) {
        readBattery();
        readTemperature();
        lastSensorRead = millis();
        
        // [P0] Incrementer index buffer
        adcBufferIndex++;
        
        // Verifier erreur ampli
        if (digitalRead(PIN_AMP_ERR) == HIGH) {
            Serial.println("ERREUR AMPLI DETECTEE!");
            displayError("AMP ERROR");
        }
    }
    
    // Traitement encodeur
    processEncoder();
    
    // Mise a jour affichage (toutes les 200ms)
    static uint32_t lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 200) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // Actions batterie faible
    if (batteryStatus.isCritical && systemEnabled) {
        Serial.println("BATTERIE CRITIQUE - Shutdown");
        emergencyShutdown();
    }
    
    delay(10);
}

// ============================================================================
// FIN FIRMWARE V1.9
// ============================================================================
