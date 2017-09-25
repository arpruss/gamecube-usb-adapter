// gamecube controller adapter
// This requires this branch of the Arduino STM32 board: 
//    https://github.com/arpruss/Arduino_STM32/tree/addMidiHID
// Software steps:
//    Install this bootloader: https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/binaries/generic_boot20_pb12.bin?raw=true
//    Instructions here: http://wiki.stm32duino.com/index.php?title=Burning_the_bootloader#Flashing_the_bootloader_onto_the_Black_Pill_via_UART_using_a_Windows_machine//
//    Install official Arduino Zero board
//    Put the contents of the above branch in your Arduino/Hardware folder
//    If on Windows, run drivers\win\install_drivers.bat
// Note: You may need to adjust permissions on some of the dll, exe and bat files.

// Facing Gamecube socket (as on console), flat on top:
//    123
//    ===
//    456

// Connections:
// Gamecube 2--PA6
// Gamecube 2--1Kohm--3.3V
// Gamecube 3--GND
// Gamecube 4--GND
// Gamecube 6--3.3V
// Put a 10 uF and 0.1 uF capacitor between 3.3V and GND right on the black pill board.
// optional: connect Gamecube 1--5V (rumble, make sure there is enough current)

// Put LEDs + resistors (100-220 ohm) between PA0,PA1,PA2,PA3 and 3.3V
// Put momentary pushbuttons between PA4,PA5 and 3.3V

#include <libmaple/iwdg.h>
#include <libmaple/usb_cdcacm.h>
#include <libmaple/usb.h>
#include "dwt.h"
#include "debounce.h"
#include "gamecube.h"

#undef SERIAL_DEBUG

const uint32_t watchdogSeconds = 10;
const uint32_t numInjectionModes = sizeof(injectors)/sizeof(*injectors);

const uint32_t indicatorLEDs[] = { PA0, PA1, PA2, PA3 };
const int numIndicators = sizeof(indicatorLEDs)/sizeof(*indicatorLEDs);
const uint32_t downButton = PA5;
const uint32_t upButton = PA4;

gpio_dev* const ledPort = GPIOB;
const uint8_t ledPin = 12;
const uint8_t ledPinID = PB12;

const uint32_t saveInjectionModeAfterMillis = 15000ul; // only save a mode if it's been used 15 seconds; this saves flash

const uint32_t gcPinID = PA6;
const uint8_t gcPin = 6;
gpio_dev* const gcPort = GPIOA;

const uint32_t cyclesPerUS = (SystemCoreClock/1000000ul);
const uint32_t quarterBitSendingCycles = cyclesPerUS*5/4;
const uint32_t bitReceiveCycles = cyclesPerUS*4;
const uint32_t halfBitReceiveCycles = cyclesPerUS*2;

uint32_t injectionMode = 0;
uint32_t savedInjectionMode = 0;
uint32_t lastChangedModeTime;

volatile uint32 *gcPortPtr;
uint32_t gcPinBitmap;

const uint8_t maxFails = 4;
uint8_t fails;
Debounce debounceDown(downButton, HIGH);
Debounce debounceUp(upButton, HIGH);

void displayNumber(uint8_t x) {
  for (int i=0; i<numIndicators; i++, x>>=1) 
    digitalWrite(indicatorLEDs[i], 1^(x&1));
}

void updateDisplay() {
  displayNumber(numInjectionModes >= 16 ? injectionMode : injectionMode+1);
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

  injectionMode = EEPROM254_getValue();
  if (injectionMode == 0xFF)
    injectionMode = 0;
  savedInjectionMode = injectionMode;
  if (injectionMode > numInjectionModes)
    injectionMode = 0;
  updateDisplay();

  lastChangedModeTime = 0;

  iwdg_init(IWDG_PRE_256, watchdogSeconds*156);
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
    nvic_globalirq_disable();
    gameCubeSendBits(0b000000001l, 9);
    nvic_globalirq_enable();
    delayMicroseconds(400);
    fails = 0;
  }              
  nvic_globalirq_disable();
  gameCubeSendBits(rumble ? 0b0100000000000011000000011l : 0b0100000000000011000000001l, 25); 
  uint8_t success = gameCubeReceiveBits(data, 64);
  nvic_globalirq_enable();
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

  iwdg_feed();
  uint32_t t0 = millis();

  while((millis()-t0) < 6) {
    if (debounceDown.wasPressed()) {
      if (injectionMode == 0)
        injectionMode = numInjectionModes-1;
      else
        injectionMode--;
      lastChangedModeTime = millis();
      updateDisplay();
    }
    
    if (debounceUp.wasPressed()) {
      injectionMode = (injectionMode+1) % numInjectionModes;
      lastChangedModeTime = millis();
      updateDisplay();
    }
  }

  if (savedInjectionMode != injectionMode && (millis()-lastChangedModeTime) >= saveInjectionModeAfterMillis) {
    EEPROM254_storeValue(injectionMode);
    savedInjectionMode = injectionMode;
  }

#ifndef SERIAL_DEBUG
  if (!usb_is_connected(USBLIB) || !usb_is_configured(USBLIB)) {
    // we're disconnected; save power by not talking to controller
    gpio_write_bit(ledPort, ledPin, 1);
    return;
  } // TODO: fix library so it doesn't send on a disconnected connection; currently, we're relying on the watchdog reset 
    // if a disconnection happens at the wrong time
#endif

  if (gameCubeReceiveReport(&data, 0)) {
#ifdef SERIAL_DEBUG
    Serial.println("buttons1 = "+String(data.buttons));  
    Serial.println("joystick = "+String(data.joystickX)+","+String(data.joystickY));  
    Serial.println("c-stick = "+String(data.cX)+","+String(data.cY));  
    Serial.println("shoulders = "+String(data.shoulderLeft)+","+String(data.shoulderRight));      
#else 
    if (usb_is_connected(USBLIB) && usb_is_configured(USBLIB)) 
      inject(injectors + injectionMode, &data);
#endif
  }
  else {
#ifdef SERIAL_DEBUG
    Serial.println("fail");
#endif
  }
}

