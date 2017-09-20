#include "dwt.h"

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
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

typedef struct {
  uint8_t buttons1;
  uint8_t buttons2;
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
  Serial.begin(115200);
}

// at most 32 bits can be sent
void gameCubeSendBits(uint32_t data, uint8_t bits) {
  pinMode(gcPin, OUTPUT);
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

// bits must be greater than 0
uint8_t gameCubeReceiveBits(void* data0, uint32_t bits) {
  uint8_t* data = (uint8_t*)data0;
  
  uint32_t timeout = bitReceiveCycles * (bits+4);
  
  uint8_t bitmap = 0x80;
  
  pinMode(gcPin, INPUT);

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
      while(gpio_read_bit(gcPort, gcPin) && --timeout) ;
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
  gameCubeSendBits(rumble ? 0b0100000000000011000000011l : 0b0100000000000011000000001l, 25); 
  if (gameCubeReceiveBits(data, 64)) {
    gpio_write_bit(ledPort, ledPin, 1);
    return 1;
  }
  else {
    gpio_write_bit(ledPort, ledPin, 0);
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
    Serial.println("buttons1 = "+String(data.buttons1));  
    Serial.println("buttons2 = "+String(data.buttons2));  
    Serial.println("joystick = "+String(data.joystickX)+","+String(data.joystickY));  
    Serial.println("c-stick = "+String(data.cX)+","+String(data.cY));  
    Serial.println("shoulders = "+String(data.shoulderLeft)+","+String(data.shoulderRight));      
  }
  else {
    Serial.println("fail");
  }
  delay(10);
}

