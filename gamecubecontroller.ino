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
// Put momentary pushbuttons between PA4 (decrement),PA5 (increment) and 3.3V

#include <libmaple/iwdg.h>
#include <libmaple/usb_cdcacm.h>
#include <libmaple/usb.h>
#include "debounce.h"
#include "gamecube.h"

Debounce debounceDown(downButton, HIGH);
Debounce debounceUp(upButton, HIGH);
Debounce debounceRotation(rotationDetector, LOW);

void displayNumber(uint8_t x) {
  for (int i=0; i<numIndicators; i++, x>>=1) 
    analogWrite(indicatorLEDs[i], (x&1) ? (255-ledBrightnessLevels[i]) : 255);
}

void updateDisplay() {
  displayNumber(numInjectionModes >= 16 ? injectionMode : injectionMode+1);
}

void setup() {
  for (int i=0; i<numIndicators; i++)
    pinMode(indicatorLEDs[i], OUTPUT);
  pinMode(downButton, INPUT_PULLDOWN);
  pinMode(upButton, INPUT_PULLDOWN);
  pinMode(rotationDetector, INPUT_PULLUP);

#ifdef ENABLE_GAMECUBE
  gameCubeInit();
#endif
#ifdef ENABLE_NUNCHUCK
  nunchuckInit();
#endif

  debounceDown.begin();
  debounceUp.begin();

  nvic_globalirq_disable();
  displayNumber(0xF);
  delay(3000);
  
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  delay(2000);
  Serial.println("gamecube controller adapter");
#else
  Joystick.setManualReportMode(true);
#endif
  pinMode(ledPinID, OUTPUT);

  EEPROM8_init();
  int i = EEPROM8_getValue(EEPROM_VARIABLE_INJECTION_MODE);
  if (i < 0)
    injectionMode = 0;
  else
    injectionMode = i; 

  if (injectionMode > numInjectionModes)
    injectionMode = 0;

  savedInjectionMode = injectionMode;
  
  updateDisplay();

  lastChangedModeTime = 0;
  iwdg_init(IWDG_PRE_256, watchdogSeconds*156);
}

uint8_t receiveReport(GameCubeData_t* data) {
  uint8_t success;
#ifdef ENABLE_GAMECUBE  
  if (validDevice == DEVICE_GAMECUBE || validDevice == DEVICE_NONE) {
    success = gameCubeReceiveReport(data);
    if (success) {
      validDevice = DEVICE_GAMECUBE;
      gpio_write_bit(ledPort, ledPin, 0);
      return 1;
    }
    validDevice = DEVICE_NONE;
  }
#endif
#ifdef ENABLE_NUNCHUCK
  if (validDevice != DEVICE_NUNCHUCK) 
    nunchuckDeviceInit();
  success = nunchuckReceiveReport(data);
  if (success) {
    validDevice = DEVICE_NUNCHUCK;
    gpio_write_bit(ledPort, ledPin, 0);
    return 1;
  }
#endif
  validDevice = DEVICE_NONE;
  gpio_write_bit(ledPort, ledPin, 1);
}

void loop() {
  GameCubeData_t data;

  iwdg_feed();
  
  uint32_t t0 = millis();
  while (digitalRead(downButton) == 1 && digitalRead(upButton) == 1 && (millis()-t0)<5000);
  
  iwdg_feed();

  if ((millis()-t0)>=5000) {
    displayNumber(0xF);
    injectionMode = 0;
    EEPROM8_reset();
    updateDisplay();
    savedInjectionMode = 0;
  }
  else {
    t0 = millis();
    do {
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
  #ifdef SERIAL_DEBUG      
        Serial.println("Changed to "+String(injectionMode));
  #endif
      }
    } while((millis()-t0) < 6);
  }

  if (savedInjectionMode != injectionMode && (millis()-lastChangedModeTime) >= saveInjectionModeAfterMillis) {
#ifdef SERIAL_DEBUG
    Serial.println("Need to store");
#endif
    EEPROM8_storeValue(EEPROM_VARIABLE_INJECTION_MODE, injectionMode);
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

  if (receiveReport(&data)) {
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
//    Serial.println("fail");
#endif
  }
}

