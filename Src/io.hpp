#ifndef IO_H
#define IO_H

#include <stdint.h>


void initIO();
void setTestPin(uint8_t mode);
void blinkLED();
void initScanTimeout();
int16_t getTriggerLevel();
void setTriggerLevel(int16_t tLvl);
void setVRange(uint8_t voltageRange);
void readInpSwitches();


#endif
