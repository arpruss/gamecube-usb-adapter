// gamecube controller adapter

#include "dwt.h"
#include "debounce.h"
#include "gamecube.h"

#undef SERIAL_DEBUG

// brainlink purple (rightmost) 3.3V
// brainlink 4 from right pin 0
// brainlink red (leftmost) GND

// 1K resistor between 3.3V and Gamecube data pin
// facing socket, flat on top: 
//    123
//    ===
//    456
// connect: 2--PA6
// connect: 2--1K--3.3V
// connect: 3--GND
// connect: 4--GND
// connect: 6--3.3V
// LEDs: PA0, PA1, PA2, PA3
// optional: connect 1--5V (rumble, make sure there is enough current)

// TODO: replace pinMode() with something fast

uint32_t injectionMode = 0;
const uint32_t numInjectionModes = sizeof(injectors)/sizeof(*injectors);

const uint32_t indicatorLEDs[] = { PA0, PA1, PA2, PA3 };
const int numIndicators = sizeof(indicatorLEDs)/sizeof(*indicatorLEDs);
const uint32_t downButton = PA4;
const uint32_t upButton = PA5;

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
const uint8_t ledPinID = PB12;

const uint32_t gcPinID = PA6;
const uint8_t gcPin = 6;
gpio_dev* const gcPort = GPIOA;
const uint32_t cyclesPerUS = (SystemCoreClock/1000000ul);
const uint32_t quarterBitSendingCycles = cyclesPerUS*5/4;
const uint32_t bitReceiveCycles = cyclesPerUS*4;
const uint32_t halfBitReceiveCycles = cyclesPerUS*2;

volatile uint32 *gcPortPtr;
uint32_t gcPinBitmap;

const uint8_t maxFails = 4;
uint8_t fails;
Debounce debounceDown(downButton, HIGH);
Debounce debounceUp(upButton, HIGH);

void updateDisplay() {
  uint8_t x = numInjectionModes >= 16 ? injectionMode : injectionMode+1;
  for (int i=0; i<numIndicators; i++, x>>=1) 
    digitalWrite(indicatorLEDs[i], x&1);
}

void setup() {
  gcPortPtr = portOutputRegister(digitalPinToPort(PA9));
  gcPinBitmap = digitalPinToBitMask(PA9);
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 1;
  fails = maxFails; // force update
  for (int i=0; i<numIndicators; i++)
    pinMode(indicatorLEDs[i], OUTPUT);
  pinMode(downButton, INPUT_PULLDOWN);
  pinMode(upButton, INPUT_PULLDOWN);
  debounceDown.begin();
  debounceUp.begin();
  
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#else
  Joystick.setManualReportMode(true);
#endif
  pinMode(ledPinID, OUTPUT);

  updateDisplay();
}

// at most 32 bits can be sent
void gameCubeSendBits(uint32_t data, uint8_t bits) {
  //pinMode(gcPinID, OUTPUT);
  gpio_set_mode(gcPort, gcPin, GPIO_OUTPUT_PP);
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

//  pinMode(gcPinID, INPUT);
  gpio_set_mode(gcPort, gcPin, GPIO_INPUT_FLOATING);

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
    disableIRQ();
    gameCubeSendBits(0b000000001l, 9);
    enableIRQ();
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
  
  if (debounceUp.wasPressed()) {
    if (injectionMode == 0)
      injectionMode = numInjectionModes-1;
    else
      injectionMode--;
    updateDisplay();
  }
  
  if (debounceDown.wasPressed()) {
    injectionMode = (injectionMode+1) % numInjectionModes;
    updateDisplay();
  }
  
  if (gameCubeReceiveReport(&data, 0)) {
#ifdef SERIAL_DEBUG
    Serial.println("buttons1 = "+String(data.buttons));  
    Serial.println("joystick = "+String(data.joystickX)+","+String(data.joystickY));  
    Serial.println("c-stick = "+String(data.cX)+","+String(data.cY));  
    Serial.println("shoulders = "+String(data.shoulderLeft)+","+String(data.shoulderRight));      
#else
    inject(injectors + injectionMode, &data);
#endif
  }
  else {
#ifdef SERIAL_DEBUG
    Serial.println("fail");
#endif
  }
  delay(6);
}

