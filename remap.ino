#include "gamecube.h"

#ifndef SERIAL_DEBUG

uint8_t prevButtons[numberOfButtons];
uint8_t curButtons[numberOfButtons];
GameCubeData_t oldData;
bool didJoystick;

void buttonizeStick(uint8_t* buttons, uint8_t x, uint8_t y) {
  int dx = abs((int)x-(int)128);
  int dy = abs((int)y-(int)128);
  if (dx > dy) {
      if(dx > directionThreshold) {
        if (x < 128)
          buttons[virtualLeft] = 1;
        else
          buttons[virtualRight] = 1;
      }
  }
  else if (dx < dy && dy > directionThreshold) {
    if (y < 128)
      buttons[virtualDown] = 1;
    else 
      buttons[virtualUp] = 1;
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

inline uint16_t remapRange(uint8_t x) {
  return (((uint16_t)x) << 2)+1;
}

void joystickBasic(const GameCubeData_t* data) {
    didJoystick = true;
    Joystick.X(remapRange(data->joystickX));
    Joystick.Y(remapRange(255-data->joystickY));
    Joystick.Z(remapRange(data->cX));
    Joystick.Zrotate(remapRange(data->cY));
}

void joystickPOV(const GameCubeData_t* data) {
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

uint16_t getEllipticalSpeed(const EllipticalData_t* ellipticalP, uint32_t multiplier) {
  if (multiplier == 0) {
    if (ellipticalP->speed == 0)
      return 512;
    else if (ellipticalP->direction)
      return 1023;
    else
      return 0;
  }
  int32_t speed = 512+multiplier*(ellipticalP->direction?(int32_t)ellipticalP->speed:-(int32_t)ellipticalP->speed)/64;
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
    Joystick.sliderLeft(remapRange(data->shoulderLeft));
    Joystick.sliderRight(remapRange(data->shoulderRight));
}

void ellipticalSliders(const GameCubeData_t* data, const EllipticalData_t* ellipticalP, int32_t multiplier) {
#ifdef ENABLE_ELLIPTICAL
    if(data->device == DEVICE_GAMECUBE && ! ellipticalP->valid)
      return;
    uint16_t datum = getEllipticalSpeed(ellipticalP, multiplier);
    Joystick.sliderLeft(datum);
    Joystick.sliderRight(datum);
#endif
}

void joystickUnifiedShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
    
    uint16_t datum;
    datum = 512+(data->shoulderRight-(int16_t)data->shoulderLeft)*2;
    Joystick.sliderLeft(datum);
    Joystick.sliderRight(datum);
}

void joystickNoShoulder(const GameCubeData_t* data) {
    joystickBasic(data);
    joystickPOV(data);
}

void inject(const Injector_t* injector, const GameCubeData_t* curDataP, const EllipticalData_t* ellipticalP) {
  didJoystick = false;

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
      Joystick.button(injector->buttons[i].value.button, curButtons[i]);
      didJoystick = true;
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
    //ellipticalSliders(curDataP, ellipticalP);
    injector->elliptical(curDataP, ellipticalP, injector->ellipticalMultiplier);

  if (didJoystick)
    Joystick.sendManualReport();

}

#endif
