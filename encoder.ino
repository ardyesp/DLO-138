/* Rotary encoder methods
	CW        CCW
	01        00
	11        10
	10        11
	00        01
*/

int encoderVal = 0;


// ------------------------
// called by ISR
int getEncoderSteps()	{
// ------------------------
	static int encoderPrevVal = 0;
	int newCount = encoderVal - encoderPrevVal;

	// 4 ticks make one step in rotary encoder
	int numSteps = newCount / 4;
	int remainder = newCount % 4;

	encoderPrevVal = encoderVal - remainder;
	return numSteps;

}



// ------------------------
// ISR
void readEncoderISR()	{
// ------------------------

	static byte lastPos = 0b00;

	byte aNow = digitalRead(ENCODER_A);
	byte bNow = digitalRead(ENCODER_B);

	byte posNow = aNow << 1 | bNow;

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

	// convert the encoder reading into rounded steps
	int steps = getEncoderSteps();
  
	if(steps != 0)	{
		// take action
		encoderChanged(steps);
	}
}

long lastABPress = 0;


// ------------------------
// ISR
void readASwitchISR()	{
// ------------------------
	if(millis() - lastABPress < BTN_DEBOUNCE_TIME)
		return;
	lastABPress = millis();

	encoderChanged(-1);
}



// ------------------------
// ISR
void readBSwitchISR()	{
// ------------------------
	if(millis() - lastABPress < BTN_DEBOUNCE_TIME)
		return;
	lastABPress = millis();
	
	encoderChanged(1);
}

//Poll the control switches on DSO-150
//Nasty, as these are shared with the display lines..
//Interrupts would be nicer especially for the encoder as
//it is very likely that we will miss some rational changes...

boolean   pollControlSwitches(void)
{
  static boolean pressed[5] ={false,false,false,false,false};
  static long pressedTime[5] = {0,0,0,0,0};
  static uint16_t pin[5] = {ENCODER_SW,BTN1,BTN2,BTN3,BTN4};
  static byte pos[5] = {0,0,0,0,0};
  static byte lastPos = 0b00;  
  int ii=0;
  static int lastEncPos = 0;
  boolean change = false;

//Set Lines to Read
setReadDir();

/*
//Read Encoder
  byte aNow = digitalRead(ENCODER_A);
  byte bNow = digitalRead(ENCODER_B);

  byte posNow = aNow << 1 | bNow;

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

  // convert the encoder reading into rounded steps
  //int steps = getEncoderSteps();
  int steps = lastEncPos - encoderVal;
  lastEncPos = encoderVal;
  
  if(steps != 0)  
  {
    // take action
    encoderChanged(steps);
    change=true;
  }
*/
//Read Buttons
//------------

  for(ii=0;ii<5;ii++)
  {
    pos[ii] = 0; 
    // btn pressed or released?
    if(!pressed[ii] && (digitalRead(pin[ii]) == LOW))  
    {
      // debounce
      if(millis() - pressedTime[ii] < BTN_DEBOUNCE_TIME)
        continue;
      pressedTime[ii] = millis();
      pressed[ii] = true;
    }
    
    if(pressed[ii] && (digitalRead(pin[ii]) == HIGH))  
    {
      // debounce
      if(millis() - pressedTime[ii] < BTN_DEBOUNCE_TIME)
        continue;
      
      pressed[ii] = false;
        
      // is it a short press
      if(millis() - pressedTime[ii] < BTN_LONG_PRESS) 
      {
        pos[ii]=1;  
      }
      else  
      {
        // long press reset parameter to default
        //resetParam();
        pos[ii]=2;
      }
    }
  }

//Debug only
/*
  DBG_PRINT("Enc:");
  DBG_PRINT(steps);
  DBG_PRINT(" Val:");
  DBG_PRINT(encoderVal);
  DBG_PRINT(" E1:");
  DBG_PRINT(digitalRead(ENCODER_A));  
  DBG_PRINT(" E2:");
  DBG_PRINTLN(digitalRead(ENCODER_B));   
 */

  //Restore lines to write
  setWriteDir();


  //Handle Buttons...

  if (pos[4] == 1)
  {      // toggle hold
    hold = !hold;
    repaintLabels();
    change=true;    
  }
  if (pos[4] == 2)
  {
    // long press reset parameter to default
    resetParam();
    change=true;    
  }

  if (pos[0] == 1)
  {
     // select different parameters to change
     focusNextLabel();
  
     // request repainting of screen labels
     repaintLabels();
  
     // manually update display if frozen
     if(hold)
       drawWaves();
       
     // break the sampling loop
     if(triggerType != TRIGGER_AUTO)
       keepSampling = false;

    change=true;       
  }  

  if (pos[1] == 1)
  {      
    setFocusLabel(L_voltagerange);
    change=true;   
  }

  if (pos[2] == 1)
  {      
    setFocusLabel(L_timebase);
    change=true;      
  }

  if (pos[3] == 1)
  {      
    setFocusLabel(L_triggerLevel);
    change=true;      
  }

  return change;
}

