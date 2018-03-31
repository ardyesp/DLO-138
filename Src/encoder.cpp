/* Rotary encoder methods
	CW        CCW
	01        00
	11        10
	10        11
	00        01
*/

#include "encoder.hpp"
#include "display.hpp"
#include "interface.hpp"
#include "zconfig.hpp"
#include "variables.h"
#include "global.h"
#include "awrap.hpp"
#include "capture.hpp"


#define ENC_BUTTON    0
#define VDIV_BUTTON   1
#define SECDIV_BUTTON 2
#define TRIG_BUTTON   3
#define OK_BUTTON     4


extern t_config config;
extern uint8_t currentFocus;
extern volatile bool hold;
extern volatile bool keepSampling;
extern volatile bool holdSampling;

volatile uint32_t pressedTime[5] = {0,0,0,0,0};
uint16_t pin[5] = {DB3_Pin,DB4_Pin,DB5_Pin,DB6_Pin,DB7_Pin};
GPIO_TypeDef* port[5] ={DB3_GPIO_Port,DB4_GPIO_Port,DB5_GPIO_Port,DB6_GPIO_Port,DB7_GPIO_Port};
volatile uint8_t pos[5] = {0,0,0,0,0};
volatile bool pressed[5] ={false,false,false,false,false};
bool requestHold = false;

int encoderVal = 0;

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

//There's some funkyness going on with variables in the IRQ routine.
//Seems variable need to be declared somewhere else and then linked in va 'extern'....

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	switch(GPIO_Pin)
	{
	case DB0_Pin:
	case DB1_Pin:
		keepSampling = false;
		readEncoderISR();
		break;
	case DB3_Pin:
		keepSampling = false;
		break;
	case DB4_Pin:
		keepSampling = false;
		break;
	case DB5_Pin:
		keepSampling = false;
		break;
	case DB6_Pin:
		keepSampling = false;
		break;
	case DB7_Pin:
		if (hold == false)
			holdSampling = true;
		break;
	}
}


// ------------------------
// called by ISR
int getEncoderSteps()
// ------------------------
{
	static int encoderPrevVal = 0;
	int newCount = encoderVal - encoderPrevVal;

	// 2 ticks make one step in rotary encoder
	int numSteps = newCount / 2;
	int remainder = newCount % 2;

	encoderPrevVal = encoderVal - remainder;
	return numSteps;
}


// ------------------------
// ISR
void readEncoderISR()
// ------------------------
  {
	static uint8_t lastPos = 0b00;

	uint8_t aNow = HAL_GPIO_ReadPin(DB0_GPIO_Port, DB0_Pin);
	uint8_t bNow = HAL_GPIO_ReadPin(DB1_GPIO_Port, DB1_Pin);


	uint8_t posNow = aNow << 1 | bNow;

	if((lastPos == 0b01) && (posNow == 0b11))
		encoderVal++;
	else if((lastPos == 0b11) && (posNow == 0b10))
		encoderVal++;
	else if((lastPos == 0b10) && (posNow == 0b00))
		encoderVal++;
	else if((lastPos == 0b00) && (posNow == 0b01))
		encoderVal++;
	else if((lastPos == 0b00) && (posNow == 0b10))
		encoderVal--;
	else if((lastPos == 0b10) && (posNow == 0b11))
		encoderVal--;
	else if((lastPos == 0b11) && (posNow == 0b01))
		encoderVal--;
	else if((lastPos == 0b01) && (posNow == 0b00))
		encoderVal--;

	lastPos = posNow;
}



//Poll the control switches on DSO-150
bool   pollControlSwitches(void)
{
  bool change = false;
  static int lastEncVal = 0;
  uint32_t ii=0;

//Process Encoder
//--------------
  if (lastEncVal != encoderVal)
  {
	// convert the encoder reading into rounded steps
	int steps = getEncoderSteps();

	if(steps != 0)
    {
		// take action
		encoderChanged(steps);
	}
	lastEncVal = encoderVal;
  }


  //Read Buttons
  //------------

    for(ii=0;ii<5;ii++)
    {
      pos[ii] = 0;
      // btn pressed or released?
      if(!pressed[ii] && (HAL_GPIO_ReadPin(port[ii], pin[ii]) == GPIO_PIN_RESET))
      {
        // debounce
        if(millis() - pressedTime[ii] < BTN_DEBOUNCE_TIME)
          continue;
        pressedTime[ii] = millis();
        pressed[ii] = true;
      }

      if(pressed[ii] && (HAL_GPIO_ReadPin(port[ii], pin[ii]) == GPIO_PIN_SET))
      {
        // debounce
        if(millis() - pressedTime[ii] < BTN_DEBOUNCE_TIME)
          continue;

        pressed[ii] = false;

        // is it a short press
        if(millis() - pressedTime[ii] < BTN_LONG_PRESS)
        {
          pos[ii]=SHORTPRESS;
        }
        else
        {
          // long press reset parameter to default
          //resetParam();
          pos[ii]=LONGPRESS;
        }
      }
    }

  //Handle Buttons...
  //-----------------

  //Knob Button
  if (pos[ENC_BUTTON] == SHORTPRESS)
  {
     // select different parameters to change
     focusNextLabel();
  
     // request repainting of screen labels
     repaintLabels();
       
     // break the sampling loop
     if(config.triggerType != TRIGGER_AUTO)
    		stopSampling();

    change=true;
    DBG_PRINT("ENC SHORT");
  }  
  else if (pos[ENC_BUTTON] == LONGPRESS)
  {
    // long press reset parameter to default
    resetParam();
    change=true;    
    DBG_PRINT("ENC LONG");
  }

  //V/Div Button
  if (pos[VDIV_BUTTON] == SHORTPRESS)
  {      
    if (currentFocus == L_voltagerange)
      nextVoltageRange();
    else       
      setFocusLabel(L_voltagerange);
    change=true;   
    DBG_PRINT("VDIV SHORT");
  }
  else if (pos[VDIV_BUTTON] == LONGPRESS)
  {
      // toggle stats printing
	  config.printVoltage = !config.printVoltage;
      if (config.printVoltage)
    	  config.printStats = false;
      DBG_PRINT("VDIV LONG");
  }
  
//Sec/Div Button
  if (pos[SECDIV_BUTTON] == SHORTPRESS)
  {      
    if (currentFocus == L_timebase)
      nextTimeBase();
    else   
      setFocusLabel(L_timebase);
    change=true;
    DBG_PRINT("SECDIV SHORT");
  }
  else if (pos[SECDIV_BUTTON] == LONGPRESS)
  {
      DBG_PRINT("SECDIV LONG");
  }

//Trigger Button
  if (pos[TRIG_BUTTON] == SHORTPRESS)
  {   
    if (currentFocus == L_triggerLevel)    
      nextTT();
    else
      setFocusLabel(L_triggerLevel);
    change=true;
    DBG_PRINT("TRIG SHORT");
  }
  else if (pos[TRIG_BUTTON] == LONGPRESS)
  {   
    togglePersistence();
    DBG_PRINT("TRIG LONG");
  }

//OK Button
  if((holdSampling) && (hold == false))
  {
	    hold = true;
	    repaintLabels();
	    change=true;
	    DBG_PRINT("OIRQ REQ HOLD");
  }
  else if (pos[OK_BUTTON] == SHORTPRESS)
  {      // toggle hold
	if (holdSampling)
	{
		holdSampling = false;
	}
	else
	{
		hold = !hold;
		repaintLabels();
		change=true;
		DBG_PRINT("OK SHORT");
	}
  }
  else if (pos[OK_BUTTON] == LONGPRESS)
  {
      // toggle stats printing
	  config.printStats = !config.printStats;
      if (config.printStats)
    	  config.printVoltage = false;
      DBG_PRINT("OK LONG");
  }
  return change;
}
