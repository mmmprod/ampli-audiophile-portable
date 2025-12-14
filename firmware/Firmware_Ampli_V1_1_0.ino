/*
 * ============================================================================
 * FIRMWARE AMPLI AUDIOPHILE PORTABLE V1.10
 * ============================================================================
 * 
 * Date: 14 decembre 2025
 * Auteur: Mehdi
 * MCU: ESP32-S3-WROOM-1-N8R8
 * Framework: Arduino
 * 
 * ============================================================================
 * CHANGELOG V1.10
 * ============================================================================
 * 
 * [A1] Sequence extinction anti-plop: MUTE -> EN -> RELAY
 * [A2] Detection coupure alimentation via GPIO8 (POWER_FAIL)
 * [A3] Interrupt POWER_FAIL pour MUTE immediat
 * [A4] Timeout I2C augmente pour level shifter
 * 
 * Rappel V1.9:
 * [S1] I2C recovery open-drain (INPUT, pas OUTPUT+HIGH)
 * [D1] NTC fail-safe: 50% volume si capteur HS
 * [D2] Flag i2cHardwareFault
 * [P0] Filtre median ADC
 * [P1] Sections critiques portMUX encoder
 * [P2] Compteur erreurs I2C
 * 
 * ============================================================================
 */

#include <Wire.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION PINS
// ============================================================================

// I2C (niveau 3.3V cote ESP32, level shifter vers 9V)
#define PIN_SDA           1
#define PIN_SCL           2

// I2S Audio
#define PIN_I2S_BCK       3
#define PIN_I2S_WS        4
#define PIN_I2S_DATA      5

// ADC
#define PIN_ADC_BATT      6
#define PIN_ADC_NTC       7

// [V1.10] Detection coupure alimentation
#define PIN_POWER_FAIL    8

// Encodeur
#define PIN_ENC_A         18
#define PIN_ENC_B         19
#define PIN_ENC_SW        20

// IR
#define PIN_IR_RX         21

// Controle Ampli MA12070
#define PIN_AMP_EN        38
#define PIN_AMP_MUTE      39
#define PIN_AMP_ERR       40

// Controle Source
#define PIN_MUX_S0        41

// Controle Relais
#define PIN_SAFE_EN       42

// ============================================================================
// ADRESSES I2C
// ============================================================================

#define I2C_ADDR_MA12070  0x20    // Ampli Class-D (domaine 9V)
#define I2C_ADDR_OLED     0x3C    // OLED SSD1306 (domaine 3.3V)
#define I2C_ADDR_TDA7439  0x44    // Processeur audio (domaine 9V)

// ============================================================================
// CONSTANTES SYSTEME
// ============================================================================

// Volume
#define VOLUME_MIN        0
#define VOLUME_MAX        100
#define VOLUME_DEFAULT    30
#define VOLUME_FAILSAFE   50      // [V1.9] Limite si NTC HS

// Temperature (NTC 10k B3950)
#define NTC_NOMINAL_R     10000.0f
#define NTC_NOMINAL_TEMP  25.0f
#define NTC_BETA          3950.0f
#define NTC_PULLUP        10000.0f

#define TEMP_WARNING      50.0f   // Reduire volume 20%
#define TEMP_CRITICAL     65.0f   // Reduire volume 50%
#define TEMP_SHUTDOWN     75.0f   // Arret urgence
#define TEMP_FAIL_LOW     -20.0f  // NTC deconnecte
#define TEMP_FAIL_HIGH    120.0f  // NTC court-circuit

// ADC
#define ADC_RESOLUTION    12
#define ADC_MAX           4095
#define ADC_VREF          3.3f
#define ADC_MEDIAN_SIZE   5       // [V1.9] Filtre median

// Batterie
#define BATT_DIVIDER      7.666f  // (220k + 33k) / 33k
#define BATT_FULL         25.2f   // 6S charge complete
#define BATT_EMPTY        18.0f   // 6S decharge

// Timeouts
#define I2C_TIMEOUT_MS    50      // [V1.10] Augmente pour level shifter
#define WATCHDOG_TIMEOUT  5000    // 5 secondes

// [V1.10] Timings sequence extinction
#define MUTE_DELAY_MS     50      // Attente apres MUTE
#define DISABLE_DELAY_MS  100     // Attente apres DISABLE
#define SHUTDOWN_DELAY_MS 50      // Attente avant coupure relay

// ============================================================================
// TYPES ET STRUCTURES
// ============================================================================

typedef enum {
    SOURCE_BLUETOOTH = 0,
    SOURCE_AUX = 1,
    SOURCE_PHONO = 2
} AudioSource_t;

typedef enum {
    AMP_STATE_OFF = 0,
    AMP_STATE_STARTING,
    AMP_STATE_RUNNING,
    AMP_STATE_MUTED,
    AMP_STATE_THERMAL_WARNING,
    AMP_STATE_THERMAL_CRITICAL,
    AMP_STATE_SHUTDOWN,
    AMP_STATE_ERROR
} AmpState_t;

typedef struct {
    float temperature;
    bool isValid;
    bool isFailed;
} TempStatus_t;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Etat systeme
volatile AmpState_t ampState = AMP_STATE_OFF;
volatile bool powerFailDetected = false;    // [V1.10]
volatile bool shutdownRequested = false;    // [V1.10]

// Audio
uint8_t currentVolume = VOLUME_DEFAULT;
AudioSource_t currentSource = SOURCE_BLUETOOTH;
int8_t eqBass = 0;
int8_t eqMid = 0;
int8_t eqTreble = 0;

// Temperature
TempStatus_t tempStatus = {25.0f, true, false};

// Encodeur
volatile int32_t encoderPosition = 0;
volatile bool encoderButtonPressed = false;
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

// I2C
bool i2cInitialized = false;
bool i2cHardwareFault = false;              // [V1.9]
uint16_t i2cErrorCount = 0;

// ADC buffers pour filtre median [V1.9]
uint16_t adcBattBuffer[ADC_MEDIAN_SIZE];
uint16_t adcNtcBuffer[ADC_MEDIAN_SIZE];
uint8_t adcBufferIndex = 0;

// Preferences
Preferences preferences;

// ============================================================================
// PROTOTYPES
// ============================================================================

// Initialisation
void initGPIO(void);
void initI2C(void);
void initADC(void);
void initEncoder(void);
void initWatchdog(void);

// I2C
bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t data);
bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* data);
bool i2cRecovery(void);
bool i2cScan(void);

// Ampli MA12070
void initMA12070(void);
void enableMA12070(bool enable);
void setMA12070Mute(bool mute);
void setMA12070Volume(uint8_t vol);

// Audio TDA7439
void initTDA7439(void);
void setTDA7439Source(AudioSource_t source);
void setTDA7439Volume(uint8_t vol);
void setTDA7439Bass(int8_t db);
void setTDA7439Mid(int8_t db);
void setTDA7439Treble(int8_t db);

// Controle global
void setVolume(uint8_t vol);
void setSource(AudioSource_t source);
void applyThermalLimits(void);

// [V1.10] Sequence extinction
void startShutdownSequence(void);
void emergencyMute(void);

// Mesures
float readBatteryVoltage(void);
float readTemperature(void);
uint16_t medianFilter(uint16_t* buffer, uint8_t size);

// Encodeur
void IRAM_ATTR encoderISR(void);
void IRAM_ATTR buttonISR(void);
int32_t readEncoderDelta(void);

// [V1.10] Power fail interrupt
void IRAM_ATTR powerFailISR(void);

// ============================================================================
// INTERRUPTS
// ============================================================================

/**
 * [V1.10] Interrupt sur detection coupure alimentation
 * Active MUTE immediatement pour eviter plop
 */
void IRAM_ATTR powerFailISR(void) {
    powerFailDetected = true;
    
    // MUTE IMMEDIAT - pas d'I2C, juste GPIO
    digitalWrite(PIN_AMP_MUTE, HIGH);
}

/**
 * Interrupt encodeur rotatif avec protection mutex
 */
void IRAM_ATTR encoderISR(void) {
    portENTER_CRITICAL_ISR(&encoderMux);
    
    static uint8_t lastState = 0;
    uint8_t a = digitalRead(PIN_ENC_A);
    uint8_t b = digitalRead(PIN_ENC_B);
    uint8_t state = (a << 1) | b;
    
    // Table de transition Gray code
    static const int8_t transTable[] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0
    };
    
    encoderPosition += transTable[(lastState << 2) | state];
    lastState = state;
    
    portEXIT_CRITICAL_ISR(&encoderMux);
}

/**
 * Interrupt bouton encodeur
 */
void IRAM_ATTR buttonISR(void) {
    encoderButtonPressed = true;
}

// ============================================================================
// INITIALISATION
// ============================================================================

void initGPIO(void) {
    // Sorties controle ampli
    pinMode(PIN_AMP_EN, OUTPUT);
    pinMode(PIN_AMP_MUTE, OUTPUT);
    pinMode(PIN_MUX_S0, OUTPUT);
    pinMode(PIN_SAFE_EN, OUTPUT);
    
    // Etat initial securise
    digitalWrite(PIN_AMP_EN, LOW);      // Ampli OFF
    digitalWrite(PIN_AMP_MUTE, HIGH);   // MUTE ON
    digitalWrite(PIN_SAFE_EN, LOW);     // Relay OFF
    
    // Entrees
    pinMode(PIN_AMP_ERR, INPUT);
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    pinMode(PIN_IR_RX, INPUT);
    
    // [V1.10] Power fail detection
    pinMode(PIN_POWER_FAIL, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_POWER_FAIL), powerFailISR, FALLING);
    
    Serial.println("GPIO: Initialise");
}

void initI2C(void) {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);  // 400kHz Fast mode
    Wire.setTimeout(I2C_TIMEOUT_MS);
    
    // Scan bus
    Serial.println("I2C: Scan bus...");
    
    bool oledFound = false;
    bool tdaFound = false;
    bool ampFound = false;
    
    // Test OLED (3.3V)
    Wire.beginTransmission(I2C_ADDR_OLED);
    if (Wire.endTransmission() == 0) {
        oledFound = true;
        Serial.println("I2C: OLED trouve @ 0x3C");
    }
    
    // Test TDA7439 (9V via level shifter)
    Wire.beginTransmission(I2C_ADDR_TDA7439);
    if (Wire.endTransmission() == 0) {
        tdaFound = true;
        Serial.println("I2C: TDA7439 trouve @ 0x44");
    }
    
    // Test MA12070 (9V via nappe)
    Wire.beginTransmission(I2C_ADDR_MA12070);
    if (Wire.endTransmission() == 0) {
        ampFound = true;
        Serial.println("I2C: MA12070 trouve @ 0x20");
    }
    
    if (!oledFound || !tdaFound) {
        Serial.println("I2C: Device manquant, tentative recovery...");
        if (i2cRecovery()) {
            Serial.println("I2C: Recovery reussi");
        } else {
            Serial.println("I2C: Recovery echec - mode degrade");
            i2cHardwareFault = true;
        }
    }
    
    i2cInitialized = true;
}

/**
 * [V1.9] I2C Recovery conforme open-drain
 * JAMAIS de OUTPUT + HIGH sur SDA!
 */
bool i2cRecovery(void) {
    Serial.println("I2C Recovery: Debut sequence...");
    
    // Relacher SDA (INPUT = haute impedance, pullup tire a HIGH)
    pinMode(PIN_SDA, INPUT);
    
    // Generer 9 pulses clock
    pinMode(PIN_SCL, OUTPUT);
    
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_SCL, HIGH);
        delayMicroseconds(5);
        
        // Verifier si SDA est libere
        if (digitalRead(PIN_SDA) == HIGH) {
            Serial.printf("I2C Recovery: SDA libre apres %d clocks\n", i + 1);
            break;
        }
        
        digitalWrite(PIN_SCL, LOW);
        delayMicroseconds(5);
    }
    
    // Generer condition STOP
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(PIN_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(5);
    pinMode(PIN_SDA, INPUT);  // Relacher SDA - pullup tire HIGH = STOP
    delayMicroseconds(5);
    
    // Reinitialiser Wire
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    
    // Verifier si bus OK
    Wire.beginTransmission(I2C_ADDR_OLED);
    bool success = (Wire.endTransmission() == 0);
    
    if (success) {
        Serial.println("I2C Recovery: SUCCES");
    } else {
        Serial.println("I2C Recovery: ECHEC");
        i2cErrorCount++;
    }
    
    return success;
}

void initADC(void) {
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);
    
    // Remplir buffers avec premieres lectures
    for (int i = 0; i < ADC_MEDIAN_SIZE; i++) {
        adcBattBuffer[i] = analogRead(PIN_ADC_BATT);
        adcNtcBuffer[i] = analogRead(PIN_ADC_NTC);
    }
    
    Serial.println("ADC: Initialise");
}

void initEncoder(void) {
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), buttonISR, FALLING);
    
    Serial.println("Encoder: Initialise");
}

void initWatchdog(void) {
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    esp_task_wdt_add(NULL);
    
    Serial.println("Watchdog: Initialise (5s)");
}

// ============================================================================
// I2C FONCTIONS
// ============================================================================

bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t data) {
    if (i2cHardwareFault) return false;
    
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        i2cErrorCount++;
        Serial.printf("I2C Write Error: addr=0x%02X reg=0x%02X err=%d\n", addr, reg, error);
        
        if (i2cErrorCount > 10) {
            i2cHardwareFault = true;
            Serial.println("I2C: Trop d'erreurs - mode degrade");
        }
        return false;
    }
    
    return true;
}

bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* data) {
    if (i2cHardwareFault) return false;
    
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        i2cErrorCount++;
        return false;
    }
    
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
        i2cErrorCount++;
        return false;
    }
    
    *data = Wire.read();
    return true;
}

// ============================================================================
// MA12070 CONTROLE
// ============================================================================

void initMA12070(void) {
    // Enable avec MUTE actif
    digitalWrite(PIN_AMP_MUTE, HIGH);
    delay(10);
    digitalWrite(PIN_AMP_EN, HIGH);
    delay(100);  // Temps de demarrage MA12070
    
    // Configuration I2C
    i2cWrite(I2C_ADDR_MA12070, 0x00, 0x00);  // Power mode normal
    i2cWrite(I2C_ADDR_MA12070, 0x01, 0x20);  // Volume initial -32dB
    
    Serial.println("MA12070: Initialise");
}

void enableMA12070(bool enable) {
    if (enable) {
        digitalWrite(PIN_AMP_EN, HIGH);
        delay(50);
    } else {
        digitalWrite(PIN_AMP_EN, LOW);
    }
}

void setMA12070Mute(bool mute) {
    digitalWrite(PIN_AMP_MUTE, mute ? HIGH : LOW);
}

void setMA12070Volume(uint8_t vol) {
    // MA12070: 0x00 = 0dB, 0x18 = -24dB, 0x30 = -48dB
    uint8_t regVal = (100 - vol) * 0x30 / 100;
    i2cWrite(I2C_ADDR_MA12070, 0x01, regVal);
}

// ============================================================================
// TDA7439 CONTROLE
// ============================================================================

void initTDA7439(void) {
    // Volume initial
    setTDA7439Volume(currentVolume);
    
    // Source par defaut
    setTDA7439Source(SOURCE_BLUETOOTH);
    
    // EQ plat
    setTDA7439Bass(0);
    setTDA7439Mid(0);
    setTDA7439Treble(0);
    
    Serial.println("TDA7439: Initialise");
}

void setTDA7439Source(AudioSource_t source) {
    // Registre 0x00 = Input selector
    // 0 = IN1, 1 = IN2, 2 = IN3
    i2cWrite(I2C_ADDR_TDA7439, 0x00, (uint8_t)source);
}

void setTDA7439Volume(uint8_t vol) {
    // Registre 0x02 = Volume
    // 0x00 = 0dB, 0x38 = -56dB
    uint8_t regVal = (100 - vol) * 56 / 100;
    i2cWrite(I2C_ADDR_TDA7439, 0x02, regVal);
}

void setTDA7439Bass(int8_t db) {
    // Registre 0x03 = Bass
    // 0x00 = -14dB, 0x07 = 0dB, 0x0E = +14dB
    if (db < -14) db = -14;
    if (db > 14) db = 14;
    uint8_t regVal = (db + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x03, regVal);
    eqBass = db;
}

void setTDA7439Mid(int8_t db) {
    // Registre 0x04 = Mid
    if (db < -14) db = -14;
    if (db > 14) db = 14;
    uint8_t regVal = (db + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x04, regVal);
    eqMid = db;
}

void setTDA7439Treble(int8_t db) {
    // Registre 0x05 = Treble
    if (db < -14) db = -14;
    if (db > 14) db = 14;
    uint8_t regVal = (db + 14) / 2;
    i2cWrite(I2C_ADDR_TDA7439, 0x05, regVal);
    eqTreble = db;
}

// ============================================================================
// CONTROLE GLOBAL
// ============================================================================

void setVolume(uint8_t vol) {
    if (vol > VOLUME_MAX) vol = VOLUME_MAX;
    
    // [V1.9] Limite si NTC defaillant
    if (tempStatus.isFailed && vol > VOLUME_FAILSAFE) {
        vol = VOLUME_FAILSAFE;
        Serial.println("Volume: Limite 50% (NTC HS)");
    }
    
    // Appliquer limites thermiques
    if (tempStatus.isValid) {
        if (tempStatus.temperature > TEMP_CRITICAL) {
            vol = vol / 2;  // -50%
        } else if (tempStatus.temperature > TEMP_WARNING) {
            vol = vol * 80 / 100;  // -20%
        }
    }
    
    currentVolume = vol;
    setTDA7439Volume(vol);
    setMA12070Volume(vol);
}

void setSource(AudioSource_t source) {
    // Mute pendant changement
    setMA12070Mute(true);
    delay(50);
    
    // Changer source
    setTDA7439Source(source);
    currentSource = source;
    
    // GPIO MUX
    digitalWrite(PIN_MUX_S0, source == SOURCE_PHONO ? HIGH : LOW);
    
    delay(50);
    setMA12070Mute(false);
}

// ============================================================================
// [V1.10] SEQUENCE EXTINCTION ANTI-PLOP
// ============================================================================

/**
 * Sequence d'extinction ordonnee pour eviter le "plop"
 * 
 * Ordre CRITIQUE:
 * 1. MUTE d'abord (coupe audio)
 * 2. DISABLE ampli (sorties haute impedance)
 * 3. Couper relay (plus de courant)
 * 
 * Cela evite les transitoires DC dans les HP
 */
void startShutdownSequence(void) {
    Serial.println("Shutdown: Debut sequence...");
    
    ampState = AMP_STATE_SHUTDOWN;
    
    // Etape 1: MUTE immediat
    setMA12070Mute(true);
    Serial.println("Shutdown: MUTE active");
    delay(MUTE_DELAY_MS);
    
    // Etape 2: Desactiver ampli
    enableMA12070(false);
    Serial.println("Shutdown: Ampli desactive");
    delay(DISABLE_DELAY_MS);
    
    // Etape 3: Couper relay
    digitalWrite(PIN_SAFE_EN, LOW);
    Serial.println("Shutdown: Relay coupe");
    delay(SHUTDOWN_DELAY_MS);
    
    ampState = AMP_STATE_OFF;
    Serial.println("Shutdown: Complete");
}

/**
 * MUTE d'urgence sur detection coupure alimentation
 * Appele depuis ISR ou en urgence
 */
void emergencyMute(void) {
    // GPIO direct - pas d'I2C (trop lent pour urgence)
    digitalWrite(PIN_AMP_MUTE, HIGH);
    
    // Desactiver aussi
    digitalWrite(PIN_AMP_EN, LOW);
    
    Serial.println("EMERGENCY: Mute active!");
}

// ============================================================================
// MESURES
// ============================================================================

/**
 * Filtre median pour rejeter les spikes [V1.9]
 */
uint16_t medianFilter(uint16_t* buffer, uint8_t size) {
    // Copier et trier
    uint16_t sorted[ADC_MEDIAN_SIZE];
    memcpy(sorted, buffer, size * sizeof(uint16_t));
    
    // Bubble sort (petit tableau)
    for (uint8_t i = 0; i < size - 1; i++) {
        for (uint8_t j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                uint16_t tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    
    return sorted[size / 2];
}

float readBatteryVoltage(void) {
    // Ajouter nouvelle lecture au buffer
    adcBattBuffer[adcBufferIndex % ADC_MEDIAN_SIZE] = analogRead(PIN_ADC_BATT);
    
    // Filtre median
    uint16_t adcFiltered = medianFilter(adcBattBuffer, ADC_MEDIAN_SIZE);
    
    float voltage = (adcFiltered * ADC_VREF / ADC_MAX) * BATT_DIVIDER;
    return voltage;
}

float readTemperature(void) {
    // Ajouter nouvelle lecture au buffer
    adcNtcBuffer[adcBufferIndex % ADC_MEDIAN_SIZE] = analogRead(PIN_ADC_NTC);
    adcBufferIndex++;
    
    // Filtre median
    uint16_t adcFiltered = medianFilter(adcNtcBuffer, ADC_MEDIAN_SIZE);
    
    float voltage = adcFiltered * ADC_VREF / ADC_MAX;
    
    // Protection division par zero
    if (voltage < 0.1f) {
        tempStatus.isValid = false;
        tempStatus.isFailed = true;
        return TEMP_FAIL_HIGH;
    }
    
    if (voltage > (ADC_VREF - 0.1f)) {
        tempStatus.isValid = false;
        tempStatus.isFailed = true;
        return TEMP_FAIL_LOW;
    }
    
    // Calcul resistance NTC
    float ntcResistance = NTC_PULLUP * voltage / (ADC_VREF - voltage);
    
    // Equation Steinhart-Hart simplifiee (Beta)
    float steinhart = ntcResistance / NTC_NOMINAL_R;
    steinhart = log(steinhart);
    steinhart /= NTC_BETA;
    steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    steinhart -= 273.15f;
    
    // Validation plage
    if (steinhart < TEMP_FAIL_LOW || steinhart > TEMP_FAIL_HIGH) {
        tempStatus.isValid = false;
        tempStatus.isFailed = true;
    } else {
        tempStatus.isValid = true;
        tempStatus.isFailed = false;
        tempStatus.temperature = steinhart;
    }
    
    return steinhart;
}

// ============================================================================
// ENCODEUR
// ============================================================================

int32_t readEncoderDelta(void) {
    portENTER_CRITICAL(&encoderMux);
    int32_t delta = encoderPosition;
    encoderPosition = 0;
    portEXIT_CRITICAL(&encoderMux);
    return delta;
}

// ============================================================================
// STARTUP SEQUENCE
// ============================================================================

void startup(void) {
    Serial.println("\n=== AMPLI AUDIOPHILE V1.10 ===");
    Serial.println("Corrections: Level Shifter I2C, Anti-Plop, Molex");
    Serial.println("");
    
    // Verifier pas de power fail au demarrage
    if (digitalRead(PIN_POWER_FAIL) == LOW) {
        Serial.println("ERREUR: Tension insuffisante au demarrage!");
        return;
    }
    
    // Activer relay (alimente le circuit)
    digitalWrite(PIN_SAFE_EN, HIGH);
    Serial.println("Relay: Active");
    delay(500);  // Attendre stabilisation + NTC chauffe
    
    // Initialiser I2C
    initI2C();
    
    // Initialiser peripheriques audio
    initTDA7439();
    initMA12070();
    
    // Demarrer ampli (toujours MUTE d'abord)
    setMA12070Mute(true);
    enableMA12070(true);
    delay(100);
    
    // Appliquer volume initial
    setVolume(currentVolume);
    
    // Unmute
    setMA12070Mute(false);
    
    ampState = AMP_STATE_RUNNING;
    Serial.println("Startup: Complete");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    
    initGPIO();
    initADC();
    initEncoder();
    initWatchdog();
    
    // Charger preferences
    preferences.begin("ampli", true);
    currentVolume = preferences.getUChar("volume", VOLUME_DEFAULT);
    currentSource = (AudioSource_t)preferences.getUChar("source", SOURCE_BLUETOOTH);
    eqBass = preferences.getChar("bass", 0);
    eqMid = preferences.getChar("mid", 0);
    eqTreble = preferences.getChar("treble", 0);
    preferences.end();
    
    // Demarrage
    startup();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    static uint32_t lastTempCheck = 0;
    static uint32_t lastBattCheck = 0;
    
    // Reset watchdog
    esp_task_wdt_reset();
    
    // [V1.10] Gestion coupure alimentation detectee
    if (powerFailDetected) {
        Serial.println("POWER FAIL DETECTE!");
        emergencyMute();
        ampState = AMP_STATE_SHUTDOWN;
        // Ne pas continuer - attendre coupure complete
        while(1) { delay(10); }
    }
    
    // Gestion demande extinction
    if (shutdownRequested) {
        startShutdownSequence();
        shutdownRequested = false;
    }
    
    // Lecture encodeur
    int32_t delta = readEncoderDelta();
    if (delta != 0) {
        int16_t newVol = (int16_t)currentVolume + delta;
        if (newVol < VOLUME_MIN) newVol = VOLUME_MIN;
        if (newVol > VOLUME_MAX) newVol = VOLUME_MAX;
        setVolume((uint8_t)newVol);
    }
    
    // Bouton encodeur
    if (encoderButtonPressed) {
        encoderButtonPressed = false;
        // TODO: Menu ou changement source
    }
    
    // Lecture temperature (toutes les 1s)
    if (millis() - lastTempCheck > 1000) {
        lastTempCheck = millis();
        float temp = readTemperature();
        
        if (tempStatus.isValid) {
            if (temp > TEMP_SHUTDOWN) {
                Serial.println("THERMAL SHUTDOWN!");
                startShutdownSequence();
            } else if (temp > TEMP_CRITICAL) {
                ampState = AMP_STATE_THERMAL_CRITICAL;
                setVolume(currentVolume);  // Reapplique limites
            } else if (temp > TEMP_WARNING) {
                ampState = AMP_STATE_THERMAL_WARNING;
                setVolume(currentVolume);
            } else if (ampState == AMP_STATE_THERMAL_WARNING || 
                       ampState == AMP_STATE_THERMAL_CRITICAL) {
                ampState = AMP_STATE_RUNNING;
            }
        }
        
        // [V1.9] NTC fail-safe
        if (tempStatus.isFailed && currentVolume > VOLUME_FAILSAFE) {
            setVolume(VOLUME_FAILSAFE);
        }
    }
    
    // Lecture batterie (toutes les 5s)
    if (millis() - lastBattCheck > 5000) {
        lastBattCheck = millis();
        float batt = readBatteryVoltage();
        
        if (batt < BATT_EMPTY) {
            Serial.println("Batterie faible - shutdown");
            startShutdownSequence();
        }
    }
    
    // Verifier erreur ampli
    if (digitalRead(PIN_AMP_ERR) == LOW) {
        Serial.println("ERREUR MA12070!");
        ampState = AMP_STATE_ERROR;
        setMA12070Mute(true);
    }
    
    delay(10);
}

// ============================================================================
// FIN FIRMWARE V1.10
// ============================================================================
