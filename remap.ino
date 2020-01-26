#include "gamecubecontroller.h"

uint8_t prevButtons[numberOfButtons];
uint8_t curButtons[numberOfButtons];
GameControllerData_t oldData;
bool didJoystick;
bool didX360;
const USBMode_t* usbMode;
const Injector_t* prevInjector = NULL;
HIDJoystick* curJoystick;
USBXBox360Controller* curX360;

void joySliderLeft(uint16_t t) {
  curJoystick->sliderLeft((1023-t)&1023);
}

void joySliderRight(uint16_t t) {
  curJoystick->sliderRight((1023-t)&1023);
}

void buttonizeStick4Dir(uint8_t* buttons, uint16_t x, uint16_t y) {
  uint32_t dx = x < 512 ? 512 - x : x - 512;
  uint32_t dy = y < 512 ? 512 - y : y - 512;
  if (dx > dy) {
      if(dx > directionThreshold) {
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

const uint32_t tan22_5 = 4142;

void buttonizeStick(uint8_t* buttons, uint16_t x, uint16_t y) {
  uint32_t dx = x < 512 ? 512 - x : x - 512;
  uint32_t dy = y < 512 ? 512 - y : y - 512;
  uint32_t r2 = dx*dx+dy*dy;
  if (r2 <= directionThreshold*(uint32_t)directionThreshold)
    return;
  if (dy * 10000 < tan22_5 * dx) {
    if (x<512)
      buttons[virtualLeft]=1;
    else
      buttons[virtualRight]=1;
  }
  else if (dx * 10000 < dy * tan22_5) {
    if (y>=512)
      buttons[virtualDown]=1;
    else
      buttons[virtualUp]=1;
  }
  else {
    if (x<512)
      buttons[virtualLeft]=1;
    else
      buttons[virtualRight]=1;
    if (y>=512)
      buttons[virtualDown]=1;
    else
      buttons[virtualUp]=1;
  }
}

void toButtonArray(uint8_t* buttons, const GameControllerData_t* data, uint8 directions) {
  for (int i=0; i<numberOfHardButtons; i++)
    buttons[i] = 0 != (data->buttons & buttonMasks[i]);
  buttons[virtualShoulderRightPartial] = data->shoulderRight>=shoulderThreshold;
  buttons[virtualShoulderLeftPartial] = data->shoulderLeft>=shoulderThreshold;
  buttons[virtualLeft] = buttons[virtualRight] = buttons[virtualDown] = buttons[virtualUp] = 0;
  if (directions==4) {
    buttonizeStick4Dir(buttons, data->joystickX, data->joystickY);
    buttonizeStick4Dir(buttons, data->cX, data->cY); 
  }
  else {
    buttonizeStick(buttons, data->joystickX, data->joystickY);
    buttonizeStick(buttons, data->cX, data->cY);     
  }
}

inline int16_t range10u16s(uint16_t x) {
  return (((int32_t)(uint32_t)x-512)*32767+255)/512;
}

void joystickBasic(const GameControllerData_t* data) {
    if (usbMode != &modeX360 && usbMode != &modeDualX360) {
      didJoystick = true;
      curJoystick->X(data->joystickX);
      curJoystick->Y(data->joystickY);
      curJoystick->Xrotate(data->cX);
      curJoystick->Yrotate(data->cY);
    }
    else {
      didX360 = true;
      curX360->X(range10u16s(data->joystickX));
      curX360->Y(-range10u16s(data->joystickY));
      curX360->XRight(range10u16s(data->cX));
      curX360->YRight(-range10u16s(data->cY));
    }
}

void joystickPOV(const GameControllerData_t* data) {
    if (usbMode == &modeX360 || usbMode == &modeDualX360)
      return;
    didJoystick = true;
    int16_t dir = -1;
    if (data->buttons & (maskDUp | maskDRight | maskDLeft | maskDDown)) {
      if (0==(data->buttons & (maskDRight| maskDLeft))) {
        if (data->buttons & maskDUp) 
          dir = 0;
        else 
          dir = 180;
      }
      else if (0==(data->buttons & (maskDUp | maskDDown))) {
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
    curJoystick->hat(dir);
}

uint16_t getExerciseMachineSpeed(const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
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
  int32_t speed = 512+multiplier*(exerciseMachineP->direction?exerciseMachineP->speed:-exerciseMachineP->speed)/64;
//  displayNumber(0xF);
  if (speed < 0)
    speed = 0;
  if (speed > 1023)
    speed = 1023;
//  return speed;
  if (lastChangedModeTime + 5000 <= millis()) {
    int absSpeed = speed < 512 ? 512-speed : speed-512;
    int numLeds = (absSpeed+1) / 128; // 511 maps 
    displayNumber((1<<numLeds)-1);
    ledIndicator = true;
  }
  return speed;  
#endif
}

void joystickDualShoulder(const GameControllerData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    if (usbMode != &modeX360 && usbMode != &modeDualX360) {
      joySliderLeft(data->shoulderLeft);
      joySliderRight(data->shoulderRight);
      didJoystick = true;
    }
    else {
      curX360->sliderLeft(data->shoulderLeft/4);
      curX360->sliderRight(data->shoulderRight/4);
      didX360 = true;
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
        if (usbMode != &modeX360 && usbMode != &modeDualX360) {
          joySliderLeft(out);
          joySliderRight(out);
          didJoystick = true; 
        }
        else {
          curX360->sliderLeft(out>>2);
          curX360->sliderRight(out>>2);
          didX360 = true;
        }
        debounceDown.cancelRelease();
        return;
       }
    }
    uint16_t datum = getExerciseMachineSpeed(exerciseMachineP, multiplier);
    if(data->device == CONTROLLER_GAMECUBE && ! exerciseMachineP->valid)
      return;
    if (usbMode != &modeX360 && usbMode != &modeDualX360) {
      joySliderLeft(datum);
      joySliderRight(datum);
      didJoystick = true; 
    }
    else {
      if (datum>=512) {
        curX360->sliderLeft(0);
        curX360->sliderRight((datum-512)>>1);
      }
      else {
        curX360->sliderRight(0);
        curX360->sliderLeft((511-datum)>>1);
      }
      didX360 = true;
    }
#endif
}

void directionSwitchSlider(const GameControllerData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
    (void)multiplier;
    if (exerciseMachineP->direction) {
      if (usbMode != &modeX360 && usbMode != &modeDualX360) {
        didJoystick = true;
        joySliderRight(1023);
      }
      else {
        didX360 = true;
        curX360->sliderRight(255);
        curX360->sliderLeft(0);
      }
    }
}

void joystickUnifiedShoulder(const GameControllerData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    
    uint16_t datum;
    datum = 512+(data->shoulderRight-(int16_t)data->shoulderLeft)/2;
    if (usbMode != &modeX360 && usbMode != &modeDualX360) {
      joySliderLeft(datum);
      joySliderRight(datum);
    }
    else {
      curX360->sliderLeft(datum>>2);
      curX360->sliderRight(datum>>2);
    }
}

void joystickNoShoulder(const GameControllerData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
}

void inject(HIDJoystick* joy, USBXBox360Controller* xbox, const Injector_t* injector, const GameControllerData_t* curDataP, const ExerciseMachineData_t* exerciseMachineP) {
  if (currentUSBMode == &modeDualJoystick && joy == NULL)
    return;
  if ((currentUSBMode == &modeX360 || currentUSBMode == &modeDualX360) && xbox == NULL)
    return;
  curJoystick = joy;
  curX360 = xbox;
  didJoystick = false;
  didX360 = false;

  if (currentUSBMode != injector->usbMode) {
    if (lastChangedModeTime + 1000 <= millis()) {
      currentUSBMode->end();
      
      delay(100);

      currentUSBMode = injector->usbMode;
      currentUSBMode->begin();
    }
    else {
      return;
    }
  }

  if (prevInjector != injector) {
    if (currentUSBMode != &modeX360 && currentUSBMode != &modeDualX360) {
      if (currentUSBMode != &modeDualJoystick) {
        Keyboard.releaseAll();
        Mouse.release(0xFF);
      }
      for (int i=0; i<32; i++)
        curJoystick->button(i, 0);
      didJoystick = true;
    }
    else {
      for (int i=0; i<16; i++)
        curX360->button(i, 0);
    }
    
    prevInjector = injector;
  }

  usbMode = injector->usbMode;

  memcpy(prevButtons, curButtons, sizeof(curButtons));
  toButtonArray(curButtons, curDataP, injector->directions);

  for (int i=0; i<numberOfButtons; i++) {
    if (injector->buttons[i].mode == KEY) {
      if (curButtons[i] != prevButtons[i]) {
        if (curButtons[i]) 
          Keyboard.press(injector->buttons[i].value.key);  
        else
          Keyboard.release(injector->buttons[i].value.key);
      }
    }
    else if (injector->buttons[i].mode == JOY) {
      if (injector->usbMode != &modeX360 && injector->usbMode != &modeDualX360) {
        curJoystick->button(injector->buttons[i].value.button, curButtons[i]);
        didJoystick = true;
      }
      else {
        curX360->button(injector->buttons[i].value.button, curButtons[i]);
        didX360 = true;
      }
    }
    else if (injector->buttons[i].mode == MOUSE_RELATIVE) {
      if (!prevButtons[i] && curButtons[i])
        Mouse.move(injector->buttons[i].value.mouseRelative.x, injector->buttons[i].value.mouseRelative.y);
    }
    else if (injector->buttons[i].mode == CLICK) {
      if (!prevButtons[i] && curButtons[i])
        Mouse.click(injector->buttons[i].value.buttons);      
    }
  }

  if (injector->stick != NULL)
    injector->stick(curDataP);

  if (injector->exerciseMachine != NULL)
    injector->exerciseMachine(curDataP, exerciseMachineP, injector->exerciseMachineMultiplier);

  if (didJoystick) {
    curJoystick->send();
  }

  if (didX360)
    curX360->send(); 
}


