#include <stdint.h>
#include "variables.h"
#include "capture.hpp"
#include "awrap.hpp"
#include "zconfig.hpp"
#include "interface.hpp"
#include "display.hpp"
#include "io.hpp"
#include "Adafruit_GFX/Adafruit_GFX.hpp"
#include <string.h>

extern "C" void HAL_GPIO_TRIG_IRQHandler(uint16_t GPIO_Pin);

extern TIM_HandleTypeDef htim2;  //TIMER ScanTimeout
extern ADC_HandleTypeDef hadc1;  //Analog ADC

extern const char* cplNames[];
extern uint8_t couplingPos;
extern const char* rngNames[];
extern const char* tbNames[];

extern volatile bool hold;

extern t_config config;
extern t_Stats wStats;
extern uint8_t rangePos;
extern uint8_t xZoom;

uint16_t ch1Capture[NUM_SAMPLES] = {0};
uint16_t bitStore[NUM_SAMPLES] = {0};

volatile bool keepSampling = true;

bool minSamplesAcquired;
long prevTime = 0;
int16_t sDly, tDly;
uint16_t tIndex = 0;
uint16_t sIndex = 0;
volatile bool triggered = false;
volatile bool hold = false;
long samplingTime;

bool trigger_lo_hi = false;
bool trigger_hi_lo = false;
uint16_t trigger_level = 2048;


// hold pointer references for updating variables in memory
uint16_t *sIndexPtr = &sIndex;
volatile bool *triggeredPtr = &triggered;

//---------------------
void stopSampling(void)
//---------------------
{
	keepSampling = false;
}


extern "C" void HAL_GPIO_TRIG_IRQHandler(uint16_t GPIO_Pin)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
    triggerISR();
  }
}



void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
	scanTimeoutISR();
}


void setTriggerSourceAndDir(uint8_t source,uint8_t dir)
{
  GPIO_TypeDef* port = GPIOA;
  uint32_t pin = 0;
  uint32_t mode = 0;

  HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
  HAL_NVIC_DisableIRQ(ADC1_2_IRQn);
  // trigger changed, break out from previous sampling loop
  stopSampling();

  //Detach old source
  setPinMode(TRIG_GPIO_Port,TRIG_Pin,GPIO_MODE_INPUT,GPIO_PULLDOWN,GPIO_SPEED_FREQ_LOW);
  setPinMode(D1_GPIO_Port,D1_Pin,GPIO_MODE_INPUT,GPIO_PULLDOWN,GPIO_SPEED_FREQ_LOW);
  setPinMode(D2_GPIO_Port,D2_Pin,GPIO_MODE_INPUT,GPIO_PULLDOWN,GPIO_SPEED_FREQ_LOW);
  setPinMode(D3_GPIO_Port,D3_Pin,GPIO_MODE_INPUT,GPIO_PULLDOWN,GPIO_SPEED_FREQ_LOW);

  //mask all trigger EXTI interrupts
  //CLEAR_BIT(EXTI->IMR, TRIG_Pin);
  //CLEAR_BIT(EXTI->IMR, D1_Pin);
  //CLEAR_BIT(EXTI->IMR, D2_Pin);
 // CLEAR_BIT(EXTI->IMR, D3_Pin);

  switch (dir)
  {
  	  case TRIGGER_RISING:
  		  mode = GPIO_MODE_IT_RISING;
  		  break;
  	  case TRIGGER_FALLING:
  		  mode = GPIO_MODE_IT_FALLING;
  		  break;
  	  case TRIGGER_ALL:
  		  mode = GPIO_MODE_IT_RISING_FALLING;
  		  break;
  }

  switch (source)
  {
  	  case TRIGSRC_A1:
  		  pin = TRIG_Pin;
  	      port = TRIG_GPIO_Port;
  		  break;
  	  case TRIGSRC_D1:
  		  pin = D1_Pin;
  	      port = D1_GPIO_Port;
  		  break;
  	  case TRIGSRC_D2:
  		  pin = D2_Pin;
  	      port = D2_GPIO_Port;
  		  break;
  	  case TRIGSRC_D3:
  		  pin = D3_Pin;
  	      port = D3_GPIO_Port;
  		  break;
  }


  //Store Vars
  config.triggerDir = dir;
  config.triggerSource = source;

  if (source == TRIGSRC_A1)
  {
	  if (dir == TRIGGER_RISING)
		  arm_trigger(true,false);
	  else if (dir == TRIGGER_FALLING)
		  arm_trigger(false,true);
	  else
		  arm_trigger(true,true);
	  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  }
  else
  {
	  setPinMode(port,pin,mode,GPIO_PULLDOWN,GPIO_SPEED_FREQ_LOW);
  }

  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// ------------------------
void sampleWaves(bool wTimeout)
// ------------------------
{
	// setup timed interrupt to terminate scanning if trigger not found
	if(wTimeout)
		startScanTimeout(tDly);

	// start sampling loop - until timeout or trigger
	startSampling(sDly);
	// disable scan timeout timer
    timerPause(&htim2);
}


// local operations below

// ------------------------
void startScanTimeout(int16_t mSec)
// ------------------------
{
	// interrupt triggers at 1
	timerSetCount(&htim2,2);
	timerSetPeriod(&htim2,mSec * 1000);
	timerResume(&htim2);
}


// ------------------------
void triggerISR(void)
// ------------------------
{
	if(!triggered)	{
		// skip this trigger if min samples not acquired
		if(!minSamplesAcquired && (sIndex < NUM_SAMPLES/2))
			return;
		
		// snap the position where trigger occurred
		tIndex = sIndex;
		// avoid multiple triggering
		triggered = true;
	}
} 

// ------------------------
void scanTimeoutISR(void)
// ------------------------
{
	stopSampling();
	// disable scan timeout timer
    timerPause(&htim2);
} 



// ------------------------
void startSampling(int16_t lDelay)
// ------------------------
{
	uint16_t lCtr = 0;
    int16_t cnt = lDelay;

	// clear old dataset
	keepSampling = true;
	minSamplesAcquired = false;
	samplingTime = 0;
	triggered = false;
	sIndex = 0;
	prevTime = micros();

	if(lDelay < 0)  //Delay < 0 (What does that even mean??? As fast as possible?)
	{
        while (keepSampling)
        {
            ch1Capture[sIndex] = hadc1.Instance->DR;
            bitStore[sIndex] = GPIOB->IDR;
            sIndex++;
            if (sIndex == NUM_SAMPLES)
            {
                sIndex = 0;
                snapMicros();
            }
            if (triggered)
            {
                lCtr++;
                if(lCtr == (NUM_SAMPLES/2))   //Why do we only sample half the numbers of samples?
                    return;
            }
        }
	}
	else if(lDelay == 0)  //Delay == 0, wait for ADC
	{
	    while (keepSampling)
        {
            while ((hadc1.Instance->SR & 0x02) == 0)
            {}
            ch1Capture[sIndex] = hadc1.Instance->DR;
            bitStore[sIndex] = GPIOB->IDR;
            sIndex++;
            if (sIndex == NUM_SAMPLES)
            {
                sIndex = 0;
                snapMicros();
            }
            if (triggered)
            {
                lCtr++;
                if(lCtr == (NUM_SAMPLES/2))
                    return;
            }
        }
	}
	else if (lDelay == 0x7FFE) //append to end...
	{
        while (keepSampling)
        {
        	for (lCtr=0;lCtr<(GRID_WIDTH*xZoom);lCtr++)
        	{
        		ch1Capture[lCtr] = ch1Capture[lCtr+1];
        		bitStore[lCtr] = bitStore[lCtr+1];
        	}
            ch1Capture[GRID_WIDTH*xZoom] = hadc1.Instance->DR;
            bitStore[GRID_WIDTH*xZoom] = GPIOB->IDR;
            sIndex = GRID_WIDTH*xZoom;
            return;
        }
	}
	else //Delay > 0, wait for some time inbetween samples
    {
        while (keepSampling)
        {
            ch1Capture[sIndex] = hadc1.Instance->DR;
            bitStore[sIndex] = GPIOB->IDR;
            sIndex++;
            if (sIndex == NUM_SAMPLES)
            {
                sIndex = 0;
                snapMicros();
            }
            if (triggered)
            {
                lCtr++;
                if(lCtr == (NUM_SAMPLES/2))
                    return;
            }
            cnt = lDelay;
            while (cnt)
                cnt--;
        }
    }
}


// ------------------------
inline void snapMicros()	
// ------------------------
{
	samplingTime = micros() - prevTime;
	prevTime = micros();
	minSamplesAcquired = true;
}


// ------------------------
void dumpSamples()
// ------------------------
{
	float timePerSample = ((float)samplingTime) / NUM_SAMPLES;
	printf("Net sampling time [us]: %ld\n",samplingTime);
	printf("Per Sample [us]: %f\n",timePerSample);
	printf("Timebase: %s /div\n",(char *)tbNames[config.currentTimeBase]);
	printf("Actual Timebase [us]: %f\n",timePerSample * 25);
	printf("Coupling: %s\n",(char*)cplNames[couplingPos]);
	printf("Range: %s /div\n",(char*)rngNames[rangePos]);
    printf("ADC Multiplier [mV]: %f\n",config.adcMultiplier[rangePos]);

   

	printf("Triggered: ");
	if(triggered)
	{
		printf("YES\n");
	}
	else
	{
		printf("NO\n");
	}

	// calculate stats on this sample set
	calculateStats();
	
	printf("CH1 Stats");
	if(wStats.mvPos)
	{
		printf(" [mV]:\n");
	}
	else	{
		printf(" [V]:\n");
	}
		
	printf("Vmax: %f \n",wStats.Vmaxf);
	printf("Vmin: %f \n",wStats.Vminf);
	printf("Vavr: %f \n",wStats.Vavrf);
	printf("Vpp: %f \n",wStats.Vmaxf - wStats.Vminf);
	printf("Vrms: %f \n",wStats.Vrmsf);

	if(wStats.pulseValid)
	{
		printf("Freq: %f Hz\n",wStats.freq);
		printf("Cycle: %f ms\n",wStats.cycle);
		printf("PW: %f ms\n",wStats.avgPW/1000.0);
		printf("Duty: %f %%\n",wStats.duty);
	}
	else	{
		printf("Freq: --\n");
		printf(", Cycle: --\n");
		printf(", PW: --\n");
		printf(", Duty: --\n");
	}
	
	printf("\n");

	printf("Count\tTime\tCH1\tD_CH1\tD_CH2\tD_CH3\n");
    
	uint16_t idx = 0;
	
	// sampling stopped at sIndex - 1
	for(uint16_t k = sIndex; k < NUM_SAMPLES; k++)
		printSample(k, timePerSample * idx++);
	
	for(uint16_t k = 0; k < sIndex; k++)
		printSample(k, timePerSample * idx++);
	
	printf("\n");
}


// ------------------------
void printSample(uint16_t k, float timeStamp)
// ------------------------
{
	printf("%d\t",k);
	printf("%f\t",timeStamp);
    printf("%d\t",ch1Capture[k] - config.zeroVoltageA1);
	printf("%d\t",(bitStore[k] & 0x2000) ? 1 : 0);
    printf("%d\t",(bitStore[k] & 0x4000) ? 1 : 0);
	printf("%d\t",(bitStore[k] & 0x8000) ? 1 : 0);
	
	if(triggered && (tIndex == k))
		printf("\t<--TRIG");
	
	printf("\n");
}


void loopThroughRange(uint8_t rangeStart,uint8_t rangecount,uint16_t *pResults)
{
  uint16_t *wave =  ch1Capture;
  int32_t sumSamples = 0;
  for(uint8_t cnt= rangeStart;cnt<rangeStart+rangecount;cnt++)
  {
      //Set Range
      setVoltageRange(cnt);
      //Wait for new Data
      sampleWaves(true);
      delayMS(400);
      //Calc Average
      sumSamples = 0; 
      for(uint16_t k = 0; k < NUM_SAMPLES; k++) 
        sumSamples += wave[k];
      pResults[cnt] = sumSamples/NUM_SAMPLES;
      printf("Range: %s\n",(char*)rngNames[cnt]);
      printf("Sum: %d\n",(int)sumSamples);
      printf("Avg: %d\n",pResults[cnt]);
  }
}

void getCalibSamples(uint8_t range,uint16_t *pResults)
{
  uint16_t *wave =  ch1Capture;
  int32_t sumSamples = 0;

  //Set Range
  setVoltageRange(range);
  //Wait for new Data
  sampleWaves(true);
  delayMS(400);
  //Calc Average
  sumSamples = 0;
  for(uint16_t k = 0; k < NUM_SAMPLES; k++)
	sumSamples += wave[k];
  *pResults = sumSamples/NUM_SAMPLES;
  printf("Range: %s\n",(char*)rngNames[range]);
  printf("Sum: %d\n",(int)sumSamples);
  printf("Avg: %d\n",*pResults);
}

void triggerTestISR(void)
{
  triggered = true;
}

void autoCal(void)
{
    //Textbox Test
    t_tb_data box;
    bool res;
    //We need to store calibration data for each voltage range
    uint16_t zeroOffset[RNG_5mV+1];
    uint8_t ii=0;

    for(ii=0;ii!=RNG_5mV;ii++)
    {
      zeroOffset[ii] = 0;
    }

    //Set Textbox Position
    box.x = 20;
    box.y = 40;
    box.w = 280;
    box.h = 160;


    showtextbox(&box,(char *)"ANALOG INPUT CALIBRATION\n"
    		         "To start calibration set coupling\n"
    				 "to GND.\n"
    		         "\nPress TRIG to cancel or OK\n"
    		         "to continue...");
    res = waitforOKorCancel();

    if(couplingPos != CPL_GND)
    {
      clearWaves();
      return;
    }

    if (res == false)
    {
        clearWaves();
    	return;
    }

	//zero cal
	//Set Trigger to NORM
	setTriggerType(TRIGGER_AUTO);
	//Set timebase to 50uS
	setTimeBase(T50US);
	//Start capturing
	hold = false;

	//Disable Waveform output
	setTestPin(TESTMODE_GND);
	//Capture full range for Zero Cal
	loopThroughRange(RNG_20V,RNG_5mV+1,zeroOffset);
	hold = true;
	delayMS(100);

    //Store Results
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
      config.zeroVoltageA1Cal[ii] = zeroOffset[ii];
     // config.adcMultiplier[ii] = gainfVal[ii];
    }
    formatSaveConfig();

    //Print results
    printf("Calibration Results:\n");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
      printf("Range:%s Offset:%d Gain:%f\n",(char *)rngNames[ii], config.zeroVoltageA1Cal[ii], config.adcMultiplier[ii]);
    }

    //Done..
    showtextbox(&box,(char *)"Calibration completed.\n"
    		         "\nPress OK to continue...");
    res = waitforOKorCancel();
    clearWaves();
    hold=false;
}

void set_trigger_level(int16_t level)
{
	trigger_level = level + 2048;
	if (hold == false)
	{
		if (hadc1.Instance->LTR == 0)
		{
			hadc1.Instance->HTR = trigger_level;
		}
		else if(hadc1.Instance->HTR == 4095)
		{
		  hadc1.Instance->LTR = trigger_level;
		}
	}
}

//Set The trigger level in ADC units. Can be - 2048 / + 2048
void arm_trigger(bool lohi,bool hilo)
{
  ADC_AnalogWDGConfTypeDef ADC_WD_Config;

  trigger_lo_hi = lohi;
  trigger_hi_lo = hilo;

  ADC_WD_Config.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
  ADC_WD_Config.Channel = ADC_CHANNEL_0;
  ADC_WD_Config.ITMode= ENABLE;
  //This is tricky... Hwo do we set the initial conditions? Need to know where the ADC currently is.... Do one ADC read...
  uint16_t ADCval = (uint16_t)HAL_ADC_GetValue(&hadc1);
  if (ADCval > trigger_level)
  {
    ADC_WD_Config.HighThreshold = 4095;
    ADC_WD_Config.LowThreshold = trigger_level;
  }
  else
  {
    ADC_WD_Config.HighThreshold = trigger_level;
    ADC_WD_Config.LowThreshold = 0;
  }
  HAL_ADC_AnalogWDGConfig(&hadc1, &ADC_WD_Config);
}

void disable_trigger(void)
{
  __HAL_ADC_DISABLE_IT(&hadc1, ADC_IT_AWD);
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
  //if low->high transition is enabled
  if (hadc->Instance->LTR == 0)
  {
      hadc->Instance->HTR = 4095;
      hadc->Instance->LTR = trigger_level;
      if (trigger_lo_hi)
        goto trigger_it;
  }
  else if(hadc->Instance->HTR == 4095)
  {
      hadc->Instance->HTR = trigger_level;
      hadc->Instance->LTR = 0;
      if (trigger_hi_lo)
        goto trigger_it;
  }
  return;
  trigger_it:
  if(!triggered)
  {
      // skip this trigger if min samples not acquired
      if(!minSamplesAcquired && (sIndex < NUM_SAMPLES/2))
          return;

      // snap the position where trigger occurred
      tIndex = sIndex;
      // avoid multiple triggering
      triggered = true;
  }
}

/*
void adc_irq(void)
{
	if (triggered)   //After Triggered
	{
		lCtr++;
		if(lCtr == (NUM_SAMPLES/2))  //Fill up half the buffer with samples so Trigger point aligns at middle of sample buffer..
		{
			//Stop sampling by disabling timer?
			//TODO
			return;
		}
	}
	//If sample index rolls over reset to zero and record start sampling time
	if (sIndex == NUM_SAMPLES)
	{
		sIndex = 0;
		snapMicros();
	}

	bitStore[sIndex] = GPIOB->IDR;  //Store GPIO
 	ch1Capture[sIndex] = hadc1.Instance->DR;  //Store ADC (Resets IrQ)
	sIndex++;  //Increase index

}



DMA IRQ
-------
  Arm Threshold IRQ



Setup capture
-------------  
Setup DMA
Setup Capture Timer
Setup Holdeoff Timer
Setup Trigger
Activate Trigger


uint16_t adc_trigger_level = 0;

//Set The trigger level in ADC units. Can be - 2048 / + 2048
void setupTriggerLevel(int16_t level)
{
  adc_trigger_level = level + 2048;

}

*/
