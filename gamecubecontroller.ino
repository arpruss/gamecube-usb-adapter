// gamecube controller adapter

#include "dwt.h"

#undef SERIAL_DEBUG

// brainlink purple (rightmost) 3.3V
// brainlink 4 from right pin 0
// brainlink red (leftmost) GND

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
const uint8_t ledPinID = PB12;

const uint32_t gcPinID = PA9;
const uint8_t gcPin = 9;
gpio_dev* const gcPort = GPIOA;
const uint32_t cyclesPerUS = (SystemCoreClock/1000000ul);
const uint32_t quarterBitSendingCycles = cyclesPerUS*5/4;
const uint32_t bitReceiveCycles = cyclesPerUS*4;
const uint32_t halfBitReceiveCycles = cyclesPerUS*2;

volatile uint32 *gcPortPtr;
uint32_t gcPinBitmap;

const uint8_t maxFails = 4;
uint8_t fails;

const uint16_t buttonA = 0x01;
const uint16_t buttonB = 0x02;
const uint16_t buttonX = 0x04;
const uint16_t buttonY = 0x08;
const uint16_t buttonStart = 0x10;
const uint16_t buttonDLeft = 0x100;
const uint16_t buttonDRight = 0x200;
const uint16_t buttonDDown = 0x400;
const uint16_t buttonDUp = 0x800;
const uint16_t buttonZ = 0x1000;
const uint16_t buttonShoulderRight = 0x2000;
const uint16_t buttonShoulderLeft = 0x4000;

typedef struct {
  uint16_t buttons;
  uint8_t joystickX;
  uint8_t joystickY;
  uint8_t cX;
  uint8_t cY;
  uint8_t shoulderLeft;
  uint8_t shoulderRight;
} GameCubeData_t;

void setup() {
  gcPortPtr = portOutputRegister(digitalPinToPort(PA9));
  gcPinBitmap = digitalPinToBitMask(PA9);
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 1;
  fails = maxFails; // force update
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#else
  Joystick.setManualReportMode(true);
#endif
  pinMode(ledPinID, OUTPUT);
}

// at most 32 bits can be sent
void gameCubeSendBits(uint32_t data, uint8_t bits) {
  pinMode(gcPinID, OUTPUT);
  data <<= 32-bits;
  DWT->CYCCNT = 0;
  uint32_t timerEnd = DWT->CYCCNT;
  do {
    gpio_write_bit(gcPort, gcPin, 0);
    if (0x80000000ul & data) 
      timerEnd += quarterBitSendingCycles;
    else
      timerEnd += 3*quarterBitSendingCycles;    
    while(DWT->CYCCNT < timerEnd) ;
    gpio_write_bit(gcPort, gcPin, 1);
    if (0x80000000ul & data) 
      timerEnd += 3*quarterBitSendingCycles;
    else
      timerEnd += quarterBitSendingCycles;    
    data <<= 1;
    bits--;
    while(DWT->CYCCNT < timerEnd) ;
  } while(bits);
}

void disableIRQ() {
  __asm volatile 
  (
      " CPSID   I        \n"
  );  
}

void enableIRQ() {
  __asm volatile 
  (
      " CPSIE   I        \n"
  );    
}

// bits must be greater than 0
uint8_t gameCubeReceiveBits(void* data0, uint32_t bits) {
  uint8_t* data = (uint8_t*)data0;
  
  uint32_t timeout = bitReceiveCycles * bits / 2 + 4;
  
  uint8_t bitmap = 0x80;

  pinMode(gcPinID, INPUT);

  *data = 0;
  do {
    if (!gpio_read_bit(gcPort, gcPin)) {
      DWT->CYCCNT = 0;
      while(DWT->CYCCNT < halfBitReceiveCycles-2) ;
      if (gpio_read_bit(gcPort, gcPin)) {
        *data |= bitmap;
      }
      bitmap >>= 1;
      bits--;
      if (bitmap == 0) {
        data++;
        bitmap = 0x80;
        if (bits) 
          *data = 0;
      }
      while(!gpio_read_bit(gcPort, gcPin) && --timeout) ;
      if (timeout==0) {
        break;
      }
    }
  } while(--timeout && bits);

  return bits==0;
}

uint8_t gameCubeReceiveReport(GameCubeData_t* data, uint8_t rumble) {
  if (fails >= maxFails) {
    gameCubeSendBits(0b000000001l, 9);
    delayMicroseconds(400);
    fails = 0;
  }              
  disableIRQ();
  gameCubeSendBits(rumble ? 0b0100000000000011000000011l : 0b0100000000000011000000001l, 25); 
  uint8_t success = gameCubeReceiveBits(data, 64);
  enableIRQ();
  if (success && 0 == (data->buttons & 0x80) && (data->buttons & 0x8000) ) {
    gpio_write_bit(ledPort, ledPin, 0);
    return 1;
  }
  else {
    gpio_write_bit(ledPort, ledPin, 1);
    fails++;
    return 0;
  }
}

uint8_t gameCubeReceiveReport(GameCubeData_t* data) {
  return gameCubeReceiveReport(data, 0);
}

void loop() {
  GameCubeData_t data;
  if (gameCubeReceiveReport(&data, 0)) {
#ifdef SERIAL_DEBUG
    Serial.println("buttons1 = "+String(data.buttons));  
    Serial.println("joystick = "+String(data.joystickX)+","+String(data.joystickY));  
    Serial.println("c-stick = "+String(data.cX)+","+String(data.cY));  
    Serial.println("shoulders = "+String(data.shoulderLeft)+","+String(data.shoulderRight));      
#else
    Joystick.X(data.joystickX);
    Joystick.Y(255-data.joystickY);
    Joystick.Z(data.cX);
    Joystick.Zrotate(data.cY);
    Joystick.sliderLeft(data.shoulderLeft);
    Joystick.sliderRight(data.shoulderRight);
    Joystick.button(1, (data.buttons & buttonA) != 0);
    Joystick.button(2, (data.buttons & buttonB) != 0);
    Joystick.button(3, (data.buttons & buttonX) != 0);
    Joystick.button(4, (data.buttons & buttonY) != 0);
    Joystick.button(5, (data.buttons & buttonStart) != 0);
    Joystick.button(6, (data.buttons & buttonZ) != 0);
    Joystick.button(7, (data.buttons & buttonShoulderLeft) != 0);
    Joystick.button(8, (data.buttons & buttonShoulderRight) != 0);

    int16_t dir = -1;
    if (data.buttons & (buttonDUp | buttonDRight | buttonDLeft | buttonDDown)) {
      if (0==(data.buttons & (buttonDRight| buttonDLeft))) {
        if (data.buttons & buttonDUp) 
          dir = 0;
        else 
          dir = 180;
      }
      else if (0==(data.buttons & (buttonDUp | buttonDDown))) {
        if (data.buttons & buttonDRight)
          dir = 90;
        else
          dir = 270;
      }
      else if (data.buttons & buttonDUp) {
        if (data.buttons & buttonDRight)
          dir = 45;
        else if (data.buttons & buttonDLeft)
          dir = 315;
      }
      else if (data.buttons & buttonDDown) {
        if (data.buttons & buttonDRight)
          dir = 135;
        else if (data.buttons & buttonDLeft)
          dir = 225;
      }
    }
    Joystick.hat(dir);
    
    Joystick.sendManualReport();
#endif
  }
  else {
#ifdef SERIAL_DEBUG
    Serial.println("fail");
#endif
  }
  delay(6);
}

