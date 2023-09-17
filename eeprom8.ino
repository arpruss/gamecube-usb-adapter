#include <EEPROM.h>
#include <flash_stm32.h>

#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE 0x400
#endif

// This is a module that lets you store up to 255 byte-long configuration variables numbered from 0 to 254 in a single
// flash page.
//
// The storage method is incompatible with the one implemented by the EEPROM-emulation library, as it's optimized
// for our single-byte usage case. 

uint32_t pageBase;
uint8 storage[255];

static boolean invalid = true;

#define EEPROM8_MEMORY_SIZE (2*EEPROM_PAGE_SIZE)
#define GET_BYTE(address) (*(__IO uint8_t*)(address))
#define GET_HALF_WORD(address) (*(__IO uint16_t*)(address))
#define GET_WORD(address) (*(__IO uint32_t*)(address))

#define EEPROM8_MAGIC (uint32_t)0x1b70f1ce

static bool erasePage(uint32_t base) {
  bool success;

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
  return erasePage(pageBase);
}
*/

uint8 EEPROM8_getValue(uint8_t variable) {
    if (variable>=255)
        return 0;
    else
        return storage[variable];
}

static bool writeHalfWord(uint32_t address, uint16_t halfWord) {
  if (!(pageBase <= address && address+1 < pageBase + EEPROM_PAGE_SIZE ))
    return false;
  
  FLASH_Unlock();
  boolean success = FLASH_COMPLETE == FLASH_ProgramHalfWord(address, halfWord);
  FLASH_Lock();  

  return success && GET_HALF_WORD(address) == halfWord;
}

boolean EEPROM8_storeValue(uint8_t variable, uint8_t value) {
  if (invalid || variable >= 255)
    return false;
    
  if (storage[variable] == value)
    return true;
    
  storage[variable] = value;
    
  for (uint32_t offset = 4 ; offset < EEPROM_PAGE_SIZE ; offset+=2) {
    if (GET_HALF_WORD(pageBase+offset) == 0xFFFF) {
      return writeHalfWord(pageBase+offset, variable | ((uint16_t)value<<8));
    }
  }

  // page is full
  if (!erasePage(pageBase))
    return false;
  uint32 offset = 4;
  bool err = false;  
  for (uint32_t variable = 0 ; variable < 255; variable++) 
    if (0 != storage[variable]) {
        if (!writeHalfWord(pageBase+offset, variable | ((uint16_t)storage[variable]<<8))) {
            err = true;
            offset += 2;
        }
        else {
            offset += 2;
        }
    }

  return !err;
}


static void EEPROM8_reset(void) {
  if (erasePage(pageBase)) {
    invalid = false;
  }
  else {
    invalid = true;
  }
  for(uint32_t i=0; i<255; i++)
    storage[i] = 0;
}


void EEPROM8_init(void) {
  uint32_t flashSize = *(uint16 *) (0x1FFFF7E0);
  pageBase = 0x8000000 + flashSize * 1024 - EEPROM_PAGE_SIZE;

  for(uint32_t i=0; i<255; i++)
    storage[i] = 0;
  if (EEPROM8_MAGIC != GET_WORD(pageBase) && ! erasePage(pageBase) ) {
    invalid = true;
    return;
  }
  for (uint32_t offset = 4 ; offset < EEPROM_PAGE_SIZE ; offset+=2) {
    uint8_t i = GET_BYTE(pageBase+offset);
    if (i < 255)
        storage[i] = GET_BYTE(pageBase+offset+1);
  }
  invalid = false;
}

