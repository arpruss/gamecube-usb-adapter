#define BEST_REASONABLE_RPM 120
#define MAX_USABLE_RPM 240
#define SLOWEST_REASONABLE_RPM 15

const uint32_t shortestReasonableRotationTime = 1000 * 60 / BEST_REASONABLE_RPM;
const uint32_t shortestAllowedRotationTime = 1000 * 60 / MAX_USABLE_RPM;
const uint32_t longestReasonableRotationTime = 1000 * 60 / SLOWEST_REASONABLE_RPM;
uint32_t newRotationPulseTime = 0;
uint32_t lastPulse = 0;

Debounce debounceRotation(rotationDetector, LOW);
Debounce debounceDirection(directionSwitch, HIGH);

uint8_t ellipticalRotationDetector;

void ellipticalInit() {
#ifdef ENABLE_ELLIPTICAL
  pinMode(rotationDetector, INPUT_PULLUP);
  pinMode(directionSwitch, INPUT_PULLDOWN);
#endif
  ellipticalSpeed = 512;
  ellipticalDirection = 1;
  ellipticalRotationDetector = debounceRotation.getState();
}

void ellipticalUpdate() {
#ifdef ENABLE_ELLIPTICAL
  ellipticalDirection = debounceDirection.getState();

  uint32_t dt = millis() - lastPulse;

  if (dt > longestReasonableRotationTime)
    ellipticalSpeed = 0;

  if (debounceRotation.wasToggled()) {
    ellipticalRotationDetector = ! ellipticalRotationDetector;
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
#endif
}

