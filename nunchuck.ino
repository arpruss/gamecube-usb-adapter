#include "gamecube.h"
#include <string.h>

#define SOFT_I2C // currently, HardWire doesn't work well for hotplugging
#undef MANUAL_DECRYPT

#ifdef SOFT_I2C
#include <Wire.h>
TwoWire HWire(MY_SCL, MY_SDA, SOFT_STANDARD); 
#else
#include <HardWire.h>
HardWire HWire(1, I2C_FAST_MODE); 
#endif

const uint8_t i2cAddress = 0x52;

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

uint8_t rescaleNunchuck(uint8_t x) {
  int32_t x1 = 128+((int32_t)x-128)*12/10;
  if (x1 < 0)
    return 0;
  else if (x1 > 255)
    return 255;
  else
    return x1;
}

uint8_t nunchuckDeviceInit() {
#ifdef MANUAL_DECRYPT
    if (!sendBytes(0x40,0x00))
        return 0; 
    delayMicroseconds(250);
#else
    if (! sendBytes(0xF0, 0x55)) 
        return 0;
    delayMicroseconds(250);
    if (! sendBytes(0xFB, 0x00)) 
        return 0;
#endif
    delayMicroseconds(250);
    return 1; 
}

uint8_t nunchuckReceiveReport(GameCubeData_t* data) {

    HWire.beginTransmission(i2cAddress);
    HWire.write(0x00);
    if (0!=HWire.endTransmission()) 
      return 0;

    delayMicroseconds(500);
#ifdef SERIAL_DEBUG
    Serial.println("Requested");
#endif

    HWire.requestFrom(i2cAddress, 6);
    int count = 0;
    while (HWire.available() && count<6) {
#ifdef MANUAL_DECRYPT
      nunchuckBuffer[count++] = ((uint8_t)0x17^(uint8_t)HWire.read()) + (uint8_t)0x17;
#else
      nunchuckBuffer[count++] = HWire.read();
#endif      
    }
    if (count < 6)
      return 0;
/*    for (int i=0;i<6;i++)
      Serial.print(String(nunchuckBuffer[i],HEX)+ " ");
    Serial.println("");  */
    data->joystickX = rescaleNunchuck(nunchuckBuffer[0]);
    data->joystickY = rescaleNunchuck(nunchuckBuffer[1]);
    data->buttons = 0;
    if (! (nunchuckBuffer[5] & 1) ) // Z
      data->buttons |= maskA;
    if (! (nunchuckBuffer[5] & 2) ) // C
      data->buttons |= maskB;
    data->device = DEVICE_NUNCHUCK;
    // ignore acceleration for now

    return 1;
}

