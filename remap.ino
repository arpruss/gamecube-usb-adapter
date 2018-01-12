#include "gamecube.h"

#ifndef SERIAL_DEBUG

uint8_t prevButtons[numberOfButtons];
uint8_t curButtons[numberOfButtons];
GameCubeData_t oldData;
bool didJoystick;
bool didX360;
void* usbMode;
const Injector_t* prevInjector = NULL;

void joySliderLeft(uint16_t t) {
  Joystick.sliderLeft(1023-t&1023);
}

void joySliderRight(uint16_t t) {
  Joystick.sliderRight(1023-t&1023);
}

void buttonizeStick(uint8_t* buttons, uint8_t x, uint8_t y) {
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
    if (usbMode == &USBHID) {
      didJoystick = true;
      Joystick.X(range8u10u(data->joystickX));
      Joystick.Y(range8u10u(255-data->joystickY));
      Joystick.Xrotate(range8u10u(data->cX));
      Joystick.Yrotate(range8u10u(255-data->cY));
    }
    else if (usbMode == &XBox360) {
      didX360 = true;
      XBox360.X(range8u16s(data->joystickX));
      XBox360.Y(-range8u16s(255-data->joystickY));
      XBox360.XRight(range8u16s(data->cX));
      XBox360.YRight(-range8u16s(255-data->cY));
    }
}

void joystickPOV(const GameCubeData_t* data) {
    if (usbMode != &USBHID)
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
    Joystick.hat(dir);
}

uint16_t getEllipticalSpeed(const EllipticalData_t* ellipticalP, int32_t multiplier) {
  if (multiplier == 0) {
    if (ellipticalP->speed == 0)
      return 512;
    else if (ellipticalP->direction)
      return 1023;
    else
      return 0;
  }
  int32_t speed = 512+multiplier*(ellipticalP->direction?ellipticalP->speed:-ellipticalP->speed)/64;
  if (speed < 0)
    return 0;
  else if (speed > 1023)
    return 1023;
  else
    return speed;  
}

void joystickDualShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    if (usbMode == &USBHID) {
      joySliderLeft(range8u10u(data->shoulderLeft));
      joySliderRight(range8u10u(data->shoulderRight));
      didJoystick = true;
    }
    else if (usbMode == &XBox360) {
      XBox360.sliderLeft(data->shoulderLeft);
      XBox360.sliderRight(data->shoulderRight);
      didX360 = true;
    }
}

void ellipticalSliders(const GameCubeData_t* data, const EllipticalData_t* ellipticalP, int32_t multiplier) {
#ifdef ENABLE_ELLIPTICAL
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
        if (usbMode == &USBHID) {
          joySliderLeft(out);
          joySliderRight(out);
          didJoystick = true; 
        }
        else if (usbMode == &XBox360){
          XBox360.sliderLeft(out>>2);
          XBox360.sliderRight(out>>2);
          didX360 = true;
        }
        debounceDown.cancelRelease();
        return;
       }
    }
    if(data->device == DEVICE_GAMECUBE && ! ellipticalP->valid)
      return;
    uint16_t datum = getEllipticalSpeed(ellipticalP, multiplier);
    if (usbMode == &USBHID) {
      joySliderLeft(datum);
      joySliderRight(datum);
      didJoystick = true; 
    }
    else if (usbMode == &XBox360){
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

void directionSwitchSlider(const GameCubeData_t* data, const EllipticalData_t* ellipticalP, int32_t multiplier) {
    (void)multiplier;
    if (ellipticalP->direction) {
      if (usbMode == &USBHID) {
        didJoystick = true;
        joySliderRight(1023);
      }
      else if (usbMode == &XBox360) {
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
    if (usbMode == &USBHID) {
      joySliderLeft(datum);
      joySliderRight(datum);
    }
    else if (usbMode == &XBox360) {
      XBox360.sliderLeft(datum>>2);
      XBox360.sliderRight(datum>>2);
    }
}

void joystickNoShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
}

void inject(const Injector_t* injector, const GameCubeData_t* curDataP, const EllipticalData_t* ellipticalP) {
  didJoystick = false;
  didX360 = false;

  if (currentUSBMode != injector->usbMode) {
    if (lastChangedModeTime + 1000 <= millis()) {
      if (currentUSBMode == &USBHID) 
        USBHID.end();
      else if (currentUSBMode == &XBox360) 
        XBox360.end();
      
      delay(100);
      if (injector->usbMode == &USBHID) 
        beginUSBHID();    
      else if (injector->usbMode == &XBox360)
        beginX360();
      currentUSBMode = injector->usbMode;
    }
    return;
  }
  
  if (prevInjector != injector) {
    if (injector->usbMode == &USBHID) {
      Keyboard.releaseAll();
      Mouse.release(0xFF);
      for (int i=0; i<32; i++)
        Joystick.button(i, 0);
    }
    else if (injector->usbMode == &XBox360) {
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
      if (injector->usbMode == &USBHID) {
        Joystick.button(injector->buttons[i].value.button, curButtons[i]);
        didJoystick = true;
      }
      else if (injector->usbMode == &XBox360) {
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

  if (injector->elliptical != NULL)
    injector->elliptical(curDataP, ellipticalP, injector->ellipticalMultiplier);

  if (didJoystick)
    Joystick.send();

  if (didX360)
    XBox360.send(); 
}

#endif

