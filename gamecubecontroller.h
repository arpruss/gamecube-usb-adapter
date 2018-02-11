#ifndef _GAMECUBE_H
#define _GAMECUBE_H

#undef SERIAL_DEBUG

#include <USBHID.h>
#include <USBXBox360.h>

#ifdef SERIAL_DEBUG
#include <USBCompositeSerial.h>
# define Serial CompositeSerial
#endif

#define FEATURE_DATA_SIZE 63

#define ENABLE_GAMECUBE
#define ENABLE_NUNCHUCK
#define ENABLE_EXERCISE_MACHINE

#define EEPROM_VARIABLE_INJECTION_MODE 0
#define DEVICE_NONE     0
#define DEVICE_GAMECUBE 1
#define DEVICE_NUNCHUCK 2

#define DIRECTION_SWITCH_FORWARD LOW
#define ROTATION_DETECTOR_CHANGE_TO_MONITOR FALLING 

#define ROTATION_DETECTOR_ACTIVE_STATE ((ROTATION_DETECTOR_CHANGE_TO_MONITOR == FALLING) ? LOW : HIGH)

typedef struct {
  uint16_t buttons;
  uint8_t joystickX;
  uint8_t joystickY;
  uint8_t cX;
  uint8_t cY;
  uint8_t shoulderLeft;
  uint8_t shoulderRight;
  uint8_t device;
} GameCubeData_t;

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

void nunchuckInit(void);
uint8_t nunchuckReceiveReport(GameCubeData_t* data);
extern HIDJoystick Joystick2;

void gameCubeInit(void);
void gameCubeSendBits(uint32_t data, uint8_t bits);
uint8_t gameCubeReceiveBits(void* data0, uint32_t bits);
uint8_t gameCubeReceiveReport(GameCubeData_t* data, uint8_t rumble);
uint8_t gameCubeReceiveReport(GameCubeData_t* data);
void updateLED(void);
void beginUSBHID();
void endUSBHID();
void beginDual();
void endDual();
void beginX360();
void endX360();

uint8_t loadInjectionMode(void);
void saveInjectionMode(uint8_t mode);

uint8_t validDevice = DEVICE_NONE;
uint8_t validUSB = 0;
volatile bool exitX360Mode = false;
uint8_t exerciseMachineRotationDetector = 0;
 
const uint32_t watchdogSeconds = 10;

#define MY_SCL PB6
#define MY_SDA PB7

const uint32_t indicatorLEDs[] = { PA0, PA1, PA2, PA3 };
const uint8_t ledBrightnessLevels[] = {20,20,20,20}; 
//const uint8_t ledBrightnessLevels[] = { 30, 30, 30, 150 }; // my PA3 LED is dimmer for some reason
const int numIndicators = sizeof(indicatorLEDs)/sizeof(*indicatorLEDs);
const uint32_t downButton = PA4;
const uint32_t upButton = PA5;
const uint32_t rotationDetector = PA7;
const uint32_t directionSwitch = PA8; // TODO: change to PA9 with screen

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
const uint8_t ledPinID = PB12;

const uint32_t saveInjectionModeAfterMillis = 15000ul; // only save a mode if it's been used 15 seconds; this saves flash

const uint32_t gcPinID = PA6;
const uint8_t gcPin = 6;
gpio_dev* const gcPort = GPIOA;

uint32_t injectionMode = 0;
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

const uint8_t shoulderThreshold = 1;
const uint8_t directionThreshold = 80;
const uint16_t buttonMasks[] = { maskA, maskB, maskX, maskY, maskStart, maskDLeft, maskDRight, maskDDown, maskDUp, maskZ, maskShoulderRight, maskShoulderLeft };
const int numberOfHardButtons = sizeof(buttonMasks)/sizeof(*buttonMasks);
const uint16_t virtualShoulderRightPartial = numberOfHardButtons;
const uint16_t virtualShoulderLeftPartial = numberOfHardButtons+1;
const uint16_t virtualLeft = numberOfHardButtons+2;
const uint16_t virtualRight = numberOfHardButtons+3;
const uint16_t virtualDown = numberOfHardButtons+4;
const uint16_t virtualUp = numberOfHardButtons+5;
const int numberOfButtons = numberOfHardButtons+6;

#define UNDEFINED 0
#define JOY 'j'
#define KEY 'k'
#define FUN 'f'
#define MOUSE_RELATIVE 'm'
#define CLICK 'c'

typedef void (*GameCubeDataProcessor_t)(const GameCubeData_t* data);
typedef void (*ExerciseMachineProcessor_t)(const GameCubeData_t* data, const ExerciseMachineData_t* exerciseMachine, int32_t multiplier);

typedef struct {
  char mode;
  union {
    uint8_t key;
    uint8_t button;
    uint8_t buttons;
    struct {
      int16_t x;
      int16_t y;
    } mouseRelative;
    GameCubeDataProcessor_t processor;
  } value;
} InjectedButton_t;

typedef struct {
  const USBMode_t* usbMode;
  InjectedButton_t const * buttons;
  GameCubeDataProcessor_t stick;
  ExerciseMachineProcessor_t exerciseMachine;
  int32_t exerciseMachineMultiplier; // 64 = default speed ; higher is faster
  const char* commandName;
  const char* description; // no more than 61 characters
} Injector_t;


void joystickNoShoulder(const GameCubeData_t* data);
void joystickDualShoulder(const GameCubeData_t* data);
void joystickUnifiedShoulder(const GameCubeData_t* data);
void exerciseMachineSliders(const GameCubeData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier);
void directionSwitchSlider(const GameCubeData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier);

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

// unsupported: XBox back, XBox left bumper, XBox button, stick buttons
const InjectedButton_t defaultXBoxButtons[numberOfButtons] = {
    { JOY, {.button = 13} },           // A
    { JOY, {.button = 14} },           // B
    { JOY, {.button = 15} },           // X
    { JOY, {.button = 16} },           // Y
    { JOY, {.button = 5} },           // Start 
    { JOY,   {.button = 3 } },             // DLeft
    { JOY,   {.button = 4 } },             // DRight
    { JOY,   {.button = 2 } },             // DDown
    { JOY,   {.button = 1 } },             // DUp
    { JOY, {.button = 10 } },          // Z
    { 0, {.key = 0 } }, //{ JOY, {.button = 8 } },           // right shoulder button
    { 0, {.key = 0 } }, //{ JOY, {.button = 7 } },           // left shoulder button
    { 0,   {.key = 0 } },           // right shoulder button partial
    { 0,   {.key = 0 } },           // left shoulder button partial
    { 0,   {.key = 0 } },           // virtual left
    { 0,   {.key = 0 } },           // virtual right
    { 0,   {.key = 0 } },           // virtual down
    { 0,   {.key = 0 } },           // virtual up
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
    { JOY,   {.button = 7 } },         // left shoulder button partial
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

const InjectedButton_t dpadZX[numberOfButtons] = {
    { KEY, {.key = 'z'} },          // A
    { KEY, {.key = 'x'} },          // B
    { KEY, {.key = ' ' } },           // X
    { KEY, {.key = KEY_BACKSPACE } },           // Y
    { KEY, {.key = ' ' } },           // Start
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

const Injector_t injectors[] = {
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 64, "defaultUnified", "joystick, unified shoulder, speed 100%" },
  { &modeUSBHID, defaultJoystickButtons, joystickDualShoulder, exerciseMachineSliders, 40, "defaultDual", "joystick, dual shoulders, speed 63%" },
  { &modeUSBHID, jetsetJoystickButtons, joystickNoShoulder, exerciseMachineSliders, 64, "jetset", "Jet Set Radio" },
  { &modeUSBHID, dpadWASDButtons, NULL, exerciseMachineSliders, 64, "wasd", "WASD" },
  { &modeUSBHID, dpadArrowWithCTRL, NULL, exerciseMachineSliders, 64, "dpadArrowCtrl", "Arrow keys with A=CTRL" },
  { &modeUSBHID, dpadArrowWithSpace, NULL, exerciseMachineSliders, 64, "dpadArrowSpace", "Arrow keys with A=SPACE" },  
  { &modeUSBHID, dpadQBert, NULL, exerciseMachineSliders, 64, "dpadQBert", "QBert with dpad" },  
  { &modeUSBHID, dpadMC, NULL, exerciseMachineSliders, 64, "dpadMC", "Minecraft with dpad" },  
#ifdef ENABLE_EXERCISE_MACHINE
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 96, "default96", "joystick, unified shoulder, speed 150%" },  
  { &modeUSBHID, defaultJoystickButtons, joystickUnifiedShoulder, exerciseMachineSliders, 128, "default128", "joystick, unified shoulder, speed 200%" },  
  { &modeUSBHID, defaultJoystickButtons, joystickDualShoulder, directionSwitchSlider, 64, "directionSwitch", "joystick, direction switch controls sliders" },
#endif
  { &modeUSBHID, dpadZX, NULL, exerciseMachineSliders, 64, "dpadZX", "Arrow keys with A=Z, B=X" },
  { &modeX360, defaultXBoxButtons, joystickDualShoulder, exerciseMachineSliders, 64, "xbox360", "XBox360, speed 100%" }
};

const uint32_t numInjectionModes = sizeof(injectors)/sizeof(*injectors);
void inject(HIDJoystick* joy, const Injector_t* injector, const GameCubeData_t* curDataP, const ExerciseMachineData_t* exerciseMachineP);

#endif // _GAMECUBE_H

