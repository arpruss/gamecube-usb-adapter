#include <USBXBox360.h>

static const char xMessage[] = "Exit2HID";
static const uint32 xMessageLen = sizeof(xMessage)-1;
static uint32 xMessagePos = 0;

void detectModeSwitch(uint8 left, uint8 right) {
  static uint32 lastRumble = 0;

  leftMotor = left;
  rightMotor = right;

  if (millis()-lastRumble > 2000) {
    xMessagePos = 0;
  }

  lastRumble = millis();

  if ((char)left == xMessage[xMessagePos] && (char)(right ^ 0x4B) == xMessage[xMessagePos]) {
    xMessagePos++;
    if (xMessagePos >= xMessageLen) {
      xMessagePos = 0;
      for (uint32 i=0; i<numInjectionModes; i++) {
        if (injectors[i].usbMode != &modeX360) {
          leftMotor = 0;
          rightMotor = 0;
          injectionMode = i;
          lastChangedModeTime = millis();
          updateDisplay();
          return;
        }
      }
    }
  }
}

void beginX360() {
 USBComposite.setProductString("XBox360 controller emulator");
 XBox360.begin();
 XBox360.setManualReportMode(true);
 XBox360.buttons(0);
 XBox360.setRumbleCallback(detectModeSwitch);
 delay(500);
 x360_1 = &XBox360;
 x360_2 = NULL;
}

void endX360() {
 XBox360.end();
}

void beginDualX360() {
 USBComposite.setProductString("Dual XBox360 controller emulator");
 DualXBox360.begin();
 x360_1 = DualXBox360.controllers;
 x360_1->setManualReportMode(true);
 x360_1->setRumbleCallback(detectModeSwitch);
 x360_1->buttons(0);
 x360_2 = DualXBox360.controllers + 1;
 x360_2->setManualReportMode(true);
 x360_2->setRumbleCallback(detectModeSwitch);
 x360_2->buttons(0);
 delay(500);
}

void endDualX360() {
 DualXBox360.end();
}


