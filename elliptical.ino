#define MAX_SPEED_VALUE 800

#define BEST_REASONABLE_RPM 188 // 120 // 188
#define MAX_USABLE_RPM      300
#define SLOWEST_REASONABLE_RPM 5

const uint32_t turnOffSliderTime = 10000l;
const uint32_t shortestReasonableRotationTime = 1000l * 60 / BEST_REASONABLE_RPM;
const uint32_t shortestAllowedRotationTime = 1000l * 60 / MAX_USABLE_RPM;
const uint32_t longestReasonableRotationTime = 1000l * 60 / SLOWEST_REASONABLE_RPM;
uint32_t lastPulse = 0;
int32_t ellipticalSpeed;
volatile uint32_t ellipticalTriggerTime = 0;
volatile uint32_t lastEllipticalPeriod = 0;
volatile bool ellipticalPulsed = 0;

Debounce debounceRotation(rotationDetector, LOW);
Debounce debounceDirection(directionSwitch, DIRECTION_SWITCH_FORWARD);

void ellipticalInterrupt() {
  uint32_t t = millis();
  uint32_t delta = (uint32_t)(t-ellipticalTriggerTime);
  if (delta < shortestAllowedRotationTime) // assume glitch
    return;
  ellipticalTriggerTime = t;
  lastEllipticalPeriod = delta;
  ellipticalPulsed = true;
}

void ellipticalInit() {
#ifdef ENABLE_ELLIPTICAL
  pinMode(rotationDetector, INPUT); //ARP
  pinMode(directionSwitch, INPUT_PULLDOWN);
  attachInterrupt(rotationDetector, ellipticalInterrupt, ROTATION_DETECTOR_CHANGE_TO_MONITOR);
  debounceDirection.begin();
#endif
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

  if (ellipticalPulsed) {
//    Serial.println("Pulse");
    ellipticalPulsed = false;
    ellipticalRotationDetector = 1;
    updateLED();
    data->valid = true;
    lastPulse = millis();
    dt = lastEllipticalPeriod;
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
  else if (ellipticalRotationDetector && dt >= 50 && ROTATION_DETECTOR_ACTIVE_STATE != digitalRead(rotationDetector)) {
      ellipticalRotationDetector = 0;
      updateLED();
  }
  
  data->speed = ellipticalSpeed;
  data->direction = debounceDirection.getState();
#ifdef SERIAL_DEBUG
  Serial.println("Speed "+String(data->speed));
#endif    
#else
  data->speed = 0;
  data->direction = 1;
#endif
}

