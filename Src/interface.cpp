#include "interface.hpp"
#include "variables.h"
#include "global.h"
#include "display.hpp"
#include "zconfig.hpp"
#include "capture.hpp"
#include "io.hpp"
#include "awrap.hpp"

extern TIM_HandleTypeDef htim4;  //TIMER Sample Timing
extern TIM_HandleTypeDef htim2;  //TIMER ScanTimeout

extern t_config config;
extern uint8_t currentFocus;
extern uint8_t xZoom;
extern int16_t sDly, tDly;
extern volatile bool hold;

uint8_t rangePos;

// sampling delay table in quarter-microseconds (not sure where the quarter-microseconds come from it's just a delayloop...)
const int16_t samplingDelay[] =   {-1,  0, 14, 38, 86, 229, 468, 948, 2385, 4776, 9570, 23940,0x7FFE};
//Timeout delay in  milliseconds
const uint16_t timeoutDelayMs[] = {75, 75, 75, 150, 150, 150, 150, 250, 500, 1000, 2000, 4000,0x7FFE};
const uint16_t samplingTimerDelay[] = {20, 30, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000,0xFFFF};
// ------------------------
void resetParam()
// ------------------------
{
  if (config.markerMode)
  {
	  config.markerMode = false;
	  clearWaves();
  }
  else
  {
		// which label has current focus
	  switch(currentFocus)
	  {
		case L_voltagerange:
			setVoltageRange(RNG_5V);
			break;
		case L_triggerLevel:
		  // set  level to 0
		  setTriggerLevel(0);
		  repaintLabels();
		  break;
	   case L_triggerEdge:
		  setTriggerDir(TRIGGER_RISING);
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
		  repaintLabels();
		  break;
		case L_vPos1:
		  //TODO add cursor control here???
			config.markerMode = true;
		  break;
		case L_vPos2:
		  changeDSize(D1);
		  break;
		case L_vPos3:
		  changeDSize(D2);
		  break;
		case L_vPos4:
		  changeDSize(D3);
		  break;
		case L_function:
		  if (config.currentFunction==FUNC_SERIAL)
		  {
			dumpSamples();
		  }
		  else if (config.currentFunction==FUNC_LOAD)
		  {
			loadWaveform();
		  }
		  else if (config.currentFunction==FUNC_SAVE)
		  {
			saveWaveform();
		  }
		  else if (config.currentFunction==FUNC_AUTOCAL)
		  {
			autoCal();
		  }
		  break;
			default:
				break;
		}
  }
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(config.triggerType != TRIGGER_AUTO)
		// break the sampling loop
		stopSampling();
}

void changeDSize(uint8_t wave)
{
	config.dsize[wave-1]++;
  if (config.dsize[wave-1] > 3)
	  config.dsize[wave-1] = 1;

  clearWaves();
  drawWaves();
}



// ------------------------
void encoderChanged(int steps)
// ------------------------
{
	// which label has current focus
	switch(currentFocus)
	{
		case L_timebase:
			if(steps > 0) decrementTimeBase(); else	incrementTimeBase();
			break;
		case L_voltagerange:
			if(steps > 0) decrementVoltageRange(); else incrementVoltageRange();
			break;
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
			changeXCursor(config.xCursor + (steps*20));
			break;
		case L_vPos1:
			if (config.markerMode)
			{
				move_marker(config.currentMarker,steps);
			}
			else
			{
				changeYCursor(A1, config.yCursors[A1] + (steps*2));
			}
			break;
		case L_vPos2:
			changeYCursor(D1, config.yCursors[D1] + (steps*2));
			break;
		case L_vPos3:
			changeYCursor(D2, config.yCursors[D2] + (steps*2));
			break;
		case L_vPos4:
			changeYCursor(D3, config.yCursors[D3] + (steps*2));
			break;
	}

	
	// manually update display if frozen
	if(hold)
  {
    repaintLabels();
		drawWaves();
  }
	if(config.triggerType != TRIGGER_AUTO)
		// break the sampling loop
		stopSampling();
}


void move_marker(uint8_t currentMarker,int8_t steps)
{
	if (currentMarker == MARKER_X1)
	{
		config.marker_x1 = config.marker_x1 + steps;
		if (config.marker_x1 < 0)
			config.marker_x1 = 0;
		if (config.marker_x1 > GRID_WIDTH)
			config.marker_x1 = GRID_WIDTH;

	}
	else if (currentMarker == MARKER_X2)
	{
		config.marker_x2 = config.marker_x2 + steps;
		if (config.marker_x2 < 0)
			config.marker_x2 = 0;
		if (config.marker_x2 > GRID_WIDTH)
			config.marker_x2 = GRID_WIDTH;

	}
	else if (currentMarker == MARKER_Y1)
	{
		config.marker_y1 = config.marker_y1 + steps;
		if (config.marker_y1 < 0)
			config.marker_y1 = 0;
		if (config.marker_y1 > GRID_HEIGHT)
			config.marker_y1 = GRID_HEIGHT;

	}
	else if (currentMarker == MARKER_Y2)
	{
		config.marker_y2 = config.marker_y2 + steps;
		if (config.marker_y2 < 0)
			config.marker_y2 = 0;
		if (config.marker_y2 > GRID_HEIGHT)
			config.marker_y2 = GRID_HEIGHT;

	}
}

// ------------------------
void incrementTLevel()
// ------------------------
{
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL + 5);
	setTriggerTimer();
	repaintLabels();
}

// ------------------------
void decrementTLevel()
// ------------------------
{
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL - 5);
	setTriggerTimer();
	repaintLabels();
}

// A1, D1, D2, D3 
// 0, 2, 3, 4
// ------------------------
void incrementWaves()
// ------------------------
{
  if(config.waves[D2])
  {
	  config.waves[D3] = true;
  } 
  else if(config.waves[D1])
  {
	  config.waves[D2] = true;
  } 
  else
  {
	  config.waves[D1] = true;
  }
  repaintLabels();
}


// ------------------------
void decrementWaves()
// ------------------------
{
	// remove waves
  if(config.waves[D3])
  {
	  config.waves[D3] = false;
  }
  else if(config.waves[D2])
  {
	  config.waves[D2] = false;
  }
  else if(config.waves[D1])
  {
	  config.waves[D1] = false;
  }
  else
  {
	  return;
  }
  repaintLabels();
}

// ------------------------
void incrementEdge()
// ------------------------
{
  if(config.triggerDir == TRIGGER_ALL)
    return;
  setTriggerDir(config.triggerDir+1);
  repaintLabels();
}

// ------------------------
void decrementEdge()
// ------------------------
{
  if(config.triggerDir == TRIGGER_RISING)
    return;
  setTriggerDir(config.triggerDir-1);
  repaintLabels();
}


// ------------------------
void nextTT()
// ------------------------
{
  if(config.triggerType == TRIGGER_SINGLE)
	  config.triggerType = TRIGGER_AUTO;
  else
	  config.triggerType++;
  setTriggerType(config.triggerType);
  //  type is not saved
}

// ------------------------
void incrementTT()
// ------------------------
{
	if(config.triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(config.triggerType + 1);
	//  type is not saved
	repaintLabels();
}

// ------------------------
void decrementTT()
// ------------------------
{
	if(config.triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(config.triggerType - 1);
	repaintLabels();
}

// ------------------------
void incrementZoom()
// ------------------------
{
  if(config.zoomFactor == ZOOM_X8)
    return;
  
  setZoomFactor(config.zoomFactor+1);
  repaintLabels();
}

// ------------------------
void decrementZoom()
// ------------------------
{
  if(config.zoomFactor == ZOOM_X1)
    return;

  setZoomFactor(config.zoomFactor-1);
  repaintLabels();
}

// ------------------------
void setZoomFactor(uint8_t zoomF)
// ------------------------
{
	config.zoomFactor = zoomF;

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
  changeXCursor((((NUM_SAMPLES)-GRID_WIDTH)/2)/xZoom);
  repaintLabels();
  clearWaves();
}

// ------------------------
void incrementTSource()
// ------------------------
{
  if(config.triggerSource == TRIGSRC_D3)
    return;
  
  setTriggerSource(config.triggerSource+1);
  repaintLabels();
}

// ------------------------
void decrementTSource()
// ------------------------
{
  if(config.triggerSource == TRIGSRC_A1)
    return;
  setTriggerSource(config.triggerSource-1);
  repaintLabels();
}


// ------------------------
void incrementFunc()
// ------------------------
{
  if(config.currentFunction == FUNC_AUTOCAL)
    return;
  config.currentFunction++;
  repaintLabels();
}

// ------------------------
void decrementFunc()
// ------------------------
{
  if(config.currentFunction == FUNC_SERIAL)
    return;
  config.currentFunction--;
  repaintLabels();
}


// ------------------------
void nextTimeBase()
// ------------------------
{
  if(config.currentTimeBase == LOOP)
	  config.currentTimeBase = T20US;
  else
	  config.currentTimeBase++;
  setTimeBase(config.currentTimeBase);
}


// ------------------------
void incrementTimeBase()
// ------------------------
{
	if(config.currentTimeBase == LOOP)
		return;
	setTimeBase(config.currentTimeBase + 1);
}

// ------------------------
void decrementTimeBase()
// ------------------------
{
	if(config.currentTimeBase == T20US)
		return;
	setTimeBase(config.currentTimeBase - 1);
}


// ------------------------
void nextVoltageRange()
// ------------------------
{
  if(config.currentVoltageRange == RNG_5mV)
	  config.currentVoltageRange = RNG_20V;
  else
	  config.currentVoltageRange++;
  setVoltageRange(config.currentVoltageRange);
}


// ------------------------
void incrementVoltageRange()
// ------------------------
{
  if(config.currentVoltageRange == RNG_5mV)
    return;
  setVoltageRange(config.currentVoltageRange + 1);
}

// ------------------------
void decrementVoltageRange()
// ------------------------
{    
  if(config.currentVoltageRange == RNG_20V)
    return;
  setVoltageRange(config.currentVoltageRange - 1);
}


// ------------------------
void setVoltageRange(uint8_t voltageRange)
// ------------------------
{
  config.currentVoltageRange = voltageRange;
  rangePos = config.currentVoltageRange;
  setVRange(config.currentVoltageRange);
  repaintLabels();
}


// ------------------------
void setTimeBase(uint8_t timeBase)
// ------------------------
{
	config.currentTimeBase = timeBase;
	setSamplingRate(config.currentTimeBase);
	repaintLabels();
}


// ------------------------
void toggleWave(uint8_t num)
// ------------------------
{
	config.waves[num] = !config.waves[num];
	repaintLabels();
}

// ------------------------
void changeYCursor(uint8_t num, int16_t yPos)
// ------------------------
{
	if(yPos > 0)
		yPos = 0;
	
	if(yPos < -GRID_HEIGHT)
		yPos = -GRID_HEIGHT;

	config.yCursors[num] = yPos;
	repaintLabels();
}


// ------------------------
void changeXCursor(int16_t xPos)
// ------------------------
{
	if(xPos < 0)
		xPos = 0;
	
	if(xPos > ((NUM_SAMPLES) - (GRID_WIDTH*xZoom)))
		xPos = (NUM_SAMPLES) - (GRID_WIDTH*xZoom);
	
	config.xCursor = xPos;
	repaintLabels();
}

// ------------------------
void setSamplingRate(uint8_t timeBase)
// ------------------------
{
#ifdef USE_TIMER_SAMPLE
  timerSetPeriod(&htim4,samplingTimerDelay[timeBase]);
#else
  sDly = samplingDelay[timeBase];
#endif

	tDly = timeoutDelayMs[timeBase];
	// sampling rate changed, break out from previous sampling loop
	stopSampling();
	// disable scan timeout timer
    timerPause(&htim2);

    //Move cursor all the way to the left in loop mode
    if (timeBase == LOOP)
    	changeXCursor(10000);
}


// ------------------------
void setTriggerType(uint8_t tType)
// ------------------------
{
  config.triggerType = tType;
  // break any running capture loop
  stopSampling();
}

// ------------------------
void setTriggerDir(uint8_t tdir)
// ------------------------
{
  config.triggerDir = tdir;
  setTriggerSourceAndDir(config.triggerSource,tdir);
}

// ------------------------
void setTriggerSource(uint8_t tsource)
// ------------------------
{
  config.triggerSource = tsource;
  setTriggerSourceAndDir(tsource,config.triggerDir);
}

