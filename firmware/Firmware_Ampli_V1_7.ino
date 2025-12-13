/*
 * ===================================================================
 * AMPLIFICATEUR AUDIOPHILE PORTABLE - FIRMWARE ESP32-S3
 * ===================================================================
 * 
 * Version  : 1.7
 * Date     : 13 décembre 2025
 * Auteur   : Mehdi + Claude
 * Board    : ESP32-S3-WROOM-1-N8R8
 * Framework: Arduino (ESP32 Core 2.0+)
 * 
 * CHANGELOG V1.7 :
 *   - [C1] ISR timing: esp_timer_get_time() remplace millis() (plus robuste)
 *   - [C2] I2C bus recovery: récupération bus bloqué au boot
 *   - [HW] LM7812 ajouté (pré-régulateur, VIN MCP1703A = 12V)
 *   - [HW] R_DROP 47Ω supprimée
 *   - [HW] D3 → 1N5822 (Vf 0.9V, PVDD nominal 24.3V)
 * 
 * CHANGELOG V1.6 (AUDIT EXHAUSTIF FIABILITÉ):
 *   - [A1] emergencyShutdown() refonte: detachInterrupt() FIRST, GPIO direct
 *   - [A2] Encodeur anti-spam: saturation ±5 pas/cycle (50ms)
 *   - [A3] NTC validation plages: détection déconnexion/court-circuit
 *   - [A4] ADC overflow validation: check >4095 avec saturation
 *   - [A5] I2C backoff exponentiel: 10+20+40ms (70ms total)
 *   - [A6] Brown-out pré-détection: seuil CRITICAL avant BMS
 *   - [A7] NVS corruption handling: vérification retour + mode dégradé
 *   - [A8] Shutdown séquence sécurisée: Mute→Disable→Save→Display
 * 
 * CHANGELOG V1.5 :
 *   - [G1] I2C timeout 10ms (Wire.setTimeOut) anti-blocage bus
 *   - [G2] Support protection PVDD (D3 Schottky -> 24.7V max)
 *   - [G3] Nappe 16 pins avec blindage GND (anti-crosstalk)
 * 
 * CHANGELOG V1.4 :
 *   - [P0] Filtre médian ADC batterie/température (anti-spike)
 *   - [P1] Section critique encodeur (atomicité ESP32)
 *   - [P2] Vérification code retour I2C avec retry
 *   - [P4] Watchdog réduit 10s -> 5s
 * 
 * HARDWARE V1.7:
 *   - LM7812 + MCP1703A cascade (VIN garanti 12V)
 *   - D3 = 1N5822 (Vf 0.9V) → PVDD_SAFE = 24.3V
 *   - Voir schéma V1.7 pour détails
 * 
 * ===================================================================
 */

// ===================================================================
// INCLUDES
// ===================================================================

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>  // [C1] V1.7: Pour esp_timer_get_time()

// ===================================================================
// VERSION ET IDENTIFICATION
// ===================================================================

#define FW_VERSION      "1.7"
#define FW_DATE         "2025-12-13"
#define DEVICE_NAME     "AmpliAudio"

// ===================================================================
// CONFIGURATION PINS GPIO
// ===================================================================

// --- I2C (MA12070 + OLED + TDA7439) ---
#define PIN_SDA         1
#define PIN_SCL         2

// --- SPI (MCP4261 Volume - backup) ---
#define PIN_SPI_CS_VOL  10
#define PIN_SPI_MOSI    11
#define PIN_SPI_CLK     12
#define PIN_SPI_MISO    13

// --- Encodeur rotatif ---
#define PIN_ENC_A       18
#define PIN_ENC_B       19
#define PIN_ENC_SW      20

// --- Contrôle ampli ---
#define PIN_AMP_EN      15
#define PIN_AMP_MUTE    16
#define PIN_AMP_ERR     17

// --- Sélection source ---
#define PIN_SRC_SEL0    5
#define PIN_SRC_SEL1    6

// --- Bluetooth ---
#define PIN_BT_STATUS   4
#define PIN_BT_RESET    7

// --- Sécurité batterie ---
#define PIN_SAFE_EN     42

// --- ADC monitoring ---
#define PIN_ADC_BATT    38
#define PIN_ADC_NTC     39
#define PIN_ADC_AUDIO_L 40
#define PIN_ADC_AUDIO_R 41

// --- IR récepteur ---
#define PIN_IR_RX       21

// --- LED status ---
#define PIN_LED_STATUS  48

// ===================================================================
// CONFIGURATION PÉRIPHÉRIQUES
// ===================================================================

// --- OLED ---
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_ADDR       0x3C
#define OLED_BRIGHTNESS_DEFAULT 128
#define OLED_BRIGHTNESS_MIN     16
#define OLED_BRIGHTNESS_MAX     255

// --- MA12070 ---
#define MA12070_ADDR    0x20

// ===================================================================
// TDA7439 CONFIGURATION
// ===================================================================

#define TDA7439_ADDR    0x44    // Adresse I2C 7-bit (0x88 en 8-bit)

// Sub-addresses TDA7439 (Table 7 datasheet)
#define TDA7439_INPUT_SEL       0x00
#define TDA7439_INPUT_GAIN      0x01
#define TDA7439_VOLUME          0x02
#define TDA7439_BASS            0x03
#define TDA7439_MID             0x04
#define TDA7439_TREBLE          0x05
#define TDA7439_SPKR_ATT_R      0x06
#define TDA7439_SPKR_ATT_L      0x07

// Input selector values
#define TDA7439_IN1             0x03
#define TDA7439_IN2             0x02
#define TDA7439_IN3             0x01
#define TDA7439_IN4             0x00

// --- MCP4261 (backup) ---
#define MCP4261_POT0    0x00
#define MCP4261_POT1    0x01

// --- Sources audio ---
#define SOURCE_BT       0
#define SOURCE_AUX      1
#define SOURCE_PHONO    2
#define SOURCE_COUNT    3

// --- Limites volume ---
#define VOL_MIN         0
#define VOL_MAX         47      // TDA7439: 0 to -47dB
#define VOL_DEFAULT     20
#define VOL_STEP        1
#define VOL_STEP_IR     2
#define VOL_FADE_STEP   1
#define VOL_FADE_DELAY  15

// --- Limites EQ TDA7439 ---
#define EQ_MIN          0       // -14dB
#define EQ_MAX          14      // +14dB
#define EQ_CENTER       7       // 0dB (flat)
#define EQ_STEP         1       // 2dB par pas

// --- Loudness ---
#define LOUDNESS_THRESHOLD  15
#define LOUDNESS_BASS_BOOST 3
#define LOUDNESS_MID_CUT    1

// --- Spatial ---
#define SPATIAL_OFF     0
#define SPATIAL_LIGHT   1
#define SPATIAL_MEDIUM  2
#define SPATIAL_STRONG  3
#define SPATIAL_MAX     3

// --- Presets ---
#define PRESET_COUNT    8
#define PRESET_FLAT     0
#define PRESET_BASS     1
#define PRESET_VOCAL    2
#define PRESET_ROCK     3
#define PRESET_JAZZ     4
#define PRESET_CINEMA   5
#define PRESET_LIVE     6
#define PRESET_CUSTOM   7

// ===================================================================
// SEUILS BATTERIE
// ===================================================================

#define BATT_FULL       3723    // 25.2V (6S pleine)
#define BATT_NOMINAL    3350    // 22.6V
#define BATT_LOW        2976    // 20.1V
#define BATT_CRITICAL   2700    // 18.2V (pré-brownout)
#define BATT_SHUTDOWN   2604    // 17.6V (avant BMS coupe à 18.0V)

// ===================================================================
// SEUILS TEMPÉRATURE
// ===================================================================

// Plages ADC NTC pour validation
#define NTC_SHORT_CIRCUIT   100     // ADC < 100 = court-circuit NTC
#define NTC_DISCONNECTED    3900    // ADC > 3900 = NTC déconnectée

// Valeurs normales NTC 10kΩ avec diviseur 10kΩ
#define TEMP_NORMAL     2200    // ~25°C
#define TEMP_WARN       1800    // ~40°C
#define TEMP_THROTTLE   1500    // ~55°C
#define TEMP_CRITICAL   1200    // ~70°C
#define TEMP_SHUTDOWN   1000    // ~85°C

// ===================================================================
// CONFIGURATION V1.7 - CORRECTIONS AUDIT CHATGPT
// ===================================================================

// [C2] I2C Bus Recovery
#define I2C_RECOVERY_CLOCKS     9       // Nombre de clocks SCL pour recovery

// ===================================================================
// CONFIGURATION V1.6 - CORRECTIONS AUDIT EXHAUSTIF
// ===================================================================

// Anti-spam encodeur
#define ENCODER_MAX_DELTA       5       // Max pas par cycle (50ms)
#define ENCODER_CYCLE_MS        50      // Période traitement encodeur

// ADC validation
#define ADC_MAX_VALID           4095    // 12-bit max
#define ADC_MIN_VALID           0       // 12-bit min

// I2C backoff exponentiel
#define I2C_RETRY_COUNT         3       // Nombre de tentatives
#define I2C_RETRY_BASE_MS       10      // Délai base (×2 à chaque retry)
#define I2C_ERROR_THRESHOLD     10      // Seuil erreurs avant alarme

// Pré-brownout
#define BROWNOUT_SAMPLES        3       // Confirmations avant action
#define BROWNOUT_SAVE_DELAY_MS  50      // Délai entre saves critiques

// NVS robustesse
#define NVS_NAMESPACE           "ampli"
#define NVS_INIT_RETRY          3       // Tentatives init NVS

// ===================================================================
// CONFIGURATION V1.5 - CORRECTIONS AUDIT GEMINI
// ===================================================================

// I2C timeout anti-blocage bus
#define I2C_TIMEOUT_MS      10      // Timeout I2C

// ===================================================================
// CONFIGURATION V1.4 - CORRECTIONS AUDIT COPILOT
// ===================================================================

// Filtrage ADC
#define ADC_FILTER_SAMPLES  5       // Nombre échantillons filtre médian
#define ADC_FILTER_DELAY_US 100     // Délai entre échantillons (µs)

// Watchdog
#define WDT_TIMEOUT         5       // 5s

// --- Timing ---
#define DEBOUNCE_MS         50
#define LONGPRESS_MS        800
#define VERYLONGPRESS_MS    2000
#define DISPLAY_REFRESH     50
#define ADC_INTERVAL        500
#define SAVE_DELAY          5000
#define MENU_TIMEOUT        10000
#define AUTOSLEEP_TIMEOUT   1800000
#define VU_DECAY_RATE       10

// ===================================================================
// CODES IR TÉLÉCOMMANDE
// ===================================================================

#define IR_CODE_POWER       0x00FF00FF
#define IR_CODE_MUTE        0x00FF807F
#define IR_CODE_VOL_UP      0x00FF40BF
#define IR_CODE_VOL_DOWN    0x00FFC03F
#define IR_CODE_SOURCE      0x00FF20DF
#define IR_CODE_NEXT        0x00FFA05F
#define IR_CODE_PREV        0x00FF609F
#define IR_CODE_OK          0x00FF906F
#define IR_CODE_MENU        0x00FFE01F
#define IR_CODE_BACK        0x00FF10EF
#define IR_CODE_NUM_0       0x00FF9867
#define IR_CODE_NUM_1       0x00FFA25D
#define IR_CODE_NUM_2       0x00FF629D
#define IR_CODE_NUM_3       0x00FFE21D
#define IR_CODE_EQ          0x00FF22DD
#define IR_CODE_LOUD        0x00FF32CD
#define IR_CODE_SPATIAL     0x00FF52AD

// ===================================================================
// OBJETS GLOBAUX
// ===================================================================

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Preferences preferences;
SPIClass *vspi = NULL;
IRrecv irrecv(PIN_IR_RX);
decode_results irResults;

// Mutex pour section critique encodeur (ESP32)
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

// ===================================================================
// STRUCTURES DE DONNÉES
// ===================================================================

struct Equalizer {
  uint8_t bass;
  uint8_t mid;
  uint8_t treble;
  uint8_t preset;
  bool enabled;
  bool loudness;
  uint8_t spatial;
};

struct PresetDef {
  const char* name;
  uint8_t bass;
  uint8_t mid;
  uint8_t treble;
};

struct Settings {
  uint8_t volume;
  uint8_t source;
  int8_t balance;
  uint8_t brightness;
  bool vuMeterEnabled;
  uint8_t sleepTimer;
  uint8_t volumeLimit;
  uint8_t volumePerSource[SOURCE_COUNT];
  Equalizer eq;
  uint8_t inputGain;
};

// Stats étendues
struct Stats {
  uint32_t totalOnTime;
  uint32_t sessionStart;
  uint16_t powerCycles;
  uint16_t errorCount;
  uint8_t maxTempReached;
  uint16_t i2cErrors;           // Compteur erreurs I2C
  uint16_t i2cRetries;          // Compteur retries I2C
  uint16_t adcSpikesFiltered;   // Spikes ADC filtrés
  uint16_t ntcErrors;           // Erreurs NTC (déconnexion/CC)
  uint16_t encoderSpamFiltered; // Spam encodeur filtré
  uint16_t brownoutWarnings;    // Alertes pré-brownout
  uint16_t i2cRecoveries;       // [V1.7] Récupérations bus I2C
};

struct VUMeter {
  uint8_t levelL;
  uint8_t levelR;
  uint8_t peakL;
  uint8_t peakR;
  uint32_t peakHoldTimeL;
  uint32_t peakHoldTimeR;
};

struct Animation {
  uint8_t type;
  uint8_t frame;
  uint32_t lastFrame;
  bool active;
};

// ===================================================================
// PRESETS ÉGALISEUR 3 BANDES
// ===================================================================

const PresetDef presets[PRESET_COUNT] = {
  {"Flat",     7,  7,  7},
  {"Bass+",   12,  6,  7},
  {"Vocal",    5,  9, 10},
  {"Rock",    10,  7, 10},
  {"Jazz",     9,  8,  9},
  {"Cinema",  11,  7,  8},
  {"Live",     8,  9,  9},
  {"Custom",   7,  7,  7}
};

// ===================================================================
// VARIABLES D'ÉTAT
// ===================================================================

Settings settings;
Stats stats;

bool eqChipPresent = false;

// Variables encodeur avec protection atomique
volatile int32_t encoderDelta = 0;
volatile bool encoderButtonPressed = false;
volatile uint32_t lastEncoderTime = 0;

// Volume
volatile int16_t targetVolume = VOL_DEFAULT;
int16_t currentVolume = 0;
volatile bool volumeChanged = false;

// Loudness
uint8_t loudnessAppliedBass = 0;
uint8_t loudnessAppliedMid = 0;

// Source
uint8_t currentSource = SOURCE_BT;
const char* sourceNames[] = {"Bluetooth", "AUX", "Phono"};
const char* sourceIcons[] = {"BT", "AX", "PH"};

// Bouton encodeur
uint32_t buttonPressTime = 0;
bool buttonHandled = false;

// États système
bool ampEnabled = false;
bool ampMuted = true;
bool btConnected = false;
bool btPairing = false;
bool systemReady = false;
bool debugMode = false;
bool testMode = false;

// NVS état
bool nvsInitialized = false;
bool nvsDegraded = false;         // Mode dégradé si NVS corrompue

// Monitoring
uint16_t batteryRaw = 0;
uint16_t tempRaw = 0;
float batteryVoltage = 0;
uint8_t batteryPercent = 0;
bool batteryLow = false;
bool batteryCharging = false;
bool tempWarning = false;
bool thermalThrottle = false;

// NTC état
bool ntcError = false;            // Erreur capteur température

// Brownout état
uint8_t brownoutCounter = 0;      // Compteur confirmations
bool brownoutPending = false;     // Sauvegarde en cours

// Flag alarme I2C
bool i2cAlarm = false;

// VU-mètre
VUMeter vuMeter = {0, 0, 0, 0, 0, 0};

// Animation
Animation animation = {0, 0, 0, false};

// Menu
enum MenuState {
  MENU_MAIN,
  MENU_SOURCE,
  MENU_BALANCE,
  MENU_EQ,
  MENU_EQ_BASS,
  MENU_EQ_MID,
  MENU_EQ_TREBLE,
  MENU_EQ_PRESET,
  MENU_EQ_LOUDNESS,
  MENU_EQ_SPATIAL,
  MENU_SETTINGS,
  MENU_SETTINGS_BRIGHTNESS,
  MENU_SETTINGS_SLEEP,
  MENU_SETTINGS_VOLLIMIT,
  MENU_SETTINGS_VUMETER,
  MENU_SETTINGS_GAIN,
  MENU_INFO,
  MENU_TEST
};
MenuState menuState = MENU_MAIN;
uint8_t menuSelection = 0;
uint32_t menuEntryTime = 0;

// Timing
uint32_t lastDisplayUpdate = 0;
uint32_t lastADCRead = 0;
uint32_t lastSaveTime = 0;
uint32_t lastActivityTime = 0;
uint32_t sleepTimerStart = 0;
bool needsSave = false;

// Debug
uint32_t loopCounter = 0;
uint32_t lastLoopTime = 0;
uint16_t loopsPerSecond = 0;

// ===================================================================
// PROTOTYPES FONCTIONS
// ===================================================================

void debugLog(const char* format, ...);
void handleShortPress();
void handleLongPress();
void handleVeryLongPress();
void emergencyShutdown(const char* reason);
void saveSettings();
void saveStats();
void eqApplyWithLoudness();
uint16_t readADCFiltered(uint8_t pin);
bool i2cWriteWithRetry(uint8_t addr, uint8_t reg, uint8_t data);
void i2cBusRecovery();  // [C2] V1.7

// ===================================================================
// [C1] V1.7 - HELPER: GET MILLISECONDS FROM ESP_TIMER
// ===================================================================
// 
// esp_timer_get_time() retourne des microsecondes (uint64_t)
// Plus fiable que millis() dans les ISR car lecture directe hardware
//
static inline uint32_t IRAM_ATTR getMillisISR() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// ===================================================================
// DEBUG LOG
// ===================================================================

void debugLog(const char* format, ...) {
  if (!debugMode) return;
  
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] ");
  Serial.println(buffer);
}

// ===================================================================
// [C2] V1.7 - I2C BUS RECOVERY
// ===================================================================
//
// Récupère un bus I2C bloqué par un périphérique qui maintient SDA low.
// Envoie 9 clocks SCL pour permettre au périphérique de relâcher SDA,
// puis génère une condition STOP.
//
// Standard I2C recovery procedure (NXP AN10216-01)
//
void i2cBusRecovery() {
  debugLog("[C2] I2C Bus Recovery...");
  
  // Configurer les pins en GPIO
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, OUTPUT);
  
  // Vérifier si SDA est bloqué LOW
  if (digitalRead(PIN_SDA) == LOW) {
    debugLog("SDA bloqué LOW, envoi clocks recovery");
    
    // Envoyer 9 clocks SCL pour libérer SDA
    for (int i = 0; i < I2C_RECOVERY_CLOCKS; i++) {
      digitalWrite(PIN_SCL, LOW);
      delayMicroseconds(5);
      digitalWrite(PIN_SCL, HIGH);
      delayMicroseconds(5);
      
      // Vérifier si SDA est libéré
      if (digitalRead(PIN_SDA) == HIGH) {
        debugLog("SDA libéré après %d clocks", i + 1);
        break;
      }
    }
    
    // Générer condition STOP (SDA LOW→HIGH pendant SCL HIGH)
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(PIN_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_SDA, HIGH);
    delayMicroseconds(5);
    
    stats.i2cRecoveries++;
    debugLog("I2C recovery terminé");
  } else {
    debugLog("SDA OK, pas de recovery nécessaire");
  }
  
  // Remettre les pins en mode I2C (seront reconfigurés par Wire.begin())
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, INPUT);
}

// ===================================================================
// [C1] V1.7 - INTERRUPTIONS AVEC ESP_TIMER
// ===================================================================
//
// Utilise esp_timer_get_time() au lieu de millis() pour le timing ISR.
// Plus robuste car lecture directe du compteur hardware 64-bit.
//
void IRAM_ATTR encoderISR() {
  static uint8_t lastState = 0;
  uint8_t state = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  
  static const int8_t encTable[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  
  uint8_t index = (lastState << 2) | state;
  int8_t delta = encTable[index];
  
  if (delta != 0) {
    // [C1] V1.7: Utilise esp_timer au lieu de millis()
    uint32_t now = getMillisISR();
    if (now - lastEncoderTime > 2) {
      // Section critique pour modification atomique
      portENTER_CRITICAL_ISR(&encoderMux);
      
      // Saturation anti-spam
      int32_t newDelta = encoderDelta + delta;
      if (newDelta > ENCODER_MAX_DELTA) {
        newDelta = ENCODER_MAX_DELTA;
      } else if (newDelta < -ENCODER_MAX_DELTA) {
        newDelta = -ENCODER_MAX_DELTA;
      }
      encoderDelta = newDelta;
      volumeChanged = true;
      
      portEXIT_CRITICAL_ISR(&encoderMux);
      lastEncoderTime = now;
    }
  }
  
  lastState = state;
}

void IRAM_ATTR buttonISR() {
  static uint32_t lastPress = 0;
  // [C1] V1.7: Utilise esp_timer au lieu de millis()
  uint32_t now = getMillisISR();
  
  if (now - lastPress > DEBOUNCE_MS) {
    encoderButtonPressed = true;
    lastPress = now;
  }
}

// ===================================================================
// FILTRE MÉDIAN ADC AVEC VALIDATION
// ===================================================================

// Tri simple pour petit tableau (5 éléments)
void sortArray(uint16_t* arr, uint8_t size) {
  for (uint8_t i = 0; i < size - 1; i++) {
    for (uint8_t j = i + 1; j < size; j++) {
      if (arr[i] > arr[j]) {
        uint16_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
      }
    }
  }
}

uint16_t readADCFiltered(uint8_t pin) {
  uint16_t samples[ADC_FILTER_SAMPLES];
  
  // Lecture multiple
  for (int i = 0; i < ADC_FILTER_SAMPLES; i++) {
    uint16_t raw = analogRead(pin);
    
    // Validation plage 12-bit
    if (raw > ADC_MAX_VALID) {
      raw = ADC_MAX_VALID;
      stats.adcSpikesFiltered++;
    }
    samples[i] = raw;
    
    delayMicroseconds(ADC_FILTER_DELAY_US);
  }
  
  // Détection spike (valeur aberrante)
  uint16_t minVal = samples[0];
  uint16_t maxVal = samples[0];
  uint32_t sum = 0;
  
  for (int i = 0; i < ADC_FILTER_SAMPLES; i++) {
    if (samples[i] < minVal) minVal = samples[i];
    if (samples[i] > maxVal) maxVal = samples[i];
    sum += samples[i];
  }
  
  // Si écart > 20% de la moyenne = spike détecté
  uint16_t avg = sum / ADC_FILTER_SAMPLES;
  if (avg > 0 && (maxVal - minVal) > (avg / 5)) {
    stats.adcSpikesFiltered++;
    debugLog("ADC spike filtré pin %d (min:%d max:%d)", pin, minVal, maxVal);
  }
  
  // Tri et retour médiane
  sortArray(samples, ADC_FILTER_SAMPLES);
  return samples[ADC_FILTER_SAMPLES / 2];
}

// ===================================================================
// I2C AVEC BACKOFF EXPONENTIEL
// ===================================================================

bool i2cWriteWithRetry(uint8_t addr, uint8_t reg, uint8_t data) {
  uint8_t attempts = 0;
  uint8_t error = 0;
  uint16_t delayMs = I2C_RETRY_BASE_MS;
  
  while (attempts < I2C_RETRY_COUNT) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data);
    error = Wire.endTransmission();
    
    if (error == 0) {
      return true;  // Succès
    }
    
    // Échec, on retry avec backoff exponentiel
    attempts++;
    stats.i2cRetries++;
    
    if (attempts < I2C_RETRY_COUNT) {
      delay(delayMs);
      delayMs *= 2;  // Backoff exponentiel (10, 20, 40ms)
    }
  }
  
  // Échec après toutes les tentatives
  stats.i2cErrors++;
  debugLog("I2C ERR: addr=0x%02X reg=0x%02X err=%d", addr, reg, error);
  
  // Vérifier seuil alarme
  if (stats.i2cErrors >= I2C_ERROR_THRESHOLD && !i2cAlarm) {
    i2cAlarm = true;
    debugLog("ALARME: Seuil erreurs I2C atteint (%d)", stats.i2cErrors);
  }
  
  return false;
}

// Version simple pour lecture (détection présence)
bool i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// ===================================================================
// FONCTIONS TDA7439 (avec I2C robuste)
// ===================================================================

bool tda7439Detect() {
  if (i2cProbe(TDA7439_ADDR)) {
    debugLog("TDA7439 détecté @ 0x%02X", TDA7439_ADDR);
    return true;
  } else {
    debugLog("TDA7439 NON détecté (EQ désactivé)");
    return false;
  }
}

bool tda7439Write(uint8_t subAddr, uint8_t data) {
  return i2cWriteWithRetry(TDA7439_ADDR, subAddr & 0x0F, data);
}

void tda7439Init() {
  eqChipPresent = tda7439Detect();
  
  if (eqChipPresent) {
    tda7439Write(TDA7439_INPUT_SEL, TDA7439_IN1);
    tda7439Write(TDA7439_INPUT_GAIN, settings.inputGain);
    tda7439Write(TDA7439_VOLUME, settings.volume);
    tda7439Write(TDA7439_BASS, 7);
    tda7439Write(TDA7439_MID, 7);
    tda7439Write(TDA7439_TREBLE, 7);
    tda7439Write(TDA7439_SPKR_ATT_L, 0);
    tda7439Write(TDA7439_SPKR_ATT_R, 0);
    
    eqApplyPreset(settings.eq.preset);
    debugLog("TDA7439 initialisé");
  }
}

void tda7439SetInput(uint8_t input) {
  if (!eqChipPresent) return;
  uint8_t regVal = 3 - (input & 0x03);
  tda7439Write(TDA7439_INPUT_SEL, regVal);
}

void tda7439SetInputGain(uint8_t gain) {
  if (!eqChipPresent) return;
  gain = constrain(gain, 0, 15);
  tda7439Write(TDA7439_INPUT_GAIN, gain);
  settings.inputGain = gain;
}

void tda7439SetVolume(uint8_t vol) {
  if (!eqChipPresent) return;
  vol = constrain(vol, 0, 48);
  tda7439Write(TDA7439_VOLUME, vol);
}

void tda7439SetBass(uint8_t value) {
  if (!eqChipPresent) return;
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;
  tda7439Write(TDA7439_BASS, regVal);
  settings.eq.bass = value;
}

void tda7439SetMid(uint8_t value) {
  if (!eqChipPresent) return;
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;
  tda7439Write(TDA7439_MID, regVal);
  settings.eq.mid = value;
}

void tda7439SetTreble(uint8_t value) {
  if (!eqChipPresent) return;
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;
  tda7439Write(TDA7439_TREBLE, regVal);
  settings.eq.treble = value;
}

void tda7439SetSpeakerAtt(uint8_t left, uint8_t right) {
  if (!eqChipPresent) return;
  left = constrain(left, 0, 79);
  right = constrain(right, 0, 79);
  tda7439Write(TDA7439_SPKR_ATT_L, left);
  tda7439Write(TDA7439_SPKR_ATT_R, right);
}

// ===================================================================
// FONCTIONS ÉGALISEUR
// ===================================================================

void eqApply() {
  if (!eqChipPresent) return;
  tda7439SetBass(settings.eq.bass);
  tda7439SetMid(settings.eq.mid);
  tda7439SetTreble(settings.eq.treble);
}

void eqApplyWithLoudness() {
  if (!eqChipPresent) return;
  
  uint8_t bass = settings.eq.bass;
  uint8_t mid = settings.eq.mid;
  uint8_t treble = settings.eq.treble;
  
  loudnessAppliedBass = 0;
  loudnessAppliedMid = 0;
  
  if (settings.eq.loudness && currentVolume > LOUDNESS_THRESHOLD) {
    uint8_t loudnessLevel = (currentVolume - LOUDNESS_THRESHOLD) / 5;
    loudnessLevel = constrain(loudnessLevel, 0, LOUDNESS_BASS_BOOST);
    
    bass = constrain(bass + loudnessLevel, EQ_MIN, EQ_MAX);
    if (loudnessLevel > 1) {
      mid = constrain(mid - 1, EQ_MIN, EQ_MAX);
      loudnessAppliedMid = 1;
    }
    
    loudnessAppliedBass = loudnessLevel;
  }
  
  if (!settings.eq.enabled) {
    tda7439SetBass(EQ_CENTER);
    tda7439SetMid(EQ_CENTER);
    tda7439SetTreble(EQ_CENTER);
  } else {
    tda7439SetBass(bass);
    tda7439SetMid(mid);
    tda7439SetTreble(treble);
  }
}

void eqApplyPreset(uint8_t presetIndex) {
  if (presetIndex >= PRESET_COUNT) presetIndex = PRESET_FLAT;
  
  settings.eq.preset = presetIndex;
  settings.eq.bass = presets[presetIndex].bass;
  settings.eq.mid = presets[presetIndex].mid;
  settings.eq.treble = presets[presetIndex].treble;
  
  eqApplyWithLoudness();
  debugLog("Preset: %s", presets[presetIndex].name);
}

void spatialApply() {
  if (!eqChipPresent) return;
  // TDA7439 n'a pas de spatial hardware
  // Effet via balance asymétrique légère
  switch (settings.eq.spatial) {
    case SPATIAL_OFF:
      tda7439SetSpeakerAtt(0, 0);
      break;
    case SPATIAL_LIGHT:
      tda7439SetSpeakerAtt(1, 0);
      break;
    case SPATIAL_MEDIUM:
      tda7439SetSpeakerAtt(2, 1);
      break;
    case SPATIAL_STRONG:
      tda7439SetSpeakerAtt(3, 2);
      break;
  }
}

// ===================================================================
// AMPLIFICATEUR MA12070
// ===================================================================

void ampInit() {
  if (i2cProbe(MA12070_ADDR)) {
    debugLog("MA12070 détecté @ 0x%02X", MA12070_ADDR);
  } else {
    debugLog("MA12070 NON détecté!");
  }
}

void ampEnable(bool enable) {
  if (enable) {
    digitalWrite(PIN_AMP_EN, LOW);   // Active low
    delay(50);
    digitalWrite(PIN_AMP_MUTE, HIGH); // Unmute
    ampEnabled = true;
    ampMuted = false;
    debugLog("Ampli ON");
  } else {
    digitalWrite(PIN_AMP_MUTE, LOW);  // Mute first
    delay(50);
    digitalWrite(PIN_AMP_EN, HIGH);   // Disable
    ampEnabled = false;
    ampMuted = true;
    debugLog("Ampli OFF");
  }
}

void ampToggleMute() {
  if (ampMuted) {
    digitalWrite(PIN_AMP_MUTE, HIGH);
    ampMuted = false;
    debugLog("Unmute");
  } else {
    digitalWrite(PIN_AMP_MUTE, LOW);
    ampMuted = true;
    debugLog("Mute");
  }
}

// ===================================================================
// BATTERIE
// ===================================================================

void batteryConnect(bool connect) {
  if (connect) {
    digitalWrite(PIN_SAFE_EN, LOW);   // Relais fermé
    debugLog("Batterie connectée");
  } else {
    digitalWrite(PIN_SAFE_EN, HIGH);  // Relais ouvert
    debugLog("Batterie déconnectée");
  }
}

void checkBattery() {
  batteryRaw = readADCFiltered(PIN_ADC_BATT);
  
  // Diviseur 220k/33k: V_ADC = V_BATT × 33/(220+33) = V_BATT × 0.1304
  // V_BATT = ADC × 3.3 / 4095 / 0.1304 = ADC × 0.00616
  batteryVoltage = batteryRaw * 0.00616;
  
  // Calcul pourcentage (linéaire simplifié)
  if (batteryRaw >= BATT_FULL) {
    batteryPercent = 100;
  } else if (batteryRaw <= BATT_SHUTDOWN) {
    batteryPercent = 0;
  } else {
    batteryPercent = map(batteryRaw, BATT_SHUTDOWN, BATT_FULL, 0, 100);
  }
  
  // Alertes
  batteryLow = (batteryRaw <= BATT_LOW);
  
  // Pré-brownout
  if (batteryRaw <= BATT_CRITICAL) {
    brownoutCounter++;
    if (brownoutCounter >= BROWNOUT_SAMPLES) {
      stats.brownoutWarnings++;
      debugLog("ALERTE: Pré-brownout! V=%.2fV", batteryVoltage);
      
      // Sauvegarde urgente
      if (!brownoutPending) {
        brownoutPending = true;
        saveSettings();
        saveStats();
        brownoutPending = false;
      }
    }
  } else {
    brownoutCounter = 0;
  }
  
  // Shutdown critique
  if (batteryRaw <= BATT_SHUTDOWN) {
    emergencyShutdown("BATT CRITIQUE");
  }
}

// ===================================================================
// TEMPÉRATURE
// ===================================================================

void checkTemperature() {
  tempRaw = readADCFiltered(PIN_ADC_NTC);
  
  // Validation NTC
  if (tempRaw < NTC_SHORT_CIRCUIT) {
    if (!ntcError) {
      ntcError = true;
      stats.ntcErrors++;
      debugLog("ERREUR: NTC court-circuit (ADC=%d)", tempRaw);
    }
    return;
  }
  if (tempRaw > NTC_DISCONNECTED) {
    if (!ntcError) {
      ntcError = true;
      stats.ntcErrors++;
      debugLog("ERREUR: NTC déconnectée (ADC=%d)", tempRaw);
    }
    return;
  }
  ntcError = false;
  
  // Alertes température
  if (tempRaw <= TEMP_SHUTDOWN) {
    emergencyShutdown("SURCHAUFFE");
  } else if (tempRaw <= TEMP_CRITICAL) {
    thermalThrottle = true;
    tempWarning = true;
    debugLog("CRITIQUE: T° très élevée (ADC=%d)", tempRaw);
  } else if (tempRaw <= TEMP_THROTTLE) {
    thermalThrottle = true;
    tempWarning = true;
  } else if (tempRaw <= TEMP_WARN) {
    thermalThrottle = false;
    tempWarning = true;
  } else {
    thermalThrottle = false;
    tempWarning = false;
  }
}

// ===================================================================
// MONITORING
// ===================================================================

void updateMonitoring() {
  if (millis() - lastADCRead < ADC_INTERVAL) return;
  lastADCRead = millis();
  
  checkBattery();
  checkTemperature();
  
  // Vérifier erreur ampli
  if (digitalRead(PIN_AMP_ERR) == LOW) {
    debugLog("ERREUR: MA12070 /ERR actif!");
    stats.errorCount++;
  }
  
  // Bluetooth status
  btConnected = (digitalRead(PIN_BT_STATUS) == HIGH);
}

// ===================================================================
// EMERGENCY SHUTDOWN
// ===================================================================

void emergencyShutdown(const char* reason) {
  // [A1] V1.6: Désactiver interruptions EN PREMIER
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_A));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_B));
  detachInterrupt(digitalPinToInterrupt(PIN_ENC_SW));
  
  // [A1] GPIO direct (pas d'abstraction)
  GPIO.out_w1ts = (1ULL << PIN_AMP_MUTE);   // MUTE HIGH (inverser si needed)
  GPIO.out_w1tc = (1ULL << PIN_AMP_MUTE);   // MUTE LOW (mute actif)
  delayMicroseconds(1000);
  GPIO.out_w1ts = (1ULL << PIN_AMP_EN);     // EN HIGH (disable)
  
  // Sauvegarde critique
  saveSettings();
  saveStats();
  
  // Affichage
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("SHUTDOWN");
  display.setTextSize(1);
  display.setCursor(10, 35);
  display.println(reason);
  display.display();
  
  debugLog("EMERGENCY SHUTDOWN: %s", reason);
  
  // Couper batterie
  batteryConnect(false);
  
  // Boucle infinie
  while (1) {
    esp_task_wdt_reset();
    delay(1000);
  }
}

// ===================================================================
// NVS
// ===================================================================

void initNVS() {
  for (int i = 0; i < NVS_INIT_RETRY; i++) {
    if (preferences.begin(NVS_NAMESPACE, false)) {
      nvsInitialized = true;
      debugLog("NVS initialisé (tentative %d)", i + 1);
      return;
    }
    delay(100);
  }
  
  // Mode dégradé
  nvsDegraded = true;
  debugLog("ERREUR: NVS non disponible, mode dégradé");
}

void loadSettings() {
  if (!nvsInitialized) {
    // Valeurs par défaut
    settings.volume = VOL_DEFAULT;
    settings.source = SOURCE_BT;
    settings.balance = 0;
    settings.brightness = OLED_BRIGHTNESS_DEFAULT;
    settings.vuMeterEnabled = true;
    settings.sleepTimer = 0;
    settings.volumeLimit = VOL_MAX;
    settings.eq.bass = EQ_CENTER;
    settings.eq.mid = EQ_CENTER;
    settings.eq.treble = EQ_CENTER;
    settings.eq.preset = PRESET_FLAT;
    settings.eq.enabled = true;
    settings.eq.loudness = false;
    settings.eq.spatial = SPATIAL_OFF;
    settings.inputGain = 0;
    return;
  }
  
  settings.volume = preferences.getUChar("volume", VOL_DEFAULT);
  settings.source = preferences.getUChar("source", SOURCE_BT);
  settings.balance = preferences.getChar("balance", 0);
  settings.brightness = preferences.getUChar("bright", OLED_BRIGHTNESS_DEFAULT);
  settings.vuMeterEnabled = preferences.getBool("vumeter", true);
  settings.sleepTimer = preferences.getUChar("sleep", 0);
  settings.volumeLimit = preferences.getUChar("vollimit", VOL_MAX);
  settings.eq.bass = preferences.getUChar("eq_bass", EQ_CENTER);
  settings.eq.mid = preferences.getUChar("eq_mid", EQ_CENTER);
  settings.eq.treble = preferences.getUChar("eq_treb", EQ_CENTER);
  settings.eq.preset = preferences.getUChar("eq_preset", PRESET_FLAT);
  settings.eq.enabled = preferences.getBool("eq_en", true);
  settings.eq.loudness = preferences.getBool("loudness", false);
  settings.eq.spatial = preferences.getUChar("spatial", SPATIAL_OFF);
  settings.inputGain = preferences.getUChar("gain", 0);
  
  targetVolume = settings.volume;
  currentSource = settings.source;
  
  debugLog("Settings chargés");
}

void saveSettings() {
  if (!nvsInitialized || nvsDegraded) {
    debugLog("NVS indisponible, sauvegarde ignorée");
    return;
  }
  
  preferences.putUChar("volume", targetVolume);
  preferences.putUChar("source", currentSource);
  preferences.putChar("balance", settings.balance);
  preferences.putUChar("bright", settings.brightness);
  preferences.putBool("vumeter", settings.vuMeterEnabled);
  preferences.putUChar("sleep", settings.sleepTimer);
  preferences.putUChar("vollimit", settings.volumeLimit);
  preferences.putUChar("eq_bass", settings.eq.bass);
  preferences.putUChar("eq_mid", settings.eq.mid);
  preferences.putUChar("eq_treb", settings.eq.treble);
  preferences.putUChar("eq_preset", settings.eq.preset);
  preferences.putBool("eq_en", settings.eq.enabled);
  preferences.putBool("loudness", settings.eq.loudness);
  preferences.putUChar("spatial", settings.eq.spatial);
  preferences.putUChar("gain", settings.inputGain);
  
  lastSaveTime = millis();
  needsSave = false;
  debugLog("Settings sauvegardés");
}

void loadStats() {
  if (!nvsInitialized) {
    memset(&stats, 0, sizeof(stats));
    stats.sessionStart = millis();
    return;
  }
  
  stats.totalOnTime = preferences.getULong("totaltime", 0);
  stats.powerCycles = preferences.getUShort("cycles", 0);
  stats.errorCount = preferences.getUShort("errors", 0);
  stats.maxTempReached = preferences.getUChar("maxtemp", 0);
  
  stats.powerCycles++;
  stats.sessionStart = millis();
  
  debugLog("Stats chargées, cycle #%d", stats.powerCycles);
}

void saveStats() {
  if (!nvsInitialized || nvsDegraded) return;
  
  stats.totalOnTime += (millis() - stats.sessionStart) / 1000;
  
  preferences.putULong("totaltime", stats.totalOnTime);
  preferences.putUShort("cycles", stats.powerCycles);
  preferences.putUShort("errors", stats.errorCount);
  preferences.putUChar("maxtemp", stats.maxTempReached);
  
  debugLog("Stats sauvegardées");
}

// ===================================================================
// DISPLAY
// ===================================================================

void displayInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    debugLog("ERREUR: OLED non détecté!");
    return;
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
  debugLog("OLED initialisé");
}

void displaySplash() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 5);
  display.println("AUDIOPHILE");
  display.setTextSize(1);
  display.setCursor(30, 30);
  display.print("Portable Amp V");
  display.println(FW_VERSION);
  display.setCursor(20, 50);
  display.println("ChatGPT Audit V1.7");
  display.display();
  delay(2000);
}

void displayMain() {
  display.clearDisplay();
  
  // Header: Source + BT + Batterie
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(sourceIcons[currentSource]);
  
  if (currentSource == SOURCE_BT) {
    display.setCursor(20, 0);
    display.print(btConnected ? "OK" : "--");
  }
  
  // Batterie
  display.setCursor(90, 0);
  display.print(batteryPercent);
  display.print("%");
  if (batteryLow) display.print("!");
  
  // Volume central
  display.setTextSize(3);
  display.setCursor(40, 18);
  if (ampMuted) {
    display.print("MUTE");
  } else {
    int volDb = -currentVolume;  // TDA7439: 0 = 0dB, 47 = -47dB
    if (volDb == 0) {
      display.print(" 0dB");
    } else {
      display.print(volDb);
      display.print("dB");
    }
  }
  
  // Barre volume
  display.drawRect(0, 45, 128, 8, SSD1306_WHITE);
  int barWidth = map(currentVolume, 47, 0, 0, 126);
  display.fillRect(1, 46, barWidth, 6, SSD1306_WHITE);
  
  // Footer: EQ info
  display.setTextSize(1);
  display.setCursor(0, 56);
  if (eqChipPresent && settings.eq.enabled) {
    display.print("EQ:");
    display.print(presets[settings.eq.preset].name);
    if (settings.eq.loudness) {
      display.print(" L");
      if (loudnessAppliedBass > 0) {
        display.print("+");
      }
    }
  } else if (!eqChipPresent) {
    display.print("EQ: N/A");
  } else {
    display.print("EQ: OFF");
  }
  
  // Alarmes
  if (thermalThrottle && (millis() / 500) % 2) {
    display.setCursor(90, 56);
    display.print("TEMP!");
  }
  
  if (i2cAlarm && (millis() / 500) % 2) {
    display.setCursor(90, 56);
    display.print("I2C!");
  }
  
  if (nvsDegraded) {
    display.setCursor(70, 56);
    display.print("NVS!");
  }
  
  display.display();
}

void updateDisplay() {
  if (millis() - lastDisplayUpdate < DISPLAY_REFRESH) return;
  lastDisplayUpdate = millis();
  
  switch (menuState) {
    case MENU_MAIN:
      displayMain();
      break;
    default:
      displayMain();
      break;
  }
}

// ===================================================================
// VU-MÈTRE
// ===================================================================

void updateVUMeter() {
  if (!settings.vuMeterEnabled) return;
  
  uint16_t rawL = readADCFiltered(PIN_ADC_AUDIO_L);
  uint16_t rawR = readADCFiltered(PIN_ADC_AUDIO_R);
  
  rawL = constrain(rawL, 0, ADC_MAX_VALID);
  rawR = constrain(rawR, 0, ADC_MAX_VALID);
  
  uint8_t levelL = map(rawL, 0, 4095, 0, 100);
  uint8_t levelR = map(rawR, 0, 4095, 0, 100);
  
  vuMeter.levelL = max(vuMeter.levelL - VU_DECAY_RATE, (int)levelL);
  vuMeter.levelR = max(vuMeter.levelR - VU_DECAY_RATE, (int)levelR);
  
  if (levelL > vuMeter.peakL) {
    vuMeter.peakL = levelL;
    vuMeter.peakHoldTimeL = millis();
  } else if (millis() - vuMeter.peakHoldTimeL > 1000) {
    vuMeter.peakL = max(vuMeter.peakL - 1, 0);
  }
  
  if (levelR > vuMeter.peakR) {
    vuMeter.peakR = levelR;
    vuMeter.peakHoldTimeR = millis();
  } else if (millis() - vuMeter.peakHoldTimeR > 1000) {
    vuMeter.peakR = max(vuMeter.peakR - 1, 0);
  }
}

// ===================================================================
// VOLUME FADE
// ===================================================================

void updateVolumeFade() {
  static uint32_t lastFade = 0;
  
  if (millis() - lastFade < VOL_FADE_DELAY) return;
  lastFade = millis();
  
  if (currentVolume < targetVolume) {
    currentVolume = min(currentVolume + VOL_FADE_STEP, (int)targetVolume);
    tda7439SetVolume(currentVolume);
    eqApplyWithLoudness();
  } else if (currentVolume > targetVolume) {
    currentVolume = max(currentVolume - VOL_FADE_STEP, (int)targetVolume);
    tda7439SetVolume(currentVolume);
    eqApplyWithLoudness();
  }
}

// ===================================================================
// ENCODEUR HANDLER
// ===================================================================

void handleEncoder() {
  // Lecture atomique delta
  portENTER_CRITICAL(&encoderMux);
  int32_t delta = encoderDelta;
  encoderDelta = 0;
  bool changed = volumeChanged;
  volumeChanged = false;
  portEXIT_CRITICAL(&encoderMux);
  
  // Vérification saturation
  if (abs(delta) >= ENCODER_MAX_DELTA) {
    stats.encoderSpamFiltered++;
    debugLog("Encodeur spam filtré (delta=%d)", delta);
  }
  
  if (delta != 0 && menuState == MENU_MAIN && !ampMuted) {
    targetVolume = constrain(targetVolume - delta, 0, settings.volumeLimit);
    needsSave = true;
    lastActivityTime = millis();
  }
  
  // Bouton
  if (encoderButtonPressed) {
    encoderButtonPressed = false;
    
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
      buttonHandled = false;
    }
  }
  
  if (buttonPressTime > 0 && digitalRead(PIN_ENC_SW) == HIGH) {
    uint32_t pressDuration = millis() - buttonPressTime;
    buttonPressTime = 0;
    
    if (!buttonHandled) {
      if (pressDuration >= VERYLONGPRESS_MS) {
        handleVeryLongPress();
      } else if (pressDuration >= LONGPRESS_MS) {
        handleLongPress();
      } else {
        handleShortPress();
      }
    }
  }
}

void handleShortPress() {
  lastActivityTime = millis();
  
  if (menuState == MENU_MAIN) {
    ampToggleMute();
  }
}

void handleLongPress() {
  lastActivityTime = millis();
  
  if (menuState == MENU_MAIN) {
    currentSource = (currentSource + 1) % SOURCE_COUNT;
    setSource(currentSource);
    needsSave = true;
  }
}

void handleVeryLongPress() {
  lastActivityTime = millis();
  debugMode = !debugMode;
  debugLog("Mode debug %s", debugMode ? "ON" : "OFF");
}

// ===================================================================
// SOURCE
// ===================================================================

void setSource(uint8_t src) {
  currentSource = src % SOURCE_COUNT;
  settings.source = currentSource;
  
  switch (currentSource) {
    case SOURCE_BT:
      digitalWrite(PIN_SRC_SEL0, LOW);
      digitalWrite(PIN_SRC_SEL1, LOW);
      tda7439SetInput(0);
      break;
    case SOURCE_AUX:
      digitalWrite(PIN_SRC_SEL0, HIGH);
      digitalWrite(PIN_SRC_SEL1, LOW);
      tda7439SetInput(0);
      break;
    case SOURCE_PHONO:
      digitalWrite(PIN_SRC_SEL0, LOW);
      digitalWrite(PIN_SRC_SEL1, HIGH);
      tda7439SetInput(1);
      break;
  }
  
  debugLog("Source: %s", sourceNames[currentSource]);
}

// ===================================================================
// IR
// ===================================================================

void irInit() {
  irrecv.enableIRIn();
  debugLog("IR initialisé");
}

void handleIR() {
  static uint32_t lastIRCode = 0;
  static uint32_t lastIRTime = 0;
  
  if (irrecv.decode(&irResults)) {
    uint32_t code = irResults.value;
    
    // Anti-rebond IR
    if (code == lastIRCode && millis() - lastIRTime < 200) {
      irrecv.resume();
      return;
    }
    
    lastIRCode = code;
    lastIRTime = millis();
    lastActivityTime = millis();
    
    switch (code) {
      case IR_CODE_POWER:
        if (ampEnabled) ampEnable(false);
        else ampEnable(true);
        break;
      case IR_CODE_MUTE:
        ampToggleMute();
        break;
      case IR_CODE_VOL_UP:
        if (!ampMuted) {
          targetVolume = constrain(targetVolume - VOL_STEP_IR, 0, settings.volumeLimit);
          needsSave = true;
        }
        break;
      case IR_CODE_VOL_DOWN:
        if (!ampMuted) {
          targetVolume = constrain(targetVolume + VOL_STEP_IR, 0, VOL_MAX);
          needsSave = true;
        }
        break;
      case IR_CODE_SOURCE:
        currentSource = (currentSource + 1) % SOURCE_COUNT;
        setSource(currentSource);
        needsSave = true;
        break;
    }
    
    irrecv.resume();
  }
}

// ===================================================================
// MCP4261 (BACKUP)
// ===================================================================

void MCP4261_init() {
  vspi = new SPIClass(FSPI);
  vspi->begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS_VOL);
  pinMode(PIN_SPI_CS_VOL, OUTPUT);
  digitalWrite(PIN_SPI_CS_VOL, HIGH);
}

// ===================================================================
// SERIAL COMMANDS
// ===================================================================

void handleSerialCommand() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  
  if (cmd == "help") {
    Serial.println("Commandes: vol, src, eq, stats, save, reset, test");
  } else if (cmd == "stats") {
    Serial.println("=== STATS V1.7 ===");
    Serial.print("Uptime: ");
    Serial.print((millis() - stats.sessionStart) / 1000);
    Serial.println("s");
    Serial.print("Total: ");
    Serial.print(stats.totalOnTime / 3600);
    Serial.println("h");
    Serial.print("I2C errors: ");
    Serial.print(stats.i2cErrors);
    Serial.print(" retries: ");
    Serial.print(stats.i2cRetries);
    Serial.print(" recoveries: ");
    Serial.println(stats.i2cRecoveries);
    Serial.print("ADC spikes: ");
    Serial.println(stats.adcSpikesFiltered);
    Serial.print("NTC errors: ");
    Serial.println(stats.ntcErrors);
    Serial.print("Encoder spam: ");
    Serial.println(stats.encoderSpamFiltered);
    Serial.print("Brownout warnings: ");
    Serial.println(stats.brownoutWarnings);
    Serial.print("NVS: ");
    Serial.println(nvsDegraded ? "DEGRADED" : "OK");
  } else if (cmd == "save") {
    saveSettings();
    saveStats();
    Serial.println("Sauvegardé!");
  }
}

// ===================================================================
// TIMERS
// ===================================================================

void checkMenuTimeout() {
  if (menuState != MENU_MAIN && millis() - menuEntryTime > MENU_TIMEOUT) {
    menuState = MENU_MAIN;
  }
}

void checkSleepTimer() {
  if (settings.sleepTimer == 0) return;
  
  uint32_t sleepMs = settings.sleepTimer * 15 * 60 * 1000UL;
  if (millis() - sleepTimerStart > sleepMs) {
    emergencyShutdown("SLEEP");
  }
}

void checkAutoSleep() {
  if (millis() - lastActivityTime > AUTOSLEEP_TIMEOUT) {
    emergencyShutdown("INACTIF");
  }
}

void checkAutoSave() {
  if (needsSave && millis() - lastSaveTime > SAVE_DELAY) {
    saveSettings();
  }
}

// ===================================================================
// SETUP
// ===================================================================

void setup() {
  // Watchdog 5s
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("================================");
  Serial.print("AMPLI AUDIOPHILE V"); Serial.println(FW_VERSION);
  Serial.println("AUDIT CHATGPT - CORRECTIONS HW");
  Serial.println("================================");
  
  debugMode = true;
  
  // GPIO
  pinMode(PIN_AMP_EN, OUTPUT);
  pinMode(PIN_AMP_MUTE, OUTPUT);
  pinMode(PIN_SRC_SEL0, OUTPUT);
  pinMode(PIN_SRC_SEL1, OUTPUT);
  pinMode(PIN_SAFE_EN, OUTPUT);
  pinMode(PIN_SPI_CS_VOL, OUTPUT);
  
  digitalWrite(PIN_AMP_EN, HIGH);     // Ampli OFF
  digitalWrite(PIN_AMP_MUTE, LOW);    // Mute ON
  digitalWrite(PIN_SAFE_EN, HIGH);    // Batterie déconnectée
  digitalWrite(PIN_SPI_CS_VOL, HIGH);
  
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_AMP_ERR, INPUT_PULLUP);
  pinMode(PIN_BT_STATUS, INPUT);
  pinMode(PIN_IR_RX, INPUT);
  
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  // [C2] V1.7: I2C Bus Recovery AVANT Wire.begin()
  i2cBusRecovery();
  
  // I2C
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(I2C_TIMEOUT_MS);  // Timeout anti-blocage
  
  // SPI (backup)
  MCP4261_init();
  
  // OLED
  displayInit();
  
  // NVS avec gestion erreur
  initNVS();
  
  // Config
  loadSettings();
  loadStats();
  
  // Splash
  displaySplash();
  
  // TDA7439 EQ
  tda7439Init();
  
  // IR
  irInit();
  
  // Batterie
  batteryConnect(true);
  delay(100);
  checkBattery();
  
  if (batteryRaw < BATT_SHUTDOWN) {
    emergencyShutdown("BATT VIDE");
  }
  
  // Ampli MA12070
  ampInit();
  
  // Interruptions
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), buttonISR, FALLING);
  
  // Démarrage ampli
  setSource(currentSource);
  ampEnable(true);
  
  if (settings.eq.enabled && eqChipPresent) {
    eqApplyWithLoudness();
    spatialApply();
  }
  
  lastActivityTime = millis();
  sleepTimerStart = millis();
  systemReady = true;
  
  debugLog("Système prêt! V1.7 Audit ChatGPT");
  Serial.println("Tapez 'help' pour les commandes");
  Serial.println();
}

// ===================================================================
// LOOP
// ===================================================================

void loop() {
  esp_task_wdt_reset();
  loopCounter++;
  
  handleSerialCommand();
  handleIR();
  handleEncoder();
  updateVolumeFade();
  updateVUMeter();
  updateMonitoring();
  updateDisplay();
  checkMenuTimeout();
  checkSleepTimer();
  checkAutoSleep();
  checkAutoSave();
  
  delay(1);
}

// ===================================================================
// FIN DU CODE V1.7 - AUDIT CHATGPT CORRECTIONS CRITIQUES
// ===================================================================
