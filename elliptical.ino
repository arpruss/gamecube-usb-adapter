#define MAX_SPEED_VALUE 800

#define BEST_REASONABLE_RPM 188 // 120 // 188
#define MAX_USABLE_RPM      300
#define SLOWEST_REASONABLE_RPM 5

const uint32_t turnOffSliderTime = 10000l;
const uint32_t shortestReasonableRotationTime = 1000l * 60 / BEST_REASONABLE_RPM;
const uint32_t shortestAllowedRotationTime = 1000l * 60 / MAX_USABLE_RPM;
const uint32_t longestReasonableRotationTime = 1000l * 60 / SLOWEST_REASONABLE_RPM;
uint32_t newRotationPulseTime = 0;
uint32_t lastPulse = 0;
int32_t ellipticalSpeed;

Debounce debounceRotation(rotationDetector, LOW);
Debounce debounceDirection(directionSwitch, DIRECTION_SWITCH_FORWARD);
//DebounceAnalog analogRotation(rotationDetector, LOW);

void ellipticalInit() {
#ifdef ENABLE_ELLIPTICAL
  pinMode(rotationDetector, INPUT); //ARP
  pinMode(directionSwitch, INPUT_PULLDOWN);
#endif
  debounceRotation.begin();
  debounceDirection.begin();
//  analogRotation.begin();
  ellipticalRotationDetector = debounceRotation.getState();
  ellipticalSpeed = 0;
}

void ellipticalUpdate(EllipticalData_t* data) {
#ifdef ENABLE_ELLIPTICAL
  uint32_t dt = millis() - lastPulse;

  if (dt > longestReasonableRotationTime) {
    ellipticalSpeed = 0;
    if (dt > turnOffSliderTime)
      data->valid = false;
  }
  else if ( 800 * shortestReasonableRotationTime < dt * ellipticalSpeed) {
    ellipticalSpeed = 800 * shortestReasonableRotationTime / dt;
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
          ellipticalSpeed = 800 * shortestReasonableRotationTime / dt;
        } 
      }
    }
  }  
  
  data->speed = ellipticalSpeed;
  data->direction = debounceDirection.getState();
#ifdef SERIAL_DEBUG
#endif    
#else
  data->speed = 0;
  data->direction = 1;
#endif
}

