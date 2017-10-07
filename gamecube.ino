#include "gamecube.h"
#include "dwt.h"

static const uint8_t maxFails = 4;
static uint8_t fails;

const uint32_t cyclesPerUS = (SystemCoreClock / 1000000ul);
const uint32_t quarterBitSendingCycles = cyclesPerUS * 5 / 4;
const uint32_t bitReceiveCycles = cyclesPerUS * 4;
const uint32_t halfBitReceiveCycles = cyclesPerUS * 2;

void gameCubeInit(void) {
  gpio_set_mode(gcPort, gcPin, GPIO_OUTPUT_OD); // set open drain output
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 1;
  fails = maxFails; // force update
}

// at most 32 bits can be sent
void gameCubeSendBits(uint32_t data, uint8_t bits) {
  data <<= 32 - bits;
  DWT->CYCCNT = 0;
  uint32_t timerEnd = DWT->CYCCNT;
  do {
    gpio_write_bit(gcPort, gcPin, 0);
    if (0x80000000ul & data)
      timerEnd += quarterBitSendingCycles;
    else
      timerEnd += 3 * quarterBitSendingCycles;
    while (DWT->CYCCNT < timerEnd) ;
    gpio_write_bit(gcPort, gcPin, 1);
    if (0x80000000ul & data)
      timerEnd += 3 * quarterBitSendingCycles;
    else
      timerEnd += quarterBitSendingCycles;
    data <<= 1;
    bits--;
    while (DWT->CYCCNT < timerEnd) ;
  } while (bits);
}

// bits must be greater than 0
uint8_t gameCubeReceiveBits(void* data0, uint32_t bits) {
  uint8_t* data = (uint8_t*)data0;

  uint32_t timeout = bitReceiveCycles * bits / 2 + 4;

  uint8_t bitmap = 0x80;

  *data = 0;
  do {
    if (!gpio_read_bit(gcPort, gcPin)) {
      DWT->CYCCNT = 0;
      while (DWT->CYCCNT < halfBitReceiveCycles - 2) ;
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
      while (!gpio_read_bit(gcPort, gcPin) && --timeout) ;
      if (timeout == 0) {
        break;
      }
    }
  } while (--timeout && bits);

  return bits == 0;
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
  data->device = DEVICE_GAMECUBE;
  nvic_globalirq_enable();
  if (success && 0 == (data->buttons & 0x80) && (data->buttons & 0x8000) ) {
    return 1;
  }
  else {
    fails++;
    return 0;
  }
}

uint8_t gameCubeReceiveReport(GameCubeData_t* data) {
  return gameCubeReceiveReport(data, 0);
}


