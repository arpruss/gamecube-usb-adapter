// This requires this library: 
//    https://github.com/arpruss/USBHID_stm32f1
// Software steps:
//    Install this bootloader: https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/binaries/generic_boot20_pb12.bin?raw=true
//    Instructions here: http://wiki.stm32duino.com/index.php?title=Burning_the_bootloader#Flashing_the_bootloader_onto_the_Black_Pill_via_UART_using_a_Windows_machine/  
//    Install official Arduino Zero board
//    Put the contents of the above branch in your Arduino/Hardware folder
//    If on Windows, run drivers\win\install_drivers.bat
//    Install the USBHID_stm32f1 library.
// Note: You may need to adjust permissions on some of the dll, exe and bat files.

// Facing GameCube socket (as on console), flat on top:
//    123
//    ===
//    456

// Connections for GameCube adapter:
// GameCube 2--PA6
// GameCube 2--1Kohm--3.3V
// GameCube 3--GND
// GameCube 4--GND
// GameCube 6--3.3V
// Put a 10 uF and 0.1 uF capacitor between 3.3V and GND right on the black pill board.
// optional: connect GameCube 1--5V (rumble, make sure there is enough current)

// Put LEDs + resistors (100-220 ohm) between PA0,PA1,PA2,PA3 and 3.3V
// Put momentary pushbuttons between PA4 (decrement),PA5 (increment) and 3.3V

// Connections for exerciseMachine/bike:
// GND--GND
// PA7--4.7Kohm--rotation detector [the resistor may not be needed]
// PA7--100Kohm--3.3V

// For direction switch, to tell the device if you're pedaling backwards or forwards
// PA8 - direction control switch, one side
// 3.3V - direction control switch, other side

// Connections for Nunchuck
// GND--GND
// 3.3V--3.3V
// PB6--10Kohm--3.3V
// PB7--10Kohm--3.3V
// PB6--SCL
// PB7--SDA

#include <stdlib.h>
#include <libmaple/iwdg.h>
#include <libmaple/usb_cdcacm.h>
#include <libmaple/usb.h>
#include "debounce.h"
#include "gamecubecontroller.h"

Debounce debounceDown(downButton, HIGH);
Debounce debounceUp(upButton, HIGH);
unsigned numDisplayableModes = 0;

void displayNumber(uint8_t x) {
  for (int i=0; i<numIndicators; i++, x>>=1) 
    analogWrite(indicatorLEDs[i], (x&1) ? (255-ledBrightnessLevels[i]) : 255);
    //digitalWrite(indicatorLEDs[i], !(x&1));
}

void updateDisplay() {
  if (injectors[injectionMode].show) {
    unsigned count = 0;
    for (unsigned i = 0 ; i < injectionMode ; i++) {
      if (injectors[injectionMode].show)
        count++;
    }
    displayNumber(numDisplayableModes >= 16 ? count : count+1);    
  }
  else {
    displayNumber(0);
  }
}

const uint8_t reportDescription[] = {
   HID_MOUSE_REPORT_DESCRIPTOR(),
   HID_KEYBOARD_REPORT_DESCRIPTOR(),
   HID_JOYSTICK_REPORT_DESCRIPTOR(HID_JOYSTICK_REPORT_ID, 
        HID_FEATURE_REPORT_DESCRIPTOR(FEATURE_DATA_SIZE))
};

const uint8_t dualJoystickReportDescription[] = {
   HID_JOYSTICK_REPORT_DESCRIPTOR(HID_JOYSTICK_REPORT_ID, 
        HID_FEATURE_REPORT_DESCRIPTOR(FEATURE_DATA_SIZE)),
   HID_JOYSTICK_REPORT_DESCRIPTOR(HID_JOYSTICK_REPORT_ID+1)
};

uint8_t featureReport[FEATURE_DATA_SIZE];
uint8_t featureBuffer[HID_BUFFER_ALLOCATE_SIZE(FEATURE_DATA_SIZE,1)];
volatile HIDBuffer_t fb { featureBuffer, HID_BUFFER_SIZE(FEATURE_DATA_SIZE,1), HID_JOYSTICK_REPORT_ID };
HIDJoystick Joystick2(HID_JOYSTICK_REPORT_ID+1);

void beginUSBHID() {
#ifdef SERIAL_DEBUG
  USBHID_begin_with_serial(reportDescription,sizeof(reportDescription),0,0x25);
#else
  USBHID.begin(reportDescription,sizeof(reportDescription),0,0x25);
#endif
  USBHID.addFeatureBuffer(&fb);
  Joystick.setManualReportMode(true);
  delay(500);
}

void endUSBHID() {
  USBHID.end();
}

void beginDual() {
#ifdef SERIAL_DEBUG
  USBHID_begin_with_serial(dualJoystickReportDescription,sizeof(dualJoystickReportDescription),0,0x26);
#else
  USBHID.begin(dualJoystickReportDescription,sizeof(dualJoystickReportDescription),0,0x26);
#endif
  USBHID.addFeatureBuffer(&fb);
  Joystick.setManualReportMode(true);
  Joystick2.setManualReportMode(true);
  delay(500);
}

void endDual() {
  USBHID.end();
}

void setup() {
  for (int i=0; i<numIndicators; i++)
    pinMode(indicatorLEDs[i], OUTPUT);
  pinMode(downButton, INPUT_PULLDOWN);
  pinMode(upButton, INPUT_PULLDOWN);

  numDisplayableModes = 0;
  for (unsigned i=0; i<numInjectionModes; i++)
    if (injectors[i].show)
      numDisplayableModes++;
  
#ifdef SERIAL_DEBUG
  displayNumber(3);
  delay(4000);
  Serial.println("gamecube controller adapter");
#endif

  exerciseMachineInit();

#ifdef ENABLE_GAMECUBE
  gameCubeInit();
#endif
#ifdef ENABLE_NUNCHUCK
  nunchuckInit();
#endif

  debounceDown.begin();
  debounceUp.begin();

  pinMode(ledPinID, OUTPUT);

  EEPROM8_init();
  int i = EEPROM8_getValue(EEPROM_VARIABLE_INJECTION_MODE);
  if (i < 0)
    injectionMode = 0;
  else
    injectionMode = i; 

  if (injectionMode >= numInjectionModes)
    injectionMode = 0;

  savedInjectionMode = injectionMode;

  currentUSBMode = injectors[injectionMode].usbMode;
  currentUSBMode->begin();
  
  updateDisplay();

  lastChangedModeTime = 0;
  iwdg_init(IWDG_PRE_256, watchdogSeconds*156);
}

//uint8_t poorManPWM = 0;
void updateLED(void) {
  if (((validDevices[0] != DEVICE_NONE) ^ exerciseMachineRotationDetector) && validUSB) {
        gpio_write_bit(ledPort, ledPin, 0); //poorManPWM);
    //poorManPWM ^= 1;
  }
  else {
    gpio_write_bit(ledPort, ledPin, 1);
    //analogWrite(ledPinID, 255);
  }
  //gpio_write_bit(ledPort, ledPin, ! (((validDevice != DEVICE_NONE) ^ exerciseMachineRotationDetector) && validUSB));
}

static uint8_t receiveReport(GameCubeData_t* data, uint8_t deviceNumber) {
  uint8_t success;
  uint8_t reservedDevice = deviceNumber > 0 ? validDevices[0] : DEVICE_NONE;
  uint8_t validDevice = validDevices[deviceNumber];

#ifdef ENABLE_GAMECUBE
  if (reservedDevice != DEVICE_GAMECUBE && ( validDevices[deviceNumber] == DEVICE_GAMECUBE || validDevice == DEVICE_NONE) ) {
    success = gameCubeReceiveReport(data);
    if (success) {
      validDevices[deviceNumber] = DEVICE_GAMECUBE;
      return 1;
    }
    validDevice = DEVICE_NONE;
  }
#endif
#ifdef ENABLE_NUNCHUCK
  if (reservedDevice != DEVICE_NUNCHUCK && ( validDevice == DEVICE_NUNCHUCK || nunchuckDeviceInit())) {
    success = nunchuckReceiveReport(data);
    if (success) {
      validDevices[deviceNumber] = DEVICE_NUNCHUCK;
      return 1;
    }
  }
#endif
  validDevices[deviceNumber] = DEVICE_NONE;

  data->joystickX = 128;
  data->joystickY = 128;
  data->cX = 128;
  data->cY = 128;
  data->buttons = 0;
  data->shoulderLeft = 0;
  data->shoulderRight = 0;
  data->device = DEVICE_NONE;

  return 0;
}

void setFeature(const void* s) {
  strcpy((char*)featureReport, (const char*)s);
  Joystick.setFeature(featureReport);
}

void intToString(char* buf, int a) {
  if (a==0) {
    *buf++ = '0'; 
  }
  else {
    if (a<0) {
      *buf++ = '-';
      a = -a;
    }
    int place = 1;
    while(place * 10 > place && a / place > 0) {
      place *= 10;
    }
    if (a/place == 0) 
      place /= 10;
    do {
      int d = a/place;
      *buf++ = '0' + d;
      a -= d*place;
      place /= 10;
    } while(place>0);
  }
  *buf = 0;
}

void pollFeatureRequests() {
  if (currentUSBMode != &modeX360 && Joystick.getFeature(featureReport)) {
    if (0==strcmp((char*)featureReport, "id?")) {
      setFeature("id=GameCubeControllerAdapter");
    }
    else if (0==strcmp((char*)featureReport, "m?") || 0==strcmp((char*)featureReport, "M?")) {
      featureReport[1] = '=';
      strcpy((char*)featureReport+2,featureReport[0]=='m' ? injectors[injectionMode].commandName : injectors[injectionMode].description);
      setFeature(featureReport);
    }
    else if (0==strncmp((char*)featureReport, "m:", 2) || 0==strncmp((char*)featureReport, "M:", 2)) {
      for (unsigned i=0; i < numInjectionModes; i++) {
        if (0==strncmp((char*)featureReport+2, featureReport[0]=='m' ? injectors[i].commandName : injectors[i].description, FEATURE_DATA_SIZE-2)) {
          injectionMode = i;
          lastChangedModeTime = millis();
          updateDisplay();
          break;
        }
      }
    }
    else if (0==strcmp((char*)featureReport, "modes?")) {
      strcpy((char*)featureReport, "modes=");
      intToString((char*)featureReport+6, numInjectionModes);
      setFeature(featureReport);
    }
    else if ((featureReport[0] == 'm' || featureReport[0] == 'M') && isdigit(featureReport[1]) && featureReport[strlen((char*)featureReport)-1]=='?') {
      unsigned n = atoi((char*)featureReport+1);
      intToString((char*)featureReport+1, n);
      strcat((char*)featureReport, "=");
      if (n<numInjectionModes) {
        strcat((char*)featureReport, featureReport[0] == 'm' ? injectors[n].commandName : injectors[n].description);
      }
      setFeature(featureReport);
    }
    else {
      setFeature("");
    }
  }
}

void adjustMode(int delta) {
  do {
    if (delta < 0 && injectionMode == 0)
      injectionMode = numInjectionModes;
    injectionMode += delta;
    injectionMode %= numInjectionModes; 
  } while (! injectors[injectionMode].show);
  lastChangedModeTime = millis();
  updateDisplay();
}

void loop() {
  GameCubeData_t data;
  GameCubeData_t data2;
  ExerciseMachineData_t exerciseMachine;
  bool dual = false;

  iwdg_feed();
  
  uint32_t t0 = millis();
  while (debounceDown.getRawState() && debounceUp.getRawState() && (millis()-t0)<5000)
        updateLED();
  
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
      if (debounceDown.wasReleased()) {
        adjustMode(-1);
      }
      
      if (debounceUp.wasPressed()) {
        adjustMode(1);
  #ifdef SERIAL_DEBUG      
        Serial.println("Changed to "+String(injectionMode));
  #endif
      }
      updateLED();
    } while((millis()-t0) < 6);
  }

  pollFeatureRequests();

  exerciseMachineUpdate(&exerciseMachine);
      
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
    validUSB = 0;
    updateLED();
    return;
  } // TODO: fix library so it doesn't send on a disconnected connection; currently, we're relying on the watchdog reset 
  else {
    validUSB = 1;
  }
    // if a disconnection happens at the wrong time
#else
  validUSB = 1;
#endif

  dual = currentUSBMode == &modeDualJoystick && injectors[injectionMode].usbMode == &modeDualJoystick;
  receiveReport(&data,0);
  if (dual)
    dual = (bool)receiveReport(&data2,1);
#ifdef SERIAL_DEBUG
//  Serial.println("buttons1 = "+String(data.buttons));  
  Serial.println("joystick = "+String(data.joystickX)+","+String(data.joystickY));  
//  Serial.println("c-stick = "+String(data.cX)+","+String(data.cY));  
//  Serial.println("shoulders = "+String(data.shoulderLeft)+","+String(data.shoulderRight));      
#else
  if (usb_is_connected(USBLIB) && usb_is_configured(USBLIB)) {
    inject(&Joystick, injectors + injectionMode, &data, &exerciseMachine);
    if (dual) {
      inject(&Joystick2, injectors + injectionMode, &data2, &exerciseMachine);
    }
}
#endif
    
  updateLED();
}

