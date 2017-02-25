#define DEBOUNCE_TIME	350


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
		if(millis() - pressedTime < DEBOUNCE_TIME)
			return;
		pressedTime = millis();
		pressed = true;
	}
	
	
	if(pressed && (digitalRead(BTN4) == HIGH))	{
		// debounce
		if(millis() - pressedTime < 5)
			return;
		
		pressed = false;
		
		// is it a short press
		if(millis() - pressedTime < 1000)	{
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
	if(millis() - lastBtnPress < DEBOUNCE_TIME)
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
		case L_triggerLevel:
			// set trigger level to 0
			setTriggerLevel(0);
			saveParameter(PARAM_TLEVEL, 0);
			repaintLabels();
			break;
		case L_window:
			// set x in the middle
			changeXCursor((NUM_SAMPLES - GRID_WIDTH)/2);
			break;
		case L_vPos1:
			// zero the trace base
			calculateTraceZero(0);
			changeYCursor(0, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos2:
			// zero the trace base
			calculateTraceZero(1);
			changeYCursor(1, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos3:
			changeYCursor(2, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos4:
			changeYCursor(3, -GRID_HEIGHT/2 - 1);
			break;
		default:
			// toggle stats printing
			printStats = !printStats;
			saveParameter(PARAM_STATS, printStats);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}




// ------------------------
void calculateTraceZero(int waveID)		{
// ------------------------
	// calculate zero only if switch is in GND position
	if(couplingPos != CPL_GND)
		return;

	if(waveID > 1)
		return;
	
	uint16_t *wave = (waveID == 0)? ch1Capture : ch2Capture;
	
	// zero the trace
	int32_t sumSamples = 0;

	for(uint16_t k = 0; k < NUM_SAMPLES; k++)	{
		sumSamples += wave[k];
	}

	uint16_t Vavr = sumSamples/NUM_SAMPLES;

	if(waveID == 0)	{
		zeroVoltageA1 = Vavr;
		saveParameter(PARAM_ZERO1, zeroVoltageA1);
	}
	else	{
		zeroVoltageA2 = Vavr;
		saveParameter(PARAM_ZERO2, zeroVoltageA2);
	}
}




// ------------------------
void encoderChanged(int steps)	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_timebase:
			if(steps > 0) decrementTimeBase(); else	incrementTimeBase();
			break;
		case L_triggerType:
			if(steps > 0) incrementTT(); else decrementTT();
			break;
		case L_triggerEdge:
			if(steps > 0) setTriggerRising(); else setTriggerFalling();
			break;
		case L_triggerLevel:
			if(steps > 0) incrementTLevel(); else decrementTLevel();
			break;
		case L_waves:
			if(steps > 0) incrementWaves(); else decrementWaves();
			break;
		case L_window:
			if(steps > 0) changeXCursor(xCursor + 40); else changeXCursor(xCursor - 40);
			break;
		case L_vPos1:
			if(steps > 0) changeYCursor(0, yCursors[0] - 5); else changeYCursor(0, yCursors[0] + 5);
			break;
		case L_vPos2:
			if(steps > 0) changeYCursor(1, yCursors[1] - 5); else changeYCursor(1, yCursors[1] + 5);
			break;
		case L_vPos3:
			if(steps > 0) changeYCursor(2, yCursors[2] - 5); else changeYCursor(2, yCursors[2] + 5);
			break;
		case L_vPos4:
			if(steps > 0) changeYCursor(3, yCursors[3] - 5); else changeYCursor(3, yCursors[3] + 5);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
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


// A1, D1, D2, A2 
// 0, 2, 3, 1
// ------------------------
void incrementWaves()	{
// ------------------------
	// add more waves
	if(waves[1])	{
		return;
	}
	else if(waves[3])	{
		waves[1] = true;
		saveParameter(PARAM_WAVES + 1, waves[1]);
	}
	else if(waves[2])	{
		waves[3] = true;
		saveParameter(PARAM_WAVES + 3, waves[3]);
	}
	else {
		waves[2] = true;
		saveParameter(PARAM_WAVES + 2, waves[2]);
	}

	repaintLabels();
}




// ------------------------
void decrementWaves()	{
// ------------------------
	// remove waves
	if(waves[1])	{
		waves[1] = false;
		saveParameter(PARAM_WAVES + 1, waves[1]);
	}
	else if(waves[3])	{
		waves[3] = false;
		saveParameter(PARAM_WAVES + 3, waves[3]);
	}
	else if(waves[2])	{
		waves[2] = false;
		saveParameter(PARAM_WAVES + 2, waves[2]);
	}
	else {
		return;
	}
	
	repaintLabels();
}



// ------------------------
void setTriggerRising()	{
// ------------------------
	if(triggerRising)
		return;
	
	setTriggerRising(true);
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
}



// ------------------------
void setTriggerFalling()	{
// ------------------------
	if(!triggerRising)
		return;
	
	setTriggerRising(false);
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
}




// ------------------------
void incrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(triggerType + 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}



// ------------------------
void decrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(triggerType - 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
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
	
	if(xPos > (NUM_SAMPLES - GRID_WIDTH))
		xPos = NUM_SAMPLES - GRID_WIDTH;
	
	xCursor = xPos;
	saveParameter(PARAM_XCURSOR, xCursor);
	repaintLabels();
}



