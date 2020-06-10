#include "io.hpp"
#include "display.hpp"
#include "variables.h"
#include "awrap.hpp"
#include "encoder.hpp"
#include "capture.hpp"
#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim3;  //PWM Test Output
extern TIM_HandleTypeDef htim2;  //TIMER ScanTimeout

extern ADC_HandleTypeDef hadc1;  //Analog ADC
extern ADC_HandleTypeDef hadc2;  //Switch ADC

extern t_config config;

uint8_t couplingPos;
volatile uint8_t ledState = 0;


// ------------------------
void initIO()
// ------------------------
{
  //Painful manual Setup for the External interrupt assignments

  //Setup Encoder IRQ
  //---------------
  //Set Pin B0,B1,B2,B4,B5,B6,B7 to trigger IRQ on Rising/Falling Edge...
  SET_BIT(EXTI->RTSR, 0x0003);
  SET_BIT(EXTI->FTSR, 0x00FB);
  //Setup EXTI0,EXTI1,EXTI3,EXTI4,EXTI5_9 to map to GPIOB
  SET_BIT(AFIO->EXTICR[0], 0x1011);
  SET_BIT(AFIO->EXTICR[1], 0x1111);
  //Enable EXTI interrupts
  SET_BIT(EXTI->IMR, 0x0FB);

  //Set Encoder Priorities
  HAL_NVIC_SetPriority(EXTI0_IRQn,0x08, 0);
  HAL_NVIC_SetPriority(EXTI1_IRQn,0x08, 0);
  HAL_NVIC_SetPriority(EXTI3_IRQn,0x08, 0);
  HAL_NVIC_SetPriority(EXTI4_IRQn,0x08, 0);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn,0x08, 0);

   //Set Trigger Priorities
  HAL_NVIC_SetPriority(EXTI15_10_IRQn,0x06, 0);
  HAL_NVIC_SetPriority(ADC1_2_IRQn,0x06, 0);

  //Set Timer Priority
  HAL_NVIC_SetPriority(TIM2_IRQn,0x0A, 0);

  //Enable NVIC Interrupts
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  blinkLED();
  HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
	
  // init scan timeout timer
  initScanTimeout();
  HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);

  //Start ADC Conversion?
  HAL_ADC_Start(&hadc1);
  HAL_ADC_Start(&hadc2);

  // start 1KHz square wave
  setTestPin(TESTMODE_PULSE);
  DBG_PRINT((char *)"Test square wave started\n");
}


void setTestPin(uint8_t mode)
{
  switch (mode)
  {
      case TESTMODE_GND:
    	  setPinMode(AMPSEL_GPIO_Port,AMPSEL_Pin,GPIO_MODE_ANALOG,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
    	  setPinMode(TESTSIG_GPIO_Port,TESTSIG_Pin,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
          HAL_GPIO_WritePin(TESTSIG_GPIO_Port, TESTSIG_Pin, GPIO_PIN_RESET);
      break;         
      case TESTMODE_3V3:
    	  setPinMode(AMPSEL_GPIO_Port,AMPSEL_Pin,GPIO_MODE_INPUT,GPIO_PULLUP,GPIO_SPEED_FREQ_LOW);
    	  setPinMode(TESTSIG_GPIO_Port,TESTSIG_Pin,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
          HAL_GPIO_WritePin(TESTSIG_GPIO_Port, TESTSIG_Pin, GPIO_PIN_SET);
      break;  
      case TESTMODE_0V1:
    	  setPinMode(AMPSEL_GPIO_Port,AMPSEL_Pin,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
          HAL_GPIO_WritePin(AMPSEL_GPIO_Port, AMPSEL_Pin, GPIO_PIN_RESET);
    	  setPinMode(TESTSIG_GPIO_Port,TESTSIG_Pin,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
          HAL_GPIO_WritePin(TESTSIG_GPIO_Port, TESTSIG_Pin, GPIO_PIN_SET);
      break;              
      case TESTMODE_PULSE:
          // start 1KHz square wave
          setPinMode(TESTSIG_GPIO_Port,TESTSIG_Pin,GPIO_MODE_AF_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
#ifdef OVERCLOCK
          timerSetPeriod(&htim3,500);  //TODO Why?????
          timerSetPWM(&htim3,TIM_CHANNEL_2,28015);
#else
          timerSetPeriod(&htim3,1000);
          timerSetPWM(&htim3,TIM_CHANNEL_2,17850);
#endif
      break;
  }  
}


// ------------------------
void blinkLED()
// ------------------------
{
  ledState = !ledState;
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, (GPIO_PinState)ledState);
}


// ------------------------
void initScanTimeout()
// ------------------------
{
	timerPause(&htim2);
	timerSetCompare(&htim2,TIM_CHANNEL_1,1);
}


// ------------------------
int16_t getTriggerLevel()
// ------------------------
{
	return config.trigLevel;
}


// ------------------------
void setTriggerLevel(int16_t tLvl)	
// ------------------------
{
	// 600 = 20% duty
	// 1800 = 50%
	if (tLvl > 2047)
		tLvl = 2047;
	if (tLvl < -2047)
		tLvl = -2047;
	config.trigLevel = tLvl;
	 set_trigger_level(config.trigLevel);
}

// ------------------------
void setVRange(uint8_t voltageRange) 
// ------------------------
{
  unsigned char val = tbBitval[voltageRange];

  config.zeroVoltageA1 = config.zeroVoltageA1Cal[voltageRange];
  
  HAL_GPIO_WritePin(SENSEL0_GPIO_Port, SENSEL0_Pin, (GPIO_PinState)((val>>0) & 0x01));
  HAL_GPIO_WritePin(SENSEL1_GPIO_Port, SENSEL1_Pin, (GPIO_PinState)((val>>1) & 0x01));
  HAL_GPIO_WritePin(SENSEL2_GPIO_Port, SENSEL2_Pin, (GPIO_PinState)((val>>2) & 0x01));
  HAL_GPIO_WritePin(SENSEL3_GPIO_Port, SENSEL3_Pin, (GPIO_PinState)((val>>3) & 0x01));
}

// ------------------------
void readInpSwitches()
// ------------------------
{
	static uint8_t couplingOld;
	uint16_t cpl;

	cpl = (uint16_t)HAL_ADC_GetValue(&hadc2);

	if(cpl < 1000)
		couplingPos = CPL_GND;
	else if(cpl > (4095-1000))
		couplingPos = CPL_AC;
	else
		couplingPos = CPL_DC;


	// check if switch position changed from previous snap
	if(couplingPos != couplingOld)	
	{
		couplingOld = couplingPos;
		repaintLabels();
	}
}

