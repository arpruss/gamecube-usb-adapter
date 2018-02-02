#include "gamecubecontroller.h"
#include <string.h>

#define SOFT_I2C // currently, HardWire doesn't work well for hotplugging
                 // Also, it probably won't well with SPI remap
//#define MANUAL_DECRYPT

#ifdef SOFT_I2C
#include <SoftWire.h>
SoftWire MyWire(MY_SCL, MY_SDA, SOFT_STANDARD);  
#else
#include <Wire.h>
HardWire MyWire(1, 0); // I2C_FAST_MODE); 
#endif

const uint8_t i2cAddress = 0x52;

static uint8_t nunchuckBuffer[6];

void nunchuckInit() {
    MyWire.begin();
}

static uint8_t sendBytes(uint8_t location, uint8_t value) {
    MyWire.beginTransmission(i2cAddress);
    MyWire.write(location);
    MyWire.write(value);
    return 0 == MyWire.endTransmission();
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
    if (!sendBytes(0x40,0x00)) {
        return 0; 
    }
    delayMicroseconds(250);
#else
    if (! sendBytes(0xF0, 0x55)) {
        return 0;
    }
    delayMicroseconds(250);
    if (! sendBytes(0xFB, 0x00)) 
        return 0;
#endif
    delayMicroseconds(250);
    return 1; 
}

uint8_t nunchuckReceiveReport(GameCubeData_t* data) {
    MyWire.beginTransmission(i2cAddress);
    MyWire.write(0x00);
    if (0!=MyWire.endTransmission()) 
      return 0;

    delayMicroseconds(500);
#ifdef SERIAL_DEBUG
//    Serial.println("Requested");
#endif

    MyWire.requestFrom(i2cAddress, 6);
    int count = 0;
    while (MyWire.available() && count<6) {
#ifdef MANUAL_DECRYPT
      nunchuckBuffer[count++] = ((uint8_t)0x17^(uint8_t)MyWire.read()) + (uint8_t)0x17;
#else
      nunchuckBuffer[count++] = MyWire.read();
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
    data->cX = 128;
    data->cY = 128;
    data->shoulderLeft = 0;
    data->shoulderRight = 0;
    data->device = DEVICE_NUNCHUCK;
    // ignore acceleration for now

    return 1;
}


