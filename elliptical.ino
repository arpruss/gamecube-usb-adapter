#define BEST_REASONABLE_RPM 120
#define MAX_USABLE_RPM 240
#define SLOWEST_REASONABLE_RPM 15

const uint32_t turnOffSliderTime = 10000l;
const uint32_t shortestReasonableRotationTime = 1000l * 60 / BEST_REASONABLE_RPM;
const uint32_t shortestAllowedRotationTime = 1000l * 60 / MAX_USABLE_RPM;
const uint32_t longestReasonableRotationTime = 1000l * 60 / SLOWEST_REASONABLE_RPM;
uint32_t newRotationPulseTime = 0;
uint32_t lastPulse = 0;
uint16_t ellipticalSpeed = 0;

Debounce debounceRotation(rotationDetector, LOW);
Debounce debounceDirection(directionSwitch, DIRECTION_SWITCH_FORWARD);

void ellipticalInit() {
#ifdef ENABLE_ELLIPTICAL
  pinMode(rotationDetector, INPUT_PULLUP);
  pinMode(directionSwitch, INPUT_PULLDOWN);
#endif
  ellipticalSpeed = 512;
  ellipticalRotationDetector = debounceRotation.getState();
}

void ellipticalUpdate(EllipticalData_t* data) {
#ifdef ENABLE_ELLIPTICAL
  uint32_t dt = millis() - lastPulse;

  if (dt > longestReasonableRotationTime) {
    ellipticalSpeed = 0;
    if (dt > turnOffSliderTime)
      data->valid = false;
  }

  if (debounceRotation.wasToggled()) {
    ellipticalRotationDetector = ! ellipticalRotationDetector;
    updateLED();
    data->valid = true;
    newRotationPulseTime = millis();
    if (ellipticalRotationDetector) {
      lastPulse = millis();
      if (dt >= shortestAllowedRotationTime) {
        if (dt > longestReasonableRotationTime) {
            ellipticalSpeed = 0;
        }
        else {
          if (dt < shortestReasonableRotationTime) {
            dt = shortestReasonableRotationTime;
          }
          ellipticalSpeed = 511 * shortestReasonableRotationTime / dt;
        } 
      }
    }
  }  
  
  data->speed = ellipticalSpeed;
  data->direction = debounceDirection.getState();
#ifdef SERIAL_DEBUG
//    Serial.println("speed="+String(data->speed));
//    Serial.println("dir="+String(data->direction));
#endif    
#else
  data->speed = 0;
  data->direction = 1;
#endif
}

