#ifndef _GAMECUBE_H
#define _GAMECUBE_H

#define ALEXS_BUILD

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

//#define SERIAL_DEBUG

#include <USBComposite.h>

USBHID HID;
HIDJoystick Joystick(HID);
HIDSwitchController Switch(HID);
HIDKeyboard Keyboard(HID);
HIDMouse Mouse(HID);
USBXBox360 XBox360; //(0x045e, 0x028f);
USBMultiXBox360<2> DualXBox360; //(0x045e, 0x028f);
USBXBox360Controller* x360_1 = NULL;
USBXBox360Controller* x360_2 = NULL;

#ifdef SERIAL_DEBUG
USBCompositeSerial CompositeSerial;
# define DEBUG(...) CompositeSerial.println(__VA_ARGS__)
#else
# define DEBUG(...)
#endif

#define MAX_RUMBLE_TIME 10000
//#define BUTTON_MONITOR_TIME_MS 6

#define VENDOR_ID 0x1EAF
#define PRODUCT_ID_SINGLE 0xe167
#define PRODUCT_ID_DUAL   0xe170

#define FEATURE_DATA_SIZE 63

#define DEADZONE_10BIT 4
//#define ENABLE_AUTO_CALIBRATE
#define ENABLE_SWITCH
#define ENABLE_GAMECUBE
#define ENABLE_NUNCHUCK
#ifdef ALEXS_BUILD
#define ENABLE_EXERCISE_MACHINE
#endif

#define EEPROM_VARIABLE_INJECTION_MODE 0

#define DIRECTION_SWITCH_FORWARD LOW
#define ROTATION_DETECTOR_CHANGE_TO_MONITOR FALLING 

#define ROTATION_DETECTOR_ACTIVE_STATE ((ROTATION_DETECTOR_CHANGE_TO_MONITOR == FALLING) ? LOW : HIGH)

/*
typedef struct {
  uint16_t buttons;
  uint8_t joystickX;
  uint8_t joystickY;
  uint8_t cX;
  uint8_t cY;
  uint8_t shoulderLeft;
  uint8_t shoulderRight;
  uint8_t device;
} GameControllerData_t;
*/

typedef struct {
  int32_t speed;
  uint8_t direction;
  uint8_t valid;
} ExerciseMachineData_t;

typedef struct {
  void (*begin)();
  void (*end)();
} USBMode_t;

void exerciseMachineUpdate(ExerciseMachineData_t* data);
void exerciseMachineInit(void);

extern HIDJoystick Joystick2;

void updateLED(void);
void beginUSBHID();
void endUSBHID();
void beginDual();
void endDual();
void beginX360();
void endX360();
void beginDualX360();
void endDualX360();
void beginSwitch();
void endSwitch();

uint8_t loadInjectionMode(void);
void saveInjectionMode(uint8_t mode);

uint8_t validDevices[2] = {CONTROLLER_NONE,CONTROLLER_NONE};
uint8_t validUSB = 0;
#ifdef ENABLE_AUTO_CALIBRATE
bool needToCalibrate = true;
#endif
volatile bool exitX360Mode = false;
uint8_t exerciseMachineRotationDetector = 0;
extern uint8_t leftMotor;
extern uint8_t rightMotor;
extern uint32_t lastRumbleOff;
 
const uint32_t watchdogSeconds = 10; // do not make it be below 6

#define MY_SCL PB6
#define MY_SDA PB7

const uint32_t indicatorLEDs[] = { PA0, PA1, PA2, PA3 };
const uint8_t ledBrightnessLevels[] = {20,20,20,20}; 
//const uint8_t ledBrightnessLevels[] = { 30, 30, 30, 150 }; // my PA3 LED is dimmer for some reason
const int numIndicators = sizeof(indicatorLEDs)/sizeof(*indicatorLEDs);
#ifdef ALEXS_BUILD
const uint32_t downButton = PA5;
const uint32_t upButton = PA4;
#else
const uint32_t upButton = PA5;
const uint32_t downButton = PA4;
#endif
const uint32_t rotationDetector = PA7;
#ifdef ENABLE_EXERCISE_MACHINE
#define directionSwitch PA8
#endif

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
const uint8_t ledPinID = PB12;

const uint32_t saveInjectionModeAfterMillis = 15000ul; // only save a mode if it's been used 15 seconds; this saves flash

const uint32_t gcPinID = PA6;

int32_t injectionMode = 0;
uint32_t savedInjectionMode = 0;
uint32_t lastChangedModeTime;

//volatile uint32 *gcPortPtr;
//uint32_t gcPinBitmap;

const uint16_t maskA = 0x01;
const uint16_t maskB = 0x02;
const uint16_t maskX = 0x04;
const uint16_t maskY = 0x08;
const uint16_t maskStart = 0x10;
const uint16_t maskDLeft = 0x100;
const uint16_t maskDRight = 0x200;
const uint16_t maskDDown = 0x400;
const uint16_t maskDUp = 0x800;
const uint16_t maskZ = 0x1000;
const uint16_t maskShoulderRight = 0x2000;
const uint16_t maskShoulderLeft = 0x4000;

const uint16_t shoulderThreshold = 1;
const uint16_t directionThreshold = 320;
const uint16_t buttonMasks[] = { maskA, maskB, maskX, maskY, maskStart, maskDLeft, maskDRight, maskDDown, maskDUp, maskZ, maskShoulderRight, maskShoulderLeft };
const int numberOfHardButtons = sizeof(buttonMasks)/sizeof(*buttonMasks);
const uint16_t virtualShoulderRightPartial = numberOfHardButtons;
const uint16_t virtualShoulderLeftPartial = numberOfHardButtons+1;
const uint16_t virtualLeft = numberOfHardButtons+2;
const uint16_t virtualRight = numberOfHardButtons+3;
const uint16_t virtualDown = numberOfHardButtons+4;
const uint16_t virtualUp = numberOfHardButtons+5;
const int numberOfUnshiftedButtons = numberOfHardButtons+6;
const int numberOfButtons = 2*numberOfUnshiftedButtons;

#define UNDEFINED 0
#define JOY 'j'
#define JOY_SWITCHABLE 'J'
#define KEY 'k'
#define FUN 'f'
#define MOUSE_RELATIVE 'm'
#define CLICK 'c'
#define SHIFT 's'

typedef void (*GameControllerDataProcessor_t)(const GameControllerData_t* data);
typedef void (*ExerciseMachineProcessor_t)(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachine, int32_t multiplier);

typedef struct {
  char mode;
  union {
    uint8_t key;
    uint8_t button;
    struct {
      uint8_t upButton;
      uint8_t downButton;
    } joySwitchable;
    uint8_t buttons;
    struct {
      int16_t x;
      int16_t y;
    } mouseRelative;
    GameControllerDataProcessor_t processor;
  } value;
} InjectedButton_t;

typedef struct {
  const USBMode_t* usbMode;
  InjectedButton_t const * buttons;
  GameControllerDataProcessor_t stick;
  ExerciseMachineProcessor_t exerciseMachine;
  int32_t exerciseMachineMultiplier; // 64 = default speed ; higher is faster
  const char* commandName;
  const char* description; // no more than 61 characters
  uint8 directions;
  bool show;
  bool rumble;
  bool dpadToJoystick;
} Injector_t;


void joystickNoShoulder(const GameControllerData_t* data);
void joystickDualShoulder(const GameControllerData_t* data);
void joystickUnifiedShoulder(const GameControllerData_t* data);
void joystickBasic(const GameControllerData_t* data);
void exerciseMachineSliders(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier);
void directionSwitchSlider(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier);

// note: Nunchuck Z maps to A, Nunchuck C maps to B
const InjectedButton_t defaultJoystickButtons[numberOfButtons] = {
    { JOY, {.button = 1} },           // A
    { JOY, {.button = 2} },           // B
    { JOY, {.button = 3} },           // X
    { JOY, {.button = 4} },           // Y
    { JOY, {.button = 5} },           // Start
    { 0,   {.key = 0 } },             // DLeft
    { 0,   {.key = 0 } },             // DRight
    { 0,   {.key = 0 } },             // DDown
    { 0,   {.key = 0 } },             // DUp
    { JOY, {.button = 6 } },          // Z
    { 0, {.key = 0 } }, //{ JOY, {.button = 8 } },           // right shoulder button
    { 0, {.key = 0 } }, //{ JOY, {.button = 7 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

const InjectedButton_t outfox[numberOfButtons] = {
    { JOY, {.button = 1} },           // A
    { JOY, {.button = 2} },           // B
    { JOY, {.button = 3} },           // X
    { JOY, {.button = 4} },           // Y
    { JOY, {.button = 5} },           // Start
    { JOY,   {.button = 6 } },             // DLeft
    { JOY,   {.button = 7 } },             // DRight
    { JOY,   {.button = 8 } },             // DDown
    { JOY,   {.button = 9 } },             // DUp
    { JOY, {.button = 10 } },          // Z
    { JOY, {.button = 11} },            // right shoulder button
    { JOY, {.button = 12} },           // left shoulder button
    { JOY,   {.button = 13 } },           // right shoulder button partial
    { JOY,   {.button = 14 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

// unsupported: XBox back, XBox left bumper, XBox button, stick buttons
const InjectedButton_t defaultXBoxButtons[numberOfButtons] = {
    { JOY, {.button = XBOX_A} },           // A
    { JOY, {.button = XBOX_B} },           // B
    { JOY, {.button = XBOX_X} },           // X
    { JOY, {.button = XBOX_Y} },           // Y
    { JOY, {.button = XBOX_START} },           // Start 
    { JOY, {.button = XBOX_DLEFT } },             // DLeft
    { JOY, {.button = XBOX_DRIGHT } },             // DRight
    { JOY, {.button = XBOX_DDOWN } },             // DDown
    { JOY, {.button = XBOX_DUP } },             // DUp
    { JOY, {.button = XBOX_RSHOULDER } },          // Z
    { JOY, {.key = XBOX_R3 } }, //{ JOY, {.button = 8 } },           // right shoulder button
    { JOY, {.key = XBOX_L3 } }, //{ JOY, {.button = 7 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

const InjectedButton_t defaultSwitchButtons[numberOfButtons] = {
    { JOY, {.button = HIDSwitchController::BUTTON_A} },           // A
    { JOY, {.button = HIDSwitchController::BUTTON_B} },           // B
    { JOY, {.button = HIDSwitchController::BUTTON_X} },           // X
    { JOY, {.button = HIDSwitchController::BUTTON_Y} },           // Y
    { SHIFT },        // Start 
    { 0,   {.key = 0 } },             // DLeft
    { 0,   {.key = 0 } },             // DRight
    { 0,   {.key = 0 } },             // DDown
    { 0,   {.key = 0 } },             // DUp
    { JOY, {.button = HIDSwitchController::BUTTON_R } },          // Z
    { JOY_SWITCHABLE, {.joySwitchable = {.upButton = HIDSwitchController::BUTTON_ZR, .downButton = HIDSwitchController::BUTTON_A } } }, // right shoulder button
    { 0, {.key = 0 } }, // left shoulder button
    { JOY, {.button = HIDSwitchController::BUTTON_ZR } },           // right shoulder button partial
    { JOY, {.button = HIDSwitchController::BUTTON_ZL } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
// shifted
    { JOY, {.button = HIDSwitchController::BUTTON_CAPTURE }  },           // A
    { JOY, {.button = HIDSwitchController::BUTTON_HOME } },           // B
    { JOY, {.button = HIDSwitchController::BUTTON_RIGHT_CLICK } },           // X
    { JOY, {.button = HIDSwitchController::BUTTON_LEFT_CLICK } },           // Y
    { JOY, {.button = HIDSwitchController::BUTTON_PLUS } },           // Start
    { 0,   {.key = 0 } },             // DLeft
    { 0,   {.key = 0 } },             // DRight
    { 0,   {.key = 0 } },             // DDown
    { 0,   {.key = 0 } },             // DUp
    { JOY, {.button = HIDSwitchController::BUTTON_MINUS } },          // Z
    { 0, {.key = 0 } }, // right shoulder button
    { 0, {.key = 0 } }, // left shoulder button
    { JOY, {.button = HIDSwitchController::BUTTON_R } },           // right shoulder button partial
    { JOY, {.button = HIDSwitchController::BUTTON_L } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } }            // virtual up
};

const InjectedButton_t jetsetJoystickButtons[numberOfButtons] = {
    { JOY, {.button = 1} },           // A
    { JOY, {.button = 2} },           // B
    { JOY, {.button = 5} },           // X
    { JOY, {.button = 3} },           // Y
    { JOY, {.button = 8} },           // Start
    { 0,   {.key = 0 } },             // DLeft
    { 0,   {.key = 0 } },             // DRight
    { 0,   {.key = 0 } },             // DDown
    { 0,   {.key = 0 } },             // DUp
    { JOY, {.button = 4 } },          // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { JOY, {.button = 6 } },           // right shoulder button partial
    { JOY,   {.button = 5 } },         // left shoulder button partial (was 7)
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

const InjectedButton_t dpadWASDButtons[numberOfButtons] = {
    { KEY, {.key = ' '} },          // A
    { KEY, {.key = KEY_RETURN} },   // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { 0,   {.key = 0 } },           // Start
    { KEY, {.key = 'a' } },         // DLeft
    { KEY, {.key = 'd' } },         // DRight
    { KEY, {.key = 's' } },         // DDown
    { KEY, {.key = 'w' } },         // DUp
    { 0,   {.key = 0 } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = 'a' } },           // virtual left
    { KEY,   {.key = 'd' } },           // virtual right
    { KEY,   {.key = 's' } },           // virtual down
    { KEY,   {.key = 'w' } },           // virtual up
};

const InjectedButton_t powerPadLeft[numberOfButtons] = {
    { KEY, {.key = 'q'} },          // A
    { KEY, {.key = 'z'} },   // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { KEY,   {.key = '=' } },           // Start
    { KEY, {.key = 'x' } },         // DLeft
    { KEY, {.key = 'w' } },         // DRight
    { KEY, {.key = 'd' } },         // DDown
    { KEY, {.key = 'a' } },         // DUp
    { KEY, {.key = '-'} },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

const InjectedButton_t dpadWASZButtons[numberOfButtons] = {
    { KEY, {.key = ' '} },          // A
    { KEY, {.key = KEY_RETURN} },   // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { 0,   {.key = 0 } },           // Start
    { KEY, {.key = 'a' } },         // DLeft
    { KEY, {.key = 's' } },         // DRight
    { KEY, {.key = 'z' } },         // DDown
    { KEY, {.key = 'w' } },         // DUp
    { 0,   {.key = 0 } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = 'a' } },           // virtual left
    { KEY,   {.key = 's' } },           // virtual right
    { KEY,   {.key = 'z' } },           // virtual down
    { KEY,   {.key = 'w' } },           // virtual up
};

const InjectedButton_t dpadArrowWithCTRL[numberOfButtons] = {
    { KEY, {.key = KEY_LEFT_CTRL} },          // A
    { KEY, {.key = ' '} },          // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { KEY, {.key = '+' } },           // Start
    { KEY, {.key = KEY_LEFT_ARROW } },         // DLeft
    { KEY, {.key = KEY_RIGHT_ARROW } },         // DRight
    { KEY, {.key = KEY_DOWN_ARROW  } },         // DDown
    { KEY, {.key = KEY_UP_ARROW  } },         // DUp
    { KEY, {.key = '-' } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = KEY_LEFT_ARROW } },           // virtual left
    { KEY,   {.key = KEY_RIGHT_ARROW } },           // virtual right
    { KEY,   {.key = KEY_DOWN_ARROW } },           // virtual down
    { KEY,   {.key = KEY_UP_ARROW } },           // virtual up
};


const InjectedButton_t mame[numberOfButtons] = {
    { KEY, {.key = KEY_LEFT_CTRL} },          // A
    { KEY, {.key = KEY_LEFT_ALT} },          // B
    { 0,   {.key = ' ' } },           // X
    { 0,   {.key = KEY_LEFT_SHIFT } },           // Y
    { KEY, {.key = '1' } },           // Start 1 player
    { KEY, {.key = KEY_LEFT_ARROW } },         // DLeft
    { KEY, {.key = KEY_RIGHT_ARROW } },         // DRight
    { KEY, {.key = KEY_DOWN_ARROW  } },         // DDown
    { KEY, {.key = KEY_UP_ARROW  } },         // DUp
    { KEY, {.key = '5' } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = KEY_LEFT_ARROW } },           // virtual left
    { KEY,   {.key = KEY_RIGHT_ARROW } },           // virtual right
    { KEY,   {.key = KEY_DOWN_ARROW } },           // virtual down
    { KEY,   {.key = KEY_UP_ARROW } },           // virtual up
};

const InjectedButton_t dpadZX[numberOfButtons] = {
    { KEY, {.key = 'z'} },          // A
    { KEY, {.key = 'x'} },          // B
    { KEY, {.key = ' ' } },           // X
    { KEY, {.key = KEY_RIGHT_SHIFT } },           // Y
    { KEY, {.key = KEY_RETURN } },           // Start
    { KEY, {.key = KEY_LEFT_ARROW } },         // DLeft
    { KEY, {.key = KEY_RIGHT_ARROW } },         // DRight
    { KEY, {.key = KEY_DOWN_ARROW  } },         // DDown
    { KEY, {.key = KEY_UP_ARROW  } },         // DUp
    { KEY, {.key = '-' } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = KEY_LEFT_ARROW } },           // virtual left
    { KEY,   {.key = KEY_RIGHT_ARROW } },           // virtual right
    { KEY,   {.key = KEY_DOWN_ARROW } },           // virtual down
    { KEY,   {.key = KEY_UP_ARROW } },           // virtual up
};

const InjectedButton_t dpadArrowWithSpace[numberOfButtons] = {
    { KEY, {.key = ' '} },          // A
    { KEY, {.key = KEY_BACKSPACE} }, // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { KEY, {.key = '+' } },           // Start
    { KEY, {.key = KEY_LEFT_ARROW } },         // DLeft
    { KEY, {.key = KEY_RIGHT_ARROW } },         // DRight
    { KEY, {.key = KEY_DOWN_ARROW  } },         // DDown
    { KEY, {.key = KEY_UP_ARROW  } },         // DUp
    { KEY, {.key = '-' } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = KEY_LEFT_ARROW } },           // virtual left
    { KEY,   {.key = KEY_RIGHT_ARROW } },           // virtual right
    { KEY,   {.key = KEY_DOWN_ARROW } },           // virtual down
    { KEY,   {.key = KEY_UP_ARROW } },           // virtual up
};

const InjectedButton_t dpadQBert[numberOfButtons] = {
    { KEY, {.key = '1'} },          // A
    { KEY, {.key = '2'} },          // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { KEY, {.key = '+' } },           // Start
    { KEY, {.key = KEY_LEFT_ARROW } },         // DLeft
    { KEY, {.key = KEY_RIGHT_ARROW } },         // DRight
    { KEY, {.key = KEY_DOWN_ARROW  } },         // DDown
    { KEY, {.key = KEY_UP_ARROW  } },         // DUp
    { KEY, {.key = '-' } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { KEY,   {.key = KEY_LEFT_ARROW } },           // virtual left
    { KEY,   {.key = KEY_RIGHT_ARROW } },           // virtual right
    { KEY,   {.key = KEY_DOWN_ARROW } },           // virtual down
    { KEY,   {.key = KEY_UP_ARROW } },           // virtual up
};

const InjectedButton_t dpadMC[numberOfButtons] = {
    { KEY, {.key = ' '} },          // A
    { KEY, {.key = KEY_LEFT_SHIFT} }, // B
    { 0,   {.key = 0 } },           // X
    { 0,   {.key = 0 } },           // Y
    { CLICK, {.buttons = 0x02 } },           // Start // TODO: check button number
    { MOUSE_RELATIVE, {.mouseRelative = {-50,0} } },         // DLeft
    { MOUSE_RELATIVE, {.mouseRelative = {50,0} } },         // DRight
    { KEY, {.key = 's' } },         // DDown
    { KEY, {.key = 'w' } },         // DUp
    { CLICK, {.buttons = 0x01 } },           // Z
    { 0,   {.key = 0 } },           // right shoulder button
    { 0,   {.key = 0 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
};

const USBMode_t modeUSBHID = {
  beginUSBHID,
  endUSBHID
};

const USBMode_t* currentUSBMode = &modeUSBHID;

const USBMode_t modeX360 = {
  beginX360, endX360
};

const USBMode_t modeDualJoystick = {
  beginDual, endDual
};

const USBMode_t modeDualX360 = {
  beginDualX360, endDualX360
};

const USBMode_t modeSwitch = {
  beginSwitch, endSwitch
};

const Injector_t injectors[] {
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 64, "defaultUnified", "joystick, unified shoulder, speed 100%", 8, true },
  { &modeUSBHID, defaultJoystickButtons, joystickDualShoulder, exerciseMachineSliders, 40, "defaultDual", "joystick, dual shoulders, speed 63%", 8, true },
  { &modeUSBHID, jetsetJoystickButtons, joystickNoShoulder, exerciseMachineSliders, 64, "jetset", "Jet Set Radio", 8, false },
  { &modeUSBHID, powerPadLeft, joystickBasic, exerciseMachineSliders, 64, "powerpad left", "PowerPad left", 4, true },
  { &modeUSBHID, dpadWASDButtons, NULL, exerciseMachineSliders, 64, "wasd", "WASD, 4-way", 4, true },
  { &modeUSBHID, dpadWASDButtons, NULL, exerciseMachineSliders, 64, "wasd", "WASD, 8-way", 8, false },
  { &modeUSBHID, dpadArrowWithCTRL, NULL, exerciseMachineSliders, 64, "dpadArrowCtrl", "Arrow keys with A=CTRL, 4-way", 4, true },
  { &modeUSBHID, dpadArrowWithCTRL, NULL, exerciseMachineSliders, 64, "dpadArrowCtrl", "Arrow keys with A=CTRL, 8-way", 8, false },
  { &modeUSBHID, dpadArrowWithSpace, NULL, exerciseMachineSliders, 64, "dpadArrowSpace", "Arrow keys with A=SPACE, 4-way", 4, true },  
  { &modeUSBHID, dpadArrowWithSpace, NULL, exerciseMachineSliders, 64, "dpadArrowSpace", "Arrow keys with A=SPACE, 8-way", 8, false },  
  { &modeUSBHID, mame, NULL, exerciseMachineSliders, 64, "mame", "MAME", 4, false },  
  { &modeUSBHID, outfox, joystickUnifiedShoulder, exerciseMachineSliders, 64, "outfox", "OutFox", 4, true },  
  //{ &modeUSBHID, dpadMC, NULL, exerciseMachineSliders, 64, "dpadMC", "Minecraft with dpad", 4, true },  
#ifdef ENABLE_SWITCH
  { &modeSwitch, defaultSwitchButtons, joystickNoShoulder, NULL, 64, "switch", "Switch Controller", 8, true, false },
#endif  
#ifdef ENABLE_EXERCISE_MACHINE
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 96, "default96", "joystick, unified shoulder, speed 150%", 8, true },  
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 128, "default128", "joystick, unified shoulder, speed 200%", 8, true },  
  { &modeUSBHID, defaultJoystickButtons, joystickDualShoulder, directionSwitchSlider, 64, "directionSwitch", "joystick, direction switch controls sliders", 8, true },
#endif
  { &modeUSBHID, dpadZX, NULL, exerciseMachineSliders, 64, "dpadZX", "Arrow keys with A=Z, B=X", 8, true },
  { &modeX360, defaultXBoxButtons, joystickDualShoulder, exerciseMachineSliders, 64, "xbox360", "XBox360, speed 100%, vibrate", 8, true, true },
  { &modeX360, defaultXBoxButtons, joystickDualShoulder, exerciseMachineSliders, 64, "xbox360nv", "XBox360, speed 100%, no vibrate", 8, false, false },
#if defined(ENABLE_GAMECUBE) && defined(ENABLE_NUNCHUCK)
  { &modeDualJoystick, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 64, "dual", "dual joystick", 8, true }, 
#endif  
  { &modeUSBHID, dpadWASZButtons, NULL, exerciseMachineSliders, 64, "wasz", "WASZ", 4, false },
#if defined(ENABLE_GAMECUBE) && defined(ENABLE_NUNCHUCK)
  { &modeDualX360, defaultXBoxButtons, joystickDualShoulder, exerciseMachineSliders, 64, "dualx360", "dual XBox360", 8, true }, 
#endif
};

const uint32_t numInjectionModes = sizeof(injectors)/sizeof(*injectors);
void inject(HIDJoystick* joy, const Injector_t* injector, const GameControllerData_t* curDataP, const ExerciseMachineData_t* exerciseMachineP);

static inline bool isModeX360() {
  return currentUSBMode == &modeX360 || currentUSBMode == &modeDualX360;
}

static inline bool isModeJoystick() {
  return currentUSBMode == &modeDualJoystick || currentUSBMode == &modeUSBHID;
}

static inline bool isModeSwitch() {
  return currentUSBMode == &modeSwitch;
}

#endif // _GAMECUBE_H

