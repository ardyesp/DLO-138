
#ifndef _AWRAP_H_
#define _AWRAP_H_

#include "stm32f1xx_hal.h"

#define DEBUG

#define DBG_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__);

extern void delayMS(unsigned long ms);
extern void delayUS(uint32_t us);
void setPinMode(GPIO_TypeDef  *GPIOx,uint32_t Pin,uint32_t Mode, uint32_t Pull,uint32_t Speed);
extern long millis(void);
extern long micros(void);

extern void timerPause(TIM_HandleTypeDef *htim);
extern void timerResume(TIM_HandleTypeDef *htim);
extern void timerSetPeriod(TIM_HandleTypeDef *htim,uint32_t ms_period);
extern void  timerSetPWM(TIM_HandleTypeDef *htim,uint32_t Channel,uint16_t perc);
extern void timerSetCompare(TIM_HandleTypeDef *htim,uint32_t Channel,uint32_t val);
extern void timerSetCount(TIM_HandleTypeDef *htim,uint32_t count);
#endif
