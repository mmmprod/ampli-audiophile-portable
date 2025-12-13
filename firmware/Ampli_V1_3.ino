/*
 * ===================================================================
 * AMPLIFICATEUR AUDIOPHILE PORTABLE - FIRMWARE ESP32-S3
 * ===================================================================
 * 
 * Version  : 1.3
 * Date     : 13 decembre 2025
 * Auteur   : Mehdi + Claude
 * Board    : ESP32-S3-WROOM-1-N8R8
 * Framework: Arduino (ESP32 Core 2.0+)
 * 
 * CHANGELOG V1.3:
 *   - Support TDA7439 DIP-30 (remplace PT2314)
 *   - EQ 3 bandes : Bass / Mid / Treble (+/-14dB)
 *   - Loudness automatique (boost bass a faible volume)
 *   - Effet Spatial/Surround virtuel
 *   - 8 presets sonores (ajout Cinema, Live, Custom2)
 *   - Menu EQ etendu avec visualisation graphique
 *   - Commandes serie etendues
 * 
 * HARDWARE REQUIS:
 *   - TDA7439 DIP-30 sur bus I2C (SDA/SCL)
 *   - Condensateurs externes pour frequences de coupure
 *   - Voir schema pour composants passifs
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

// ===================================================================
// VERSION ET IDENTIFICATION
// ===================================================================

#define FW_VERSION      "1.3"
#define FW_DATE         "2025-12-13"
#define DEVICE_NAME     "AmpliAudio"

// ===================================================================
// CONFIGURATION PINS GPIO
// ===================================================================

// --- I2C (MA12070 + OLED + TDA7439) ---
#define PIN_SDA         1
#define PIN_SCL         2

// --- SPI (MCP4261 Volume) ---
#define PIN_SPI_CS_VOL  10
#define PIN_SPI_MOSI    11
#define PIN_SPI_CLK     12
#define PIN_SPI_MISO    13

// --- Encodeur rotatif ---
#define PIN_ENC_A       18
#define PIN_ENC_B       19
#define PIN_ENC_SW      20

// --- Controle ampli ---
#define PIN_AMP_EN      15
#define PIN_AMP_MUTE    16
#define PIN_AMP_ERR     17

// --- Selection source ---
#define PIN_SRC_SEL0    5
#define PIN_SRC_SEL1    6

// --- Bluetooth ---
#define PIN_BT_STATUS   4
#define PIN_BT_RESET    7

// --- Securite batterie ---
#define PIN_SAFE_EN     42

// --- ADC monitoring ---
#define PIN_ADC_BATT    38
#define PIN_ADC_NTC     39
#define PIN_ADC_AUDIO_L 40
#define PIN_ADC_AUDIO_R 41

// --- IR recepteur ---
#define PIN_IR_RX       21

// --- LED status ---
#define PIN_LED_STATUS  48

// ===================================================================
// CONFIGURATION PERIPHERIQUES
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
#define TDA7439_INPUT_SEL       0x00    // Input selector + input gain
#define TDA7439_INPUT_GAIN      0x01    // Input gain
#define TDA7439_VOLUME          0x02    // Volume control
#define TDA7439_BASS            0x03    // Bass control
#define TDA7439_MID             0x04    // Mid-range control
#define TDA7439_TREBLE          0x05    // Treble control
#define TDA7439_SPKR_ATT_R      0x06    // Speaker attenuation Right
#define TDA7439_SPKR_ATT_L      0x07    // Speaker attenuation Left

// Input selector values (0-3 pour IN1-IN4)
#define TDA7439_IN1             0x03
#define TDA7439_IN2             0x02
#define TDA7439_IN3             0x01
#define TDA7439_IN4             0x00

// --- MCP4261 ---
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
// Valeurs: 0-14 correspondent a -14dB to +14dB (pas de 2dB)
// 7 = 0dB (flat)
#define EQ_MIN          0       // -14dB
#define EQ_MAX          14      // +14dB
#define EQ_CENTER       7       // 0dB (flat)
#define EQ_STEP         1       // 2dB par pas

// --- Loudness ---
#define LOUDNESS_THRESHOLD  15  // Volume sous lequel loudness s'active
#define LOUDNESS_BASS_BOOST 3   // +6dB bass boost
#define LOUDNESS_MID_CUT    1   // -2dB mid cut

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

// --- Seuils batterie ---
#define BATT_FULL       3723
#define BATT_NOMINAL    3350
#define BATT_LOW        2976
#define BATT_CRITICAL   2604
#define BATT_SHUTDOWN   2380

// --- Seuils temperature ---
#define TEMP_NORMAL     2200
#define TEMP_WARN       1800
#define TEMP_THROTTLE   1500
#define TEMP_CRITICAL   1200
#define TEMP_SHUTDOWN   1000

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
#define WDT_TIMEOUT         10

// ===================================================================
// CODES IR TELECOMMANDE
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
#define IR_CODE_LOUD        0x00FF32CD  // Nouveau: Loudness toggle
#define IR_CODE_SPATIAL     0x00FF52AD  // Nouveau: Spatial toggle

// ===================================================================
// OBJETS GLOBAUX
// ===================================================================

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Preferences preferences;
SPIClass *vspi = NULL;
IRrecv irrecv(PIN_IR_RX);
decode_results irResults;

// ===================================================================
// STRUCTURES DE DONNEES
// ===================================================================

// --- Egaliseur 3 bandes ---
struct Equalizer {
  uint8_t bass;       // 0-14 (-14dB to +14dB)
  uint8_t mid;        // 0-14 (-14dB to +14dB) - NOUVEAU V1.3
  uint8_t treble;     // 0-14 (-14dB to +14dB)
  uint8_t preset;     // Index preset actif
  bool enabled;       // EQ actif/bypass
  bool loudness;      // Loudness actif - NOUVEAU V1.3
  uint8_t spatial;    // Niveau spatial 0-3 - NOUVEAU V1.3
};

// --- Definition preset (3 bandes) ---
struct PresetDef {
  const char* name;
  uint8_t bass;       // 0-14
  uint8_t mid;        // 0-14
  uint8_t treble;     // 0-14
};

// --- Configuration sauvegardee ---
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
  uint8_t inputGain;  // Gain entree TDA7439 (0-15)
};

// --- Statistiques ---
struct Stats {
  uint32_t totalOnTime;
  uint32_t sessionStart;
  uint16_t powerCycles;
  uint16_t errorCount;
  uint8_t maxTempReached;
};

// --- VU-metre ---
struct VUMeter {
  uint8_t levelL;
  uint8_t levelR;
  uint8_t peakL;
  uint8_t peakR;
  uint32_t peakHoldTimeL;
  uint32_t peakHoldTimeR;
};

// --- Animation ---
struct Animation {
  uint8_t type;
  uint8_t frame;
  uint32_t lastFrame;
  bool active;
};

// ===================================================================
// PRESETS EGALISEUR 3 BANDES
// ===================================================================

// Format: {nom, bass (0-14), mid (0-14), treble (0-14)}
// 7 = 0dB (flat), 0 = -14dB, 14 = +14dB
const PresetDef presets[PRESET_COUNT] = {
  {"Flat",     7,  7,  7},   // 0dB / 0dB / 0dB
  {"Bass+",   12,  6,  7},   // +10dB / -2dB / 0dB
  {"Vocal",    5,  9, 10},   // -4dB / +4dB / +6dB
  {"Rock",    10,  7, 10},   // +6dB / 0dB / +6dB
  {"Jazz",     9,  8,  9},   // +4dB / +2dB / +4dB
  {"Cinema",  11,  7,  8},   // +8dB / 0dB / +2dB (sub boost)
  {"Live",     8,  9,  9},   // +2dB / +4dB / +4dB (presence)
  {"Custom",   7,  7,  7}    // Reglable par user
};

// ===================================================================
// VARIABLES D'ETAT
// ===================================================================

// --- Configuration ---
Settings settings;
Stats stats;

// --- EQ ---
bool eqChipPresent = false;   // TDA7439 detecte?

// --- Volume ---
volatile int16_t targetVolume = VOL_DEFAULT;
int16_t currentVolume = 0;
volatile bool volumeChanged = false;

// --- Loudness state ---
uint8_t loudnessAppliedBass = 0;
uint8_t loudnessAppliedMid = 0;

// --- Source ---
uint8_t currentSource = SOURCE_BT;
const char* sourceNames[] = {"Bluetooth", "AUX", "Phono"};
const char* sourceIcons[] = {"BT", "AX", "PH"};

// --- Encodeur ---
volatile int32_t encoderDelta = 0;
volatile bool encoderButtonPressed = false;
volatile uint32_t lastEncoderTime = 0;
uint32_t buttonPressTime = 0;
bool buttonHandled = false;

// --- Etats systeme ---
bool ampEnabled = false;
bool ampMuted = true;
bool btConnected = false;
bool btPairing = false;
bool systemReady = false;
bool debugMode = false;
bool testMode = false;

// --- Monitoring ---
uint16_t batteryRaw = 0;
uint16_t tempRaw = 0;
float batteryVoltage = 0;
uint8_t batteryPercent = 0;
bool batteryLow = false;
bool batteryCharging = false;
bool tempWarning = false;
bool thermalThrottle = false;

// --- VU-metre ---
VUMeter vuMeter = {0, 0, 0, 0, 0, 0};

// --- Animation ---
Animation animation = {0, 0, 0, false};

// --- Menu ---
enum MenuState {
  MENU_MAIN,
  MENU_SOURCE,
  MENU_BALANCE,
  MENU_EQ,
  MENU_EQ_BASS,
  MENU_EQ_MID,          // NOUVEAU V1.3
  MENU_EQ_TREBLE,
  MENU_EQ_PRESET,
  MENU_EQ_LOUDNESS,     // NOUVEAU V1.3
  MENU_EQ_SPATIAL,      // NOUVEAU V1.3
  MENU_SETTINGS,
  MENU_SETTINGS_BRIGHTNESS,
  MENU_SETTINGS_SLEEP,
  MENU_SETTINGS_VOLLIMIT,
  MENU_SETTINGS_VUMETER,
  MENU_SETTINGS_GAIN,   // NOUVEAU V1.3
  MENU_INFO,
  MENU_TEST
};
MenuState menuState = MENU_MAIN;
uint8_t menuSelection = 0;
uint32_t menuEntryTime = 0;

// --- Timing ---
uint32_t lastDisplayUpdate = 0;
uint32_t lastADCRead = 0;
uint32_t lastSaveTime = 0;
uint32_t lastActivityTime = 0;
uint32_t sleepTimerStart = 0;
bool needsSave = false;

// --- Debug ---
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

// ===================================================================
// INTERRUPTIONS
// ===================================================================

void IRAM_ATTR encoderISR() {
  static uint8_t lastState = 0;
  uint8_t state = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  
  static const int8_t encTable[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  
  uint8_t index = (lastState << 2) | state;
  int8_t delta = encTable[index];
  
  if (delta != 0) {
    uint32_t now = millis();
    if (now - lastEncoderTime > 2) {
      encoderDelta += delta;
      volumeChanged = true;
      lastEncoderTime = now;
    }
  }
  
  lastState = state;
}

void IRAM_ATTR buttonISR() {
  static uint32_t lastPress = 0;
  uint32_t now = millis();
  
  if (now - lastPress > DEBOUNCE_MS) {
    encoderButtonPressed = true;
    lastPress = now;
  }
}

// ===================================================================
// FONCTIONS TDA7439
// ===================================================================

bool tda7439Detect() {
  Wire.beginTransmission(TDA7439_ADDR);
  uint8_t error = Wire.endTransmission();
  
  if (error == 0) {
    debugLog("TDA7439 detecte @ 0x%02X", TDA7439_ADDR);
    return true;
  } else {
    debugLog("TDA7439 NON detecte (EQ desactive)");
    return false;
  }
}

void tda7439Write(uint8_t subAddr, uint8_t data) {
  Wire.beginTransmission(TDA7439_ADDR);
  Wire.write(subAddr & 0x0F);  // Sub-address (4 bits)
  Wire.write(data);
  Wire.endTransmission();
}

void tda7439Init() {
  eqChipPresent = tda7439Detect();
  
  if (eqChipPresent) {
    // Input selector: IN1 par defaut
    tda7439Write(TDA7439_INPUT_SEL, TDA7439_IN1);
    
    // Input gain: 0dB par defaut (valeur 0)
    tda7439Write(TDA7439_INPUT_GAIN, settings.inputGain);
    
    // Volume: valeur par defaut
    tda7439Write(TDA7439_VOLUME, settings.volume);
    
    // EQ flat par defaut
    tda7439Write(TDA7439_BASS, 7);    // 0dB
    tda7439Write(TDA7439_MID, 7);     // 0dB
    tda7439Write(TDA7439_TREBLE, 7);  // 0dB
    
    // Speaker attenuation: 0dB (pas d'attenuation)
    tda7439Write(TDA7439_SPKR_ATT_L, 0);
    tda7439Write(TDA7439_SPKR_ATT_R, 0);
    
    // Appliquer preset sauvegarde
    eqApplyPreset(settings.eq.preset);
    
    debugLog("TDA7439 initialise");
  }
}

void tda7439SetInput(uint8_t input) {
  // Input: 0-3 pour IN1-IN4
  // Valeurs inversees dans le registre (IN1=3, IN4=0)
  if (!eqChipPresent) return;
  
  uint8_t regVal = 3 - (input & 0x03);
  tda7439Write(TDA7439_INPUT_SEL, regVal);
  
  debugLog("TDA7439 Input: %d", input + 1);
}

void tda7439SetInputGain(uint8_t gain) {
  // Gain: 0-15 (0dB to 30dB, pas de 2dB)
  if (!eqChipPresent) return;
  
  gain = constrain(gain, 0, 15);
  tda7439Write(TDA7439_INPUT_GAIN, gain);
  settings.inputGain = gain;
  
  debugLog("TDA7439 Gain: +%ddB", gain * 2);
}

void tda7439SetVolume(uint8_t vol) {
  // Volume: 0-47 (0dB to -47dB, pas de 1dB)
  // 48 = mute
  if (!eqChipPresent) return;
  
  vol = constrain(vol, 0, 48);
  tda7439Write(TDA7439_VOLUME, vol);
  
  if (vol == 48) {
    debugLog("TDA7439 Volume: MUTE");
  } else {
    debugLog("TDA7439 Volume: -%ddB", vol);
  }
}

void tda7439SetBass(uint8_t value) {
  // Bass: 0-14 (-14dB to +14dB)
  // Registre: 0=+14dB, 7=0dB, 14=-14dB (inverse!)
  if (!eqChipPresent) return;
  
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;  // Inverser
  tda7439Write(TDA7439_BASS, regVal);
  
  settings.eq.bass = value;
  debugLog("Bass: %+ddB", ((int)value - 7) * 2);
}

void tda7439SetMid(uint8_t value) {
  // Mid: 0-14 (-14dB to +14dB)
  if (!eqChipPresent) return;
  
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;
  tda7439Write(TDA7439_MID, regVal);
  
  settings.eq.mid = value;
  debugLog("Mid: %+ddB", ((int)value - 7) * 2);
}

void tda7439SetTreble(uint8_t value) {
  // Treble: 0-14 (-14dB to +14dB)
  if (!eqChipPresent) return;
  
  value = constrain(value, EQ_MIN, EQ_MAX);
  uint8_t regVal = 14 - value;
  tda7439Write(TDA7439_TREBLE, regVal);
  
  settings.eq.treble = value;
  debugLog("Treble: %+ddB", ((int)value - 7) * 2);
}

void tda7439SetSpeakerAtt(uint8_t left, uint8_t right) {
  // Attenuation: 0-79 (0dB to -79dB)
  // Utilise pour balance et spatial
  if (!eqChipPresent) return;
  
  left = constrain(left, 0, 79);
  right = constrain(right, 0, 79);
  
  tda7439Write(TDA7439_SPKR_ATT_L, left);
  tda7439Write(TDA7439_SPKR_ATT_R, right);
}

// ===================================================================
// FONCTIONS EGALISEUR
// ===================================================================

void eqApply() {
  if (!eqChipPresent) return;
  
  tda7439SetBass(settings.eq.bass);
  tda7439SetMid(settings.eq.mid);
  tda7439SetTreble(settings.eq.treble);
  
  debugLog("EQ applique: B%+d M%+d T%+d", 
           ((int)settings.eq.bass - 7) * 2,
           ((int)settings.eq.mid - 7) * 2, 
           ((int)settings.eq.treble - 7) * 2);
}

void eqApplyWithLoudness() {
  if (!eqChipPresent) return;
  
  uint8_t bass = settings.eq.bass;
  uint8_t mid = settings.eq.mid;
  uint8_t treble = settings.eq.treble;
  
  // Appliquer loudness si actif et volume bas
  if (settings.eq.loudness && currentVolume > LOUDNESS_THRESHOLD) {
    // Volume bas = index eleve (inverse)
    // Calculer boost proportionnel au volume
    uint8_t loudnessLevel = (currentVolume - LOUDNESS_THRESHOLD) / 5;
    loudnessLevel = constrain(loudnessLevel, 0, LOUDNESS_BASS_BOOST);
    
    // Boost bass
    bass = constrain(bass + loudnessLevel, EQ_MIN, EQ_MAX);
    // Leger cut mid pour plus de clarte
    if (loudnessLevel > 1) {
      mid = constrain(mid - 1, EQ_MIN, EQ_MAX);
    }
    
    loudnessAppliedBass = loudnessLevel;
    loudnessAppliedMid = (loudnessLevel > 1) ? 1 : 0;
  } else {
    loudnessAppliedBass = 0;
    loudnessAppliedMid = 0;
  }
  
  // Appliquer avec loudness
  if (!settings.eq.enabled) {
    // Bypass: flat
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
  
  // Custom garde les valeurs actuelles
  if (presetIndex != PRESET_CUSTOM) {
    settings.eq.bass = presets[presetIndex].bass;
    settings.eq.mid = presets[presetIndex].mid;
    settings.eq.treble = presets[presetIndex].treble;
  }
  
  eqApplyWithLoudness();
  
  needsSave = true;
  lastSaveTime = millis();
  
  debugLog("Preset: %s", presets[presetIndex].name);
}

void eqNextPreset() {
  uint8_t next = (settings.eq.preset + 1) % PRESET_COUNT;
  eqApplyPreset(next);
}

void eqToggle() {
  settings.eq.enabled = !settings.eq.enabled;
  eqApplyWithLoudness();
  
  needsSave = true;
  lastSaveTime = millis();
  
  debugLog("EQ: %s", settings.eq.enabled ? "ON" : "OFF (bypass)");
}

void loudnessToggle() {
  settings.eq.loudness = !settings.eq.loudness;
  eqApplyWithLoudness();
  
  needsSave = true;
  lastSaveTime = millis();
  
  debugLog("Loudness: %s", settings.eq.loudness ? "ON" : "OFF");
}

// ===================================================================
// FONCTIONS SPATIAL / SURROUND
// ===================================================================

void spatialApply() {
  if (!eqChipPresent) return;
  
  // Effet spatial via difference d'attenuation L/R
  // Cree une impression de largeur stereo
  // Note: Pour vrai surround, il faudrait un DSP avec delay
  
  uint8_t attL = 0;
  uint8_t attR = 0;
  
  // Appliquer balance d'abord
  if (settings.balance < 0) {
    attR = abs(settings.balance);
  } else if (settings.balance > 0) {
    attL = settings.balance;
  }
  
  // Ajouter effet spatial (micro-differences)
  // Plus le niveau est eleve, plus la difference est marquee
  switch (settings.eq.spatial) {
    case SPATIAL_OFF:
      // Pas d'effet
      break;
    case SPATIAL_LIGHT:
      // Leger: +1dB difference
      attL += 1;
      break;
    case SPATIAL_MEDIUM:
      // Medium: +2dB difference alternee
      attL += 1;
      attR += 1;
      break;
    case SPATIAL_STRONG:
      // Fort: +3dB avec inversion partielle
      attL += 2;
      attR += 1;
      break;
  }
  
  tda7439SetSpeakerAtt(attL, attR);
  
  debugLog("Spatial: %d (L:%d R:%d)", settings.eq.spatial, attL, attR);
}

void spatialNext() {
  settings.eq.spatial = (settings.eq.spatial + 1) % (SPATIAL_MAX + 1);
  spatialApply();
  
  needsSave = true;
  lastSaveTime = millis();
}

const char* spatialName(uint8_t level) {
  switch (level) {
    case SPATIAL_OFF:    return "OFF";
    case SPATIAL_LIGHT:  return "Light";
    case SPATIAL_MEDIUM: return "Medium";
    case SPATIAL_STRONG: return "Wide";
    default:             return "?";
  }
}

// ===================================================================
// FONCTIONS UTILITAIRES EQ
// ===================================================================

// Convertit valeur EQ (0-14) en string dB
const char* eqValueToString(uint8_t value) {
  static char buf[8];
  int8_t db = ((int8_t)value - 7) * 2;  // Pas de 2dB
  if (db > 0) {
    sprintf(buf, "+%ddB", db);
  } else if (db < 0) {
    sprintf(buf, "%ddB", db);
  } else {
    sprintf(buf, "0dB");
  }
  return buf;
}

// ===================================================================
// FONCTIONS MCP4261 (VOLUME SPI - backup)
// ===================================================================

void MCP4261_init() {
  vspi = new SPIClass(HSPI);
  vspi->begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS_VOL);
  pinMode(PIN_SPI_CS_VOL, OUTPUT);
  digitalWrite(PIN_SPI_CS_VOL, HIGH);
  
  debugLog("MCP4261 initialise");
}

void MCP4261_write(uint8_t pot, uint8_t value) {
  uint16_t command = ((pot & 0x0F) << 12) | (value & 0x1FF);
  
  vspi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_SPI_CS_VOL, LOW);
  delayMicroseconds(1);
  vspi->transfer16(command);
  digitalWrite(PIN_SPI_CS_VOL, HIGH);
  vspi->endTransaction();
}

void setVolumeMCP(uint8_t left, uint8_t right) {
  // Utilise uniquement si TDA7439 absent
  MCP4261_write(MCP4261_POT0, left);
  MCP4261_write(MCP4261_POT1, right);
}

// ===================================================================
// FONCTIONS VOLUME
// ===================================================================

void setVolume(uint8_t vol) {
  if (eqChipPresent) {
    // Utiliser volume TDA7439
    tda7439SetVolume(vol);
  } else {
    // Fallback sur MCP4261
    uint8_t mcpVol = map(vol, 0, 47, 255, 0);
    setVolumeMCP(mcpVol, mcpVol);
  }
  currentVolume = vol;
}

void updateVolumeWithBalance() {
  if (eqChipPresent) {
    // Balance via attenuation speaker TDA7439
    spatialApply();  // Inclut balance
    tda7439SetVolume(currentVolume);
  } else {
    // Balance via MCP4261
    uint8_t leftVol = map(currentVolume, 0, 47, 255, 0);
    uint8_t rightVol = leftVol;
    
    if (settings.balance < 0) {
      rightVol = rightVol * (10 + settings.balance) / 10;
    } else if (settings.balance > 0) {
      leftVol = leftVol * (10 - settings.balance) / 10;
    }
    
    setVolumeMCP(leftVol, rightVol);
  }
  
  // Recalculer loudness si volume change
  if (settings.eq.loudness) {
    eqApplyWithLoudness();
  }
}

void fadeToVolume(int16_t target, bool blocking) {
  if (blocking) {
    while (currentVolume != target) {
      if (currentVolume < target) {
        currentVolume = min(currentVolume + VOL_FADE_STEP, target);
      } else {
        currentVolume = max(currentVolume - VOL_FADE_STEP, target);
      }
      setVolume(currentVolume);
      delay(VOL_FADE_DELAY);
    }
  } else {
    targetVolume = target;
  }
}

void updateVolumeFade() {
  static uint32_t lastFade = 0;
  
  if (millis() - lastFade >= VOL_FADE_DELAY) {
    lastFade = millis();
    
    if (currentVolume != targetVolume) {
      if (currentVolume < targetVolume) {
        currentVolume = min(currentVolume + VOL_FADE_STEP, (int16_t)targetVolume);
      } else {
        currentVolume = max(currentVolume - VOL_FADE_STEP, (int16_t)targetVolume);
      }
      setVolume(currentVolume);
      updateVolumeWithBalance();
    }
  }
}

// ===================================================================
// FONCTIONS SELECTION SOURCE
// ===================================================================

void setSource(uint8_t source) {
  settings.volumePerSource[currentSource] = targetVolume;
  
  currentSource = source % SOURCE_COUNT;
  
  // Selecteur analogique CD4053
  switch (currentSource) {
    case SOURCE_BT:
      digitalWrite(PIN_SRC_SEL0, LOW);
      digitalWrite(PIN_SRC_SEL1, LOW);
      break;
    case SOURCE_AUX:
      digitalWrite(PIN_SRC_SEL0, HIGH);
      digitalWrite(PIN_SRC_SEL1, LOW);
      break;
    case SOURCE_PHONO:
      digitalWrite(PIN_SRC_SEL0, LOW);
      digitalWrite(PIN_SRC_SEL1, HIGH);
      break;
  }
  
  // Selecteur TDA7439 (si present)
  if (eqChipPresent) {
    tda7439SetInput(currentSource);
  }
  
  targetVolume = settings.volumePerSource[currentSource];
  
  settings.source = currentSource;
  needsSave = true;
  lastSaveTime = millis();
  lastActivityTime = millis();
  
  debugLog("Source: %s", sourceNames[currentSource]);
}

void nextSource() {
  setSource((currentSource + 1) % SOURCE_COUNT);
}

void prevSource() {
  setSource((currentSource + SOURCE_COUNT - 1) % SOURCE_COUNT);
}

// ===================================================================
// FONCTIONS CONTROLE AMPLI MA12070
// ===================================================================

void ampEnable(bool enable) {
  ampEnabled = enable;
  digitalWrite(PIN_AMP_EN, enable ? LOW : HIGH);
  
  if (enable) {
    delay(50);
    ampInit();
    delay(50);
    ampSetMute(false);
    fadeToVolume(targetVolume, true);
  } else {
    fadeToVolume(VOL_MAX, true);  // Volume min = attenuation max
    ampSetMute(true);
  }
  
  debugLog("Ampli %s", enable ? "ON" : "OFF");
}

void ampSetMute(bool mute) {
  ampMuted = mute;
  digitalWrite(PIN_AMP_MUTE, mute ? LOW : HIGH);
  
  // Mute TDA7439 aussi
  if (eqChipPresent) {
    if (mute) {
      tda7439SetVolume(48);  // Mute
    } else {
      tda7439SetVolume(currentVolume);
    }
  }
  
  debugLog("Mute %s", mute ? "ON" : "OFF");
}

void ampToggleMute() {
  if (ampMuted) {
    ampSetMute(false);
    fadeToVolume(targetVolume, false);
  } else {
    fadeToVolume(VOL_MAX, true);
    ampSetMute(true);
  }
  lastActivityTime = millis();
}

bool ampCheckError() {
  return digitalRead(PIN_AMP_ERR) == LOW;
}

void ampInit() {
  Wire.beginTransmission(MA12070_ADDR);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
  
  Wire.beginTransmission(MA12070_ADDR);
  Wire.write(0x01);
  Wire.write(0x08);
  Wire.endTransmission();
  
  Wire.beginTransmission(MA12070_ADDR);
  Wire.write(0x02);
  Wire.write(0x18);
  Wire.endTransmission();
  
  debugLog("MA12070 initialise");
}

// ===================================================================
// FONCTIONS BLUETOOTH
// ===================================================================

void btCheckStatus() {
  bool newStatus = (digitalRead(PIN_BT_STATUS) == HIGH);
  
  if (newStatus != btConnected) {
    btConnected = newStatus;
    debugLog("Bluetooth %s", btConnected ? "connecte" : "deconnecte");
    
    if (btConnected) {
      animationStart(1);
    }
  }
}

void btReset() {
  debugLog("Reset Bluetooth...");
  
  #ifdef PIN_BT_RESET
  pinMode(PIN_BT_RESET, OUTPUT);
  digitalWrite(PIN_BT_RESET, LOW);
  delay(100);
  digitalWrite(PIN_BT_RESET, HIGH);
  delay(1000);
  #endif
  
  btConnected = false;
  btPairing = false;
}

void btStartPairing() {
  debugLog("Mode appairage BT");
  btPairing = true;
  btReset();
  animationStart(2);
}

// ===================================================================
// FONCTIONS SECURITE BATTERIE
// ===================================================================

void batteryConnect(bool connect) {
  digitalWrite(PIN_SAFE_EN, connect ? LOW : HIGH);
  debugLog("Batterie %s", connect ? "connectee" : "deconnectee");
}

void checkBattery() {
  batteryRaw = analogRead(PIN_ADC_BATT);
  batteryVoltage = (batteryRaw * 3.3 / 4095.0) * 11.0;
  batteryPercent = constrain(map(batteryRaw, BATT_SHUTDOWN, BATT_FULL, 0, 100), 0, 100);
  
  bool wasLow = batteryLow;
  batteryLow = (batteryRaw < BATT_LOW);
  
  if (batteryLow && !wasLow) {
    debugLog("ALERTE: Batterie faible %.1fV", batteryVoltage);
  }
  
  if (batteryRaw < BATT_CRITICAL) {
    debugLog("CRITIQUE: Batterie %.1fV", batteryVoltage);
    if (targetVolume < 30) {
      targetVolume = 30;  // Limiter volume (index eleve = attenuation)
    }
  }
  
  if (batteryRaw < BATT_SHUTDOWN) {
    emergencyShutdown("BATT VIDE");
  }
}

void checkTemperature() {
  tempRaw = analogRead(PIN_ADC_NTC);
  
  bool wasWarning = tempWarning;
  tempWarning = (tempRaw < TEMP_WARN);
  
  if (tempRaw < TEMP_THROTTLE && !thermalThrottle) {
    thermalThrottle = true;
    debugLog("Thermal throttle actif");
    if (targetVolume < 15) {
      targetVolume = 15;
    }
  } else if (tempRaw > TEMP_NORMAL && thermalThrottle) {
    thermalThrottle = false;
    debugLog("Thermal throttle desactive");
  }
  
  if (tempRaw < TEMP_CRITICAL) {
    ampSetMute(true);
    debugLog("CRITIQUE: Temperature elevee");
  }
  
  if (tempRaw < TEMP_SHUTDOWN) {
    emergencyShutdown("SURCHAUFFE");
  }
  
  uint8_t tempApprox = map(tempRaw, 4095, 0, 0, 100);
  if (tempApprox > stats.maxTempReached) {
    stats.maxTempReached = tempApprox;
  }
}

void emergencyShutdown(const char* reason) {
  debugLog("ARRET URGENCE: %s", reason);
  
  ampSetMute(true);
  ampEnable(false);
  
  saveStats();
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("ARRET");
  display.setCursor(10, 35);
  display.println(reason);
  display.display();
  
  delay(3000);
  
  batteryConnect(false);
  
  esp_deep_sleep_start();
}

// ===================================================================
// FONCTIONS IR TELECOMMANDE
// ===================================================================

void irInit() {
  irrecv.enableIRIn();
  debugLog("Recepteur IR initialise");
}

void handleIR() {
  if (irrecv.decode(&irResults)) {
    uint32_t code = irResults.value;
    
    debugLog("IR recu: 0x%08X", code);
    
    static uint32_t lastIRCode = 0;
    static uint32_t lastIRTime = 0;
    
    if (code == 0xFFFFFFFF) {
      code = lastIRCode;
    }
    
    if (code == lastIRCode && millis() - lastIRTime < 200) {
      irrecv.resume();
      return;
    }
    
    lastIRCode = code;
    lastIRTime = millis();
    lastActivityTime = millis();
    
    switch (code) {
      case IR_CODE_POWER:
        if (ampEnabled) {
          ampEnable(false);
        } else {
          ampEnable(true);
        }
        break;
        
      case IR_CODE_MUTE:
        ampToggleMute();
        break;
        
      case IR_CODE_VOL_UP:
        if (!ampMuted) {
          // Volume: index bas = fort, donc on decremente
          targetVolume = constrain(targetVolume - VOL_STEP_IR, 0, settings.volumeLimit);
          volumeChanged = true;
        }
        break;
        
      case IR_CODE_VOL_DOWN:
        if (!ampMuted) {
          targetVolume = constrain(targetVolume + VOL_STEP_IR, 0, settings.volumeLimit);
          volumeChanged = true;
        }
        break;
        
      case IR_CODE_SOURCE:
      case IR_CODE_NEXT:
        nextSource();
        break;
        
      case IR_CODE_PREV:
        prevSource();
        break;
        
      case IR_CODE_OK:
        handleShortPress();
        break;
        
      case IR_CODE_MENU:
        handleLongPress();
        break;
        
      case IR_CODE_BACK:
        menuState = MENU_MAIN;
        break;
        
      case IR_CODE_EQ:
        menuState = MENU_EQ;
        menuSelection = 0;
        menuEntryTime = millis();
        break;
        
      case IR_CODE_LOUD:
        loudnessToggle();
        break;
        
      case IR_CODE_SPATIAL:
        spatialNext();
        break;
        
      case IR_CODE_NUM_1:
        setSource(SOURCE_BT);
        break;
      case IR_CODE_NUM_2:
        setSource(SOURCE_AUX);
        break;
      case IR_CODE_NUM_3:
        setSource(SOURCE_PHONO);
        break;
        
      case IR_CODE_NUM_0:
        testMode = !testMode;
        if (testMode) {
          menuState = MENU_TEST;
        }
        break;
    }
    
    irrecv.resume();
  }
}

// ===================================================================
// FONCTIONS VU-METRE
// ===================================================================

void updateVUMeter() {
  if (!settings.vuMeterEnabled) return;
  
  uint16_t rawL = analogRead(PIN_ADC_AUDIO_L);
  uint16_t rawR = analogRead(PIN_ADC_AUDIO_R);
  
  uint8_t newL = map(rawL, 0, 4095, 0, 100);
  uint8_t newR = map(rawR, 0, 4095, 0, 100);
  
  if (newL > vuMeter.levelL) {
    vuMeter.levelL = newL;
  } else {
    vuMeter.levelL = max(0, vuMeter.levelL - VU_DECAY_RATE);
  }
  
  if (newR > vuMeter.levelR) {
    vuMeter.levelR = newR;
  } else {
    vuMeter.levelR = max(0, vuMeter.levelR - VU_DECAY_RATE);
  }
  
  uint32_t now = millis();
  
  if (vuMeter.levelL > vuMeter.peakL) {
    vuMeter.peakL = vuMeter.levelL;
    vuMeter.peakHoldTimeL = now;
  } else if (now - vuMeter.peakHoldTimeL > 1000) {
    vuMeter.peakL = max(0, vuMeter.peakL - 2);
  }
  
  if (vuMeter.levelR > vuMeter.peakR) {
    vuMeter.peakR = vuMeter.levelR;
    vuMeter.peakHoldTimeR = now;
  } else if (now - vuMeter.peakHoldTimeR > 1000) {
    vuMeter.peakR = max(0, vuMeter.peakR - 2);
  }
}

// ===================================================================
// FONCTIONS ANIMATION
// ===================================================================

void animationStart(uint8_t type) {
  animation.type = type;
  animation.frame = 0;
  animation.lastFrame = millis();
  animation.active = true;
}

void animationStop() {
  animation.active = false;
}

void animationUpdate() {
  if (!animation.active) return;
  
  if (millis() - animation.lastFrame < 50) return;
  animation.lastFrame = millis();
  
  animation.frame++;
  
  if (animation.frame > 60) {
    animationStop();
  }
}

// ===================================================================
// FONCTIONS AFFICHAGE OLED
// ===================================================================

void displayInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Erreur OLED!");
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.dim(settings.brightness < 128);
  display.display();
  
  debugLog("OLED initialise");
}

void displaySetBrightness(uint8_t brightness) {
  settings.brightness = constrain(brightness, OLED_BRIGHTNESS_MIN, OLED_BRIGHTNESS_MAX);
  display.dim(settings.brightness < 128);
}

void displaySplash() {
  for (int i = 0; i <= 100; i += 5) {
    display.clearDisplay();
    
    int logoY = map(i, 0, 100, -20, 5);
    display.setTextSize(2);
    display.setCursor(20, logoY);
    display.println("AMPLI");
    display.setCursor(5, logoY + 25);
    display.println("AUDIOPHILE");
    
    display.drawRect(10, 52, 108, 8, SSD1306_WHITE);
    display.fillRect(12, 54, i, 4, SSD1306_WHITE);
    
    if (i > 50) {
      display.setTextSize(1);
      display.setCursor(45, 45);
      display.print("V");
      display.print(FW_VERSION);
    }
    
    display.display();
    delay(30);
  }
  
  delay(500);
}

void displayMain() {
  display.clearDisplay();
  
  // Barre superieure
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(sourceIcons[currentSource]);
  
  // Indicateurs EQ/Loudness/Spatial
  int xStatus = 18;
  if (eqChipPresent) {
    if (settings.eq.enabled) {
      display.setCursor(xStatus, 0);
      display.print(presets[settings.eq.preset].name);
      xStatus += strlen(presets[settings.eq.preset].name) * 6 + 3;
    }
    if (settings.eq.loudness) {
      display.setCursor(xStatus, 0);
      display.print("L");
      xStatus += 8;
    }
    if (settings.eq.spatial > 0) {
      display.setCursor(xStatus, 0);
      display.print("S");
      display.print(settings.eq.spatial);
    }
  }
  
  // BT status
  if (currentSource == SOURCE_BT) {
    display.setCursor(105, 0);
    if (btPairing) {
      if ((millis() / 300) % 2) {
        display.print("PAIR");
      }
    } else {
      display.print(btConnected ? "[BT]" : "[--]");
    }
  }
  
  // Zone centrale (volume)
  if (ampMuted) {
    display.setTextSize(3);
    display.setCursor(25, 18);
    display.print("MUTE");
  } else {
    display.setTextSize(3);
    // TDA7439: 0 = max volume, 47 = min
    // Afficher en % inverse
    uint8_t volPercent = map(currentVolume, 47, 0, 0, 100);
    
    int xVol = 30;
    if (volPercent < 10) xVol = 45;
    else if (volPercent < 100) xVol = 35;
    
    display.setCursor(xVol, 15);
    display.print(volPercent);
    display.setTextSize(2);
    display.print("%");
    
    // Indicateur loudness actif
    if (settings.eq.loudness && loudnessAppliedBass > 0) {
      display.setTextSize(1);
      display.setCursor(0, 20);
      display.print("+");
      display.print(loudnessAppliedBass * 2);
      display.print("dB");
    }
  }
  
  // Barre de volume (inversee: gauche = min)
  display.drawRect(0, 42, 128, 10, SSD1306_WHITE);
  uint8_t barWidth = map(currentVolume, 47, 0, 0, 124);
  display.fillRect(2, 44, barWidth, 6, SSD1306_WHITE);
  
  if (settings.volumeLimit < VOL_MAX) {
    uint8_t limitX = map(settings.volumeLimit, 47, 0, 0, 124);
    display.drawLine(limitX + 2, 42, limitX + 2, 51, SSD1306_WHITE);
  }
  
  // VU-metre
  if (settings.vuMeterEnabled && !ampMuted) {
    int vuX = 115;
    int vuH = 30;
    int vuY = 12;
    
    int hL = map(vuMeter.levelL, 0, 100, 0, vuH);
    display.fillRect(vuX, vuY + vuH - hL, 5, hL, SSD1306_WHITE);
    int peakYL = vuY + vuH - map(vuMeter.peakL, 0, 100, 0, vuH);
    display.drawLine(vuX, peakYL, vuX + 5, peakYL, SSD1306_WHITE);
    
    int hR = map(vuMeter.levelR, 0, 100, 0, vuH);
    display.fillRect(vuX + 7, vuY + vuH - hR, 5, hR, SSD1306_WHITE);
    int peakYR = vuY + vuH - map(vuMeter.peakR, 0, 100, 0, vuH);
    display.drawLine(vuX + 7, peakYR, vuX + 12, peakYR, SSD1306_WHITE);
  }
  
  // Barre inferieure
  display.setTextSize(1);
  
  // Batterie
  display.drawRect(0, 55, 22, 9, SSD1306_WHITE);
  display.fillRect(22, 57, 2, 5, SSD1306_WHITE);
  uint8_t battBar = map(batteryPercent, 0, 100, 0, 18);
  display.fillRect(2, 57, battBar, 5, SSD1306_WHITE);
  
  display.setCursor(26, 56);
  display.print(batteryPercent);
  display.print("% ");
  display.print(batteryVoltage, 1);
  display.print("V");
  
  // Balance / Spatial
  if (settings.balance != 0) {
    display.setCursor(85, 56);
    display.print("B");
    display.print(settings.balance > 0 ? "+" : "");
    display.print(settings.balance);
  } else if (settings.eq.spatial > 0) {
    display.setCursor(90, 56);
    display.print("S:");
    display.print(spatialName(settings.eq.spatial));
  }
  
  // Alertes
  if (batteryLow && (millis() / 500) % 2) {
    display.fillRect(0, 55, 128, 9, SSD1306_BLACK);
    display.setCursor(30, 56);
    display.print("! BATTERIE FAIBLE !");
  }
  
  if (thermalThrottle && (millis() / 500) % 2) {
    display.fillRect(0, 0, 128, 9, SSD1306_BLACK);
    display.setCursor(20, 0);
    display.print("! TEMP ELEVEE !");
  }
  
  display.display();
}

void displaySourceMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(35, 2);
  display.print("SOURCE");
  
  for (uint8_t i = 0; i < SOURCE_COUNT; i++) {
    int y = 16 + i * 16;
    
    if (i == menuSelection) {
      display.fillRect(0, y, 128, 14, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.setCursor(10, y + 3);
    display.print(sourceNames[i]);
    
    if (i == currentSource) {
      display.setCursor(100, y + 3);
      display.print("*");
    }
  }
  
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("Tourner=Nav  Clic=OK");
  
  display.display();
}

void displayBalanceMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(35, 2);
  display.print("BALANCE");
  
  display.setTextSize(3);
  display.setCursor(45, 20);
  if (settings.balance > 0) display.print("+");
  display.print(settings.balance);
  
  display.drawRect(14, 48, 100, 8, SSD1306_WHITE);
  display.drawLine(64, 48, 64, 55, SSD1306_WHITE);
  
  uint8_t pos = map(settings.balance, -10, 10, 15, 111);
  display.fillRect(pos - 2, 49, 5, 6, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(5, 56);
  display.print("L");
  display.setCursor(60, 56);
  display.print("C");
  display.setCursor(115, 56);
  display.print("R");
  
  display.display();
}

// ===================================================================
// AFFICHAGE MENU EQ (V1.3 - 3 bandes + options)
// ===================================================================

void displayEQMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(25, 2);
  display.print("EGALISEUR 3B");
  
  if (!eqChipPresent) {
    display.setCursor(105, 2);
    display.print("N/A");
  }
  
  const char* items[] = {"Preset", "Bass", "Mid", "Treble", "Loudness", "Spatial"};
  const int itemCount = 6;
  
  for (int i = 0; i < itemCount; i++) {
    int y = 13 + i * 8;
    
    if (i == menuSelection) {
      display.fillRect(0, y, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.setCursor(3, y);
    display.print(items[i]);
    
    // Valeurs
    display.setCursor(60, y);
    switch (i) {
      case 0:
        display.print(presets[settings.eq.preset].name);
        break;
      case 1:
        display.print(eqValueToString(settings.eq.bass));
        if (loudnessAppliedBass > 0) {
          display.print("+");
          display.print(loudnessAppliedBass * 2);
        }
        break;
      case 2:
        display.print(eqValueToString(settings.eq.mid));
        break;
      case 3:
        display.print(eqValueToString(settings.eq.treble));
        break;
      case 4:
        display.print(settings.eq.loudness ? "ON" : "OFF");
        break;
      case 5:
        display.print(spatialName(settings.eq.spatial));
        break;
    }
  }
  
  display.setTextColor(SSD1306_WHITE);
  
  // Mini graphique EQ
  drawEQGraph(0, 55, 128, 9);
  
  display.display();
}

void drawEQGraph(int x, int y, int w, int h) {
  // Dessine un mini graphique 3 bandes
  int barW = w / 5;
  int centerY = y + h / 2;
  
  // Ligne centrale
  display.drawLine(x, centerY, x + w, centerY, SSD1306_WHITE);
  
  // Bass (gauche)
  int bassH = map(settings.eq.bass, 0, 14, -h/2, h/2);
  if (bassH > 0) {
    display.fillRect(x + barW/2, centerY - bassH, barW, bassH, SSD1306_WHITE);
  } else if (bassH < 0) {
    display.fillRect(x + barW/2, centerY, barW, -bassH, SSD1306_WHITE);
  }
  
  // Mid (centre)
  int midH = map(settings.eq.mid, 0, 14, -h/2, h/2);
  if (midH > 0) {
    display.fillRect(x + w/2 - barW/2, centerY - midH, barW, midH, SSD1306_WHITE);
  } else if (midH < 0) {
    display.fillRect(x + w/2 - barW/2, centerY, barW, -midH, SSD1306_WHITE);
  }
  
  // Treble (droite)
  int trebH = map(settings.eq.treble, 0, 14, -h/2, h/2);
  if (trebH > 0) {
    display.fillRect(x + w - barW - barW/2, centerY - trebH, barW, trebH, SSD1306_WHITE);
  } else if (trebH < 0) {
    display.fillRect(x + w - barW - barW/2, centerY, barW, -trebH, SSD1306_WHITE);
  }
}

void displayEQBandMenu(const char* title, uint8_t value) {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(50, 2);
  display.print(title);
  
  display.setTextSize(3);
  display.setCursor(30, 22);
  display.print(eqValueToString(value));
  
  // Barre graphique
  display.drawRect(10, 50, 108, 10, SSD1306_WHITE);
  display.drawLine(64, 50, 64, 59, SSD1306_WHITE);  // Centre
  
  uint8_t barPos = map(value, 0, 14, 0, 104);
  display.fillRect(12 + barPos - 2, 52, 5, 6, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("-14");
  display.setCursor(55, 56);
  display.print("0");
  display.setCursor(105, 56);
  display.print("+14");
  
  display.display();
}

void displayEQPresetMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(35, 2);
  display.print("PRESETS");
  
  // Afficher 6 presets visibles
  int startIdx = 0;
  if (menuSelection > 4) startIdx = menuSelection - 4;
  
  for (int i = 0; i < min(6, PRESET_COUNT - startIdx); i++) {
    int idx = startIdx + i;
    int y = 13 + i * 8;
    
    if (idx == menuSelection) {
      display.fillRect(0, y, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.setCursor(3, y);
    display.print(presets[idx].name);
    
    // Valeurs preset
    display.setCursor(50, y);
    display.print("B");
    display.print(eqValueToString(presets[idx].bass));
    display.setCursor(80, y);
    display.print("M");
    display.print(((int)presets[idx].mid - 7) * 2);
    display.setCursor(100, y);
    display.print("T");
    display.print(((int)presets[idx].treble - 7) * 2);
    
    if (idx == settings.eq.preset) {
      display.setCursor(120, y);
      display.print("*");
    }
  }
  
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void displayLoudnessMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(30, 2);
  display.print("LOUDNESS");
  
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.print(settings.eq.loudness ? "ON" : "OFF");
  
  display.setTextSize(1);
  display.setCursor(5, 45);
  display.print("Boost bass auto");
  display.setCursor(5, 54);
  display.print("a faible volume");
  
  if (settings.eq.loudness && loudnessAppliedBass > 0) {
    display.setCursor(80, 45);
    display.print("Actif:");
    display.setCursor(80, 54);
    display.print("+");
    display.print(loudnessAppliedBass * 2);
    display.print("dB");
  }
  
  display.display();
}

void displaySpatialMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(35, 2);
  display.print("SPATIAL");
  
  const char* levels[] = {"OFF", "Light", "Medium", "Wide"};
  
  for (int i = 0; i <= SPATIAL_MAX; i++) {
    int y = 15 + i * 12;
    
    if (i == menuSelection) {
      display.fillRect(0, y, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.setCursor(20, y + 2);
    display.print(levels[i]);
    
    if (i == settings.eq.spatial) {
      display.setCursor(100, y + 2);
      display.print("*");
    }
  }
  
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void displaySettingsMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.drawRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setCursor(30, 2);
  display.print("PARAMETRES");
  
  const char* items[] = {"Luminosite", "Sleep Timer", "Volume Max", "VU-metre", "Gain entree", "Infos"};
  const int itemCount = 6;
  
  for (int i = 0; i < itemCount; i++) {
    int y = 13 + i * 8;
    
    if (i == menuSelection) {
      display.fillRect(0, y, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.setCursor(3, y);
    display.print(items[i]);
    
    display.setCursor(85, y);
    switch (i) {
      case 0: display.print(settings.brightness > 128 ? "HI" : "LO"); break;
      case 1: 
        if (settings.sleepTimer == 0) display.print("OFF");
        else { display.print(settings.sleepTimer * 15); display.print("m"); }
        break;
      case 2: display.print(map(settings.volumeLimit, 47, 0, 0, 100)); display.print("%"); break;
      case 3: display.print(settings.vuMeterEnabled ? "ON" : "OFF"); break;
      case 4: display.print("+"); display.print(settings.inputGain * 2); display.print("dB"); break;
      case 5: display.print(">"); break;
    }
  }
  
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void displayInfoScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.print("AMPLI AUDIOPHILE V");
  display.println(FW_VERSION);
  
  display.print("EQ: ");
  display.println(eqChipPresent ? "TDA7439 OK" : "Non detecte");
  
  display.print("Uptime: ");
  uint32_t uptime = (millis() - stats.sessionStart) / 1000;
  display.print(uptime / 3600); display.print("h ");
  display.print((uptime % 3600) / 60); display.println("m");
  
  display.print("Total: ");
  display.print(stats.totalOnTime / 3600); display.println("h");
  
  display.print("Cycles: ");
  display.println(stats.powerCycles);
  
  display.print("Temp max: ");
  display.print(stats.maxTempReached);
  display.println("C");
  
  display.setCursor(0, 56);
  display.print("Clic = Retour");
  
  display.display();
}

void displayTestScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.println("=== MODE TEST ===");
  
  display.print("Vol: ");
  display.print(currentVolume);
  display.print(" (");
  display.print(map(currentVolume, 47, 0, 0, 100));
  display.println("%)");
  
  display.print("EQ: B");
  display.print(settings.eq.bass);
  display.print(" M");
  display.print(settings.eq.mid);
  display.print(" T");
  display.println(settings.eq.treble);
  
  display.print("Loud: ");
  display.print(settings.eq.loudness ? "ON" : "OFF");
  display.print(" Spat: ");
  display.println(settings.eq.spatial);
  
  display.print("Batt: ");
  display.print(batteryVoltage, 1);
  display.println("V");
  
  display.print("Loop/s: ");
  display.println(loopsPerSecond);
  
  display.setCursor(0, 56);
  display.print("0=Exit");
  
  display.display();
}

void updateDisplay() {
  if (millis() - lastDisplayUpdate < DISPLAY_REFRESH) return;
  lastDisplayUpdate = millis();
  
  switch (menuState) {
    case MENU_MAIN:
      displayMain();
      break;
    case MENU_SOURCE:
      displaySourceMenu();
      break;
    case MENU_BALANCE:
      displayBalanceMenu();
      break;
    case MENU_EQ:
      displayEQMenu();
      break;
    case MENU_EQ_BASS:
      displayEQBandMenu("BASS", settings.eq.bass);
      break;
    case MENU_EQ_MID:
      displayEQBandMenu("MID", settings.eq.mid);
      break;
    case MENU_EQ_TREBLE:
      displayEQBandMenu("TREBLE", settings.eq.treble);
      break;
    case MENU_EQ_PRESET:
      displayEQPresetMenu();
      break;
    case MENU_EQ_LOUDNESS:
      displayLoudnessMenu();
      break;
    case MENU_EQ_SPATIAL:
      displaySpatialMenu();
      break;
    case MENU_SETTINGS:
    case MENU_SETTINGS_BRIGHTNESS:
    case MENU_SETTINGS_SLEEP:
    case MENU_SETTINGS_VOLLIMIT:
    case MENU_SETTINGS_VUMETER:
    case MENU_SETTINGS_GAIN:
      displaySettingsMenu();
      break;
    case MENU_INFO:
      displayInfoScreen();
      break;
    case MENU_TEST:
      displayTestScreen();
      break;
    default:
      displayMain();
      break;
  }
  
  animationUpdate();
}

// ===================================================================
// FONCTIONS SAUVEGARDE NVS
// ===================================================================

void loadSettings() {
  preferences.begin("ampli", true);
  
  settings.volume = preferences.getUChar("volume", VOL_DEFAULT);
  settings.source = preferences.getUChar("source", SOURCE_BT);
  settings.balance = preferences.getChar("balance", 0);
  settings.brightness = preferences.getUChar("bright", OLED_BRIGHTNESS_DEFAULT);
  settings.vuMeterEnabled = preferences.getBool("vumeter", false);
  settings.sleepTimer = preferences.getUChar("sleep", 0);
  settings.volumeLimit = preferences.getUChar("vollim", VOL_MAX);
  settings.inputGain = preferences.getUChar("ingain", 0);
  
  // EQ V1.3
  settings.eq.bass = preferences.getUChar("eqbass", EQ_CENTER);
  settings.eq.mid = preferences.getUChar("eqmid", EQ_CENTER);
  settings.eq.treble = preferences.getUChar("eqtreb", EQ_CENTER);
  settings.eq.preset = preferences.getUChar("eqpreset", PRESET_FLAT);
  settings.eq.enabled = preferences.getBool("eqon", true);
  settings.eq.loudness = preferences.getBool("eqloud", false);
  settings.eq.spatial = preferences.getUChar("eqspat", SPATIAL_OFF);
  
  for (int i = 0; i < SOURCE_COUNT; i++) {
    char key[10];
    sprintf(key, "vol%d", i);
    settings.volumePerSource[i] = preferences.getUChar(key, VOL_DEFAULT);
  }
  
  preferences.end();
  
  targetVolume = settings.volume;
  currentSource = settings.source;
  
  debugLog("Parametres charges");
}

void saveSettings() {
  preferences.begin("ampli", false);
  
  preferences.putUChar("volume", targetVolume);
  preferences.putUChar("source", currentSource);
  preferences.putChar("balance", settings.balance);
  preferences.putUChar("bright", settings.brightness);
  preferences.putBool("vumeter", settings.vuMeterEnabled);
  preferences.putUChar("sleep", settings.sleepTimer);
  preferences.putUChar("vollim", settings.volumeLimit);
  preferences.putUChar("ingain", settings.inputGain);
  
  // EQ V1.3
  preferences.putUChar("eqbass", settings.eq.bass);
  preferences.putUChar("eqmid", settings.eq.mid);
  preferences.putUChar("eqtreb", settings.eq.treble);
  preferences.putUChar("eqpreset", settings.eq.preset);
  preferences.putBool("eqon", settings.eq.enabled);
  preferences.putBool("eqloud", settings.eq.loudness);
  preferences.putUChar("eqspat", settings.eq.spatial);
  
  for (int i = 0; i < SOURCE_COUNT; i++) {
    char key[10];
    sprintf(key, "vol%d", i);
    preferences.putUChar(key, settings.volumePerSource[i]);
  }
  
  preferences.end();
  
  needsSave = false;
  debugLog("Parametres sauvegardes");
}

void loadStats() {
  preferences.begin("stats", true);
  
  stats.totalOnTime = preferences.getULong("ontime", 0);
  stats.powerCycles = preferences.getUShort("cycles", 0);
  stats.errorCount = preferences.getUShort("errors", 0);
  stats.maxTempReached = preferences.getUChar("maxtemp", 0);
  
  preferences.end();
  
  stats.sessionStart = millis();
  stats.powerCycles++;
}

void saveStats() {
  stats.totalOnTime += (millis() - stats.sessionStart) / 1000;
  
  preferences.begin("stats", false);
  
  preferences.putULong("ontime", stats.totalOnTime);
  preferences.putUShort("cycles", stats.powerCycles);
  preferences.putUShort("errors", stats.errorCount);
  preferences.putUChar("maxtemp", stats.maxTempReached);
  
  preferences.end();
}

// ===================================================================
// FONCTIONS GESTION ENCODEUR
// ===================================================================

void handleEncoder() {
  // Rotation
  if (volumeChanged || encoderDelta != 0) {
    volumeChanged = false;
    lastActivityTime = millis();
    
    int32_t delta = encoderDelta;
    encoderDelta = 0;
    
    switch (menuState) {
      case MENU_MAIN:
        // TDA7439: volume haut = index bas
        targetVolume = constrain(targetVolume - (delta * VOL_STEP), 0, settings.volumeLimit);
        settings.volume = targetVolume;
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_SOURCE:
        menuSelection = (menuSelection + delta + SOURCE_COUNT) % SOURCE_COUNT;
        break;
        
      case MENU_BALANCE:
        settings.balance = constrain(settings.balance + delta, -10, 10);
        spatialApply();
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_EQ:
        menuSelection = (menuSelection + delta + 6) % 6;
        break;
        
      case MENU_EQ_BASS:
        settings.eq.bass = constrain(settings.eq.bass + delta, EQ_MIN, EQ_MAX);
        settings.eq.preset = PRESET_CUSTOM;
        eqApplyWithLoudness();
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_EQ_MID:
        settings.eq.mid = constrain(settings.eq.mid + delta, EQ_MIN, EQ_MAX);
        settings.eq.preset = PRESET_CUSTOM;
        eqApplyWithLoudness();
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_EQ_TREBLE:
        settings.eq.treble = constrain(settings.eq.treble + delta, EQ_MIN, EQ_MAX);
        settings.eq.preset = PRESET_CUSTOM;
        eqApplyWithLoudness();
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_EQ_PRESET:
        menuSelection = (menuSelection + delta + PRESET_COUNT) % PRESET_COUNT;
        break;
        
      case MENU_EQ_SPATIAL:
        menuSelection = (menuSelection + delta + SPATIAL_MAX + 1) % (SPATIAL_MAX + 1);
        break;
        
      case MENU_SETTINGS:
        menuSelection = (menuSelection + delta + 6) % 6;
        break;
        
      case MENU_SETTINGS_BRIGHTNESS:
        settings.brightness = constrain(settings.brightness + delta * 32, OLED_BRIGHTNESS_MIN, OLED_BRIGHTNESS_MAX);
        displaySetBrightness(settings.brightness);
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_SETTINGS_SLEEP:
        settings.sleepTimer = constrain(settings.sleepTimer + delta, 0, 8);
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_SETTINGS_VOLLIMIT:
        settings.volumeLimit = constrain(settings.volumeLimit + delta, 5, VOL_MAX);
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      case MENU_SETTINGS_GAIN:
        settings.inputGain = constrain(settings.inputGain + delta, 0, 15);
        tda7439SetInputGain(settings.inputGain);
        needsSave = true;
        lastSaveTime = millis();
        break;
        
      default:
        break;
    }
  }
  
  // Bouton
  if (encoderButtonPressed) {
    encoderButtonPressed = false;
    
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
      buttonHandled = false;
    }
  }
  
  if (buttonPressTime > 0 && !buttonHandled) {
    if (digitalRead(PIN_ENC_SW) == HIGH) {
      uint32_t pressDuration = millis() - buttonPressTime;
      
      if (pressDuration < LONGPRESS_MS) {
        handleShortPress();
      }
      buttonPressTime = 0;
    } else if (millis() - buttonPressTime > VERYLONGPRESS_MS && !buttonHandled) {
      handleVeryLongPress();
      buttonHandled = true;
    } else if (millis() - buttonPressTime > LONGPRESS_MS && !buttonHandled) {
      handleLongPress();
      buttonHandled = true;
    }
  }
}

void handleShortPress() {
  lastActivityTime = millis();
  
  switch (menuState) {
    case MENU_MAIN:
      ampToggleMute();
      break;
      
    case MENU_SOURCE:
      setSource(menuSelection);
      menuState = MENU_MAIN;
      break;
      
    case MENU_BALANCE:
    case MENU_INFO:
    case MENU_TEST:
      menuState = MENU_MAIN;
      testMode = false;
      break;
      
    case MENU_EQ:
      switch (menuSelection) {
        case 0:
          menuState = MENU_EQ_PRESET;
          menuSelection = settings.eq.preset;
          break;
        case 1:
          menuState = MENU_EQ_BASS;
          break;
        case 2:
          menuState = MENU_EQ_MID;
          break;
        case 3:
          menuState = MENU_EQ_TREBLE;
          break;
        case 4:
          loudnessToggle();
          break;
        case 5:
          menuState = MENU_EQ_SPATIAL;
          menuSelection = settings.eq.spatial;
          break;
      }
      break;
      
    case MENU_EQ_BASS:
    case MENU_EQ_MID:
    case MENU_EQ_TREBLE:
    case MENU_EQ_LOUDNESS:
      menuState = MENU_EQ;
      menuSelection = 0;
      break;
      
    case MENU_EQ_PRESET:
      eqApplyPreset(menuSelection);
      menuState = MENU_EQ;
      menuSelection = 0;
      break;
      
    case MENU_EQ_SPATIAL:
      settings.eq.spatial = menuSelection;
      spatialApply();
      needsSave = true;
      lastSaveTime = millis();
      menuState = MENU_EQ;
      menuSelection = 5;
      break;
      
    case MENU_SETTINGS:
      switch (menuSelection) {
        case 0: menuState = MENU_SETTINGS_BRIGHTNESS; break;
        case 1: menuState = MENU_SETTINGS_SLEEP; break;
        case 2: menuState = MENU_SETTINGS_VOLLIMIT; break;
        case 3: 
          settings.vuMeterEnabled = !settings.vuMeterEnabled;
          needsSave = true;
          lastSaveTime = millis();
          break;
        case 4: menuState = MENU_SETTINGS_GAIN; break;
        case 5: menuState = MENU_INFO; break;
      }
      break;
      
    case MENU_SETTINGS_BRIGHTNESS:
    case MENU_SETTINGS_SLEEP:
    case MENU_SETTINGS_VOLLIMIT:
    case MENU_SETTINGS_VUMETER:
    case MENU_SETTINGS_GAIN:
      menuState = MENU_SETTINGS;
      break;
      
    default:
      menuState = MENU_MAIN;
      break;
  }
}

void handleLongPress() {
  lastActivityTime = millis();
  
  switch (menuState) {
    case MENU_MAIN:
      menuState = MENU_SOURCE;
      menuSelection = currentSource;
      menuEntryTime = millis();
      break;
      
    case MENU_SOURCE:
      menuState = MENU_BALANCE;
      menuEntryTime = millis();
      break;
      
    case MENU_BALANCE:
      settings.balance = 0;
      spatialApply();
      needsSave = true;
      lastSaveTime = millis();
      break;
      
    case MENU_EQ:
    case MENU_EQ_BASS:
    case MENU_EQ_MID:
    case MENU_EQ_TREBLE:
    case MENU_EQ_PRESET:
    case MENU_EQ_LOUDNESS:
    case MENU_EQ_SPATIAL:
      menuState = MENU_MAIN;
      break;
      
    case MENU_SETTINGS:
    case MENU_INFO:
      menuState = MENU_MAIN;
      break;
      
    default:
      break;
  }
}

void handleVeryLongPress() {
  lastActivityTime = millis();
  
  switch (menuState) {
    case MENU_MAIN:
      menuState = MENU_EQ;
      menuSelection = 0;
      menuEntryTime = millis();
      break;
      
    case MENU_SOURCE:
      if (menuSelection == SOURCE_BT) {
        btStartPairing();
      }
      break;
      
    case MENU_EQ:
      menuState = MENU_SETTINGS;
      menuSelection = 0;
      menuEntryTime = millis();
      break;
      
    default:
      break;
  }
}

// ===================================================================
// FONCTIONS SLEEP TIMER
// ===================================================================

uint32_t getSleepTimerDuration() {
  const uint32_t durations[] = {0, 15*60*1000, 30*60*1000, 45*60*1000, 60*60*1000, 
                                 90*60*1000, 120*60*1000, 180*60*1000, 240*60*1000};
  if (settings.sleepTimer > 8) return 0;
  return durations[settings.sleepTimer];
}

uint32_t getSleepTimerRemaining() {
  if (settings.sleepTimer == 0 || sleepTimerStart == 0) return 0;
  
  uint32_t duration = getSleepTimerDuration();
  uint32_t elapsed = millis() - sleepTimerStart;
  
  if (elapsed >= duration) return 0;
  return (duration - elapsed) / 1000;
}

void checkSleepTimer() {
  if (settings.sleepTimer == 0) return;
  
  if (sleepTimerStart == 0) {
    sleepTimerStart = millis();
  }
  
  uint32_t duration = getSleepTimerDuration();
  if (millis() - sleepTimerStart >= duration) {
    debugLog("Sleep timer expire");
    emergencyShutdown("SLEEP");
  }
}

void checkAutoSleep() {
  if (millis() - lastActivityTime > AUTOSLEEP_TIMEOUT) {
    debugLog("Auto-sleep (inactivite)");
    
    settings.volumePerSource[currentSource] = targetVolume;
    saveSettings();
    saveStats();
    
    fadeToVolume(VOL_MAX, true);
    ampSetMute(true);
    ampEnable(false);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 28);
    display.print("Auto Sleep...");
    display.display();
    
    delay(2000);
    
    display.clearDisplay();
    display.display();
    
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ENC_SW, 0);
    esp_deep_sleep_start();
  }
}

void checkMenuTimeout() {
  if (menuState != MENU_MAIN && menuEntryTime > 0) {
    if (millis() - menuEntryTime > MENU_TIMEOUT) {
      menuState = MENU_MAIN;
      menuEntryTime = 0;
      debugLog("Menu timeout");
    }
  }
}

// ===================================================================
// FONCTIONS MONITORING
// ===================================================================

void updateMonitoring() {
  if (millis() - lastADCRead < ADC_INTERVAL) return;
  lastADCRead = millis();
  
  checkBattery();
  checkTemperature();
  btCheckStatus();
  
  if (ampCheckError() && ampEnabled) {
    stats.errorCount++;
    debugLog("Erreur ampli detectee, reset...");
    ampEnable(false);
    delay(200);
    ampEnable(true);
  }
  
  loopsPerSecond = loopCounter;
  loopCounter = 0;
}

void checkAutoSave() {
  if (needsSave && (millis() - lastSaveTime >= SAVE_DELAY)) {
    saveSettings();
  }
}

// ===================================================================
// FONCTIONS DEBUG SERIE
// ===================================================================

void debugLog(const char* format, ...) {
  if (!debugMode && !Serial) return;
  
  char buffer[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] ");
  Serial.println(buffer);
}

void handleSerialCommand() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  
  if (cmd == "help" || cmd == "?") {
    Serial.println("=== COMMANDES V1.3 ===");
    Serial.println("status    - Etat complet");
    Serial.println("vol N     - Volume (0-47, 0=max)");
    Serial.println("mute      - Toggle mute");
    Serial.println("src N     - Source (0-2)");
    Serial.println("bal N     - Balance (-10 a +10)");
    Serial.println("--- EQ ---");
    Serial.println("eq        - Status EQ complet");
    Serial.println("bass N    - Bass (0-14, 7=0dB)");
    Serial.println("mid N     - Mid (0-14, 7=0dB)");
    Serial.println("treble N  - Treble (0-14, 7=0dB)");
    Serial.println("preset N  - Preset (0-7)");
    Serial.println("eqon      - Toggle EQ on/off");
    Serial.println("loud      - Toggle Loudness");
    Serial.println("spatial N - Spatial (0-3)");
    Serial.println("gain N    - Input gain (0-15)");
    Serial.println("--- AUTRES ---");
    Serial.println("bright N  - Luminosite OLED");
    Serial.println("btpair    - Mode appairage BT");
    Serial.println("test      - Mode test ON/OFF");
    Serial.println("debug     - Mode debug ON/OFF");
    Serial.println("stats     - Statistiques");
    Serial.println("save      - Sauvegarde manuelle");
    Serial.println("reset     - Reset parametres");
    Serial.println("reboot    - Redemarrage");
  }
  else if (cmd == "status") {
    Serial.println("=== STATUS V1.3 ===");
    Serial.print("Volume: "); Serial.print(currentVolume); 
    Serial.print(" ("); Serial.print(map(currentVolume, 47, 0, 0, 100)); Serial.println("%)");
    Serial.print("Source: "); Serial.println(sourceNames[currentSource]);
    Serial.print("Mute: "); Serial.println(ampMuted ? "ON" : "OFF");
    Serial.print("Balance: "); Serial.println(settings.balance);
    Serial.print("Batterie: "); Serial.print(batteryVoltage, 2); 
    Serial.print("V ("); Serial.print(batteryPercent); Serial.println("%)");
    Serial.print("EQ chip: "); Serial.println(eqChipPresent ? "TDA7439 OK" : "N/A");
  }
  else if (cmd == "eq") {
    Serial.println("=== EGALISEUR TDA7439 ===");
    Serial.print("Chip: "); Serial.println(eqChipPresent ? "TDA7439 detecte" : "NON detecte");
    Serial.print("Actif: "); Serial.println(settings.eq.enabled ? "OUI" : "NON (bypass)");
    Serial.print("Preset: "); Serial.print(settings.eq.preset); 
    Serial.print(" ("); Serial.print(presets[settings.eq.preset].name); Serial.println(")");
    Serial.print("Bass: "); Serial.print(settings.eq.bass);
    Serial.print(" ("); Serial.print(eqValueToString(settings.eq.bass)); Serial.println(")");
    Serial.print("Mid: "); Serial.print(settings.eq.mid);
    Serial.print(" ("); Serial.print(eqValueToString(settings.eq.mid)); Serial.println(")");
    Serial.print("Treble: "); Serial.print(settings.eq.treble);
    Serial.print(" ("); Serial.print(eqValueToString(settings.eq.treble)); Serial.println(")");
    Serial.print("Loudness: "); Serial.print(settings.eq.loudness ? "ON" : "OFF");
    if (settings.eq.loudness && loudnessAppliedBass > 0) {
      Serial.print(" (actif: +"); Serial.print(loudnessAppliedBass * 2); Serial.print("dB)");
    }
    Serial.println();
    Serial.print("Spatial: "); Serial.print(settings.eq.spatial);
    Serial.print(" ("); Serial.print(spatialName(settings.eq.spatial)); Serial.println(")");
    Serial.print("Input Gain: +"); Serial.print(settings.inputGain * 2); Serial.println("dB");
    Serial.println("--- Presets ---");
    for (int i = 0; i < PRESET_COUNT; i++) {
      Serial.print(i); Serial.print(": "); Serial.print(presets[i].name);
      Serial.print(" B:"); Serial.print(eqValueToString(presets[i].bass));
      Serial.print(" M:"); Serial.print(eqValueToString(presets[i].mid));
      Serial.print(" T:"); Serial.println(eqValueToString(presets[i].treble));
    }
  }
  else if (cmd.startsWith("vol ")) {
    int val = cmd.substring(4).toInt();
    targetVolume = constrain(val, 0, VOL_MAX);
    Serial.print("Volume: "); Serial.print(targetVolume);
    Serial.print(" ("); Serial.print(map(targetVolume, 47, 0, 0, 100)); Serial.println("%)");
  }
  else if (cmd == "mute") {
    ampToggleMute();
    Serial.print("Mute: "); Serial.println(ampMuted ? "ON" : "OFF");
  }
  else if (cmd.startsWith("src ")) {
    int val = cmd.substring(4).toInt();
    setSource(val);
    Serial.print("Source: "); Serial.println(sourceNames[currentSource]);
  }
  else if (cmd.startsWith("bal ")) {
    int val = cmd.substring(4).toInt();
    settings.balance = constrain(val, -10, 10);
    spatialApply();
    Serial.print("Balance: "); Serial.println(settings.balance);
  }
  else if (cmd.startsWith("bass ")) {
    int val = cmd.substring(5).toInt();
    val = constrain(val, EQ_MIN, EQ_MAX);
    settings.eq.bass = val;
    settings.eq.preset = PRESET_CUSTOM;
    eqApplyWithLoudness();
    Serial.print("Bass: "); Serial.println(eqValueToString(settings.eq.bass));
  }
  else if (cmd.startsWith("mid ")) {
    int val = cmd.substring(4).toInt();
    val = constrain(val, EQ_MIN, EQ_MAX);
    settings.eq.mid = val;
    settings.eq.preset = PRESET_CUSTOM;
    eqApplyWithLoudness();
    Serial.print("Mid: "); Serial.println(eqValueToString(settings.eq.mid));
  }
  else if (cmd.startsWith("treble ")) {
    int val = cmd.substring(7).toInt();
    val = constrain(val, EQ_MIN, EQ_MAX);
    settings.eq.treble = val;
    settings.eq.preset = PRESET_CUSTOM;
    eqApplyWithLoudness();
    Serial.print("Treble: "); Serial.println(eqValueToString(settings.eq.treble));
  }
  else if (cmd.startsWith("preset ")) {
    int val = cmd.substring(7).toInt();
    if (val >= 0 && val < PRESET_COUNT) {
      eqApplyPreset(val);
      Serial.print("Preset: "); Serial.println(presets[settings.eq.preset].name);
    } else {
      Serial.println("Preset invalide (0-7)");
    }
  }
  else if (cmd == "eqon") {
    eqToggle();
    Serial.print("EQ: "); Serial.println(settings.eq.enabled ? "ON" : "OFF (bypass)");
  }
  else if (cmd == "loud") {
    loudnessToggle();
    Serial.print("Loudness: "); Serial.println(settings.eq.loudness ? "ON" : "OFF");
  }
  else if (cmd.startsWith("spatial ")) {
    int val = cmd.substring(8).toInt();
    settings.eq.spatial = constrain(val, 0, SPATIAL_MAX);
    spatialApply();
    Serial.print("Spatial: "); Serial.println(spatialName(settings.eq.spatial));
  }
  else if (cmd.startsWith("gain ")) {
    int val = cmd.substring(5).toInt();
    tda7439SetInputGain(val);
    Serial.print("Input Gain: +"); Serial.print(settings.inputGain * 2); Serial.println("dB");
  }
  else if (cmd.startsWith("bright ")) {
    int val = cmd.substring(7).toInt();
    displaySetBrightness(val);
    Serial.print("Luminosite: "); Serial.println(settings.brightness);
  }
  else if (cmd == "btpair") {
    btStartPairing();
    Serial.println("Mode appairage BT active");
  }
  else if (cmd == "test") {
    testMode = !testMode;
    if (testMode) menuState = MENU_TEST;
    else menuState = MENU_MAIN;
    Serial.print("Mode test: "); Serial.println(testMode ? "ON" : "OFF");
  }
  else if (cmd == "debug") {
    debugMode = !debugMode;
    Serial.print("Mode debug: "); Serial.println(debugMode ? "ON" : "OFF");
  }
  else if (cmd == "stats") {
    Serial.println("=== STATISTIQUES ===");
    Serial.print("Temps total: "); Serial.print(stats.totalOnTime / 3600); Serial.println("h");
    Serial.print("Cycles: "); Serial.println(stats.powerCycles);
    Serial.print("Erreurs: "); Serial.println(stats.errorCount);
    Serial.print("Temp max: "); Serial.print(stats.maxTempReached); Serial.println("C");
  }
  else if (cmd == "save") {
    saveSettings();
    saveStats();
    Serial.println("Sauvegarde effectuee");
  }
  else if (cmd == "reset") {
    preferences.begin("ampli", false);
    preferences.clear();
    preferences.end();
    Serial.println("Parametres reinitialises. Redemarrage...");
    delay(1000);
    ESP.restart();
  }
  else if (cmd == "reboot") {
    saveSettings();
    saveStats();
    Serial.println("Redemarrage...");
    delay(500);
    ESP.restart();
  }
  else {
    Serial.print("Commande inconnue: "); Serial.println(cmd);
    Serial.println("Tapez 'help' pour la liste");
  }
}

// ===================================================================
// SETUP
// ===================================================================

void setup() {
  // Watchdog
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("================================");
  Serial.print("AMPLI AUDIOPHILE V"); Serial.println(FW_VERSION);
  Serial.println("TDA7439 3-Band EQ + Loudness");
  Serial.println("================================");
  
  debugMode = true;
  
  // GPIO
  pinMode(PIN_AMP_EN, OUTPUT);
  pinMode(PIN_AMP_MUTE, OUTPUT);
  pinMode(PIN_SRC_SEL0, OUTPUT);
  pinMode(PIN_SRC_SEL1, OUTPUT);
  pinMode(PIN_SAFE_EN, OUTPUT);
  pinMode(PIN_SPI_CS_VOL, OUTPUT);
  
  digitalWrite(PIN_AMP_EN, HIGH);
  digitalWrite(PIN_AMP_MUTE, LOW);
  digitalWrite(PIN_SAFE_EN, HIGH);
  digitalWrite(PIN_SPI_CS_VOL, HIGH);
  
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_AMP_ERR, INPUT_PULLUP);
  pinMode(PIN_BT_STATUS, INPUT);
  pinMode(PIN_IR_RX, INPUT);
  
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  // I2C
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  
  // SPI (backup si pas TDA7439)
  MCP4261_init();
  
  // OLED
  displayInit();
  
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
  
  // Demarrage ampli
  setSource(currentSource);
  ampEnable(true);
  
  // Appliquer EQ + Loudness + Spatial
  if (settings.eq.enabled && eqChipPresent) {
    eqApplyWithLoudness();
    spatialApply();
  }
  
  lastActivityTime = millis();
  systemReady = true;
  
  debugLog("Systeme pret!");
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
// FIN DU CODE V1.3
// ===================================================================
