//
//   ZOOM MS50G, G3 AND G5 CONTROLLER - MIDI over USB
//           (c)2020, Massimo Procopio
//              info@massimoprocopio.com
//
// Library Used:
// USB Host Shield Library 2.0 (By Oleg Mazurov V 1.5.0)
// LiquidCrystal I2C (By Frank de Brabander V 1.1.2)
//
//
//

#include <Usb.h>
#include <usbhub.h>
#include <usbh_midi.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>  // for memory storage in program space

#define MS_50G 0x58
#define MS_70C 0x61
#define MS_60B 0x5F
#define G5N 0x6E
#define G5 0x5B
#define G3 0x59


//#define DEBUG
//#define TESTRAM
//#define ENCODERTEST
//#define DEBUG_ALL_SEND
//#define DEBUG_ALL_RECEIVE

//#define INVERT_EXTRA_FOOTSWITCHES
//#define USE_INTERNAL_PULLUP  (In this case it is not necessary to use external resistors)

//#define EXCLUDE_MS_70C  //Free about 150 bytes of storage space
//#define EXCLUDE_G5      //Free about 130 bytes of storage space
//#define EXCLUDE_G3      //Free about 130 bytes of storage space
//#define EXCLUDE_UNKNOWN //Free about 230 bytes of storage space



#define MIN_BOUNCE_TIME 200
#define NUM_FOOTSWITCH 6
#define NUM_CONF_PATCHES 50
#define NUM_PATCHES_FOR_EPATCH (NUM_FOOTSWITCH + 1)
#define NUM_BYTES_FOR_PATCHES (2 * NUM_PATCHES_FOR_EPATCH)
#define NUM_CONF_EFFECTS 10
#define MAX_TRY 3

#define UP_DOWN_PATCH_MODE 0
#define PRESELECT_MODE 1

#define NORMAL_MODE 0
#define PATCH_MODE 1
#define EFFECTS_MODE 2
#define SCROLL_MODE 3

#define MAX_FAKE_LOW 5

#define EEPROM_PATCH_INITIAL_ADDR 11
#define EEPROM_PATCH_END_ADDR (NUM_CONF_PATCHES * NUM_BYTES_FOR_PATCHES + (NUM_BYTES_FOR_PATCHES + EEPROM_PATCH_INITIAL_ADDR))
#define EEPROM_USER_NAME (EEPROM_PATCH_END_ADDR + 2)
#define EEPROM_END_USER_NAME (EEPROM_USER_NAME + 19)
#define EEPROM_USER_ID (EEPROM_END_USER_NAME + 2)
#define EEPROM_END_USER_ID (EEPROM_USER_ID + 19)
#define EEPROM_FIRMWARE (EEPROM_END_USER_ID + 2)
#define EEPROM_END_FIRMWARE (EEPROM_FIRMWARE + 19)
#define EEPROM_EFFECTS_INITIAL_ADDR (EEPROM_END_FIRMWARE + 2)
#define EEPROM_EFFECTS_END_ADDR ((NUM_CONF_EFFECTS * NUM_FOOTSWITCH) + EEPROM_EFFECTS_INITIAL_ADDR)
#define EEPROM_TOGGLE_MODE_ADDR 0;
#define EEPROM_FOOTSWITCHES_DEFAULT_ADDR 1;
#define EEPROM_EXTRA_FUNC_DELAY 2;
#define EEPROM_STARTING_MODE 3;

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

const char custom[][8] PROGMEM = {
  // Custom character definitions
  { 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 },  // char 1
  { 0x18, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F },  // char 2
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x03 },  // char 3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F },  // char 4
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C, 0x18 },  // char 5
  { 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F },  // char 6
  { 0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F },  // char 7
  { 0x03, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F },  // char 8
};

// BIG FONT Character Set
// - Each character coded as 1-4 byte sets {top_row(0), top_row(1)... bot_row(0), bot_row(0)..."}
// - number of bytes terminated with 0x00; Character is made from [number_of_bytes/2] wide, 2 high
// - codes are: 0x01-0x08 => custom characters, 0x20 => space, 0xFF => black square

const char bigChars[][8] PROGMEM = {
  { 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // Space
  { 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // !
  { 0x05, 0x05, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 },  // "
  { 0x04, 0xFF, 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01 },  // #
  { 0x08, 0xFF, 0x06, 0x07, 0xFF, 0x05, 0x00, 0x00 },  // $
  { 0x01, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x04 },  // %
  { 0x08, 0x06, 0x02, 0x20, 0x03, 0x07, 0x02, 0x04 },  // &
  { 0x05, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // '
  { 0x08, 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00 },  // (
  { 0x01, 0x02, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00 },  // )
  { 0x01, 0x04, 0x04, 0x01, 0x04, 0x01, 0x01, 0x04 },  // *
  { 0x04, 0xFF, 0x04, 0x01, 0xFF, 0x01, 0x00, 0x00 },  // +
  { 0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  //
  { 0x20, 0x04, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00 },  // -
  { 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // .
  { 0x20, 0x20, 0x04, 0x01, 0x04, 0x01, 0x20, 0x20 },  // /
  { 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 },  // 0
  { 0x01, 0x02, 0x20, 0x04, 0xFF, 0x04, 0x00, 0x00 },  // 1
  { 0x06, 0x06, 0x02, 0xFF, 0x07, 0x07, 0x00, 0x00 },  // 2
  { 0x01, 0x06, 0x02, 0x04, 0x07, 0x05, 0x00, 0x00 },  // 3
  { 0x03, 0x04, 0xFF, 0x20, 0x20, 0xFF, 0x00, 0x00 },  // 4
  { 0xFF, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 },  // 5
  { 0x08, 0x06, 0x06, 0x03, 0x07, 0x05, 0x00, 0x00 },  // 6
  { 0x01, 0x01, 0x02, 0x20, 0x08, 0x20, 0x00, 0x00 },  // 7
  { 0x08, 0x06, 0x02, 0x03, 0x07, 0x05, 0x00, 0x00 },  // 8
  { 0x08, 0x06, 0x02, 0x07, 0x07, 0x05, 0x00, 0x00 },  // 9
  { 0xA5, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // :
  { 0x04, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  // ;
  { 0x20, 0x04, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00 },  // <
  { 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00 },  // =
  { 0x01, 0x04, 0x20, 0x04, 0x01, 0x01, 0x00, 0x00 },  // >
  { 0x01, 0x06, 0x02, 0x20, 0x07, 0x20, 0x00, 0x00 },  // ?
  { 0x08, 0x06, 0x02, 0x03, 0x04, 0x04, 0x00, 0x00 },  // @
  { 0x08, 0x06, 0x02, 0xFF, 0x20, 0xFF, 0x00, 0x00 },  // A
  { 0xFF, 0x06, 0x05, 0xFF, 0x07, 0x02, 0x00, 0x00 },  // B
  { 0x08, 0x01, 0x01, 0x03, 0x04, 0x04, 0x00, 0x00 },  // C
  { 0xFF, 0x01, 0x02, 0xFF, 0x04, 0x05, 0x00, 0x00 },  // D
  { 0xFF, 0x06, 0x06, 0xFF, 0x07, 0x07, 0x00, 0x00 },  // E
  { 0xFF, 0x06, 0x06, 0xFF, 0x20, 0x20, 0x00, 0x00 },  // F
  { 0x08, 0x01, 0x01, 0x03, 0x04, 0x02, 0x00, 0x00 },  // G
  { 0xFF, 0x04, 0xFF, 0xFF, 0x20, 0xFF, 0x00, 0x00 },  // H
  { 0x01, 0xFF, 0x01, 0x04, 0xFF, 0x04, 0x00, 0x00 },  // I
  { 0x20, 0x20, 0xFF, 0x04, 0x04, 0x05, 0x00, 0x00 },  // J
  { 0xFF, 0x04, 0x05, 0xFF, 0x20, 0x02, 0x00, 0x00 },  // K
  { 0xFF, 0x20, 0x20, 0xFF, 0x04, 0x04, 0x00, 0x00 },  // L
  { 0x08, 0x03, 0x05, 0x02, 0xFF, 0x20, 0x20, 0xFF },  // M
  { 0xFF, 0x02, 0x20, 0xFF, 0xFF, 0x20, 0x03, 0xFF },  // N
  { 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00 },  // 0
  { 0x08, 0x06, 0x02, 0xFF, 0x20, 0x20, 0x00, 0x00 },  // P
  { 0x08, 0x01, 0x02, 0x20, 0x03, 0x04, 0xFF, 0x04 },  // Q
  { 0xFF, 0x06, 0x02, 0xFF, 0x20, 0x02, 0x00, 0x00 },  // R
  { 0x08, 0x06, 0x06, 0x07, 0x07, 0x05, 0x00, 0x00 },  // S
  { 0x01, 0xFF, 0x01, 0x20, 0xFF, 0x20, 0x00, 0x00 },  // T
  { 0xFF, 0x20, 0xFF, 0x03, 0x04, 0x05, 0x00, 0x00 },  // U
  { 0x03, 0x20, 0x20, 0x05, 0x20, 0x02, 0x08, 0x20 },  // V
  { 0xFF, 0x20, 0x20, 0xFF, 0x03, 0x08, 0x02, 0x05 },  // W
  { 0x03, 0x04, 0x05, 0x08, 0x20, 0x02, 0x00, 0x00 },  // X
  { 0x03, 0x04, 0x05, 0x20, 0xFF, 0x20, 0x00, 0x00 },  // Y
  { 0x01, 0x06, 0x05, 0x08, 0x07, 0x04, 0x00, 0x00 },  // Z
  { 0xFF, 0x01, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00 },  // [
  { 0x01, 0x04, 0x20, 0x20, 0x20, 0x20, 0x01, 0x04 },  // Backslash
  { 0x01, 0xFF, 0x04, 0xFF, 0x00, 0x00, 0x00, 0x00 },  // ]
  { 0x08, 0x02, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00 },  // ^
  { 0x20, 0x20, 0x20, 0x04, 0x04, 0x04, 0x00, 0x00 }   // _
};

//FootSwitch
const uint8_t SXbuttonPin = 5;       // the number of the pushbutton pin
const uint8_t DXbuttonPin = 3;       // the number of the pushbutton pin
const uint8_t CenterButtonPin = 4;   // the number of the pushbutton pin
const uint8_t FS4ButtonPin = 14;     // the number of the pushbutton pin
const uint8_t FS5ButtonPin = 15;     // the number of the pushbutton pin
const uint8_t FS6ButtonPin = 16;     // the number of the pushbutton pin
const uint8_t FSModeButtonPin = 17;  // the number of the pushbutton pin

//Encoder
const uint8_t pinClk = 2;
const uint8_t pinDt = 6;
const uint8_t pinSw = 7;
// Holds last encoder position
bool prevClk;
// Holds encoder counter
int16_t encoderCounter;
int16_t prevEncoderCounter;
bool redrawMenu = true;
byte offsetMenuPos = 0;
byte activeFootswitchNum = 6;

USB Usb;
USBH_MIDI Midi(&Usb);
byte zoomModel = MS_50G;
byte maxEffects = 3;
bool effectsStatus[9];
uint16_t effectsTypes[9];
uint16_t lastEffectsTypes[9] = { 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000 };

int16_t Preset = 0;
int16_t ActualPreset = 0;

byte modeSelect = UP_DOWN_PATCH_MODE;
char patchName[11] = { "\0\0\0\0\0\0\0\0\0\0\0" };
char nextPatchName[11] = { "\0\0\0\0\0\0\0\0\0\0\0" };
char reqPatchName[11] = { "\0\0\0\0\0\0\0\0\0\0\0" };
char effectName[9] = { "\0\0\0\0\0\0\0\0\0" };

char displayLine[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
uint16_t ciclesLoop = 0;
uint16_t ciclesLoopEffectsCheck = 0;
uint16_t displayCiclesLoop = 0;
uint16_t ciclesBlinkLoop = 0;
bool menuActive = false;
byte menuPos = 0;
byte selectedFootswitch = 0;
byte selectedEffect = 0;
byte selectedConf = 0;
byte selectedConfEffects = 0;
byte selectedConfPatch = 0;
char modeString[21]{ "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
bool blinkStatus = false;
bool menuInitialized = false;
bool isAssigningPatch = false;
bool isAssigningCloning = false;
bool isAssigningEffect = false;
bool isAssigningConf = false;
bool isAssigningToggleMode = false;
bool isAssigningActiveFootswitches = false;
bool isAssigningExtraDelay = false;
bool isResettingAll = false;
bool isStartingMode = false;
bool resetAll = false;
byte startingMode = 0;
bool unknownDevice = false;
byte destBank = 0;
byte assignedEffectNum = 0;
byte assignedEffects[NUM_FOOTSWITCH];
int16_t patchPreset[NUM_PATCHES_FOR_EPATCH];
byte assignedFootswitch = 0;
byte menuId = 0;
byte actualMode = 0;
byte prevMode = 0;
uint16_t ciclesEffectsFootswitch[] = { 0, 0, 0 };
uint16_t countCiclesTransitions = 0;
bool SwButtonWaitReset = false;
bool ModeButtonWaitReset = false;
uint8_t maxPresetNumber = 50;
uint8_t minPresetNumber = 1;
byte toggleMode = 0;
uint16_t extraFuncDelay = 750;
uint16_t extraFuncDelayTemp = 750;
uint8_t lastPatchEnabled = 0;
uint8_t lastSelectedConfPatch = 255;
byte dataMessageLen = 164;
byte dataMessageNameLen = 174;
uint32_t elapsedTime = 0;

const char menu[][19] PROGMEM = {
  { "CLONE BANK         " },
  { "ASSIGN PATCH       " },
  { "ASSIGN EFFECT      " },
  { "SET FOOTSWITCHES   " },
  { "TOGGLE MODE        " },
  { "LONG PRESS DELAY   " },
  { "STARTING MODE      " },
  { "RESET MEMORY       " },
  { "FIRMWARE           " },
  { "EXIT               " },
  { "NORMAL MODE        " },
  { "E-PATCH MODE       " },
  { "E-EFFECT MODE      " },
  { "SELECT FOOTSWITCH: " },
  { "SELECT BANK:       " },
  { "FOOTSWITCH x EFF.: " },
  { "SELECT BANK:       " },
  { " ALL MODES         " },
  { "E-PATCH - E-EFFECTS" },
  { "TOGGLE MODE:       " },
  { "FOOTSWITCHES NUM:  " },
  { "CHANGE PRESS DELAY:" },
  { "RESET MEMORY:      " },
  { "CHOOSE MODE:       " },
  { "NORMAL - E-EFFECTS " },
  { " NORMAL - E-PATCH  " },
  { "SELECT DEST BANK:  " },
};

struct Zoom_FX_type_struct {  // Combines all the data we need for controlling a parameter in a device
  uint8_t Type;               // The MS70CDR has a byte for the FX type
  uint8_t Number;             // And a number
  char Name[8];               // The name for the label
};

const PROGMEM Zoom_FX_type_struct Zoom_FX_types[] = {
  // Table with the name and colour for every effect of the Zoom MS70-CDR
  { 0, 0, "--------" },     // 00
  { 12, 195, "CoronaCh" },  // 01
  { 12, 224, "Chorus  " },
  { 12, 33, "VintagCE" },
  { 12, 227, "ANA234Ch" },
  { 12, 3, "CE-Cho5 " },
  { 12, 35, "CloneCho" },
  { 12, 161, "SuperCho" },
  { 12, 163, "MirageCh" },
  { 12, 65, "StereoCh" },
  { 12, 4, "CoronaTr" },
  { 12, 97, "Ensemble" },
  { 12, 131, "SilkyCho" },
  { 12, 1, "Detune  " },
  { 12, 240, "BassCho " },
  { 12, 17, "BassDtun" },
  { 12, 113, "BassEnsb" },
  { 12, 193, "Flanger " },
  { 12, 209, "BassFlng" },
  { 12, 2, "Vibrato " },
  { 12, 96, "Phaser  " },
  { 12, 128, "TheVibe " },
  { 12, 66, "PtchSHFT" },
  { 12, 130, "HPS     " },
  { 12, 114, "BassPtch" },
  { 12, 32, "Duo-Trem" },
  { 12, 16, "Tremolo " },
  { 12, 64, "Slicer  " },
  { 12, 104, "DuoPhase" },
  { 12, 112, "WarpPhsr" },
  { 12, 177, "VinFLNGR" },
  { 12, 225, "DynFLNGR" },
  { 12, 34, "Octave  " },
  { 12, 50, "BassOctv" },
  { 12, 98, "MonoPtch" },
  { 12, 162, "Bend Cho" },
  { 12, 194, "MojoRoll" },
  { 12, 226, "Ring Mod" },
  { 12, 67, "StonePhs" },
  { 12, 99, "BF FLG 2" },
  { 14, 160, "Auto-Pan" },  // 40
  { 14, 192, "RotoClst" },
  { 14, 32, "BitCrush" },
  { 14, 64, "Bomber  " },
  { 14, 96, "MonoSynt" },
  { 14, 128, "Z-Organ " },
  { 16, 193, "DuoDigDl" },  // 46
  { 16, 16, "Delay   " },
  { 16, 129, "StompDly" },
  { 16, 33, "StereoDL" },
  { 16, 225, "CarbonDL" },
  { 16, 96, "AnalogDL" },
  { 16, 32, "TapeEcho" },
  { 16, 161, "TapeEch3" },
  { 16, 2, "DrivEcho" },
  { 16, 34, "SlapbkDL" },
  { 16, 66, "SmoothDL" },
  { 16, 160, "MultiTDL" },
  { 16, 128, "ReversDL" },
  { 16, 98, "Lo-FiDly" },
  { 16, 130, "SlowAtDL" },
  { 16, 192, "DynaDly " },
  { 16, 162, "TremolDL" },
  { 16, 64, "ModDelay" },
  { 16, 97, "TrgHldDL" },
  { 16, 194, "FilPP-DL" },
  { 16, 224, "FilterDL" },
  { 16, 65, "PhaseDly" },
  { 16, 226, "AutPanDL" },
  { 16, 1, "PitchDly" },
  { 16, 3, "ICEDelay" },
  { 16, 80, "ModDly2 " },
  { 18, 24, "HDReverb" },  // 72
  { 18, 128, "Spring  " },
  { 18, 129, "Spring63" },
  { 18, 33, "Plate   " },
  { 18, 64, "Room    " },
  { 18, 96, "TledRoom" },
  { 18, 161, "Chamber " },
  { 18, 193, "LoFiRvrb" },
  { 18, 32, "Hall    " },
  { 18, 16, "HD Hall " },
  { 18, 225, "Church  " },
  { 18, 160, "ArnaRvrb" },
  { 18, 2, "Cave    " },
  { 18, 34, "Ambience" },
  { 18, 224, "AIR     " },
  { 18, 192, "ErlyRflc" },
  { 18, 66, "GateRvrb" },
  { 18, 98, "RverRvrb" },
  { 18, 97, "SlpBkRvr" },
  { 18, 130, "Echo    " },
  { 18, 65, "ModRvrb " },
  { 18, 162, "TrmloRvr" },
  { 18, 194, "HolyFlrb" },
  { 18, 226, "DynRevrb" },
  { 18, 3, "ShimRvrb" },
  { 18, 35, "PartRvrb" },
  { 18, 67, "SpacHole" },
  { 18, 99, "MangldSp" },
  { 18, 131, "DualRvrb" },
  { 2, 160, "ZNR     " },  // 101
  { 2, 16, "COMP    " },
  { 2, 32, "RockComp" },
  { 2, 64, "M Comp  " },
  { 2, 96, "OptoComp" },
  { 2, 104, "160 COMP" },
  { 2, 112, "Limiter " },
  { 2, 128, "SlwAttck" },
  { 2, 192, "NoiseGat" },
  { 2, 224, "DirtyGat" },
  { 2, 1, "OrngeLim" },
  { 2, 33, "GrayComp" },
  { 2, 65, "DualComp" },
  { 4, 193, "StGtrGEQ" },  // 114
  { 4, 2, "StBasGEQ" },
  { 4, 16, "LineSel " },
  { 4, 32, "Graph EQ" },
  { 4, 48, "BsGrphEQ" },
  { 4, 64, "Para EQ " },
  { 4, 72, "BsParaEQ" },
  { 4, 80, "Splitter" },
  { 4, 88, "Bottom B" },
  { 4, 96, "Exciter " },
  { 4, 128, "CombFLTR" },
  { 4, 160, "Auto Wah" },
  { 4, 176, "BsAutWah" },
  { 4, 192, "Resnance" },
  { 4, 224, "Cry     " },
  { 4, 1, "SlwFlter" },
  { 4, 17, "Z Tron  " },
  { 4, 33, "M-Filter" },
  { 4, 41, "A-Filter" },
  { 4, 49, "Bass Cry" },
  { 4, 65, "STEP    " },
  { 4, 97, "SEQFlter" },
  { 4, 129, "RndmFltr" },
  { 4, 161, "FCycle  " },
  { 8, 16, "FD Combo" },  // 138
  { 8, 32, "Deluxe-R" },
  { 8, 64, "FD Vibro" },
  { 8, 96, "US Blues" },
  { 8, 128, "VX Combo" },
  { 8, 160, "VX Jimi " },
  { 8, 192, "BGCrunch" },
  { 8, 224, "Match30 " },
  { 8, 1, "CarDrive" },
  { 8, 33, "TW Rock " },
  { 8, 65, "ToneCity" },
  { 8, 97, "HW Stack" },
  { 8, 129, "Tangrine" },
  { 8, 161, "BBreaker" },
  { 8, 193, "MSCrunch" },
  { 8, 225, "MS 1959 " },
  { 8, 2, "MS Drive" },
  { 8, 34, "BGN DRV " },
  { 8, 66, "BG Drive" },
  { 8, 98, "DZ Drive" },
  { 8, 130, "Alien   " },
  { 8, 162, "Revo 1  " },
  { 6, 16, "Booster " },  // 160
  { 6, 32, "Ovrdrive" },
  { 6, 64, "T Scream" },
  { 6, 96, "Governor" },
  { 6, 128, "Dist +  " },
  { 6, 160, "Dist 1  " },
  { 6, 192, "Squeak  " },
  { 6, 224, "FuzzSmle" },
  { 6, 1, "GreatMuf" },
  { 6, 65, "HotBox  " },
  { 6, 97, "Z Clean " },
  { 6, 129, "Z MP1   " },
  { 6, 161, "Z Bottom" },
  { 6, 193, "Z Dream " },
  { 6, 225, "Z Scream" },
  { 6, 2, "Z Neos  " },
  { 6, 34, "Z Wild  " },
  { 6, 33, "MetlWRLD" },
  { 6, 66, "Z-Lead  " },
  { 6, 98, "ExtrmDst" },
  { 6, 130, "Acoustic" },
  { 6, 162, "CentGold" },
  { 6, 194, "NYC Muff" },
  { 6, 226, "TS Drive" },
  { 6, 3, "BGThrttl" },
  { 6, 35, "Oct Fuzz" },
  { 6, 67, "BG Grid " },
  { 6, 99, "RedCrnch" },
  { 6, 131, "TBMK 1.5" },
  { 6, 163, "SwtDrive" },
  { 6, 227, "RC Boost" },
  { 6, 36, "DynDrive" },
  { 0, 1, "Cho-Dly " },
  { 0, 2, "Cho-Rev " },
  { 0, 3, "Dly-Rev " },
  { 0, 4, "CompPhsr" },
  { 0, 5, "CompAWah" },
  { 0, 6, "FlngVCho" },
  { 0, 7, "Comp-OD " },
  { 0, 8, "PedalWah" },
  { 0, 9, "PedalCry" },
  { 0, 10, "PedalPtc" },
  { 0, 11, "PdlMnPit" },
  { 0, 12, "Exiter  " },
  { 0, 13, "Wah100  " },
  // Unique effects from MS-60B that do not crash the MS70-cdr
  /*  {10, 128, "SMR     "},
    {10, 160, "Flip Top"},
    {10, 1,   "Monotone"},
    {10, 33,  "Super B "},
    {10, 65,  "G-Kruegr"},
    {10, 97,  "TheHeavn"},
    {24, 65,  "T Scream"},
    {14, 112, "BasSynth"},
    {14, 224, "Std Syn "},
    {14, 1,   "Syn Tlk "},
    {14, 65,  "Defret  "},
    {14, 129, "V-Syn   "},
    {14, 161, "4VoicSyn"},
    {2, 48,   "D Comp  "},*/
};

//byte0 mask 11111110 -> shift 1 && byte 2 mask 00000001 <- shift 8  (-60 decimale)
const char pedalG5Effx[][8] PROGMEM = {
  { "Filter  " },
  { "" },
  { "Talk-Pdl" },
  { "RNDMTalk" },
  { "SpacWorm" },
  { "" },
  { "" },
  { "" },
  { "HotSpice" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "" },
  { "FuzzBack" },
  { "" },
  { "" },
  { "Custom  " },
  { "Starship" },
  { "" },
  { "W-Shift " },
  { "ChaosDly" },
  { "Cho-Rev " },
  { "FlngrDly" },
  { "" },
  { "TremPhsr" },
  { "" },
  { "" },
  { "" },
  { "Granular" },
  { "Pitch   " },
  { "RotaryZ " },
  { "EchoZ   " },
  { "FlangerZ" },
  { "Tremolo " },
  { "VolBoost" },
  { "SelFxTyp" },
};

//byte0 mask 11111110 -> shift 1 && byte1 mask 01000000 <- shift 1 && byte2 mask 00000001 <- shift 8
const PROGMEM uint8_t Zoom_G5_FX_types[] = {
  131,  //M-Filter
  21,   //TheVibe
  45,   //Z-Organ
  27,   //Slicer
  67,   //PhaseDly
  66,   //FilterDly
  69,   //PitchDelay
  49,   //StereoDelay
  42,   //BitCrush
  43,   //Bomber
  28,   //Duo-Phase
  44,   //MonoSynth
  135,  //SEQ Filter
  136,  //Random Filter
  29,   //WarpPhase
  64,   //TrgHoldDly
  192,  //Chorus-Delay
  193,  //Cho-Rev
  194,  //Dly-Rev
  195,  //Comp-Phsr
  196,  //Comp-Awah
  197,  //Flngr-Vcho
  198,  //Comp-OD
  102,  //COMP
  103,  //ROCKCOMP
  104,  //M-COMP
  108,  //Slow Attack
  101,  //ZNR
  109,  //NOISE GATE
  110,  //Dirty Gate
  117,  //Graph Eq
  119,  //PARA EQ
  124,  //Combi Filter
  125,  //AutoWah
  127,  //Resonance
  134,  //STEP
  128,  //CRY
  32,   //Octave
  26,   //Tremolo
  20,   //Phaser
  37,   //RingMod
  2,    //Chorus
  13,   //Detune
  3,    //VintageCE
  9,    //StereoCho
  11,   //Ensemble
  30,   //VinFLNGR
  31,   //DynaFLNGR
  19,   //Vibrato
  22,   //PitchSHFT
  35,   //BendCho
  34,   //MonoPitch
  23,   //HPS
  47,   //Delay
  52,   //TapeEcho
  63,   //ModDelay
  51,   //AnalogDly
  58,   //Reverse Delay
  57,   //MultiTapDelay
  61,   //DynaDelay
  80,   //HALL
  76,   //ROOM
  77,   //TiledRM
  73,   //Spring
  83,   //ArenaReverb
  87,   //EarlyReflection
  86,   //AIR
  199,  //Pedal-Wah
  200,  //Pedal-Cry
  201,  //PDL Pitch
  202,  //PdlMnPit
  160,  //Booster
  161,  //Overdrive
  162,  //T Scream
  163,  //Governor
  164,  //Dist+
  165,  //Dist1
  166,  //Sqeak
  167,  //FuzzSmile
  168,  //Great Muff
  177,  //Metal World
  169,  //HotBox
  176,  //Z-Wild
  178,  //LEAD
  179,  //Extreme Distortion
  180,  //Acoustic
  170,  //Z-Clean
  171,  //ZMp1
  172,  //BOTTOM
  173,  //Zdream
  174,  //Z-Scream
  175,  //Z-NEOS
  138,  //FD COMBO
  142,  //VX COMBO
  141,  //US BLUES
  144,  //BG CRUNCH
  149,  //HW STACK
  150,  //TANGERINE
  152,  //MS CRUNCH
  154,  //MS DRIVE
  156,  //BG DRIVE
  157,  //DZ DRIVE
  147,  //TW ROCK
  145,  //MATCH30
  140,  //FD VIBRO
  72,   //HD Reverb
  17,   //Flanger
  0,    //EMPTY
  148,  //TONE CITY
  151,  //B-BREAKER
  155,  //BGN DRV
  139,  //DELUXE-R
  158,  //Alien
  159,  //REVO-1
  146,  //CAR DRIVE
  203,  //Exiter
  40,   //AutoPan
  25,   //Duo-Trem
  137,  //F-Cycle
  129,  //SLOW Filter
  0,
  0,
  204,  //Wah100
  41,   //RotoCloset
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  105,  //OPT-COMP
  0,
  0,
  0,
  0,
  153,  //MS 1959
  143,  //VX JMI
  36,   //Mojo Roller
};

const uint8_t ZOOM_NUMBER_OF_FX = sizeof(Zoom_FX_types) / sizeof(Zoom_FX_types[0]);


const char string0[20] PROGMEM = "** ** FAILED! **  **";
const char string1[20] PROGMEM = "  USB Host Shield   ";
const char string2[20] PROGMEM = "   MP ELECTRONICS   ";
const char string3[20] PROGMEM = "        MIDI        ";
const char string4[20] PROGMEM = "     CONTROLLER     ";
const char string5[20] PROGMEM = "   WAITING USB...   ";
const char string6[20] PROGMEM = "-   ------------   +";
const char string7[20] PROGMEM = " HW NOT RECOGNIZED  ";
const char string8[20] PROGMEM = " CHANGE PATCH ONLY  ";
const char string9[4] PROGMEM = "ZOOM";
const char string10[6] PROGMEM = "MS-50G";
const char string12[6] PROGMEM = "MS-70C";
const char string13[6] PROGMEM = "  G5  ";
const char string14[6] PROGMEM = "  G3  ";
const char string15[20] PROGMEM = "< -----     ------ >";
const char string17[20] PROGMEM = " >  .....  .....  < ";
const char string18[20] PROGMEM = "-   <<<<<-->>>>>   +";
const char string19[20] PROGMEM = "       CANCEL       ";
const char string20[20] PROGMEM = "       SAVED!       ";
const char string21[20] PROGMEM = "  Firmware Version  ";
const char string22[20] PROGMEM = "    NOT ALLOWED     ";
const char string23[20] PROGMEM = ">>  E-EFFECTS     <<";
const char string25[20] PROGMEM = "      2.2.800       ";
const char string26[20] PROGMEM = " First Developed by ";
const char string27[20] PROGMEM = "  Massimo Procopio  ";
const char string28[20] PROGMEM = " Current FW Ver. by ";
const char string29[20] PROGMEM = "  Massimo Procopio  ";
const char string30[20] PROGMEM = "   GPL Licensed     ";
const char string31[20] PROGMEM = "     ENABLED        ";
const char string32[20] PROGMEM = "     DISABLED       ";
const char string35[20] PROGMEM = "  RESET ALL MEMORY  ";
const char string36[20] PROGMEM = "CLEARING ADDRESS... ";
const char string37[20] PROGMEM = "SETTING DEFAULT.... ";
const char string39[20] PROGMEM = "    PRESS KNOB      ";
const char string40[20] PROGMEM = "    INSERT CODE     ";
const char string47[20] PROGMEM = "    BASIC PATCH     ";
const char string48[20] PROGMEM = "massimoprocopio.com ";
const char string50[20] PROGMEM = "    ------------    ";



//*****************************************************************************************//
//                                      Init Big Font
//*****************************************************************************************//
void initBigFont() {
  byte nb;
  byte bb[8];
  byte bc = 0;
  for (nb = 0; nb < 8; nb++) {  // create 8 custom characters
    for (bc = 0; bc < 8; bc++)
      bb[bc] = pgm_read_byte(&custom[nb][bc]);
    lcd.createChar(nb + 1, bb);
  }
  clearAll();
}
// ********************************************************************************** //
//                                      SUBROUTINES
// ********************************************************************************** //
// writeBigChar: writes big character 'ch' to column x, row y; returns number of columns used by 'ch'
byte writeBigChar(char ch, byte x, byte y) {
  if (ch >= 'a' && ch <= 'z')
    ch = ch - ('a' - 'A');
  if (ch < ' ' || ch > '_')
    return 0;  // If outside table range, do nothing

  byte nb = 0;
  byte bb[8];
  byte bc = 0;  // character byte counter
  for (bc = 0; bc < 8; bc++) {
    bb[bc] = pgm_read_byte(&bigChars[ch - ' '][bc]);  // read 8 bytes from PROGMEM
    if (bb[bc] != 0) nb++;
  }

  bc = 0;
  byte col, row;

  for (row = y; row < y + 2; row++) {
    byte offChar = 0;
    for (col = x; col < x + nb / 2; col++) {
      lcd.setCursor(col - offChar, row);  // move to position
      if (col - offChar < 20) {
        lcd.write(bb[bc]);  // write byte and increment to next
      }
      bc++;
    }
  }
  return nb / 2;  // returns number of columns used by char
}

void clear(byte y) {
  lcd.setCursor(0, y);
  lcd.print("                    ");
}

void clearChars(byte y, byte x, byte charsNum) {
  lcd.setCursor(x, y);
  for (byte i = 0; i < charsNum; i++) {
    lcd.print(" ");
  }
}

void clearAll() {
  lcd.clear();
}
// writeBigString: writes out each letter of string
void writeBigString(char *str, byte x, byte y) {
  char c;
  while ((c = *str++)) {
    x += writeBigChar(c, x, y);
  }
}

byte checkModeButtonState() {
  uint8_t modeButtonState = digitalRead(FSModeButtonPin);

  if (modeButtonState == HIGH) {
    uint16_t cicles = 0;

    uint8_t countFakeLow = 0;
    do {
      modeButtonState = digitalRead(FSModeButtonPin);
      cicles++;
      delay(1);
      if (modeButtonState == LOW)
        countFakeLow++;
      else
        countFakeLow = 0;
    } while (countFakeLow < MAX_FAKE_LOW && cicles < extraFuncDelay);

    if (cicles == extraFuncDelay)
      return 2;
    else
      return 1;
  }

  return 0;
}

byte checkSwButtonState() {
  uint16_t cicles = 0;
  uint8_t countFakeLow = 0;
  uint8_t SwbuttonState = digitalRead(pinSw);
  if (SwbuttonState == LOW && !SwButtonWaitReset) {
    SwButtonWaitReset = true;
    do {
      SwbuttonState = digitalRead(pinSw);
      cicles++;
      delay(1);
      if (SwbuttonState == HIGH)
        countFakeLow++;
      else
        countFakeLow = 0;
    } while (countFakeLow < MAX_FAKE_LOW && cicles < 400);

    if (cicles == 400)
      return 2;
    else
      return 1;
  } else if (SwbuttonState == HIGH) {
    SwButtonWaitReset = false;
  }
  return 0;
}

void checkEncoderPosition() {
  bool currClk = digitalRead(pinClk);
  bool up = false;

  if (currClk != prevClk && !SwButtonWaitReset) {
    if (currClk)
      up = digitalRead(pinDt);
    else
      up = !digitalRead(pinDt);

    if (millis() - elapsedTime >= MIN_BOUNCE_TIME) {
      if (up)
        encoderCounter--;
      else
        encoderCounter++;
      elapsedTime = millis();
    }
    prevClk = currClk;
  }

  // Shows encoderCounter state
#ifdef DEBUG
#ifdef ENCODERTEST
  Serial.print(" = ");
  Serial.println(encoderCounter);
#endif
#endif
}

void displayProgmemDisplayLine(byte y, char *source) {
  memcpy_P(displayLine, source, 20);
  lcd.setCursor(0, y);
  lcd.print(displayLine);
}

void setup() {
  pinMode(SXbuttonPin, INPUT);
  pinMode(DXbuttonPin, INPUT);
  pinMode(CenterButtonPin, INPUT);
  pinMode(FSModeButtonPin, INPUT);

#ifdef USE_INTERNAL_PULLUP
  pinMode(FS4ButtonPin, INPUT_PULLUP);
  pinMode(FS5ButtonPin, INPUT_PULLUP);
  pinMode(FS6ButtonPin, INPUT_PULLUP);
#else
  pinMode(FS4ButtonPin, INPUT);
  pinMode(FS5ButtonPin, INPUT);
  pinMode(FS6ButtonPin, INPUT);
#endif


  pinMode(pinClk, INPUT);
  pinMode(pinDt, INPUT);
  pinMode(pinSw, INPUT);


  attachInterrupt(digitalPinToInterrupt(pinClk), checkEncoderPosition, CHANGE);

  // First pin reading
  prevClk = digitalRead(pinClk);

  //encoderCounter initialization
  encoderCounter = 0;

#ifdef DEBUG
  Serial.begin(115200);
#endif

  lcd.init();  // initialize LCD
  lcd.backlight();

  initBigFont();

  checkEepromData();

  if (Usb.Init() == -1) {
    clearAll();
    displayProgmemDisplayLine(1, string1);
    displayProgmemDisplayLine(2, string0);
    while (1)
      ;  // DO NOTHING
  }
  delay(200);
  displayProgmemDisplayLine(1, string2);

  clear(2);

  delay(1000);
  clearAll();
  displayProgmemDisplayLine(1, string3);
  displayProgmemDisplayLine(2, string4);
  delay(1000);

  clear(1);
  displayProgmemDisplayLine(2, string5);

  do {
    Usb.Task();
  } while (Usb.getUsbTaskState() != USB_STATE_RUNNING);

  memcpy_P(modeString, string6, 20);

  checkModel();
}

void checkEepromData() {
  char data;
  bool checkFailed = false;
  uint16_t addr = EEPROM_USER_NAME;
  memcpy_P(modeString, string29, 20);
  for (byte i = 0; i < 20; i++) {
    EEPROM.get(addr + i, data);
    if (data != modeString[i]) {
      checkFailed = true;
    }
  }
  addr = EEPROM_USER_ID;
  memcpy_P(modeString, string30, 20);
  for (byte i = 0; i < 20; i++) {
    EEPROM.get(addr + i, data);
    if (data != modeString[i]) {
      checkFailed = true;
    }
  }
  addr = EEPROM_FIRMWARE;
  memcpy_P(modeString, string25, 20);
  for (byte i = 0; i < 20; i++) {
    EEPROM.get(addr + i, data);
    if (data != modeString[i]) {
      checkFailed = true;
    }
  }

  if (checkFailed) {
    resetAll = true;
    resetAllMemory(true);

    addr = EEPROM_USER_NAME;
    memcpy_P(modeString, string29, 20);
    for (byte i = 0; i < 20; i++)
      EEPROM.put(addr + i, modeString[i]);

    addr = EEPROM_USER_ID;
    memcpy_P(modeString, string30, 20);
    for (byte i = 0; i < 20; i++)
      EEPROM.put(addr + i, modeString[i]);

    addr = EEPROM_FIRMWARE;
    memcpy_P(modeString, string25, 20);
    for (byte i = 0; i < 20; i++)
      EEPROM.put(addr + i, modeString[i]);
  }
}

void setCharArray(char *array, uint8_t sizeRecv, char *source) {
  for (uint8_t i = 0; i < sizeRecv; i++) {
    array[i] = source[i];
  }
}
void enableControl() {
  byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x50, 0xf7 };
  SendToMidi(Message, 6);  // Send the message to disabling communication to show Patch Name on Zoom
  delay(10);
}

void disableControl() {
  byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x51, 0xf7 };
  SendToMidi(Message, 6);  // Send the message to disabling communication to show Patch Name on Zoom
  delay(10);
}

void checkModel() {
  byte Message[6] = { 0xf0, 0x7e, 0x00, 0x06, 0x01, 0xf7 };

  bool received = false;
  byte contMaxTry = 0;

  while (!received && contMaxTry < MAX_TRY) {
    SendToMidi(Message, 6);  // Send the message to check model
    delay(10);
    // F0 7E 00 06 02 52 5B 00 00 00 31 2E 32 30 F7    zoom g5
    // F0 7E 00 06 02 52 58 00 00 00 33 2E 30 30 F7    zoom ms50g
    byte recMessage[15];
    received = RecvMidi(recMessage, 15, 2000, false);  // get the message
    if (!received)
      contMaxTry++;
    else
      zoomModel = recMessage[6];
  }

  clearAll();
  char brand[5] = { "\0\0\0\0\0" };
  char model[7] = { "\0\0\0\0\0\0\0" };
  memcpy_P(brand, string9, 4);
  if (zoomModel == MS_50G) {
    writeBigString(brand, 3, 0);
    memcpy_P(model, string10, 6);
    writeBigString(model, 0, 2);
    maxEffects = 6;
    maxPresetNumber = 50;
    minPresetNumber = 1;
    dataMessageLen = 146;
    dataMessageNameLen = 159;
  }
#ifndef EXCLUDE_MS_70C
  else if (zoomModel == MS_70C) {
    writeBigString(brand, 3, 0);
    memcpy_P(model, string12, 6);
    writeBigString(model, 0, 2);
    maxEffects = 6;
    maxPresetNumber = 50;
    minPresetNumber = 1;
    dataMessageLen = 146;
    dataMessageNameLen = 159;
  }
#endif
#ifndef EXCLUDE_G5
  else if (zoomModel == G5) {
    writeBigString(brand, 3, 0);
    memcpy_P(model, string13, 6);
    writeBigString(model, 4, 2);
    maxEffects = 9;
    maxPresetNumber = 296;
    minPresetNumber = 0;
    dataMessageLen = 164;
    dataMessageNameLen = 174;
  }
#endif
#ifndef EXCLUDE_G3
  else if (zoomModel == G3) {
    writeBigString(brand, 3, 0);
    memcpy_P(model, string14, 6);
    writeBigString(model, 4, 2);
    maxEffects = 6;
    maxPresetNumber = 98;
    minPresetNumber = 0;
    dataMessageLen = 123;
    dataMessageNameLen = 159;
  }
#endif
#ifndef EXCLUDE_UNKNOWN
  else {
    displayProgmemDisplayLine(1, string7);
    displayProgmemDisplayLine(2, string8);
    maxEffects = 0;
    maxPresetNumber = 98;
    minPresetNumber = 0;
    unknownDevice = true;
  }
#endif
  delay(1000);
  getExtraFuncDelay();
  getSavedToggleMode();
  getSavedActiveFootswitches();

  clearAll();

  enableControl();

  checkPatchChange(true);

  getStartingMode();
}

void getPatchCurrentName() {
  cleanPatchName();
  if (!unknownDevice) {
    bool received = false;
    byte contMaxTry = 0;

    byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x29, 0xf7 };
    while (!received && contMaxTry < MAX_TRY) {
      cleanMidiRec(50);
      SendToMidi(Message, 6);  // Send the message to request patch data
      if (zoomModel == MS_50G || zoomModel == MS_70C) {
        byte recMessage[dataMessageLen];
        if (RecvMidi(recMessage, dataMessageLen, 1000, true)) {
          uint16_t posByte = 132;
          for (byte i = 0; i < 10; i++) {
            patchName[i] = recMessage[posByte];
            posByte++;
            if (i == 0 || i == 7)
              posByte++;
          }
          received = true;
        } else
          contMaxTry++;
      } else if (zoomModel == G5 || zoomModel == G3) {
        byte recMessage[dataMessageLen];
        if (RecvMidi(recMessage, dataMessageLen, 1000, true)) {
          uint16_t posByte = 151;
          if (zoomModel == G3)
            posByte = 110;
          for (byte i = 0; i < 10; i++) {
            patchName[i] = recMessage[posByte];
            posByte++;
            if (i == 5)
              posByte++;
          }
          received = true;
        } else
          contMaxTry++;
      }
    }
  }
}

void cleanPatchName() {
  for (byte i = 0; i < 10; i++)
    patchName[i] = ' ';
}

void cleanReqPatchName() {
  for (byte i = 0; i < 10; i++)
    reqPatchName[i] = ' ';
}

void cleanNextPatchName() {
  for (byte i = 0; i < 10; i++)
    nextPatchName[i] = ' ';
}


void getReqestedPatchName(int16_t reqPatch) {
  cleanReqPatchName();
  if (!unknownDevice) {
    if (reqPatch >= minPresetNumber && reqPatch <= maxPresetNumber) {

      byte OtherPreset;
      byte OtherPreset2 = 0x00;

      if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
        OtherPreset = reqPatch - 1;
      } else if (zoomModel == G5) {
        uint16_t Bank = reqPatch / 3;
        byte patch = reqPatch % 3;
        if (Bank == 100)
          Bank = 0;
        if (patch == 3)
          patch = 0;

        OtherPreset = patch;
        OtherPreset2 = Bank;
      }

      bool received = false;
      byte contMaxTry = 0;
      byte NextDataMessage[9] = { 0xf0, 0x52, 0x00, zoomModel, 0x09, 0x00, OtherPreset2, OtherPreset, 0xf7 };  
      while (!received && contMaxTry < MAX_TRY) {
        cleanMidiRec(10);
        SendToMidi(NextDataMessage, 9);  // Send the message to request patch data
        delay(10);
        if (zoomModel == MS_50G || zoomModel == MS_70C) {
          byte recMessage[dataMessageNameLen];
          if (RecvMidi(recMessage, dataMessageNameLen, 2000, true)) {
            uint16_t posByte = 137;
            for (byte i = 0; i < 10; i++) {
              reqPatchName[i] = recMessage[posByte];
              posByte++;
              if (i == 0 || i == 7)
                posByte++;
            }
            received = true;
          } else
            contMaxTry++;
        } else if (zoomModel == G5 || zoomModel == G3) {
          byte recMessage[dataMessageNameLen];
          if (RecvMidi(recMessage, dataMessageNameLen, 2000, true)) {
            uint16_t posByte = 156;
            if (zoomModel == G3)
              posByte = 120;
            for (byte i = 0; i < 10; i++) {
              reqPatchName[i] = recMessage[posByte];
              posByte++;
              if (i == 5)
                posByte++;
            }
            received = true;
          } else
            contMaxTry++;
        }
      }
    }
  }
}


#ifdef DEBUG
void sendStringToTest(char *title, byte *message, int lenght) {
  Serial.println(title);
  for (int i = 0; i < lenght; i++) {
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}
#endif

void getOtherPatchName() {
  cleanNextPatchName();
  if (!unknownDevice) {
    byte OtherPreset;
    byte OtherPreset2 = 0x00;

    if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
      OtherPreset = Preset - 1;
    } else if (zoomModel == G5) {
      uint16_t Bank = Preset / 3;
      byte patch = Preset % 3;
      if (Bank == 100)
        Bank = 0;
      if (patch == 3)
        patch = 0;

      OtherPreset = patch;
      OtherPreset2 = Bank;
    }

    byte NextDataMessage[9] = { 0xf0, 0x52, 0x00, zoomModel, 0x09, 0x00, OtherPreset2, OtherPreset, 0xf7 };
    bool received = false;
    byte contMaxTry = 0;
    while (!received && contMaxTry < MAX_TRY) {
      cleanMidiRec(50);
      SendToMidi(NextDataMessage, 9);  // Send the message to request patch data
      delay(10);
      if (zoomModel == MS_50G || zoomModel == MS_70C) {
        byte recMessage[dataMessageNameLen];
        if (RecvMidi(recMessage, dataMessageNameLen, 2000, true)) {
          uint16_t posByte = 137;
          for (byte i = 0; i < 10; i++) {
            nextPatchName[i] = recMessage[posByte];
            posByte++;
            if (i == 0 || i == 7)
              posByte++;
          }
          received = true;
        } else
          contMaxTry++;
      } else if (zoomModel == G5 || zoomModel == G3) {
        byte recMessage[dataMessageNameLen];
        if (RecvMidi(recMessage, dataMessageNameLen, 2000, true)) {
          uint16_t posByte = 156;
          if (zoomModel == G3)
            posByte = 120;
          for (byte i = 0; i < 10; i++) {
            nextPatchName[i] = recMessage[posByte];
            posByte++;
            if (i == 5)
              posByte++;
          }
          received = true;
        } else
          contMaxTry++;
      }
    }
  }
}

void exitMenu() {
  clearAll();
  menuId = 0;
  menuActive = false;
  menuInitialized = false;

  changeActualMode();

  delay(300);
}

void changeActualMode() {
  if (actualMode == NORMAL_MODE)
    DisplayCurrentPatch();
  else if (actualMode == PATCH_MODE)
    DisplayPatchMode();
  else if (actualMode == EFFECTS_MODE)
    DisplayEffectsMode();

  modeSelect = UP_DOWN_PATCH_MODE;
}

void displaySelectedFootswitch() {
  clear(2);
  char footStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  if (selectedFootswitch == 0) {
    memcpy_P(footStr, string19, 20);
    lcd.setCursor(0, 2);
    lcd.print(footStr);
  } else if (selectedFootswitch == 1) {
    memcpy_P(footStr, string47, 20);
    lcd.setCursor(0, 2);
    lcd.print(footStr);
  } else {
    lcd.setCursor(9, 2);
    lcd.print(selectedFootswitch - 1);
  }
}

void displayToggleModeOptions(byte num) {
  clear(2);
  char boolStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  switch (num) {
    case 0:
      memcpy_P(boolStr, menu[17], 19);
      break;
    case 1:
      memcpy_P(boolStr, menu[18], 19);
      break;
    case 2:
      memcpy_P(boolStr, menu[24], 19);
      break;
    case 3:
      memcpy_P(boolStr, menu[25], 19);
      break;
  }

  lcd.setCursor(0, 2);
  lcd.print(boolStr);
}

void displayResetOptions() {
  clear(2);
  char boolStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  if (resetAll)
    memcpy_P(boolStr, string35, 20);
  else
    memcpy_P(boolStr, string19, 20);

  lcd.setCursor(0, 2);
  lcd.print(boolStr);
}

void displayStartingMode() {
  clear(2);
  char boolStr[20] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  switch (startingMode) {
    case 0:
      memcpy_P(boolStr, menu[10], 19);
      break;
    case 1:
      memcpy_P(boolStr, menu[11], 19);
      break;
    case 2:
      memcpy_P(boolStr, menu[12], 19);
      break;
  }
  lcd.setCursor(0, 2);
  lcd.print(boolStr);
}

void displayNumFootswitches(byte num) {
  clear(2);

  lcd.setCursor(9, 2);
  lcd.print(num);
}

void displayExtraFuncDelay() {
  clear(2);
  lcd.setCursor(6, 2);
  lcd.print(extraFuncDelayTemp);
  if (extraFuncDelayTemp >= 1000)
    lcd.setCursor(11, 2);
  else
    lcd.setCursor(10, 2);
  lcd.print("ms");
}

void displaySelectedEffect() {
  lcd.setCursor(11, 0);
  lcd.print(assignedEffectNum + 1);
  clear(2);
  char footStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  if (selectedEffect == 0) {
    memcpy_P(footStr, string19, 20);
    lcd.setCursor(0, 2);
    lcd.print(footStr);
  } else {
    lcd.setCursor(9, 2);
    lcd.print(selectedEffect);
  }
}

void displaySelectedDestBank() {
  clear(2);
  char confStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  if (destBank == 0) {
    memcpy_P(confStr, string19, 20);
    lcd.setCursor(0, 2);
    lcd.print(confStr);
  } else {
    lcd.setCursor(9, 2);
    lcd.print(destBank - 1);
  }
  if (actualMode == NORMAL_MODE) {
    displayProgmemDisplayLine(2, string22);
  }
}

void displaySelectedConf() {
  clear(2);
  char confStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  if (selectedConf == 0) {
    memcpy_P(confStr, string19, 20);
    lcd.setCursor(0, 2);
    lcd.print(confStr);
  } else {
    lcd.setCursor(9, 2);
    lcd.print(selectedConf - 1);
  }
}

void saveToggleMode() {
  uint16_t addr = EEPROM_TOGGLE_MODE_ADDR;

  EEPROM.put(addr, toggleMode);

  clear(1);
  char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  memcpy_P(labelStr, string20, 20);
  lcd.setCursor(0, 1);
  lcd.print(labelStr);
  delay(500);
}

void saveActiveFootswitches() {
  uint16_t addr = EEPROM_FOOTSWITCHES_DEFAULT_ADDR;

  EEPROM.put(addr, activeFootswitchNum);

  clear(1);
  clear(2);
  char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  memcpy_P(labelStr, string20, 20);
  lcd.setCursor(0, 1);
  lcd.print(labelStr);

  lcd.setCursor(9, 2);
  lcd.print(activeFootswitchNum);
  delay(500);
}

void resetAllMemory(bool reinitialize) {
  if (resetAll) {
    clear(1);
    clear(2);
    char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
    memcpy_P(labelStr, string36, 20);
    lcd.setCursor(0, 0);
    lcd.print(labelStr);

    for (uint16_t i = 0; i < 1024; i++) {
      if (reinitialize) {
        EEPROM.put(i, 0x00);
      }
      lcd.setCursor(8, 2);
      lcd.print(i);
    }
    resetAll = false;
  }
}

void saveStartingMode() {
  uint16_t addr = EEPROM_STARTING_MODE;

  EEPROM.put(addr, startingMode);

  clear(1);
  clear(2);
  char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  memcpy_P(labelStr, string20, 20);
  lcd.setCursor(0, 1);
  lcd.print(labelStr);

  char resStr[20] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  switch (startingMode) {
    case 0:
      memcpy_P(resStr, menu[10], 19);
      break;
    case 1:
      memcpy_P(resStr, menu[11], 19);
      break;
    case 2:
      memcpy_P(resStr, menu[12], 19);
      break;
  }
  lcd.setCursor(0, 2);
  lcd.print(resStr);
  delay(500);
}

void saveExtraFunctionsDelay() {
  extraFuncDelay = extraFuncDelayTemp;
  uint16_t addr = EEPROM_EXTRA_FUNC_DELAY;

  byte val = extraFuncDelay / 250;
  EEPROM.put(addr, val);

  clear(1);
  clear(2);
  char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
  memcpy_P(labelStr, string20, 20);
  lcd.setCursor(0, 1);
  lcd.print(labelStr);

  lcd.setCursor(6, 2);
  lcd.print(extraFuncDelay);
  if (extraFuncDelayTemp >= 1000)
    lcd.setCursor(11, 2);
  else
    lcd.setCursor(10, 2);

  lcd.print("ms");

  delay(500);
}

void getSavedToggleMode() {
  uint16_t addr = EEPROM_TOGGLE_MODE_ADDR;
  byte val;
  EEPROM.get(addr, val);
  if (val < 4)
    toggleMode = val;
  else {
    toggleMode = 0;
    clearAll();
    char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
    memcpy_P(labelStr, string37, 20);
    lcd.setCursor(0, 0);
    lcd.print(labelStr);
    saveToggleMode();
  }
}

void getSavedActiveFootswitches() {
  uint16_t addr = EEPROM_FOOTSWITCHES_DEFAULT_ADDR;
  byte val;
  EEPROM.get(addr, val);
  if (val <= NUM_FOOTSWITCH && val >= 3)
    activeFootswitchNum = val;
  else {
    activeFootswitchNum = 3;
    clearAll();
    char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
    memcpy_P(labelStr, string37, 20);
    lcd.setCursor(0, 0);
    lcd.print(labelStr);
    saveActiveFootswitches();
  }
}

void getStartingMode() {
  uint16_t addr = EEPROM_STARTING_MODE;
  byte val;
  EEPROM.get(addr, val);
  switch (val) {
    case 0:
      enableNormalModeUpDownPatch();
      break;
    case 1:
      enablePatchMode();
      break;
    case 2:
      enableEffectsMode();
      break;
  }
}

void getExtraFuncDelay() {
  uint16_t addr = EEPROM_EXTRA_FUNC_DELAY;
  byte val;
  EEPROM.get(addr, val);

  if (val > 0 && val <= 20)
    extraFuncDelay = val * 250;
  else {
    extraFuncDelay = 750;
    clearAll();
    char labelStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
    memcpy_P(labelStr, string37, 20);
    lcd.setCursor(0, 0);
    lcd.print(labelStr);

    saveExtraFunctionsDelay();
  }
}

void menuModeLoop() {
  byte menuPosMax = 0;
  byte initialPos = 0;
  if (menuId == 0)  //MAIN MENU
  {
    menuPosMax = 9;
    initialPos = 0;
  } else if (menuId == 1)  //CLONING
  {
    menuPosMax = 0;
    initialPos = 26;
  } else if (menuId == 2)  //ASSIGN PATCH
  {
    menuPosMax = 0;
    initialPos = 13;
  } else if (menuId == 3)  //ASSIGN CONFIRM (PATCH AND EFFECTS)
  {
    menuPosMax = 0;
    initialPos = 14;
  } else if (menuId == 4)  //ASSIGN EFFECTS
  {
    menuPosMax = 0;
    if (assignedEffectNum == activeFootswitchNum)
      initialPos = 16;
    else
      initialPos = 15;
  } else if (menuId == 5)  // TOOGLE MODE
  {
    menuPosMax = 0;
    initialPos = 19;
  } else if (menuId == 6)  //NUM FOOTSWITCHES
  {
    menuPosMax = 0;
    initialPos = 20;
  } else if (menuId == 7)  //LONG PRESS DELAY
  {
    menuPosMax = 0;
    initialPos = 21;
  } else if (menuId == 8)  // RESET MEMORY
  {
    menuPosMax = 0;
    initialPos = 22;
  } else if (menuId == 9)  // STARTING MODE
  {
    menuPosMax = 0;
    initialPos = 23;
  }

  if (!menuInitialized) {
    menuPos = 0;
    offsetMenuPos = 0;
    destBank = 0;
    selectedFootswitch = 0;
    selectedConf = 0;
    selectedEffect = 0;
    menuInitialized = true;
    redrawMenu = true;
    extraFuncDelayTemp = extraFuncDelay;
  }

  if (encoderCounter > prevEncoderCounter && !SwButtonWaitReset) {
    prevEncoderCounter = encoderCounter;

    if (isAssigningCloning) {
      if (actualMode == EFFECTS_MODE) {
        if (destBank < NUM_CONF_EFFECTS)
          destBank++;
      } else if (actualMode == PATCH_MODE) {
        if (destBank < NUM_CONF_PATCHES)
          destBank++;
      }
      displaySelectedDestBank();
    }

    if (isAssigningToggleMode) {
      if (toggleMode < 3) {
        toggleMode++;
        displayToggleModeOptions(toggleMode);
      }
    }
    if (isAssigningExtraDelay) {
      if (extraFuncDelayTemp < 5000)
        extraFuncDelayTemp = extraFuncDelayTemp + 250;
      displayExtraFuncDelay();
    }
    if (isAssigningActiveFootswitches) {
      if (activeFootswitchNum < NUM_FOOTSWITCH) {
        activeFootswitchNum++;
        displayNumFootswitches(activeFootswitchNum);
      }
    }
    if (isResettingAll) {
      if (!resetAll) {
        resetAll = true;
        displayResetOptions();
      }
    }

    if (isStartingMode) {
      startingMode++;
      if (startingMode > 2)
        startingMode = 2;
      displayStartingMode();
    }

    if (menuPos < menuPosMax)
      menuPos++;

    if (menuPos - offsetMenuPos > 3) {
      byte diffMenuPos = menuPos - 3;
      initialPos = initialPos + diffMenuPos;
      if (diffMenuPos != offsetMenuPos) {
        redrawMenu = true;
      }
      offsetMenuPos = diffMenuPos;
    }

    if (selectedEffect < maxEffects)
      selectedEffect++;


    if (isAssigningPatch) {
      if (selectedFootswitch < activeFootswitchNum + 1)
        selectedFootswitch++;

      if (selectedConf < NUM_CONF_PATCHES)
        selectedConf++;

      if (isAssigningConf) {
        displaySelectedConf();
      } else {
        displaySelectedFootswitch();
      }
    }
    if (isAssigningEffect) {
      if (selectedFootswitch < activeFootswitchNum)
        selectedFootswitch++;

      if (selectedConf < NUM_CONF_EFFECTS)
        selectedConf++;

      if (isAssigningConf) {
        displaySelectedConf();
      } else {
        displaySelectedEffect();
      }
    }
  }
  if (encoderCounter < prevEncoderCounter && !SwButtonWaitReset) {
    prevEncoderCounter = encoderCounter;

    if (isAssigningCloning) {
      if (destBank > 0)
        destBank--;
      displaySelectedDestBank();
    }
    if (isAssigningToggleMode) {
      if (toggleMode > 0) {
        toggleMode--;
        displayToggleModeOptions(toggleMode);
      }
    }
    if (isAssigningExtraDelay) {
      if (extraFuncDelayTemp > 250)
        extraFuncDelayTemp = extraFuncDelayTemp - 250;
      displayExtraFuncDelay();
    }
    if (isAssigningActiveFootswitches) {
      if (activeFootswitchNum > 3) {
        activeFootswitchNum--;
        displayNumFootswitches(activeFootswitchNum);
      }
    }
    if (isResettingAll) {
      if (resetAll) {
        resetAll = false;
        displayResetOptions();
      }
    }

    if (isStartingMode) {
      startingMode--;
      if (startingMode > 254)
        startingMode = 0;
      displayStartingMode();
    }


    if (menuPos == offsetMenuPos && offsetMenuPos > 0) {
      redrawMenu = true;
      offsetMenuPos--;
      initialPos = initialPos + offsetMenuPos;
    }

    if (menuPos > 0)
      menuPos--;

    if (selectedFootswitch > 0)
      selectedFootswitch--;

    if (selectedEffect > 0)
      selectedEffect--;

    if (selectedConf > 0)
      selectedConf--;

    if (isAssigningPatch) {
      if (isAssigningConf) {
        displaySelectedConf();
      } else {
        displaySelectedFootswitch();
      }
    }

    if (isAssigningEffect) {
      if (isAssigningConf) {
        displaySelectedConf();
      } else {
        displaySelectedEffect();
      }
    }
  }

  if (redrawMenu) {

    for (byte i = 0; i <= menuPosMax && i < 4; i++) {
      memcpy_P(displayLine, menu[initialPos + i], 19);
      lcd.setCursor(0, i);
      lcd.print(displayLine);
    }
    if (menuPosMax < 3) {
      for (byte j = menuPosMax + 1; j < 4; j++)
        clear(j);
    }

    if (isAssigningPatch) {
      if (isAssigningConf)
        displaySelectedConf();
      else
        displaySelectedFootswitch();
    }

    if (isAssigningActiveFootswitches) {
      displayNumFootswitches(activeFootswitchNum);
    }
    if (isAssigningToggleMode) {
      displayToggleModeOptions(toggleMode);
    }
    if (isAssigningExtraDelay) {
      displayExtraFuncDelay();
    }
    if (isResettingAll) {
      displayResetOptions();
    }
    if (isStartingMode) {
      displayStartingMode();
    }
    if (isAssigningCloning) {
      displaySelectedDestBank();
    }
    if (isAssigningEffect) {
      if (isAssigningConf)
        displaySelectedConf();
      else
        displaySelectedEffect();
    }

    redrawMenu = false;
  }
  if (isAssigningPatch) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        if (selectedFootswitch > 0) {
          assignedFootswitch = selectedFootswitch - 1;
          isAssigningConf = true;
          menuId = 3;
          menuInitialized = false;
          delay(300);
        } else {
          isAssigningPatch = false;
          exitMenu();
        }
      }
    } else {
      if (checkSwButtonState() >= 1) {
        if (selectedConf > 0) {
          assignPatch(selectedConf, Preset);
          exitMenu();
        } else {
          isAssigningPatch = false;
          isAssigningConf = false;
          exitMenu();
        }
      }
    }
  } else if (isAssigningActiveFootswitches) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        saveActiveFootswitches();
        isAssigningActiveFootswitches = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else if (isAssigningToggleMode) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        saveToggleMode();
        isAssigningToggleMode = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else if (isAssigningExtraDelay) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        saveExtraFunctionsDelay();
        isAssigningExtraDelay = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else if (isResettingAll) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        resetAllMemory(false);
        isResettingAll = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else if (isStartingMode) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        saveStartingMode();
        isStartingMode = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else if (isAssigningEffect) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        if (selectedEffect > 0) {
          if (assignedEffectNum == activeFootswitchNum - 1) {
            assignedEffects[assignedEffectNum] = selectedEffect;
            isAssigningConf = true;
            menuId = 3;
          } else {
            assignedEffects[assignedEffectNum] = selectedEffect;
            selectedEffect = 0;
            assignedEffectNum++;
          }
          menuInitialized = false;
          delay(300);

        } else {
          isAssigningEffect = false;
          exitMenu();
        }
      }
    } else {
      if (checkSwButtonState() >= 1) {
        if (selectedConf > 0) {
          assignEffect(selectedConf);
          exitMenu();
        } else {
          isAssigningEffect = false;
          isAssigningConf = false;
          assignedEffectNum = 0;
          exitMenu();
        }
      }
    }
  } else if (isAssigningCloning) {
    lcd.setCursor(19, 0);
    lcd.print(' ');

    if (!isAssigningConf) {
      if (checkSwButtonState() >= 1) {
        clone();
        isAssigningCloning = false;
        isAssigningConf = false;
        exitMenu();
      }
    }
  } else {
    lcd.setCursor(19, menuPos - offsetMenuPos);
    lcd.print('<');
    for (int k = 0; k <= menuPosMax && k < 4; k++) {
      if (k != menuPos - offsetMenuPos) {
        lcd.setCursor(19, k);
        lcd.print(" ");
      }
    }

    if (checkSwButtonState() >= 1) {
      if (menuId == 0)  //MAIN MENU
      {
        if (menuPos == 0)  //Cloning
        {
          menuId = 1;
          menuInitialized = false;
          isAssigningCloning = true;
          delay(300);
        } else if (menuPos == 1)  // ASSIGN PATCH
        {
          menuId = 2;
          selectedFootswitch = 0;
          selectedConf = 0;
          menuInitialized = false;
          isAssigningPatch = true;
          delay(300);
        } else if (menuPos == 2)  //ASSIGN EFFECT
        {
          menuId = 4;
          selectedEffect = 0;
          selectedConf = 0;
          menuInitialized = false;
          isAssigningEffect = true;
          assignedEffectNum = 0;
          delay(300);
        } else if (menuPos == 3)  //NUM FOOTSWITCHES
        {
          menuId = 6;
          menuInitialized = false;
          isAssigningActiveFootswitches = true;
          delay(300);
        } else if (menuPos == 4)  //LONG PRESS MODE
        {
          menuId = 5;
          menuInitialized = false;
          isAssigningToggleMode = true;
          delay(300);
        } else if (menuPos == 5)  //LONG PRESS DELAY
        {
          menuId = 7;
          menuInitialized = false;
          isAssigningExtraDelay = true;
          delay(300);
        } else if (menuPos == 6)  //STARTING MODE
        {
          menuId = 9;
          menuInitialized = false;
          isStartingMode = true;

          delay(300);
        } else if (menuPos == 7)  //RESET MEMORY
        {
          menuId = 8;
          menuInitialized = false;
          isResettingAll = true;
          delay(300);
        } else if (menuPos == 8)  //FIRMWARE VERSION
        {
          displayFirmware();
        } else if (menuPos == 9)  //EXIT
        {
          exitMenu();
        }
      }
    }
  }
}

void displayFirmware() {
  clearAll();
  displayProgmemDisplayLine(1, string21);
  displayProgmemDisplayLine(2, string25);
  delay(2000);
  clearAll();
  displayProgmemDisplayLine(0, string26);
  displayProgmemDisplayLine(2, string27);
  displayProgmemDisplayLine(3, string48);
  delay(5000);
  clearAll();
  displayProgmemDisplayLine(0, string28);
  displayProgmemDisplayLine(2, string29);
  displayProgmemDisplayLine(3, string30);
  delay(3000);
  exitMenu();
}

void clone() {
  clearAll();

  if (destBank > 0) {
    if (actualMode == PATCH_MODE) {
      char boolStr[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
      memcpy_P(boolStr, menu[1], 19);
      lcd.setCursor(0, 0);
      lcd.print(boolStr);
      for (byte i = 0; i <= NUM_FOOTSWITCH; i++) {
        assignedFootswitch = i;
        lcd.setCursor(15, 0);
        lcd.print(i);
        assignPatch(destBank, patchPreset[i]);
      }
    } else if (actualMode == EFFECTS_MODE) {
      assignEffect(destBank);
    }
  }
}

void assignPatch(byte conf, int16_t demandedPreset) {
  isAssigningPatch = false;
  isAssigningConf = false;

  uint16_t addr = EEPROM_PATCH_INITIAL_ADDR + (conf - 1) * NUM_BYTES_FOR_PATCHES + assignedFootswitch * 2;

  EEPROM.put(addr, demandedPreset);

  displayProgmemDisplayLine(2, string20);
  delay(1000);
  //exitMenu();
}
void assignEffect(byte conf) {
  isAssigningEffect = false;
  isAssigningConf = false;

  uint16_t addr = EEPROM_EFFECTS_INITIAL_ADDR + (conf - 1) * NUM_FOOTSWITCH;
  for (byte i = 0; i < activeFootswitchNum; i++) {
    EEPROM.put(addr, assignedEffects[i]);
    addr++;
  }

  displayProgmemDisplayLine(2, string20);
  delay(1000);
  //exitMenu();
}

void toggleChoosenModes() {
  switch (toggleMode) {
    case 0:
      toggleModes();
      break;
    case 1:
      if (actualMode == NORMAL_MODE)
        enablePatchMode();
      else if (actualMode == PATCH_MODE)
        enableEffectsMode();
      else if (actualMode == EFFECTS_MODE)
        enablePatchMode();
      break;
    case 2:
      if (actualMode == NORMAL_MODE)
        enableEffectsMode();
      else if (actualMode == PATCH_MODE)
        enableEffectsMode();
      else if (actualMode == EFFECTS_MODE)
        enableNormalModeUpDownPatch();
      break;
    case 3:
      if (actualMode == NORMAL_MODE)
        enablePatchMode();
      else if (actualMode == PATCH_MODE)
        enableNormalModeUpDownPatch();
      else if (actualMode == EFFECTS_MODE)
        enableNormalModeUpDownPatch();
      break;
  }
}

void toggleModes() {
  if (actualMode == NORMAL_MODE)
    enablePatchMode();
  else if (actualMode == PATCH_MODE)
    enableEffectsMode();
  else if (actualMode == EFFECTS_MODE)
    enableNormalModeUpDownPatch();
}

void showEPatchTitle() {
  if (activeFootswitchNum > 3) {
    lcd.setCursor(12, 0);
    lcd.print("-  EP-");
    lcd.print(selectedConfPatch);
  } else {
    lcd.setCursor(15, 0);
    lcd.print("EP-");
    lcd.print(selectedConfPatch);

    for (byte k = 0; k <= 3; k++) {
      lcd.setCursor(0, k);
      lcd.print(k);
    }
  }
}

void unsetScrollLine() {
  if (actualMode == EFFECTS_MODE) {
    clear(0);
    displayEEfectsTitle();
  } else if (actualMode == PATCH_MODE) {
    lcd.setCursor(10, 0);
    lcd.print("         ");
    showEPatchTitle();
  }
}
void setScrollLine() {
  if (prevMode == EFFECTS_MODE) {
    lcd.setCursor(0, 0);
    lcd.print("      <<< >>> ");
    lcd.setCursor(18, 0);
    lcd.print("  ");
  } else if (prevMode == PATCH_MODE) {
    lcd.setCursor(10, 0);
    lcd.print("<<< ");
    lcd.setCursor(14, 0);
    lcd.print(">>> ");
  }
}
void loop() {
  byte modeState = checkModeButtonState();
  if (!menuActive) {
    if (actualMode != SCROLL_MODE && modeState == 1) {
      toggleChoosenModes();
    } else if (actualMode != SCROLL_MODE && actualMode != NORMAL_MODE && modeState == 2) {
      prevMode = actualMode;
      actualMode = SCROLL_MODE;
      setScrollLine();

      ModeButtonWaitReset = true;
    } else if (actualMode != SCROLL_MODE && modeState == 0) {
      byte stateSw = checkSwButtonState();
      if (stateSw == 1) {
        toggleModes();
      } else if (stateSw == 2) {
        menuId = 0;
        menuActive = true;
        delay(300);
      } else {
        if (actualMode == NORMAL_MODE)
          normalModeLoop();
        else if (actualMode == PATCH_MODE)
          patchModeLoop();
        else if (actualMode == EFFECTS_MODE)
          effectsModeLoop();
      }
    } else if (actualMode == SCROLL_MODE) {
      uint8_t timeDelayButton = 100;
      uint8_t button1 = digitalRead(SXbuttonPin);
      uint8_t button3 = digitalRead(DXbuttonPin);
      uint8_t button2 = digitalRead(CenterButtonPin);

      if (modeState == 0) {
        ModeButtonWaitReset = false;
      }

      if (modeState == 1 && !ModeButtonWaitReset) {
        actualMode = prevMode;
        delay(timeDelayButton);
        unsetScrollLine();

      } else {
        if (button1 == HIGH) {
          if (prevMode == PATCH_MODE) {
            if (selectedConfPatch > 0) {
              selectedConfPatch--;
            }
            getSavedPatches();
          } else if (prevMode == EFFECTS_MODE) {
            if (selectedConfEffects > 0) {
              selectedConfEffects--;
            }
            getSavedEffects();
          }
          delay(timeDelayButton);
        } else if (button3 == HIGH) {
          if (prevMode == PATCH_MODE) {
            if (selectedConfPatch < NUM_CONF_PATCHES - 1) {
              selectedConfPatch++;
            }
            getSavedPatches();
          } else if (prevMode == EFFECTS_MODE) {
            if (selectedConfEffects < NUM_CONF_EFFECTS - 1) {
              selectedConfEffects++;
            }
            getSavedEffects();
          }
          delay(timeDelayButton);
        }
        setScrollLine();
      }
    }
  } else {
    menuModeLoop();
    delay(150);
  }
  //To Check Free Ram
#ifdef TESTRAM
  lcd.setCursor(0, 3);
  lcd.print("R: ");
  lcd.print(freeRam());
  lcd.print(" B");
#endif
}

void DisplayPatchMode() {
  //selectedConf = 0;
  getSavedPatches();
}

void DisplayEffectsMode() {
  //selectedConf = 0;
  getSavedEffects();
}

void getSavedPatches() {
  uint16_t addr = EEPROM_PATCH_INITIAL_ADDR + selectedConfPatch * NUM_BYTES_FOR_PATCHES;

  for (byte i = 0; i < activeFootswitchNum + 1; i++) {
    int16_t val;
    EEPROM.get(addr, val);
    if (val > 0)
      patchPreset[i] = val;
    else
      patchPreset[i] = 0;

    addr = addr + 2;
  }
  displayEPatches();
}

void displayEPatches() {
  clearAll();
  byte row = 0;
  byte posIndexOffset = 0;

  for (byte i = 0; i < activeFootswitchNum + 1; i++) {
    if (activeFootswitchNum > 3) {
      if (i > 3) {
        row = i - 3;
        posIndexOffset = 11;
      } else {
        row = i;
        posIndexOffset = 0;
      }
    } else {
      row = i;
      posIndexOffset = 2;
    }

    int8_t limit = 0;
    if (zoomModel == G5)
      limit = -1;
    if (patchPreset[i] > limit) {
      getReqestedPatchName(patchPreset[i]);

      if (activeFootswitchNum > 3)
        clearChars(row, posIndexOffset, 10);
      else
        clear(row);

      lcd.setCursor(1 + posIndexOffset, row);
      if (!unknownDevice) {
        if (activeFootswitchNum > 3) {
          char nameLimited[9] = { "\0\0\0\0\0\0\0\0\0" };
          memcpy(nameLimited, reqPatchName, 8);
          lcd.print(nameLimited);
        } else
          lcd.print(reqPatchName);
      } else {
        lcd.print(patchPreset[i]);
      }
    } else {
      if (activeFootswitchNum > 3)
        clearChars(row, posIndexOffset, 10);
      else
        clear(row);
    }
  }

  if (lastSelectedConfPatch == selectedConfPatch) {
    row = lastPatchEnabled;

    if (lastPatchEnabled > 3) {
      row = lastPatchEnabled - 3;
      posIndexOffset = 11;
    } else {
      row = lastPatchEnabled;
      if (activeFootswitchNum > 3)
        posIndexOffset = 0;
      else
        posIndexOffset = 2;
    }


    lcd.setCursor(0 + posIndexOffset, row);
    lcd.write(0xff);
    //displaySpecial(1, 4);
  }

  showEPatchTitle();
}

void getEffectName(uint16_t number) {
  memcpy(effectName, "\0\0\0\0\0\0\0\0\0", 9);
  Zoom_FX_type_struct zoomData;
  memcpy_P(&zoomData, &Zoom_FX_types[number], sizeof(Zoom_FX_types[0]));
  memcpy(effectName, zoomData.Name, 8);
}

void getEffectPedalG5Name(uint8_t number) {
  memcpy_P(effectName, pedalG5Effx[number], 8);
}

uint16_t FXsearch(uint8_t type, uint8_t number) {
  uint16_t my_type = 0;
  Zoom_FX_type_struct zoomData;

  uint16_t initialPos = 0;
  switch (type) {
    case 0:
      initialPos = 0;
      break;
    case 12:
      initialPos = 1;
      break;
    case 14:
      initialPos = 40;
      break;
    case 16:
      initialPos = 46;
      break;
    case 18:
      initialPos = 72;
      break;
    case 2:
      initialPos = 101;
      break;
    case 4:
      initialPos = 114;
      break;
    case 8:
      initialPos = 138;
      break;
    case 6:
      initialPos = 160;
      break;
  }
  for (uint16_t i = initialPos; i < ZOOM_NUMBER_OF_FX; i++) {
    memcpy_P(&zoomData, &Zoom_FX_types[i], sizeof(Zoom_FX_types[0]));
    if ((zoomData.Type == type) && (zoomData.Number == number)) {
      my_type = i;
      break;  // break loop when found
    }
  }
  return my_type;  // Return index to the effect type
}

uint16_t FXtypeMangler(uint8_t byte1, uint8_t byte2, uint8_t byte4) {
  // We will read three bytes from the sysex stream. All of them contain some bits that together make the FX type
  // Byte1 is the first byte of every fx slot
  // Bit one of Byte1 signals whether the effect is on or off
  // Bit 5-8 are all relevant for the effect type

  // Byte2 is the second byte of every fx slot
  // AFAIK only the first three bits are relevant for the effect type

  // Byte4 is the fourth byte of every fx stream - this one contain the effect type. The following values are known
  // 02: Reverb?
  // 04: Eq effects
  // 12: Chorus effects
  // 14: Roto effects
  // 16: Delay effects
  // 18: Reverb effects

  // We will search for the right effect type
  return FXsearch((byte4 & 0x1F), (byte1 & 0xF8) + (byte2 & 0x0F));  // We include bit 4 from byte1, so the difference between HD Hall and HD reverb is clear.
}

void checkEffectEnabled() {
  cleanMidiRec(10);
  byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x29, 0xf7 };
  SendToMidi(Message, 6);  // Send the message to request patch data
  byte eff[maxEffects];
  uint16_t effTypes[maxEffects];
  if (zoomModel == MS_50G || zoomModel == MS_70C) {
    byte recMessage[dataMessageLen];
    if (RecvMidi(recMessage, dataMessageLen, 1000, true)) {
      eff[0] = recMessage[6];
      eff[1] = recMessage[26];
      eff[2] = recMessage[47];

      effectsTypes[0] = FXtypeMangler(recMessage[6] + ((recMessage[5] & B01000000) << 1), recMessage[7], recMessage[9]);
      effectsTypes[1] = FXtypeMangler(recMessage[26] + ((recMessage[21] & B00000100) << 5), recMessage[27], recMessage[30]);
      effectsTypes[2] = FXtypeMangler(recMessage[47] + ((recMessage[45] & B00100000) << 2), recMessage[48], recMessage[50]);

      if (maxEffects >= 3) {
        eff[3] = recMessage[67];
        effectsTypes[3] = FXtypeMangler(recMessage[67] + ((recMessage[61] & B00000010) << 6), recMessage[68], recMessage[71]);
      }
      if (maxEffects >= 4) {
        eff[4] = recMessage[88];
        effectsTypes[4] = FXtypeMangler(recMessage[88] + ((recMessage[85] & B00010000) << 3), recMessage[89], recMessage[91]);
      }
      if (maxEffects >= 5) {
        eff[5] = recMessage[108];
        effectsTypes[5] = FXtypeMangler(recMessage[108] + ((recMessage[101] & B00000001) << 7), recMessage[110], recMessage[112]);
      }

      for (byte i = 0; i < maxEffects; i++) {
        effectsStatus[i] = eff[i] & 1U;
      }
    }
  } else if (zoomModel == G5 || zoomModel == G3) {
    byte recMessage[dataMessageLen];
    if (RecvMidi(recMessage, dataMessageLen, 1000, true)) {
      eff[0] = recMessage[6];
      eff[1] = recMessage[19];
      eff[2] = recMessage[33];
      effectsTypes[0] = ((recMessage[6] & B1111110) >> 1) + ((recMessage[5] & B01000000)) + ((recMessage[7] & B00000001) << 7);
      effectsTypes[1] = ((recMessage[19] & B1111110) >> 1) + ((recMessage[13] & B00000010) << 5) + ((recMessage[20] & B00000001) << 7);
      effectsTypes[2] = ((recMessage[33] & B1111110) >> 1) + ((recMessage[29] & B00001000) << 3) + ((recMessage[34] & B00000001) << 7);

      if (maxEffects >= 3) {
        eff[3] = recMessage[47];
        effectsTypes[3] = ((recMessage[47] & B1111110) >> 1) + ((recMessage[45] & B00100000) << 1) + ((recMessage[48] & B00000001) << 7);
      }
      if (maxEffects >= 4) {
        eff[4] = recMessage[60];
        effectsTypes[4] = ((recMessage[60] & B1111110) >> 1) + ((recMessage[53] & B00000001) << 6) + ((recMessage[62] & B00000001) << 7);
      }
      if (maxEffects >= 5) {
        eff[5] = recMessage[74];
        effectsTypes[5] = ((recMessage[74] & B1111110) >> 1) + ((recMessage[69] & B00000100) << 4) + ((recMessage[75] & B00000001) << 7);
      }
      if (maxEffects >= 6) {
        eff[6] = recMessage[88];
        effectsTypes[6] = ((recMessage[88] & B1111110) >> 1) + ((recMessage[85] & B00010000) << 2) + ((recMessage[89] & B00000001) << 7);
      }
      if (maxEffects >= 7) {
        eff[7] = recMessage[102];
        effectsTypes[7] = ((recMessage[102] & B1111110) >> 1) + ((recMessage[101] & B01000000)) + ((recMessage[103] & B00000001) << 7);
      }
      if (maxEffects >= 8) {
        eff[8] = recMessage[115];
        effectsTypes[8] = ((recMessage[115] & B1111110) >> 1) + ((recMessage[116] & B00000001) << 6) - 60;
      }

      for (byte i = 0; i < maxEffects; i++) {
        effectsStatus[i] = eff[i] & 1U;
      }
    }
  }
}

void getSavedEffects() {
  for (byte i = 0; i < 9; i++)
    lastEffectsTypes[i] = 0x8000;

  uint16_t addr = EEPROM_EFFECTS_INITIAL_ADDR + selectedConfEffects * NUM_FOOTSWITCH;

  for (byte i = 0; i < activeFootswitchNum; i++) {
    byte val;
    EEPROM.get(addr, val);
    assignedEffects[i] = val;
    addr++;
  }
  displayEEffects();
}

void displayEEfectsTitle() {
  displayProgmemDisplayLine(0, string23);

  lcd.setCursor(15, 0);
  lcd.print(selectedConfEffects);
}

void displayEEffects() {
  clearAll();
  displayEEfectsTitle();
  showEffectsModeStatus();
}

void showEffectsModeStatus() {
  byte row = 0;
  byte posIndexOffset = 0;
  for (byte i = 0; i < activeFootswitchNum; i++) {
    if (activeFootswitchNum > 3) {
      if (i > 2) {
        row = i - 3;
        posIndexOffset = 11;
      } else {
        row = i;
        posIndexOffset = 0;
      }
    } else {
      row = i;
      posIndexOffset = 2;
      lcd.setCursor(0, row + 1);
      lcd.print(row + 1);
    }

    if (assignedEffects[i] > 0 && assignedEffects[i] <= maxEffects) {
      lcd.setCursor(0 + posIndexOffset, row + 1);
      if (effectsStatus[assignedEffects[i] - 1]) {
        lcd.write(0xff);
        //displaySpecial(1, 4);
      } else {
        //lcd.print(i + 1);
        lcd.print(" ");
      }

      if (lastEffectsTypes[assignedEffects[i] - 1] != effectsTypes[assignedEffects[i] - 1]) {
        if (zoomModel == G5 || zoomModel == G3) {
          if (assignedEffects[i] < 9) {
            uint8_t val;
            memcpy_P(&val, &Zoom_G5_FX_types[effectsTypes[assignedEffects[i] - 1]], sizeof(uint8_t));
            getEffectName(val);
          } else
            getEffectPedalG5Name(effectsTypes[assignedEffects[i] - 1]);

        } else {
          getEffectName(effectsTypes[assignedEffects[i] - 1]);
        }
        lcd.setCursor(1 + posIndexOffset, row + 1);
        lcd.print(effectName);
        lastEffectsTypes[assignedEffects[i] - 1] = effectsTypes[assignedEffects[i] - 1];
      }
    } else {
      lcd.setCursor(0 + posIndexOffset, row + 1);
      if (activeFootswitchNum > 3)
        lcd.print(" ------- ");
      else
        lcd.print(" ----------- ");
    }
  }
}

void activePatchModePatch(byte button) {
  if (lastPatchEnabled == button) {
    Preset = patchPreset[0];
    lastPatchEnabled = 0;
  } else {
    Preset = patchPreset[button];
    lastPatchEnabled = button;
  }
  lastSelectedConfPatch = selectedConfPatch;

  ChangePatch();
  clearAll();
  DisplayCurrentPatch();
  displayCiclesLoop = 0;
}

void patchModeLoop() {
  uint8_t timeDelayButton = 10;
  uint8_t button1 = digitalRead(SXbuttonPin);
  uint8_t button2 = digitalRead(CenterButtonPin);
  uint8_t button3 = digitalRead(DXbuttonPin);

#ifdef INVERT_EXTRA_FOOTSWITCHES
  uint8_t button4 = !digitalRead(FS4ButtonPin);
  uint8_t button5 = !digitalRead(FS5ButtonPin);
  uint8_t button6 = !digitalRead(FS6ButtonPin);
#else
  uint8_t button4 = digitalRead(FS4ButtonPin);
  uint8_t button5 = digitalRead(FS5ButtonPin);
  uint8_t button6 = digitalRead(FS6ButtonPin);
#endif

  if (displayCiclesLoop == 50000) {
    displayCiclesLoop++;
    if (actualMode == PATCH_MODE)
      displayEPatches();
  } else if (displayCiclesLoop < 50000)
    displayCiclesLoop++;

  if (ciclesLoop > 25000) {
    checkPatchChange(false);
    ciclesLoop = 0;
  } else
    ciclesLoop++;

  if (button1 == LOW && button2 == LOW && button3 == LOW && (button4 == LOW | activeFootswitchNum < 4) && (button5 == LOW | activeFootswitchNum < 5) && (button6 == LOW | activeFootswitchNum < 6)) {
    if (encoderCounter > prevEncoderCounter) {
      if (selectedConfPatch < NUM_CONF_PATCHES - 1)
        selectedConfPatch++;

      getSavedPatches();
    } else if (encoderCounter < prevEncoderCounter) {
      if (selectedConfPatch > 0)
        selectedConfPatch--;


      getSavedPatches();
    }
    prevEncoderCounter = encoderCounter;
  } else if (button1 == HIGH) {
    if (countCiclesTransitions == 0) {
      activePatchModePatch(1);
      delay(timeDelayButton);
    }
  } else if (button2 == HIGH) {
    if (countCiclesTransitions == 0) {
      activePatchModePatch(2);
      delay(timeDelayButton);
    }
  } else if (button3 == HIGH) {
    if (countCiclesTransitions == 0) {
      activePatchModePatch(3);
      delay(timeDelayButton);
    }
  } else if (button4 == HIGH && activeFootswitchNum > 3 && (countCiclesTransitions == 0)) {
    activePatchModePatch(4);
    delay(timeDelayButton);
  } else if (button5 == HIGH && activeFootswitchNum > 4 && (countCiclesTransitions == 0)) {
    activePatchModePatch(5);
    delay(timeDelayButton);
  } else if (button6 == HIGH && activeFootswitchNum > 5 && (countCiclesTransitions == 0)) {
    activePatchModePatch(6);
    delay(timeDelayButton);
  }

  if (actualMode == PATCH_MODE) {
    if (button1 == LOW && button2 == LOW && button3 == LOW && (button4 == LOW | activeFootswitchNum < 4) && (button5 == LOW | activeFootswitchNum < 5) && (button6 == LOW | activeFootswitchNum < 6)) {
      countCiclesTransitions = 0;
    }
  }
}

void effectsModeLoop() {
  uint16_t timeDelayButton = 100;

  uint8_t button1 = digitalRead(SXbuttonPin);
  uint8_t button2 = digitalRead(CenterButtonPin);
  uint8_t button3 = digitalRead(DXbuttonPin);
#ifdef INVERT_EXTRA_FOOTSWITCHES
  uint8_t button4 = !digitalRead(FS4ButtonPin);
  uint8_t button5 = !digitalRead(FS5ButtonPin);
  uint8_t button6 = !digitalRead(FS6ButtonPin);
#else
  uint8_t button4 = digitalRead(FS4ButtonPin);
  uint8_t button5 = digitalRead(FS5ButtonPin);
  uint8_t button6 = digitalRead(FS6ButtonPin);
#endif

  if (ciclesLoopEffectsCheck > 10000) {
    if (button1 == LOW && button2 == LOW && button3 == LOW) {
      checkEffectEnabled();
      showEffectsModeStatus();
    }
    ciclesLoopEffectsCheck = 0;
  } else
    ciclesLoopEffectsCheck++;

  if (ciclesLoop > 20000) {
    if (button1 == LOW && button2 == LOW && button3 == LOW) {
      checkPatchChange(false);
    }
    ciclesLoop = 0;
  } else
    ciclesLoop++;

  if (button1 == LOW && button2 == LOW && button3 == LOW && (button4 == LOW | activeFootswitchNum < 4) && (button5 == LOW | activeFootswitchNum < 5) && (button6 == LOW | activeFootswitchNum < 6)) {
    if (encoderCounter > prevEncoderCounter) {
      if (selectedConfEffects < NUM_CONF_EFFECTS - 1)
        selectedConfEffects++;

      getSavedEffects();
    } else if (encoderCounter < prevEncoderCounter) {
      if (selectedConfEffects > 0)
        selectedConfEffects--;

      getSavedEffects();
    }
    prevEncoderCounter = encoderCounter;
  } else if (button1 == HIGH) {
    enableDisableEffect(assignedEffects[0] - 1);
    delay(timeDelayButton);
  } else if (button2 == HIGH) {
    enableDisableEffect(assignedEffects[1] - 1);
    delay(timeDelayButton);
  } else if (button3 == HIGH) {
    enableDisableEffect(assignedEffects[2] - 1);
    delay(timeDelayButton);
  } else if (button4 == HIGH && activeFootswitchNum > 3 && (countCiclesTransitions == 0)) {
    enableDisableEffect(assignedEffects[3] - 1);
    delay(timeDelayButton);
  } else if (button5 == HIGH && activeFootswitchNum > 4 && (countCiclesTransitions == 0)) {
    enableDisableEffect(assignedEffects[4] - 1);
    delay(timeDelayButton);
  } else if (button6 == HIGH && activeFootswitchNum > 5 && (countCiclesTransitions == 0)) {
    enableDisableEffect(assignedEffects[5] - 1);
    delay(timeDelayButton);
  }

  if (actualMode == EFFECTS_MODE) {
    if (button1 == LOW && button2 == LOW && button3 == LOW && (button4 == LOW | activeFootswitchNum < 4) && (button5 == LOW | activeFootswitchNum < 5) && (button6 == LOW | activeFootswitchNum < 6)) {
      countCiclesTransitions = 0;
    }
  }
}



void enableDisableEffect(byte numEffect) {
  bool enable = effectsStatus[numEffect];
  effectsStatus[numEffect] = !enable;

  if ((zoomModel == MS_50G || zoomModel == MS_70C) && numEffect > 2) {
    cleanMidiRec(10);
    byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x29, 0xf7 };
    SendToMidi(Message, 6);  // Send the message to request patch data
    byte recMessage[dataMessageLen];
    if (RecvMidi(recMessage, dataMessageLen, 1000, true)) {
      byte effectPos = 67;
      switch (numEffect) {
        case 3:
          effectPos = 67;
          break;
        case 4:
          effectPos = 88;
          break;
        case 5:
          effectPos = 108;
          break;
      }
      byte byte0 = recMessage[effectPos];
      byte cByte0 = (byte0 >> 0) & 1U;

      if (!enable)
        cByte0 = 0x01;
      else
        cByte0 = 0x00;

      byte0 ^= (-cByte0 ^ byte0) & (1U << 0);
      recMessage[effectPos] = byte0;

      SendToMidi(recMessage, dataMessageLen);  // Send the message to enable effect;
    }
  } else {
    byte statusL = 0x00;
    if (!enable)
      statusL = 0x01;
    else
      statusL = 0x00;

    byte Message[10] = { 0xf0, 0x52, 0x00, zoomModel, 0x31, numEffect, 0x00, statusL, 0x00, 0xf7 };
    SendToMidi(Message, 10);  // Send the message to set effect
  }
  showEffectsModeStatus();
  delay(100);
}

void decreasePreset() {
  Preset -= 1;
  if (Preset < minPresetNumber)
    Preset = maxPresetNumber;
}

void increasePreset() {
  Preset += 1;
  if (Preset > maxPresetNumber)
    Preset = minPresetNumber;
}
void normalModeLoop() {
  uint8_t SXbuttonState = digitalRead(SXbuttonPin);
  uint8_t DXbuttonState = digitalRead(DXbuttonPin);
  uint8_t CenterButtonState = digitalRead(CenterButtonPin);
#ifdef INVERT_EXTRA_FOOTSWITCHES
  uint8_t button4 = !digitalRead(FS4ButtonPin);
  uint8_t button5 = !digitalRead(FS5ButtonPin);
  uint8_t button6 = !digitalRead(FS6ButtonPin);
#else
  uint8_t button4 = digitalRead(FS4ButtonPin);
  uint8_t button5 = digitalRead(FS5ButtonPin);
  uint8_t button6 = digitalRead(FS6ButtonPin);
#endif
  uint8_t timeDelayButton = 100;

  if (countCiclesTransitions > 0) {
    countCiclesTransitions++;
    if (countCiclesTransitions > 5000)
      countCiclesTransitions = 0;
  }

  if (modeSelect == PRESELECT_MODE && countCiclesTransitions == 0) {
    if (ciclesBlinkLoop > 25000) {
      blinkSelect();
      ciclesBlinkLoop = 0;
    } else
      ciclesBlinkLoop++;
  }

  if (ciclesLoop > 20000) {
    if (modeSelect == UP_DOWN_PATCH_MODE)
      checkPatchChange(false);
    ciclesLoop = 0;
  } else
    ciclesLoop++;


  if (modeSelect == UP_DOWN_PATCH_MODE && countCiclesTransitions == 0) {
    if (encoderCounter > prevEncoderCounter) {
      increasePreset();
      ChangePatch();
      prevEncoderCounter = encoderCounter;
    } else if (encoderCounter < prevEncoderCounter) {
      decreasePreset();
      ChangePatch();
      prevEncoderCounter = encoderCounter;
    } else {
      if (SXbuttonState == HIGH) {
        decreasePreset();
        ChangePatch();
        delay(timeDelayButton);
      }

      if (DXbuttonState == HIGH) {
        increasePreset();
        ChangePatch();
        delay(timeDelayButton);
      }
      if (CenterButtonState == HIGH) {
        enablePreselectMode();
        delay(timeDelayButton);
      }
    }
  } else if (modeSelect == PRESELECT_MODE && countCiclesTransitions == 0) {
    if (encoderCounter > prevEncoderCounter) {
      increasePreset();
      DisplayPreselect();
      prevEncoderCounter = encoderCounter;
      delay(timeDelayButton);
    } else if (encoderCounter < prevEncoderCounter) {
      decreasePreset();
      DisplayPreselect();
      prevEncoderCounter = encoderCounter;
      delay(timeDelayButton);
    } else {
      if (SXbuttonState == HIGH) {
        decreasePreset();
        DisplayPreselect();
        delay(timeDelayButton);
      }
      if (DXbuttonState == HIGH) {
        increasePreset();
        DisplayPreselect();
        delay(timeDelayButton);
      }
      if (CenterButtonState == HIGH) {
        ChangePatch();
        enableNormalModeUpDownPatch();
        delay(timeDelayButton);
      }
    }
  }
}

void enablePatchMode() {
  countCiclesTransitions = 1;
  actualMode = PATCH_MODE;
  prevMode = actualMode;
  lastPatchEnabled = 0;
  displayCiclesLoop = 51000;
  exitMenu();
  //delay(100);
}

void enablePreselectMode() {
  modeSelect = PRESELECT_MODE;  //Preselect
  DisplayPreselect();
}

void enableEffectsMode() {
  if (!unknownDevice) {
    countCiclesTransitions = 1;
    actualMode = EFFECTS_MODE;
  }
  exitMenu();
  //delay(100);
}


void enableNormalModeUpDownPatch() {
  actualMode = NORMAL_MODE;
  prevMode = actualMode;
  modeSelect = UP_DOWN_PATCH_MODE;  //Normal Mode
  lastSelectedConfPatch = 255;

  clearAll();
  DisplayCurrentPatch();
  delay(300);
}

// Receive from MIDI Message
bool SendToMidi(byte *buff, uint8_t sizeReq) {
  Usb.Task();
  if (Usb.getUsbTaskState() == USB_STATE_RUNNING) {
#ifdef DEBUG
#ifdef DEBUG_ALL_SEND
    byte sendBuff[sizeReq];
    for (int i = 0; i < sizeReq; i++)
      sendBuff[i] = buff[i];
    sendStringToTest("Send: ", sendBuff, sizeReq);
#endif
#endif
    Midi.SendData(buff);
  }
}

// Receive from MIDI Message
bool RecvMidi(byte *buff, uint8_t sizeReq, uint16_t maxCiclesWait, bool checkModel) {
  if (!unknownDevice) {
    Usb.Task();
    if (Usb.getUsbTaskState() == USB_STATE_RUNNING) {
      uint8_t sizeRecv = 0;
      uint16_t cicles = 0;
      uint8_t receivedSize = 0;
      uint8_t i = 0;
      while (receivedSize < sizeReq && cicles < maxCiclesWait) {
        byte tempbuff[200];
        sizeRecv = Midi.RecvData(tempbuff);
        if (sizeRecv > 0) {
          for (uint8_t k = 0; k < sizeRecv; k++) {
            buff[i + k] = tempbuff[k];
          }
          receivedSize += sizeRecv;
          i = i + sizeRecv;
        } else {
          cicles++;
        }
      }
      do {
        byte extrabuff[200];
        sizeRecv = Midi.RecvData(extrabuff);
        if (sizeRecv > 0) {
#ifdef DEBUG
#ifdef DEBUG_ALL_RECEIVE
          Serial.print("ERROR: Received Extra Data. Size = ");
          Serial.println(sizeRecv);
          sendStringToTest("Received Extra: ", extrabuff, sizeRecv);
#endif
#endif
        }
      } while (sizeRecv > 0);

      if (receivedSize == sizeReq) {
#ifdef DEBUG
#ifdef DEBUG_ALL_RECEIVE

        sendStringToTest("Received: ", buff, sizeReq);
#endif
#endif
        if (checkModel) {
          if (buff[0] == 0xf0 && buff[1] == 0x52 && buff[2] == 0x00 && buff[3] == zoomModel)
            return true;
          else
            return false;
        } else
          return true;
      } else {
        return false;
      }
    }
  }
}

void cleanMidiRec(uint8_t timePause) {
  byte tempbuff[200];
  uint8_t sizeRecv = 1;
  bool foundSomething = false;
  while (sizeRecv > 0) {
    sizeRecv = Midi.RecvData(tempbuff);
    if (!foundSomething && sizeRecv > 0)
      foundSomething = true;
  }

  if (foundSomething)
    delay(timePause);
}

void checkPatchChange(bool first) {
  if (!unknownDevice) {
    uint16_t CheckPreset = 0;
    byte Message[6] = { 0xf0, 0x52, 0x00, zoomModel, 0x33, 0xf7 };
    SendToMidi(Message, 6);  // Send the message to request current patch
    byte recMessage[8];
    if (RecvMidi(recMessage, 8, 1000, false)) {
      if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
        CheckPreset = recMessage[7] + 1;
      } else if (zoomModel == G5) {
        byte Bank = 0x00;
        byte Patch = 0x00;
        Bank = recMessage[5];
        Patch = recMessage[7];
        CheckPreset = 3 * (Bank) + (Patch + 1) - 1;
      }
      if (CheckPreset != ActualPreset || first) {
        lastPatchEnabled = 0;
        Preset = CheckPreset;
        ActualPreset = Preset;
        if (actualMode != EFFECTS_MODE) {
          clearAll();
          DisplayCurrentPatch();

          char updateString[21] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
          if (!first)
            memcpy_P(updateString, string17, 20);
          lcd.setCursor(0, 2);
          lcd.print(updateString);
        }
      } else if (actualMode != PATCH_MODE && actualMode != EFFECTS_MODE) {
        lcd.setCursor(0, 2);
        lcd.print(modeString);
      }
    }
  }
}

// Send "Program Change" MIDI Message

void ChangePatch() {
  if (ActualPreset != Preset && (Preset >= minPresetNumber && Preset <= maxPresetNumber)) {
    byte PatchMessage[2];
    PatchMessage[0] = 0xC0;  // 0xC0 is for Program Change (Change to MIDI channel 0)

    if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
      if (Preset == 0)
        Preset = 1;
      PatchMessage[1] = Preset - 1;
      SendToMidi(PatchMessage, 2);  // Send the change patch
    } else if (zoomModel == G5) {
      uint16_t Bank = Preset / 3;
      byte patch = Preset % 3;
      if (Bank == 100)
        Bank = 0;
      if (patch == 3)
        patch = 0;
      byte Message[3] = { 0xB0, 0x00, 0x00 };  // Construct the midi message (2 bytes)
      SendToMidi(Message, 3);                  // Send the message
      Message[1] = 0x20;
      Message[2] = Bank;
      SendToMidi(Message, 3);  // Send the change bank

      PatchMessage[1] = patch;      // Number is the program/patch (Only 0 to 99 is valid for ZOOM G3)
      SendToMidi(PatchMessage, 2);  // Send the change patch
    } else {
      if (Preset == 0)
        Preset = 1;
      PatchMessage[1] = Preset - 1;
      SendToMidi(PatchMessage, 2);  // Send the change patch
    }

    ActualPreset = Preset;
    if (zoomModel == G5) {
      disableControl();
      enableControl();
    }
    DisplayCurrentPatch();
  }
}

void blinkSelect() {
  blinkStatus = !blinkStatus;

  if (blinkStatus) {
    writePreselectPatch();
  } else {
    clear(0);
    clear(1);
  }
}

void writePreselectPatch() {
  if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
    char presetNum[3];
    presetNum[0] = '0' + Preset / 10;
    presetNum[1] = '0' + Preset % 10;
    presetNum[2] = '\0';
    writeBigString(presetNum, 7, 0);
  } else if (zoomModel == G5) {
    uint16_t Bank = Preset / 3 + 1;
    byte patch = Preset % 3 + 1;
    if (Bank == 100)
      Bank = 1;
    if (patch == 4)
      patch = 1;

    char presetNum[7] = { "      \0" };
    presetNum[2] = '0' + Bank / 10;
    presetNum[3] = '0' + Bank % 10;
    presetNum[4] = '-';
    presetNum[5] = '0' + patch;
    writeBigString(presetNum, 2, 0);
  } else {
    char presetNum[3];
    presetNum[0] = '0' + Preset / 10;
    presetNum[1] = '0' + Preset % 10;
    presetNum[2] = '\0';
    writeBigString(presetNum, 7, 0);
  }
}

byte calcNameLenght(char *name) {
  byte cont = 0;
  for (byte i = 0; i < 10; i++) {
    if (name[i] != ' ')
      cont++;
  }
  return cont;
}

void DisplayPreselect() {
  //clear(2);
  memcpy_P(modeString, string18, 20);
  lcd.setCursor(0, 2);
  lcd.print(modeString);

  getOtherPatchName();
  clear(3);
  byte len = calcNameLenght(nextPatchName);
  lcd.setCursor((20 - len) / 2, 3);
  for (byte c = 0; c < 10; c++)
    lcd.print(nextPatchName[c]);

  writePreselectPatch();

  blinkStatus = false;
  blinkSelect();
}

void displaySpecial(uint8_t num, byte special) {
  for (uint8_t i = 0; i < num; i++)
    lcd.write(special);
}

void DisplayCurrentPatch() {
  if (actualMode == NORMAL_MODE) {
    memcpy_P(modeString, string6, 20);
    lcd.setCursor(0, 2);
    lcd.print(modeString);
  } else if (actualMode == PATCH_MODE) {
    memcpy_P(modeString, string50, 20);
    lcd.setCursor(0, 2);
    lcd.print(modeString);
  }

  getPatchCurrentName();
  clear(3);
  if (actualMode == PATCH_MODE) {
    lcd.setCursor(0, 0);
    lcd.print("EP");
    if (selectedConfPatch < 10)
      lcd.setCursor(19, 0);
    else
      lcd.setCursor(18, 0);
    lcd.print(selectedConfPatch);
  }

  byte len = calcNameLenght(patchName);
  lcd.setCursor((20 - len) / 2, 3);

  for (byte c = 0; c < 10; c++)
    lcd.print(patchName[c]);

  if (zoomModel == MS_50G || zoomModel == MS_70C || zoomModel == G3) {
    char presetNum[3];
    presetNum[0] = '0' + Preset / 10;
    presetNum[1] = '0' + Preset % 10;
    presetNum[2] = '\0';
    writeBigString(presetNum, 7, 0);
  } else if (zoomModel == G5) {
    uint16_t Bank = Preset / 3 + 1;
    byte patch = Preset % 3 + 1;
    if (Bank == 100)
      Bank = 1;
    if (patch == 4)
      patch = 1;

    char presetNum[7] = { "      \0" };
    presetNum[2] = '0' + Bank / 10;
    presetNum[3] = '0' + Bank % 10;
    presetNum[4] = '-';
    presetNum[5] = '0' + patch;
    writeBigString(presetNum, 2, 0);

  } else {
    char presetNum[3];
    presetNum[0] = '0' + Preset / 10;
    presetNum[1] = '0' + Preset % 10;
    presetNum[2] = '\0';
    writeBigString(presetNum, 7, 0);
  }
}

#ifdef TESTRAM
// FREERAM: Returns the number of bytes currently free in RAM
int freeRam(void) {
  extern int __bss_end, *__brkval;
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  } else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}
#endif
