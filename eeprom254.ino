#include <EEPROM.h>
#include <flash_stm32.h>

// This is a module that lets you store almost a byte of configuration data in the two EEPROM-emulating flash pages.
// Almost byte = value from 0x00 to 0xFE.

// The storage method is incompatible with the one implemented by the EEPROM-emulation library, as it's optimized
// for our almost-byte usage case. The two flash pages are treated as a single unit, and the last non-0xFF byte
// stored in them is the current data value. To write a new byte, first we check if we can change the last byte
// to the desired one by programming some bits to zero. If not, we write a new byte. If we've run out of space on the
// two pages, then we erase them both.

// Note: Flash storage is rated for 10K cycles.
// There are 1020 bytes available per page, making for about 20M cycles.
// Worst case scenario (and very unrealistic) is when GameCubeController's configuration changes every 15 seconds.
// This will result in 9.5 years of operation. 
// That's good enough!

static boolean invalid = true;
static uint32_t lastBase = 0;
static uint32_t lastOffset = 0;

#define EEPROM254_MEMORY_SIZE (2*EEPROM_PAGE_SIZE)
#define GET_BYTE(address) (*(__io uint8_t*)(address))
#define GET_HALF_WORD(address) (*(__io uint16_t*)(address))
#define GET_WORD(address) (*(__io uint32_t*)(address))

#define EEPROM254_MAGIC (uint32_t)0x1b70f1cc

static void findValidValueAddress(void) {
  if (GET_BYTE(EEPROM_PAGE0_BASE+4) == 0xFF) { // page 0 is blank
    lastBase = 0; // no data at all
    return;
  }
  
  if (GET_BYTE(EEPROM_PAGE1_BASE+4) != 0xFF) // is there any data on page 1?
    lastBase = EEPROM_PAGE1_BASE; 
  else
    lastBase = EEPROM_PAGE0_BASE;

  for (uint32_t address = EEPROM_PAGE_SIZE-1 ; address >= 4 ; address--) {
    if (0xFF != GET_BYTE(lastBase + address)) {
        lastOffset = address;
        return;
    }
  }

  // weird! something changed!
  lastBase = 0;
  invalid = true;
}

static bool erasePage(uint32_t base) {
  bool success;

  FLASH_Unlock();
  success = ( FLASH_COMPLETE == FLASH_ErasePage(base) );

  success = success && 
    FLASH_COMPLETE == FLASH_ProgramHalfWord(base, (uint16_t)EEPROM254_MAGIC) &&
    FLASH_COMPLETE == FLASH_ProgramHalfWord(base+2, (uint16_t)(EEPROM254_MAGIC>>16));
  FLASH_Lock();  
    
  return success && EEPROM254_MAGIC == GET_WORD(base);
}

static bool erasePages() {
  return erasePage(EEPROM_PAGE0_BASE) && erasePage(EEPROM_PAGE1_BASE);
}

static bool writeByte(uint32_t address, uint8_t value) {
  uint16_t halfWord;
  
  if (address & 1) {
    address--;
    halfWord = GET_BYTE(address) | ( (uint16_t)value << 8);
  }
  else {
    halfWord = value | ( (uint16_t)GET_BYTE(address+1) << 8);
  }

  FLASH_Unlock();
  boolean success = FLASH_COMPLETE == FLASH_ProgramHalfWord(address, halfWord);
  FLASH_Lock();  

  return success;
}

uint8_t EEPROM254_getValue(void) {
  if (invalid)
    return 0xFF;
    
  if (lastBase == 0) {
    findValidValueAddress();
    if (lastBase == 0) 
      return 0xFF;
  }

#ifdef SERIAL_DEBUG
  Serial.println("Found value at "+String(lastBase,HEX)+" "+String(lastOffset,HEX)+": "+String(GET_BYTE(lastBase+lastOffset),HEX));
#endif

  return GET_BYTE(lastBase+lastOffset);
}

bool EEPROM254_storeValue(uint8_t value) {
#ifdef SERIAL_DEBUG
  Serial.println("Storage "+String(value,HEX));
#endif
  if (invalid) 
    return false;
  
  if (lastBase == 0) {
#ifdef SERIAL_DEBUG
    Serial.println("Searching...");
#endif    
    findValidValueAddress();
    if (invalid)
      return false;
    if (lastBase == 0) {
#ifdef SERIAL_DEBUG
      Serial.println("Not found...");
#endif    
      lastBase = EEPROM_PAGE0_BASE;
      lastOffset = 4;
    }
  }

  uint8_t cur = GET_BYTE(lastBase+lastOffset);
  if (cur == value)
    return true;

  if (cur != 0xFF) {
    lastOffset++;
    
    if (lastOffset >= EEPROM_PAGE_SIZE) {
      if (lastBase == EEPROM_PAGE0_BASE) {
        lastBase = EEPROM_PAGE1_BASE;
        lastOffset = 4;
      }
      else {
        // on page 1, and out of space
        if (!erasePages()) {
            lastBase = 0;
            lastOffset = 0;
            invalid = true;
            return false;
        }
        lastBase = EEPROM_PAGE0_BASE;
        lastOffset = 4;
      }
    }
  }

#ifdef SERIAL_DEBUG
  Serial.println("Writing "+String(lastBase,HEX)+" "+String(lastOffset,HEX)+" "+String(value,HEX));
#endif
  return writeByte(lastBase+lastOffset, value);
}

static void EEPROM254_reset(void) {
  if (erasePage(EEPROM_PAGE0_BASE) && erasePage(EEPROM_PAGE1_BASE)) {
    lastBase = EEPROM_PAGE0_BASE;
    lastOffset = 4;
    invalid = false;
  }
  else {
    lastBase = 0;
    invalid = true;
  }
}

static void EEPROM254_init(void) {
#ifdef SERIAL_DEBUG  
  Serial.println("eeprom254 init");
#endif
  if (EEPROM254_MAGIC != GET_WORD(EEPROM_PAGE0_BASE) && ! erasePage(EEPROM_PAGE0_BASE) ) {
    lastBase = 0;
    invalid = true;
    return;
  }
  if (EEPROM254_MAGIC != GET_WORD(EEPROM_PAGE1_BASE) && ! erasePage(EEPROM_PAGE1_BASE) ) {
    lastBase = 0;
    invalid = true;
    return;
  }
  invalid = false;
}

