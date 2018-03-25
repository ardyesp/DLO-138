#ifndef ZCONFIG_H
#define ZCONFIG_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define PREAMBLE_VALUE	2859

void autoSafe(void);
void loadConfig(bool reset);
void loadDefaults();
void formatSaveConfig();
void printConfig(void);

HAL_StatusTypeDef readEEPROM(uint8_t i2cAddr,uint16_t address, uint8_t* MemTarget,uint16_t Size);
HAL_StatusTypeDef writeEEPROM(uint8_t i2cAddr,uint16_t address, uint8_t* MemTarget,uint16_t Size);


void loadWaveform();
void saveWaveform();


#endif
