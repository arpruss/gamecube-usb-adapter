#include <USBXBox360.h>

static const char xMessage[] = "Exit2HID";
static const uint32 xMessageLen = sizeof(xMessage)-1;
static uint32 xMessagePos = 0;

void detectModeSwitch(uint8 left, uint8 right) {
  static uint32 lastRumble = 0;

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
 XBox360.begin();
 XBox360.setManualReportMode(true);
 XBox360.setRumbleCallback(detectModeSwitch);
 delay(500);
}

void endX360() {
 XBox360.end();
}


