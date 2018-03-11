#define MAX_SPEED_VALUE 800

#define BEST_REASONABLE_RPM 145 // 120 // 188
#define MAX_USABLE_RPM      300
#define SLOWEST_REASONABLE_RPM 6

const uint32_t turnOffSliderTime = 10000l;
const uint32_t shortestReasonableRotationTime = 1000l * 60 / BEST_REASONABLE_RPM;
const uint32_t shortestAllowedRotationTime = 1000l * 60 / MAX_USABLE_RPM;
const uint32_t longestReasonableRotationTime = 1000l * 60 / SLOWEST_REASONABLE_RPM;
uint32_t lastPulse = 0;
int32_t exerciseMachineSpeed;
volatile uint32_t exerciseMachineTriggerTime = 0;
volatile uint32_t lastExerciseMachinePeriod = 0;
volatile bool exerciseMachinePulsed = 0;

Debounce debounceRotation(rotationDetector, LOW);
Debounce debounceDirection(directionSwitch, DIRECTION_SWITCH_FORWARD);

void exerciseMachineInterrupt() {
  uint32_t t = millis();
  uint32_t delta = (uint32_t)(t-exerciseMachineTriggerTime);
  if (delta < shortestAllowedRotationTime) // assume glitch
    return;
  exerciseMachineTriggerTime = t;
  lastExerciseMachinePeriod = delta;
  exerciseMachinePulsed = true;
}

void exerciseMachineInit() {
#ifdef ENABLE_EXERCISE_MACHINE
  pinMode(rotationDetector, INPUT); //ARP
  pinMode(directionSwitch, INPUT_PULLDOWN);
  attachInterrupt(rotationDetector, exerciseMachineInterrupt, ROTATION_DETECTOR_CHANGE_TO_MONITOR);
  debounceDirection.begin();
#endif
  exerciseMachineRotationDetector = debounceRotation.getState();
  exerciseMachineSpeed = 0;
}

void exerciseMachineUpdate(ExerciseMachineData_t* data) {
#ifdef ENABLE_EXERCISE_MACHINE
  uint32_t dt = millis() - lastPulse;

  if (dt > longestReasonableRotationTime) {
    exerciseMachineSpeed = 0;
    if (dt > turnOffSliderTime)
      data->valid = false;
  }
  else if ( 800 * shortestReasonableRotationTime < dt * exerciseMachineSpeed) {
    exerciseMachineSpeed = 800 * shortestReasonableRotationTime / dt;
  }

  if (exerciseMachinePulsed) {
    exerciseMachinePulsed = false;
    exerciseMachineRotationDetector = 1;
    updateLED();
    data->valid = true;
    lastPulse = millis();
    dt = lastExerciseMachinePeriod;
    if (dt > longestReasonableRotationTime) {
        exerciseMachineSpeed = 0;
    }
    else {
      if (dt < shortestReasonableRotationTime) {
        dt = shortestReasonableRotationTime;
      }
      exerciseMachineSpeed = 800 * shortestReasonableRotationTime / dt;
    } 
  }
  else if (exerciseMachineRotationDetector && dt >= 50 && ROTATION_DETECTOR_ACTIVE_STATE != digitalRead(rotationDetector)) {
      exerciseMachineRotationDetector = 0;
      updateLED();
  }
  
  data->speed = exerciseMachineSpeed;
  data->direction = debounceDirection.getState();
  DEBUG("Speed "+String(data->speed));
#else
  data->speed = 0;
  data->direction = 1;
#endif
}

