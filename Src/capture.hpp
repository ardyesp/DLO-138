#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdbool.h>


void setTriggerSourceAndDir(uint8_t source,uint8_t dir);
void sampleWaves(bool wTimeout);
void startScanTimeout(int16_t mSec);
void triggerISR(void);
void scanTimeoutISR(void);
void startSampling(int16_t lDelay);
void stopSampling(void);

void disable_trigger(void);
void arm_trigger(bool lohi,bool hilo);
void set_trigger_level(int16_t level);

void snapMicros();
void dumpSamples();
void printSample(uint16_t k, float timeStamp);

void loopThroughRange(uint8_t rangeStart,uint8_t rangecount,uint16_t *pResults);
void triggerTestISR(void);
void autoCal(void);
void getCalibSamples(uint8_t range,uint16_t *pResults);
#endif
