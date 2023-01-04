#include "gamecubecontroller.h"

uint8_t prevButtons[numberOfButtons];
uint8_t curButtons[numberOfButtons];
GameControllerData_t oldData;
int32 shiftButton = -1;
bool pressedSomethingElseWithShift;
const Injector_t* prevInjector = NULL;
HIDJoystick* curJoystick;
USBXBox360Controller* curX360;

static void joySliderLeft(uint16_t t) {
  curJoystick->sliderLeft((1023 - t) & 1023);
}

static void joySliderRight(uint16_t t) {
  curJoystick->sliderRight((1023 - t) & 1023);
}

static void buttonizeStick4Dir(uint8_t* buttons, uint16_t x, uint16_t y) {
  uint32_t dx = x < 512 ? 512 - x : x - 512;
  uint32_t dy = y < 512 ? 512 - y : y - 512;
  if (dx > dy) {
    if (dx > directionThreshold) {
      if (x < 512 ) {
        buttons[virtualLeft] = 1;
      }
      else {
        buttons[virtualRight] = 1;
      }
    }
  }
  else if (dx < dy && dy > directionThreshold) {
    if (y >= 512) {
      buttons[virtualDown] = 1;
    }
    else {
      buttons[virtualUp] = 1;
    }
  }
}

static const uint32_t tan22_5 = 4142;

static void buttonizeStick(uint8_t* buttons, uint16_t x, uint16_t y) {
  uint32_t dx = x < 512 ? 512 - x : x - 512;
  uint32_t dy = y < 512 ? 512 - y : y - 512;
  uint32_t r2 = dx * dx + dy * dy;
  if (r2 <= directionThreshold * (uint32_t)directionThreshold)
    return;
  if (dy * 10000 < tan22_5 * dx) {
    if (x < 512)
      buttons[virtualLeft] = 1;
    else
      buttons[virtualRight] = 1;
  }
  else if (dx * 10000 < dy * tan22_5) {
    if (y >= 512)
      buttons[virtualDown] = 1;
    else
      buttons[virtualUp] = 1;
  }
  else {
    if (x < 512)
      buttons[virtualLeft] = 1;
    else
      buttons[virtualRight] = 1;
    if (y >= 512)
      buttons[virtualDown] = 1;
    else
      buttons[virtualUp] = 1;
  }
}

static void toButtonArray(uint8_t* buttons, const GameControllerData_t* data, uint8 directions) {
  for (int i = 0; i < numberOfHardButtons; i++)
    buttons[i] = 0 != (data->buttons & buttonMasks[i]);
  buttons[virtualShoulderRightPartial] = data->shoulderRight >= shoulderThreshold;
  buttons[virtualShoulderLeftPartial] = data->shoulderLeft >= shoulderThreshold;
  buttons[virtualLeft] = buttons[virtualRight] = buttons[virtualDown] = buttons[virtualUp] = 0;
  if (directions == 4) {
    buttonizeStick4Dir(buttons, data->joystickX, data->joystickY);
    buttonizeStick4Dir(buttons, data->cX, data->cY);
  }
  else {
    buttonizeStick(buttons, data->joystickX, data->joystickY);
    buttonizeStick(buttons, data->cX, data->cY);
  }
}

static inline int16_t range10u16s(uint16_t x) {
  return (((int32_t)(uint32_t)x - 512) * 32767 + 255) / 512;
}

void joystickBasic(const GameControllerData_t* data) {
  if (isModeJoystick()) {
    curJoystick->X(data->joystickX);
    curJoystick->Y(data->joystickY);
    curJoystick->Xrotate(data->cX);
    curJoystick->Yrotate(data->cY);
  }
  else if (isModeX360()) {
    curX360->X(range10u16s(data->joystickX));
    curX360->Y(-range10u16s(data->joystickY));
    curX360->XRight(range10u16s(data->cX));
    curX360->YRight(-range10u16s(data->cY));
  }
  else {
    Switch.X(data->joystickX >> 2);
    Switch.Y(data->joystickY >> 2);
    Switch.XRight(data->cX >> 2);
    Switch.YRight(data->cY >> 2);
  }
}

static void joystickPOV(const GameControllerData_t* data) {
  if (isModeX360() )
    return;

  int16_t dir = -1;
  if (data->buttons & (maskDUp | maskDRight | maskDLeft | maskDDown)) {
    if (0 == (data->buttons & (maskDRight | maskDLeft))) {
      if (data->buttons & maskDUp)
        dir = 0;
      else
        dir = 180;
    }
    else if (0 == (data->buttons & (maskDUp | maskDDown))) {
      if (data->buttons & maskDRight)
        dir = 90;
      else
        dir = 270;
    }
    else if (data->buttons & maskDUp) {
      if (data->buttons & maskDRight)
        dir = 45;
      else if (data->buttons & maskDLeft)
        dir = 315;
    }
    else if (data->buttons & maskDDown) {
      if (data->buttons & maskDRight)
        dir = 135;
      else if (data->buttons & maskDLeft)
        dir = 225;
    }
  }

  if (isModeJoystick()) {
    curJoystick->hat(dir);
  }
  else {
    if (dir < 0)
      Switch.dpad(HIDSwitchController::DPAD_NEUTRAL);
    else 
      Switch.dpad(dir / 45);
  }
}

static uint16_t getExerciseMachineSpeed(const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
#ifndef ENABLE_EXERCISE_MACHINE
  return 512;
#else
  static bool ledIndicator = false;
  if (! exerciseMachineP->valid || exerciseMachineP->speed == 0) {
    if (ledIndicator) {
      updateDisplay();
      ledIndicator = false;
    }
    return 512;
  }
  if (multiplier == 0) {
    if (exerciseMachineP->direction)
      return 1023;
    else
      return 0;
  }
  int32_t speed = 512 + multiplier * (exerciseMachineP->direction ? exerciseMachineP->speed : -exerciseMachineP->speed) / 64;
  //  displayNumber(0xF);
  if (speed < 0)
    speed = 0;
  if (speed > 1023)
    speed = 1023;
  //  return speed;
  if (lastChangedModeTime + 5000 <= millis()) {
    int absSpeed = speed < 512 ? 512 - speed : speed - 512;
    int numLeds = (absSpeed + 1) / 128; // 511 maps
    displayNumber((1 << numLeds) - 1);
    ledIndicator = true;
  }
  return speed;
#endif
}

void joystickDualShoulder(const GameControllerData_t* data) {
  joystickBasic(data);
  joystickPOV(data);
  if (isModeJoystick()) {
    joySliderLeft(data->shoulderLeft);
    joySliderRight(data->shoulderRight);
  }
  else if (isModeX360()) {
    curX360->sliderLeft(data->shoulderLeft / 4);
    curX360->sliderRight(data->shoulderRight / 4);
  }
}

void exerciseMachineSliders(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
#ifdef ENABLE_EXERCISE_MACHINE
  if (debounceDown.getRawState() && data->device == CONTROLLER_NUNCHUCK) {
    // useful for calibration and settings for games: when downButton is pressed, joystickY controls both sliders
    if (data->joystickY >= 512 + 160 || data->joystickY <= 512 - 160) {
      int32_t delta = -((int32_t)data->joystickY - 512) * 49 / 40;
      uint16_t out;
      if (delta <= -511)
        out = 0;
      else if (delta >= 511)
        out = 1023;
      else
        out = 512 + delta;
      if (isModeJoystick()) {
        joySliderLeft(out);
        joySliderRight(out);
      }
      else if (isModeX360()) {
        curX360->sliderLeft(out >> 2);
        curX360->sliderRight(out >> 2);
      }
      debounceDown.cancelRelease();
      return;
    }
  }
  uint16_t datum = getExerciseMachineSpeed(exerciseMachineP, multiplier);
  if (data->device == CONTROLLER_GAMECUBE && ! exerciseMachineP->valid)
    return;
  if (isModeJoystick()) {
    joySliderLeft(datum);
    joySliderRight(datum);
  }
  else if (isModeX360()) {
    if (datum >= 512) {
      curX360->sliderLeft(0);
      curX360->sliderRight((datum - 512) >> 1);
    }
    else {
      curX360->sliderRight(0);
      curX360->sliderLeft((511 - datum) >> 1);
    }
  }
#endif
}

void directionSwitchSlider(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
  (void)multiplier;
  (void)data;
  if (exerciseMachineP->direction) {
    if (isModeJoystick()) {
      joySliderRight(1023);
    }
    else if (isModeX360()) {
      curX360->sliderRight(255);
      curX360->sliderLeft(0);
    }
  }
}

void joystickUnifiedShoulder(const GameControllerData_t* data) {
  joystickBasic(data);
  joystickPOV(data);

  uint16_t datum;
  datum = 512 + (data->shoulderRight - (int16_t)data->shoulderLeft) / 2;
  if (isModeJoystick()) {
    joySliderLeft(datum);
    joySliderRight(datum);
  }
  else if (isModeX360()) {
    curX360->sliderLeft(datum >> 2);
    curX360->sliderRight(datum >> 2);
  }
}

void joystickNoShoulder(const GameControllerData_t* data) {
  joystickBasic(data);
  joystickPOV(data);
}

// todo: reset two joystick
void inject(HIDJoystick* joy, USBXBox360Controller* xbox, const Injector_t* injector, const GameControllerData_t* curDataP, const ExerciseMachineData_t* exerciseMachineP) {
  curJoystick = joy;
  curX360 = xbox;
  uint8_t* curReport;
  static uint8_t prevReport[64]; 
  uint16_t reportSize;
  bool force = false;

  if (currentUSBMode != injector->usbMode) {
    if (lastChangedModeTime + 1000 <= millis()) {
      currentUSBMode->end();

      delay(100);

      currentUSBMode = injector->usbMode;
      currentUSBMode->begin();
    }
    return;
  }

  if (currentUSBMode == &modeDualJoystick && joy == NULL)
    return;

  if (isModeX360() && xbox == NULL)
    return;

  if (prevInjector != injector) {
    if (currentUSBMode == &modeUSBHID) {
      Keyboard.releaseAll();
      Mouse.release(0xFF);
    }

    if (isModeJoystick())
      curJoystick->buttons(0);
    else if (isModeX360())
      curX360->buttons(0);
    else // Switch
      Switch.buttons(0);

    prevInjector = injector;

    shiftButton = -1;
    for (int i = 0; i < numberOfUnshiftedButtons; i++)
      if (injector->buttons[i].mode == SHIFT) {
        shiftButton = i;
        pressedSomethingElseWithShift = false;
        break;
      }

    force = true;
  }
  else {
    if (isModeJoystick()) {
      curReport = curJoystick->getReport();
      reportSize = curJoystick->getReportSize();
    }
    else if (isModeX360()) {
      curReport = curX360->getReport();
      reportSize = curX360->getReportSize();
    }
    else {
      curReport = Switch.getReport();
      reportSize = Switch.getReportSize();
    }
    if (reportSize > sizeof(prevReport))
      reportSize = sizeof(prevReport);
    memcpy(prevReport, curReport, reportSize);
  }

  memcpy(prevButtons, curButtons, sizeof(curButtons));
  memset(curButtons, 0, sizeof(curButtons));  
  toButtonArray(curButtons, curDataP, injector->directions);
  if (shiftButton >= 0 && curButtons[shiftButton]) {
    if (!pressedSomethingElseWithShift) {
      for(int i=0; i<numberOfUnshiftedButtons; i++) {
        if (i != shiftButton && curButtons[i]) {
          pressedSomethingElseWithShift = true;
          break;
        }
      }
    }
    memcpy(curButtons+numberOfUnshiftedButtons, curButtons, numberOfUnshiftedButtons*sizeof(curButtons[0]));
    memset(curButtons, 0, numberOfUnshiftedButtons*sizeof(curButtons[0]));
    curButtons[shiftButton] = 1;
    curButtons[numberOfUnshiftedButtons + shiftButton] = 0;
  }
  else {
    memset(curButtons+numberOfUnshiftedButtons, 0, numberOfUnshiftedButtons*sizeof(curButtons[0]));
    if (shiftButton >= 0 && prevButtons[shiftButton] && !pressedSomethingElseWithShift) {
      curButtons[numberOfUnshiftedButtons+shiftButton] = 1;
      pressedSomethingElseWithShift = true;
    }
    else {
      pressedSomethingElseWithShift = false;
    }
  }

  const InjectedButton_t* buttonMap = injector->buttons;

  int num = shiftButton < 0 ? numberOfUnshiftedButtons : numberOfButtons;

  if (isModeJoystick()) {
    curJoystick->buttons(0);
  }
  else if (isModeX360()) {
    curX360->buttons(0);
  }
  else {
    Switch.buttons(0);
  }

  int8_t directionSwitchUp = -1;
  
  for (int i = 0; i < num; i++) {
    if (buttonMap[i].mode == KEY) {
      if (curButtons[i] != prevButtons[i]) {
        if (curButtons[i])
          Keyboard.press(buttonMap[i].value.key);
        else
          Keyboard.release(buttonMap[i].value.key);
      }
    }
    else if (buttonMap[i].mode == JOY || buttonMap[i].mode == JOY_SWITCHABLE) {
      if (curButtons[i]) {
        uint8_t b;
        if (buttonMap[i].mode == JOY) {
          b = buttonMap[i].value.button;
        }
        else {
#ifdef directionSwitch
          if (directionSwitchUp < 0)
            directionSwitchUp = digitalRead(directionSwitch) == DIRECTION_SWITCH_FORWARD;
            
          b = directionSwitchUp ? buttonMap[i].value.joySwitchable.upButton : buttonMap[i].value.joySwitchable.downButton;
#else
          b = buttonMap[i].value.joySwitchable.upButton;
#endif    
        }
        if (isModeJoystick()) {
          if (curButtons[i]) 
            curJoystick->button(b, 1);
        }
        else if (isModeX360()) {
          if (curButtons[i]) 
            curX360->button(b, 1);
        }
        else {
          if (curButtons[i]) 
            Switch.button(b, 1);
        }
      }
    }
    else if (buttonMap[i].mode == MOUSE_RELATIVE) {
      if (!prevButtons[i] && curButtons[i])
        Mouse.move(buttonMap[i].value.mouseRelative.x, injector->buttons[i].value.mouseRelative.y);
    }
    else if (buttonMap[i].mode == CLICK) {
      if (!prevButtons[i] && curButtons[i])
        Mouse.click(buttonMap[i].value.buttons);
    }
  }

  if (injector->stick != NULL)
    injector->stick(curDataP);

  if (injector->exerciseMachine != NULL)
    injector->exerciseMachine(curDataP, exerciseMachineP, injector->exerciseMachineMultiplier);

  if (force || memcmp(curReport, prevReport, reportSize)) {
    if (isModeJoystick()) 
      curJoystick->send();
    else if (isModeX360())
      curX360->send();
    else 
      Switch.send();
  }
}


