
// ------------------------
const char* getTimebaseLabel()	{
// ------------------------
	return tbNames[currentTimeBase];
}

// interface operations defined below
long lastBtnPress = 0;

// ------------------------
void btn4ISR()	{
// ------------------------
	static boolean pressed = false;
	static long pressedTime = 0;

	// btn pressed or released?
	if(!pressed && (digitalRead(BTN4) == LOW))	{
		// debounce
		if(millis() - pressedTime < BTN_DEBOUNCE_TIME)
			return;
		pressedTime = millis();
		pressed = true;
	}
	
	if(pressed && (digitalRead(BTN4) == HIGH))	{
		// debounce
		if(millis() - pressedTime < BTN_DEBOUNCE_TIME)
			return;
		
		pressed = false;
		
		// is it a short press
		if(millis() - pressedTime < BTN_LONG_PRESS)	{
			// toggle hold
			hold = !hold;
			repaintLabels();
		}
		else	{
			// long press reset parameter to default
			resetParam();
		}
	}
}

// ------------------------
void readESwitchISR()	{
// ------------------------
	// debounce
	if(millis() - lastBtnPress < BTN_DEBOUNCE_TIME)
		return;
	lastBtnPress = millis();

	// select different parameters to change
	focusNextLabel();
	
	// request repainting of screen labels
	repaintLabels();
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}


// ------------------------
void resetParam()	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
#ifdef DSO_150
		case L_voltagerange:
			setVoltageRange(RNG_5V);
			break;
#endif      
    case L_triggerLevel:
      // set  level to 0
      setTriggerLevel(0);
      saveParameter(PARAM_TLEVEL, 0);
      repaintLabels();
      break;  
   case L_triggerEdge:
      setTriggerDir(TRIGGER_RISING);
      saveParameter(PARAM_TRIGDIR, triggerDir);
      repaintLabels();
      break;     
		case L_window:
			// set x in the middle
			changeXCursor((NUM_SAMPLES - GRID_WIDTH)/2);
			break;
    case L_triggerSource:
      setTriggerSource(TRIGSRC_A1);
      repaintLabels();
      break;
    case L_zoom: 
      setZoomFactor(ZOOM_X1);
      saveParameter(PARAM_ZOOM, xZoom);
      repaintLabels();  
      break;   
		case L_vPos1:
			// zero the trace base
			calculateTraceZero(0);
			changeYCursor(A1, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos2:
			// zero the trace base
			calculateTraceZero(1);
			changeYCursor(A2, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos3:
      changeDSize(D1);
			break;
		case L_vPos4:
      changeDSize(D2);
			break;
    case L_vPos5:
      changeDSize(D3);
      break; 
    case L_function:
      if (currentFunction==FUNC_SERIAL)
      {
        dumpSamples();
      }
#ifdef DSO_150_EEPROM      
      else if (currentFunction==FUNC_LOAD)
      {
        loadWaveform();        
      }
      else if (currentFunction==FUNC_SAVE)
      {
        saveWaveform();        
      }

#endif    
      else if (currentFunction==FUNC_AUTOCAL)
      {
        autoCal();
      }  
      break;      
		default:
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}

void changeDSize(uint8_t wave)
{
  dsize[wave-2]++;
  if (dsize[wave-2] > 3)
    dsize[wave-2] = 1;

  saveParameter(PARAM_DSIZE+wave-2, dsize[wave-2]);  

  clearWaves();
  drawWaves();
}



// ------------------------
void encoderChanged(int steps)	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_timebase:
			if(steps > 0) decrementTimeBase(); else	incrementTimeBase();
			break;
#ifdef DSO_150      
    case L_voltagerange:
      if(steps > 0) decrementVoltageRange(); else incrementVoltageRange();
      break;  
#endif         
    case L_function:
      if(steps > 0) incrementFunc(); else decrementFunc();
      break; 
    case L_triggerSource:
      if(steps > 0) incrementTSource(); else decrementTSource();
      break; 
    case L_zoom:
      if(steps > 0) incrementZoom(); else decrementZoom();
      break;      
		case L_triggerType:
			if(steps > 0) incrementTT(); else decrementTT();
			break;
		case L_triggerEdge:
			if(steps > 0) incrementEdge(); else decrementEdge();
			break;
		case L_triggerLevel:
			if(steps > 0) incrementTLevel(); else decrementTLevel();
			break;
		case L_waves:
			if(steps > 0) incrementWaves(); else decrementWaves();
			break;
		case L_window:
			if(steps > 0) changeXCursor(xCursor + XCURSOR_STEP); else changeXCursor(xCursor - XCURSOR_STEP);
			break;
		case L_vPos1:
			if(steps > 0) changeYCursor(0, yCursors[A1] - YCURSOR_STEP); else changeYCursor(A1, yCursors[A1] + YCURSOR_STEP);
			break;
		case L_vPos2:
			if(steps > 0) changeYCursor(A2, yCursors[A2] - YCURSOR_STEP); else changeYCursor(A2, yCursors[A2] + YCURSOR_STEP);
			break;
		case L_vPos3:
			if(steps > 0) changeYCursor(D1, yCursors[D1] - YCURSOR_STEP); else changeYCursor(D1, yCursors[D1] + YCURSOR_STEP);
			break;
		case L_vPos4:
			if(steps > 0) changeYCursor(D2, yCursors[D2] - YCURSOR_STEP); else changeYCursor(D2, yCursors[D2] + YCURSOR_STEP);
			break;
    case L_vPos5:
      if(steps > 0) changeYCursor(D3, yCursors[D3] - YCURSOR_STEP); else changeYCursor(D3, yCursors[D3] + YCURSOR_STEP);
      break;      
	}
	
	// manually update display if frozen
	if(hold)
  {
    repaintLabels();
		drawWaves();
  }
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}

// ------------------------
void incrementTLevel()	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL + 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}

// ------------------------
void decrementTLevel()	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL - 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}

// A1, D1, D2, D3, A2 
// 0, 2, 3, 4, 1
// ------------------------
void incrementWaves()	{
// ------------------------
	// add more waves
  if(waves[A2]) {
    return;
  }

#ifdef ADD_AN2  
  else if(waves[D3])  {
    waves[A2] = true;
  }
#endif
#ifdef ADD_D3
  else if(waves[D2]) {
    waves[D3] = true;
  } 
#else  
#ifdef ADD_AN2
  else if(waves[D2]) {
    waves[A2] = true;
  } 
#endif
#endif
  else if(waves[D1]) {
    waves[D2] = true;
  } 
  else {
    waves[D1] = true;
  }

  saveParameter(PARAM_WAVES + 0, waves[A1]);
  saveParameter(PARAM_WAVES + 1, waves[A2]);
  saveParameter(PARAM_WAVES + 2, waves[D1]);
  saveParameter(PARAM_WAVES + 3, waves[D2]);
  saveParameter(PARAM_WAVES + 4, waves[D3]);
	repaintLabels();
}


// ------------------------
void decrementWaves()	{
// ------------------------
	// remove waves
	if(waves[A2])	{
		waves[A2] = false;
	}
  else if(waves[D3]) {
    waves[D3] = false;
  }
	else if(waves[D2])	{
		waves[D2] = false;
	}
	else if(waves[D1])	{
		waves[D1] = false;
	}
	else {
		return;
	}

  saveParameter(PARAM_WAVES + 0, waves[A1]);
  saveParameter(PARAM_WAVES + 1, waves[A2]);
  saveParameter(PARAM_WAVES + 2, waves[D1]);
  saveParameter(PARAM_WAVES + 3, waves[D2]);
  saveParameter(PARAM_WAVES + 4, waves[D3]);
	repaintLabels();
}

// ------------------------
void incrementEdge()
// ------------------------
{
  if(triggerDir == TRIGGER_ALL)
    return;
  setTriggerDir(triggerDir+1);
  saveParameter(PARAM_TRIGDIR, triggerDir);
  repaintLabels();
}

// ------------------------
void decrementEdge()
// ------------------------
{
  if(triggerDir == TRIGGER_RISING)
    return;
  setTriggerDir(triggerDir-1);
  saveParameter(PARAM_TRIGDIR, triggerDir);
  repaintLabels();
}


// ------------------------
void nextTT()  {
// ------------------------
  if(triggerType == TRIGGER_SINGLE)
   triggerType = TRIGGER_AUTO;
  else
  triggerType++;
  setTriggerType(triggerType);
  //  type is not saved
  saveParameter(PARAM_TRIGTYPE, triggerType);
}

// ------------------------
void incrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(triggerType + 1);
	//  type is not saved
	saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}

// ------------------------
void decrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(triggerType - 1);
	//  type is not saved
	saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}

// ------------------------
void incrementZoom()  {
// ------------------------
  if(zoomFactor == ZOOM_X8)
    return;
  
  setZoomFactor(zoomFactor+1);

  saveParameter(PARAM_ZOOM, zoomFactor);
  repaintLabels();
}

// ------------------------
void decrementZoom()  {
// ------------------------
  if(zoomFactor == ZOOM_X1)
    return;

  setZoomFactor(zoomFactor-1);

  saveParameter(PARAM_ZOOM, zoomFactor);
  repaintLabels();
}

// ------------------------
void setZoomFactor(uint8_t zoomF)
// ------------------------
{
  zoomFactor = zoomF;

  switch(zoomF)
  {
     case ZOOM_X1:
        xZoom = 1;
        break;    
     case ZOOM_X2:
        xZoom = 2;
        break;    
     case ZOOM_X4:
        xZoom = 4;
        break;    
     case ZOOM_X8:
        xZoom = 8;
        break;                                      
  }
  clearWaves();
}

// ------------------------
void incrementTSource()  {
// ------------------------
#ifdef ADD_D3
  if(triggerSource == TRIGSRC_D3)
#else
  if(triggerSource == TRIGSRC_D2)
#endif
    return;
  
  setTriggerSource(triggerSource + 1);
  //  type is not saved
  saveParameter(PARAM_TSOURCE, triggerSource);
  repaintLabels();
}

// ------------------------
void decrementTSource()  {
// ------------------------
  if(triggerSource == TRIGSRC_A1)
    return;
  setTriggerSource(triggerSource - 1);
  //  type is not saved
  saveParameter(PARAM_TSOURCE, triggerSource);
  repaintLabels();
}


// ------------------------
void incrementFunc()
// ------------------------
{
#ifdef DSO_150  
  if(currentFunction == FUNC_AUTOCAL)
#else
  if(currentFunction == FUNC_SERIAL)
#endif
    return;
  currentFunction++;
  //  type is not saved
  saveParameter(PARAM_FUNC,currentFunction);
  repaintLabels();
}

// ------------------------
void decrementFunc()
// ------------------------
{
  if(currentFunction == FUNC_SERIAL)
    return;
  currentFunction--;
  //  type is not saved
  saveParameter(PARAM_FUNC,currentFunction);
  repaintLabels();
}


// ------------------------
void nextTimeBase()  {
// ------------------------
  if(currentTimeBase == T50MS)
   currentTimeBase = T20US;
  else
  currentTimeBase++;
  setTimeBase(currentTimeBase);
}


// ------------------------
void incrementTimeBase()	{
// ------------------------
	if(currentTimeBase == T50MS)
		return;
	
	setTimeBase(currentTimeBase + 1);
}

// ------------------------
void decrementTimeBase()	{
// ------------------------
	if(currentTimeBase == T20US)
		return;
	
	setTimeBase(currentTimeBase - 1);
}


#ifdef DSO_150
// ------------------------
void nextVoltageRange()  
// ------------------------
{
#ifdef DSO_150     
  if(currentVoltageRange == RNG_5mV)
      currentVoltageRange = RNG_20V;
#else
  if(currentVoltageRange == RNG_10mV)
      currentVoltageRange = RNG_5V;
#endif
  else
    currentVoltageRange++;
  setVoltageRange(currentVoltageRange);
}


// ------------------------
void incrementVoltageRange()  
// ------------------------
{
#ifdef DSO_150     
  if(currentVoltageRange == RNG_5mV)
#else
  if(currentVoltageRange == RNG_10mV)
#endif  
    return;
  
  setVoltageRange(currentVoltageRange + 1);
}

// ------------------------
void decrementVoltageRange() 
// ------------------------
{  
#ifdef DSO_150    
  if(currentVoltageRange == RNG_20V)
#else
  if(currentVoltageRange == RNG_5V)
#endif
    return;
  
  setVoltageRange(currentVoltageRange - 1);
}


// ------------------------
void setVoltageRange(uint8_t voltageRange)  {
// ------------------------
  currentVoltageRange = voltageRange;
  rangePos = voltageRange;
  setVRange(voltageRange);
  saveParameter(PARAM_VRANGE, currentVoltageRange);
  // request repainting of screen labels
  repaintLabels();
}
#endif

// ------------------------
void setTimeBase(uint8_t timeBase)	{
// ------------------------
	currentTimeBase = timeBase;
	setSamplingRate(timeBase);
	saveParameter(PARAM_TIMEBASE, currentTimeBase);
	// request repainting of screen labels
	repaintLabels();
}


// ------------------------
void toggleWave(uint8_t num)	{
// ------------------------
	waves[num] = !waves[num];
	saveParameter(PARAM_WAVES + num, waves[num]);
	repaintLabels();
}

// ------------------------
void changeYCursor(uint8_t num, int16_t yPos)	{
// ------------------------
	if(yPos > 0)
		yPos = 0;
	
	if(yPos < -GRID_HEIGHT)
		yPos = -GRID_HEIGHT;

	yCursors[num] = yPos;
	saveParameter(PARAM_YCURSOR + num, yCursors[num]);
	repaintLabels();
}


// ------------------------
void changeXCursor(int16_t xPos)	{
// ------------------------
	if(xPos < 0)
		xPos = 0;
	
	if(xPos > ((NUM_SAMPLES) - (GRID_WIDTH*xZoom)))
		xPos = (NUM_SAMPLES) - (GRID_WIDTH*xZoom);
	
	xCursor = xPos;
	saveParameter(PARAM_XCURSOR, xCursor);
	repaintLabels();
}



