#include "gamecube.h"
#include <string.h>
#include <HardWire.h>

const uint8_t i2cAddress = 0x52;

HardWire HWire(1, I2C_FAST_MODE);
static uint8_t nunchuckBuffer[6];

void nunchuckInit() {
  HWire.begin();
}

static uint8_t sendBytes(uint8_t location, uint8_t value) {
    HWire.beginTransmission(i2cAddress);
    HWire.write(location);
    HWire.write(value);
    return 0 == HWire.endTransmission();
}

uint8_t nunchuckDeviceInit() {
    if (! sendBytes(0x55, 0xF0))
      return 0;
    if (! sendBytes(0x00, 0xFB))
      return 0;
    return 1;    
}

uint8_t nunchuckReceiveReport(GameCubeData_t* data) {
    HWire.requestFrom(i2cAddress, 6);
    int count = 0;
    while (HWire.available() && count<6) {
      nunchuckBuffer[count++] = HWire.read();
    }
    if (count < 6)
      return 0;
    memset(data,0,sizeof(GameCubeData_t));
    data->joystickX = nunchuckBuffer[0];
    data->joystickY = nunchuckBuffer[1];
    data->buttons = 0;
    if (! (nunchuckBuffer[5] & 1) ) // Z
      data->buttons |= maskA;
    if (! (nunchuckBuffer[5] & 2) ) // C
      data->buttons |= maskB;
    // ignore acceleration for now
    return 1;
}

