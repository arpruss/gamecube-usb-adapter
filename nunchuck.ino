#include "gamecube.h"
#include <string.h>
#include <Wire.h>
//#include <HardWire.h>

const uint8_t i2cAddress = 0x52;

TwoWire HWire(MY_SCL, MY_SDA, SOFT_STANDARD); 
//HardWire HWire(1, I2C_FAST_MODE); 
static uint8_t nunchuckBuffer[6];

void nunchuckInit() {
  HWire.begin();
//  pinMode(MY_SCL, INPUT_PULLUP);
//  pinMode(MY_SDA, INPUT_PULLUP); 
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
#ifdef SERIAL_DEBUG  
    Serial.println("nunchuck init try");
#endif
/*    if (! sendBytes(0x55, 0xF0)) {
#ifdef SERIAL_DEBUG  
    Serial.println("nunchuck init fail 1");
#endif
      return 0;
    }
    if (! sendBytes(0x00, 0xFB)) {
#ifdef SERIAL_DEBUG  
    Serial.println("nunchuck init fail 1");
#endif
      return 0;  
    } */
    if (!sendBytes(0x40,0x00))
      return 0; 
#ifdef SERIAL_DEBUG  
    Serial.println("nunchuck init success");
#endif
    delay(1);
    return 1;    
}

uint8_t nunchuckReceiveReport(GameCubeData_t* data) {

    HWire.beginTransmission(i2cAddress);
    HWire.write(0x00);
    if (0!=HWire.endTransmission()) 
      return 0;

    delay(1);

    HWire.requestFrom(i2cAddress, 6);
    int count = 0;
    while (HWire.available() && count<6) {
      uint8_t datum = HWire.read();
      nunchuckBuffer[count++] = (datum^(uint8_t)0x17)+(uint8_t)0x17;
    }
    if (count < 6)
      return 0;
    memset(data,0,sizeof(GameCubeData_t));
/*    for (int i=0;i<6;i++)
      Serial.print(String(nunchuckBuffer[i],HEX)+ " ");
    Serial.println("");  */
    //delay(5);
    data->joystickX = rescaleNunchuck(nunchuckBuffer[0]);
    data->joystickY = rescaleNunchuck(nunchuckBuffer[1]);
    data->buttons = 0;
    if (! (nunchuckBuffer[5] & 1) ) // Z
      data->buttons |= maskA;
    if (! (nunchuckBuffer[5] & 2) ) // C
      data->buttons |= maskB;
    // ignore acceleration for now

    return 1;
}

