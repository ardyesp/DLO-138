#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdbool.h>
#include "stm32f1xx_hal.h"

//#define   _EEPROM_F1_LOW_DESTINY
#define   _EEPROM_F1_MEDIUM_DESTINY
//#define   _EEPROM_F1_HIGH_DESTINY

#define     _EEPROM_USE_FLASH_PAGE              127

//Support individual Address write. This requires one Page of Ram for temporary storage...
//#define INDIVIDUAL_ADDR_WRITE

//################################################################################################################
bool    EE_LL_Format(uint16_t startpage, uint16_t pages);
bool    EE_LL_Read(uint16_t startpage,uint16_t addr,uint16_t size, uint8_t* Data);
bool    EE_LL_Write(uint16_t startpage,uint16_t addr,uint16_t size, uint8_t* Data);

bool	EE_Format();
bool 	EE_Read(uint16_t VirtualAddress, uint8_t* Data);
#ifdef INDIVIDUAL_ADDR_WRITE
bool 	EE_Write(uint16_t VirtualAddress, uint8_t Data);
#endif

bool	EE_Reads(uint16_t VirtualAddress,uint16_t HowMuchToRead,uint32_t* Data);
bool 	EE_Writes(uint16_t VirtualAddress,uint16_t HowMuchToWrite,uint32_t* Data);

//################################################################################################################

#endif
