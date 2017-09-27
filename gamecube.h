#ifndef _GAMECUBE_H
#define _GAMECUBE_H

//#define SERIAL_DEBUG

#define EEPROM_VARIABLE_INJECTION_MODE 0

uint8_t loadInjectionMode(void);
void saveInjectionMode(uint8_t mode);

typedef struct {
  uint16_t buttons;
  uint8_t joystickX;
  uint8_t joystickY;
  uint8_t cX;
  uint8_t cY;
  uint8_t shoulderLeft;
  uint8_t shoulderRight;
} GameCubeData_t;

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
const uint8_t directionThreshold = 100;
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
  InjectedButton_t const * buttons;
  GameCubeDataProcessor_t stick;
} Injector_t;

#ifndef SERIAL_DEBUG

void joystickNoShoulder(const GameCubeData_t* data);
void joystickDualShoulder(const GameCubeData_t* data);
void joystickUnifiedShoulder(const GameCubeData_t* data);

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
    { JOY, {.button = 8 } },           // right shoulder button
    { JOY, {.button = 7 } },           // left shoulder button
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
    { KEY, {.key = '2'} }, // B
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

const Injector_t injectors[] = {
  { defaultJoystickButtons, joystickUnifiedShoulder },
  { defaultJoystickButtons, joystickDualShoulder },
  { jetsetJoystickButtons, joystickNoShoulder },
  { dpadWASDButtons, NULL },
  { dpadArrowWithCTRL, NULL },
  { dpadArrowWithSpace, NULL },  
  { dpadQBert, NULL },  
  { dpadMC, NULL },  
};

#else // SERIAL_DEBUG
const Injector_t injectors[] = { {NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL} };
#endif

#endif // _GAMECUBE_H

