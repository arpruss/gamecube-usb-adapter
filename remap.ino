#include "gamecubecontroller.h"

#ifndef SERIAL_DEBUG

uint8_t prevButtons[numberOfButtons];
uint8_t curButtons[numberOfButtons];
GameCubeData_t oldData;
bool didJoystick;
bool didX360;
const USBMode_t* usbMode;
const Injector_t* prevInjector = NULL;
HIDJoystick* curJoystick;

void joySliderLeft(uint16_t t) {
  curJoystick->sliderLeft((1023-t)&1023);
}

void joySliderRight(uint16_t t) {
  curJoystick->sliderRight((1023-t)&1023);
}

void buttonizeStick4dir(uint8_t* buttons, uint8_t x, uint8_t y) {
  uint8_t dx = x < 128 ? 128 - x : x - 128;
  uint8_t dy = y < 128 ? 128 - y : y - 128;
  if (dx > dy) {
      if(dx > directionThreshold) {
        if (x < 128) {
          buttons[virtualLeft] = 1;
        }
        else {
          buttons[virtualRight] = 1;
        }
      }
  }
  else if (dx < dy && dy > directionThreshold) {
    if (y < 128) {
      buttons[virtualDown] = 1;
    }
    else {
      buttons[virtualUp] = 1;
    }
  }
}

const uint32_t tan22_5 = 4142;

void buttonizeStick(uint8_t* buttons, uint8_t x, uint8_t y) {
  uint32_t dx = x < 128 ? 128 - x : x - 128;
  uint32_t dy = y < 128 ? 128 - y : y - 128;
  uint32_t r2 = dx*dx+dy*dy;
  if (r2 <= directionThreshold*(uint32_t)directionThreshold)
    return;
  if (dy * 10000 < tan22_5 * dx) {
    if (x<128)
      buttons[virtualLeft]=1;
    else
      buttons[virtualRight]=1;
  }
  else if (dx * 10000 < dy * tan22_5) {
    if (y<128)
      buttons[virtualDown]=1;
    else
      buttons[virtualUp]=1;
  }
  else {
    if (x<128)
      buttons[virtualLeft]=1;
    else
      buttons[virtualRight]=1;
    if (y<128)
      buttons[virtualDown]=1;
    else
      buttons[virtualUp]=1;
  }
}

void toButtonArray(uint8_t* buttons, const GameCubeData_t* data) {
  for (int i=0; i<numberOfHardButtons; i++)
    buttons[i] = 0 != (data->buttons & buttonMasks[i]);
  buttons[virtualShoulderRightPartial] = data->shoulderRight>=shoulderThreshold;
  buttons[virtualShoulderLeftPartial] = data->shoulderLeft>=shoulderThreshold;
  buttons[virtualLeft] = buttons[virtualRight] = buttons[virtualDown] = buttons[virtualUp] = 0;
  buttonizeStick(buttons, data->joystickX, data->joystickY);
  buttonizeStick(buttons, data->cX, data->cY); 
}

inline uint16_t range8u10u(uint8_t x) {
  return (((uint16_t)x) << 2)+1;
}

inline int16_t range8u16s(uint8_t x) {
  return (((int32_t)(uint32_t)x-128)*32767+64)/127;
}

void joystickBasic(const GameCubeData_t* data) {
    if (usbMode != &modeX360) {
      didJoystick = true;
      curJoystick->X(range8u10u(data->joystickX));
      curJoystick->Y(range8u10u(255-data->joystickY));
      curJoystick->Xrotate(range8u10u(data->cX));
      curJoystick->Yrotate(range8u10u(255-data->cY));
    }
    else {
      didX360 = true;
      XBox360.X(range8u16s(data->joystickX));
      XBox360.Y(-range8u16s(255-data->joystickY));
      XBox360.XRight(range8u16s(data->cX));
      XBox360.YRight(-range8u16s(255-data->cY));
    }
}

void joystickPOV(const GameCubeData_t* data) {
    if (usbMode == &modeX360)
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

void joystickDualShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    if (usbMode != &modeX360) {
      joySliderLeft(range8u10u(data->shoulderLeft));
      joySliderRight(range8u10u(data->shoulderRight));
      didJoystick = true;
    }
    else {
      XBox360.sliderLeft(data->shoulderLeft);
      XBox360.sliderRight(data->shoulderRight);
      didX360 = true;
    }
}

void exerciseMachineSliders(const GameCubeData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
#ifdef ENABLE_EXERCISE_MACHINE
    if (debounceDown.getRawState() && data->device == DEVICE_NUNCHUCK) {
      // useful for calibration and settings for games: when downButton is pressed, joystickY controls both sliders
      if (data->joystickY >= 128+40 || data->joystickY <= 128-40) {
        int32_t delta = ((int32_t)data->joystickY - 128) * 49 / 10;
        uint16_t out;
        if (delta <= -511)
          out = 0;
        else if (delta >= 511)
          out = 1023;
        else
          out = 512 + delta;
        if (usbMode != &modeX360) {
          joySliderLeft(out);
          joySliderRight(out);
          didJoystick = true; 
        }
        else {
          XBox360.sliderLeft(out>>2);
          XBox360.sliderRight(out>>2);
          didX360 = true;
        }
        debounceDown.cancelRelease();
        return;
       }
    }
    uint16_t datum = getExerciseMachineSpeed(exerciseMachineP, multiplier);
    if(data->device == DEVICE_GAMECUBE && ! exerciseMachineP->valid)
      return;
    if (usbMode != &modeX360) {
      joySliderLeft(datum);
      joySliderRight(datum);
      didJoystick = true; 
    }
    else {
      if (datum>=512) {
        XBox360.sliderLeft(0);
        XBox360.sliderRight((datum-512)>>1);
      }
      else {
        XBox360.sliderRight(0);
        XBox360.sliderLeft((511-datum)>>1);
      }
      didX360 = true;
    }
#endif
}

void directionSwitchSlider(const GameCubeData_t* data, const ExerciseMachineData_t* exerciseMachineP, int32_t multiplier) {
    (void)multiplier;
    if (exerciseMachineP->direction) {
      if (usbMode != &modeX360) {
        didJoystick = true;
        joySliderRight(1023);
      }
      else {
        didX360 = true;
        XBox360.sliderRight(255);
        XBox360.sliderLeft(0);
      }
    }
}

void joystickUnifiedShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    
    uint16_t datum;
    datum = 512+(data->shoulderRight-(int16_t)data->shoulderLeft)*2;
    if (usbMode != &modeX360) {
      joySliderLeft(datum);
      joySliderRight(datum);
    }
    else {
      XBox360.sliderLeft(datum>>2);
      XBox360.sliderRight(datum>>2);
    }
}

void joystickNoShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
}

void inject(HIDJoystick* joy, const Injector_t* injector, const GameCubeData_t* curDataP, const ExerciseMachineData_t* exerciseMachineP) {
  curJoystick = joy;
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
    if (currentUSBMode != &modeX360) {
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
        XBox360.button(i, 0);
    }
    
    prevInjector = injector;
  }

  usbMode = injector->usbMode;

  memcpy(prevButtons, curButtons, sizeof(curButtons));
  toButtonArray(curButtons, curDataP);

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
      if (injector->usbMode != &modeX360) {
        curJoystick->button(injector->buttons[i].value.button, curButtons[i]);
        didJoystick = true;
      }
      else {
        XBox360.button(injector->buttons[i].value.button, curButtons[i]);
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
    XBox360.send(); 
}

#endif

