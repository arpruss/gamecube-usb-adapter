#include <EEPROM.h>
#include <flash_stm32.h>

#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE 0x400
#endif

#ifndef EEPROM_START_ADDRESS  
#define EEPROM_START_ADDRESS ((uint32)(0x8000000 + 128 * 1024 - 2 * EEPROM_PAGE_SIZE))
#endif 

#ifndef EEPROM_PAGE0_BASE
#define EEPROM_PAGE0_BASE   ((uint32)(EEPROM_START_ADDRESS + 0x000))
#endif

#ifndef EEPROM_PAGE1_BASE
#define EEPROM_PAGE1_BASE   ((uint32)(EEPROM_START_ADDRESS + EEPROM_PAGE_SIZE))
#endif

// This is a module that lets you store up to 255 byte-long configuration variables numbered from 0 to 254.
//
// The storage method is incompatible with the one implemented by the EEPROM-emulation library, as it's optimized
// for our single-byte usage case. 
//
// Note: Flash storage is rated for 10K cycles.
// There are 5020 half-words (16-bit entities) available per page, making for about 10M cycles.
// Worst case scenario (and very unrealistic) is when GameCubeController's configuration changes every 15 seconds.
// This will result in 4.25 years of constant operation. 
// That should be good enough.

static boolean invalid = true;
static uint8_t currentPage = 0;

#define EEPROM8_MEMORY_SIZE (2*EEPROM_PAGE_SIZE)
#define GET_BYTE(address) (*(__IO uint8_t*)(address))
#define GET_HALF_WORD(address) (*(__IO uint16_t*)(address))
#define GET_WORD(address) (*(__IO uint32_t*)(address))

#define EEPROM8_MAGIC (uint32_t)0x1b70f1cd

const uint32_t pageBases[2] = { EEPROM_PAGE0_BASE, EEPROM_PAGE1_BASE };

static bool erasePage(uint32_t base) {
  bool success;

  if (base != EEPROM_PAGE0_BASE && base != EEPROM_PAGE1_BASE) {
    return false;
  }

  FLASH_Unlock();
  
  success = ( FLASH_COMPLETE == FLASH_ErasePage(base) );
  
  success = success && 
    FLASH_COMPLETE == FLASH_ProgramHalfWord(base, (uint16_t)EEPROM8_MAGIC) &&
    FLASH_COMPLETE == FLASH_ProgramHalfWord(base+2, (uint16_t)(EEPROM8_MAGIC>>16));
  FLASH_Lock();  
    
  return success && EEPROM8_MAGIC == GET_WORD(base);
}

/*
static bool erasePages() {
  return erasePage(EEPROM_PAGE0_BASE) && erasePage(EEPROM_PAGE1_BASE);
}
*/

int EEPROM8_getValue(uint8_t variable) {
  if (invalid)
    return -1;

  uint32_t base = pageBases[currentPage];
  for (uint32_t offset = EEPROM_PAGE_SIZE-2 ; offset >= 4 ; offset-=2) {
    if (GET_BYTE(base+offset) == variable) {
      return GET_BYTE(base+offset+1);
    }
  }

  return -1;
}

static bool writeHalfWord(uint32_t address, uint16_t halfWord) {
  if (! ( EEPROM_PAGE0_BASE <= address && address+1 < EEPROM_PAGE0_BASE + EEPROM_PAGE_SIZE ) &&
     ! ( EEPROM_PAGE1_BASE <= address && address+1 < EEPROM_PAGE1_BASE + EEPROM_PAGE_SIZE ) ) {
    return false;
  }
  
  FLASH_Unlock();
  boolean success = FLASH_COMPLETE == FLASH_ProgramHalfWord(address, halfWord);
  FLASH_Lock();  

  return success && GET_HALF_WORD(address) == halfWord;
}

boolean EEPROM8_storeValue(uint8_t variable, uint8_t value) {
  if (invalid)
    return false;

  if ((uint16_t)value == (uint16_t)EEPROM8_getValue(variable))
    return true;
  
  uint32_t base = pageBases[currentPage];
  bool err = false;
  
  for (uint32_t offset = 4 ; offset < EEPROM_PAGE_SIZE ; offset+=2) {
    if (GET_HALF_WORD(base+offset) == 0xFFFF) {
      return writeHalfWord(base+offset, variable | ((uint16_t)value<<8));
    }
  }

  uint32_t otherBase = pageBases[1-currentPage];
  // page is full, need to move to other page
  if (! erasePage(otherBase)) {
    return false;
  }

  // now, copy data
  uint32_t outOffset = 4;

  for (uint32_t offset = EEPROM_PAGE_SIZE; offset >= 4 ; offset-=2) {
    uint16_t data;
    
    if (offset == EEPROM_PAGE_SIZE)
      data = variable | ((uint16_t)value<<8); // give new data value priority
    else
      data = GET_HALF_WORD(base+offset);
      
    if (data != 0xFFFF) {
      uint8_t variable = (uint8_t)data;
      uint32_t j;
      for (j = 4 ; j < outOffset ; j+=2) {
        if (GET_BYTE(otherBase+j) == variable) 
          break;
      }
      if (j == outOffset) {
        // we don't yet have a value for this variable
        if (writeHalfWord(otherBase+outOffset,data))
          outOffset += 2;
        else
          err = true;
      }
    }
  }

  if (!erasePage(pageBases[currentPage]))
    err = true;
  currentPage = 1-currentPage;

  return !err;
}


static void EEPROM8_reset(void) {
  if (erasePage(EEPROM_PAGE0_BASE) && erasePage(EEPROM_PAGE1_BASE)) {
    currentPage = 0;
    invalid = false;
  }
  else {
    invalid = true;
  }
}

void EEPROM8_init(void) {
  if (EEPROM8_MAGIC != GET_WORD(EEPROM_PAGE0_BASE) && ! erasePage(EEPROM_PAGE0_BASE) ) {
    invalid = true;
    return;
  }
  if (EEPROM8_MAGIC != GET_WORD(EEPROM_PAGE1_BASE) && ! erasePage(EEPROM_PAGE1_BASE) ) {
    invalid = true;
    return;
  }
  if (GET_HALF_WORD(EEPROM_PAGE0_BASE+4) != 0xFFFF) {
    currentPage = 0;
  }
  else if (GET_HALF_WORD(EEPROM_PAGE1_BASE+4) != 0xFFFF) {
    currentPage = 1;
  }
  else { // both pages are blank
    currentPage = 1;
  }
  invalid = false;
}

