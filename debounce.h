#ifndef _DEBOUNCE_H
#define _DEBOUNCE_H

class Debounce {
  private:
    uint32_t lastToggleTime;
    uint8_t activeValue;
    uint8_t curValue;
    uint32_t debounceTime = 20;
    int pin;
    bool releaseCanceled = false;

    bool update(void) {
      uint8_t v = digitalRead(pin);
      uint32_t t = millis();
      if (v != curValue && t - lastToggleTime >= debounceTime) {
        lastToggleTime = t;
        curValue = v;
        return true;
      }
      return false;
    }

  public:
    Debounce(int p, uint8_t a) {
      pin = p;
      activeValue = a;
    }

    Debounce(int p, uint8_t a, uint32_t t) {
      pin = p;
      activeValue = a;
      debounceTime = t;
    }

    void begin(void) {
      curValue = digitalRead(pin);      
      lastToggleTime = millis();
    }

    // Code using Debounce should choose exactly one of three polling methods:
    //   wasToggled(), getState() and wasPressed()
    // and should not switch between them, with the exception that a single initial call to
    // getState() is acceptable. 

    // for toggle monitoring
    inline bool wasToggled(void) {
      return update();
    }
    
    // for state monitoring
    bool getState(void) {
      update();
      return curValue == activeValue;
    }

    // for press monitoring
    bool wasPressed(void) {
      return wasToggled() && curValue == activeValue;
    }

    void cancelRelease(void) {
      releaseCanceled = true;
    }

    // for release monitoring
    bool wasReleased(void) {
      if (wasToggled() && curValue == ! activeValue) {
        if (releaseCanceled) {
          releaseCanceled = false;
          return false;
        }
        else {
          return true;
        }
      }
      return false;
    }
};

#endif

