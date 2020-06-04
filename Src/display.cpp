#include <math.h>
#include "variables.h"
#include "display.hpp"
#include "interface.hpp"
#include "awrap.hpp"
#include "capture.hpp"
#include "io.hpp"


extern t_config config;
extern uint16_t ch1Capture[NUM_SAMPLES];
extern uint8_t couplingPos;

extern uint8_t rangePos;
extern uint16_t sIndex;
extern volatile bool hold;
extern uint32_t samplingTime;

bool fade_color_clear = false;
bool cDisplayed = false;
bool paintLabels = false;  // repaint the labels on screen in draw loop

const char* cplNames[] = {"GND","AC","DC"};
const char* functionNames[] = {"SERIAL","LOAD","SAVE","AUTOCAL"};
const char* zoomNames[] = {"x1","x2","x4","x8"};
const char* trigModeNames[] = {"AUTO","NORM","SNGL"};
const char* trigSourceNames[] = {"A1","D1","D2","D3"};
const char* rngNames[] = {"20V","10V","5V","2V","1V","0.5V","0.2V","0.1V","50mV","20mV","10mV","5mV"};
const char* tbNames[] = {"20 uS","30 uS","50 uS","0.1 mS","0.2 mS","0.5 mS","1 mS","2 mS","5 mS","10 mS","20 mS","50 mS","LOOP"};

// grid variables
uint8_t hOffset = (TFT_WIDTH - GRID_WIDTH)/2;
uint8_t vOffset = (TFT_HEIGHT - GRID_HEIGHT)/2;
uint8_t dHeight = GRID_HEIGHT/8;

uint8_t xZoom;
t_Stats wStats;
uint32_t triggertimer = 0;

const uint16_t sampleUs[] =   {20,  30, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000,0};

// rendered waveform data is stored here for erasing
int16_t ch1Old[GRID_WIDTH] = {0};
int8_t bitOld[GRID_WIDTH] = {0};
uint8_t currentFocus = L_timebase;


void setFocusLabel(uint8_t label)
{
  currentFocus = label;
  repaintLabels();
}

// ------------------------
void focusNextLabel()
// ------------------------
{
	if (config.markerMode)
	{
		if (currentFocus != L_vPos1)
		{
			currentFocus = L_vPos1;
			repaintLabels();
		}
		else
		{
			if (config.currentMarker != MARKER_Y2)
				config.currentMarker++;
			else
				config.currentMarker = MARKER_X1;
			draw_markers();
		}
	}
	else
	{
	currentFocus++;

	  //TODO limit this according to number of channels...
		if((currentFocus == L_vPos1) && !config.waves[A1])
			currentFocus++;

		if((currentFocus == L_vPos2) && !config.waves[D1])
			currentFocus++;

		if((currentFocus == L_vPos3) && !config.waves[D2])
			currentFocus++;

		if((currentFocus == L_vPos4) && !config.waves[D3])
			currentFocus++;

		if(currentFocus > L_vPos4)
			currentFocus = L_voltagerange;
	}
}


// ------------------------
void repaintLabels()
// ------------------------
{
	paintLabels = true;
}



void erease_markers()
{
	tft_writeFastVLine(hOffset + config.old_x1_marker + 1, vOffset + 1, GRID_HEIGHT, ILI9341_BLACK);
	tft_writeFastVLine(hOffset + config.old_x2_marker + 1, vOffset + 1, GRID_HEIGHT, ILI9341_BLACK);
	tft_writeFastHLine(hOffset + 1 , vOffset + config.old_y1_marker + 1, GRID_WIDTH, ILI9341_BLACK);
	tft_writeFastHLine(hOffset + 1 , vOffset + config.old_y2_marker+ 1, GRID_WIDTH, ILI9341_BLACK);
}

void draw_markers()
{
	uint16_t col;

	if (config.currentMarker == MARKER_X1)
		col = ILI9341_PINK;
	else
	    col = ILI9341_LIGHTGREY;
	tft_writeFastVLine(hOffset + config.marker_x1 + 1, vOffset + 1, GRID_HEIGHT, col);

	if (config.currentMarker == MARKER_X2)
		col = ILI9341_PINK;
	else
	    col = ILI9341_LIGHTGREY;
	tft_writeFastVLine(hOffset + config.marker_x2 + 1, vOffset + 1, GRID_HEIGHT, col);

	if (config.currentMarker == MARKER_Y1)
		col = ILI9341_PINK;
	else
	    col = ILI9341_LIGHTGREY;
	tft_writeFastHLine(hOffset + 1, vOffset + config.marker_y1 + 1, GRID_WIDTH, col);

	if (config.currentMarker == MARKER_Y2)
		col = ILI9341_PINK;
	else
	    col = ILI9341_LIGHTGREY;
	tft_writeFastHLine(hOffset + 1, vOffset + config.marker_y2 + 1, GRID_WIDTH, col);

	config.old_x1_marker = config.marker_x1;
	config.old_x2_marker = config.marker_x2;
	config.old_y1_marker = config.marker_y1;
	config.old_y2_marker = config.marker_y2;
}

// ------------------------
void drawWaves()
// ------------------------
{
  static bool printStatsOld = false;
  static bool printVoltageOld = false;

  if (config.markerMode)
  {
	  erease_markers();
  }

  if(printStatsOld && !config.printStats)
		clearStats();
  printStatsOld = config.printStats;

  if(printVoltageOld && !config.printVoltage)
    clearStats();
  printVoltageOld = config.printVoltage;
  
  // draw the grid
  drawGrid();

  if (config.markerMode)
  {
	  draw_markers();
	  draw_markerstats();
  }

  // clear and draw signal traces
  clearNDrawSignals();

  // if requested update the stats
  if(config.printStats)
    drawStats();

  if(config.printVoltage)
    drawVoltageStat();



  if (triggertimer)
  {
	  if (millis() < triggertimer)
	  {
		  drawVoltageLarge((float)(config.trigLevel + 2048- config.zeroVoltageA1) * config.adcMultiplier[rangePos], 210,ILI9341_YELLOW);
	  }
	  else
	  {
		  clearWaves();
		  triggertimer = 0;
	  }
  }

	// if label repaint requested - do so now
	if(paintLabels)
	{
		drawLabels();
		paintLabels = false;
	}
}


void setTriggerTimer(void)
{
	triggertimer = millis() + 2000;
}

// ------------------------
void clearWaves()
// ------------------------
{
	// clear screen
	tft_fillScreen(ILI9341_BLACK);
	// and paint o-scope
	drawGrid();
	drawLabels();
}


uint16_t fade_color(uint16_t color) 
{ // 565 
   return fade_color_clear ? ILI9341_BLACK : color << 2; 
} 

void clearPersistence() 
{ 
  fade_color_clear = true; 
  tft_writeFillRect(hOffset+1, vOffset+1, GRID_WIDTH-2, GRID_HEIGHT-2, ILI9341_BLACK); // clear graph area
} 

void setPersistence(uint8_t pers)
{
	config.persistence_on = pers;
  clearPersistence(); 
}

void togglePersistence() 
{ 
	config.persistence_on = !config.persistence_on;
  clearPersistence(); 
} 


// ------------------------
void indicateCapturing()
// ------------------------
{
	if((config.currentTimeBase > T2MS) || (config.triggerType != TRIGGER_AUTO))	{
		cDisplayed = true;
  
		tft_setTextColor(ILI9341_PINK, ILI9341_BLACK);
		tft_setCursor(140, 20);
		tft_print((char*)"Sampling...");
	}
}

// ------------------------
void indicateCapturingDone()
// ------------------------
{
	if(cDisplayed)
	{
		tft_writeFillRect(140, 20, 66, 8, ILI9341_BLACK);
		cDisplayed = false;
	}
}


// ------------------------
void clearNDrawSignals()
// ------------------------
{
	static bool wavesOld[4] = {false,false,false,false};
	static int16_t yCursorsOld[4];
	
	// snap the values to prevent interrupt from changing mid-draw
	int16_t xCursorSnap = config.xCursor;
	int16_t zeroVoltageA1Snap = config.zeroVoltageA1;
	int16_t yCursorsSnap[4];
	bool wavesSnap[4];

	yCursorsSnap[A1] = config.yCursors[A1];
	yCursorsSnap[D1] = config.yCursors[D1];
	yCursorsSnap[D2] = config.yCursors[D2];
    yCursorsSnap[D3] = config.yCursors[D3];
	wavesSnap[A1] = config.waves[A1];
	wavesSnap[D1] = config.waves[D1];
	wavesSnap[D2] = config.waves[D2];
    wavesSnap[D3] = config.waves[D3];

	// draw the GRID_WIDTH section of the waveform from xCursorSnap
	int16_t val1, val2;
	int16_t transposedPt1, transposedPt2;

	// sampling stopped at sIndex - 1
	int j = sIndex + xCursorSnap;
	if(j >= NUM_SAMPLES)
		j = j - NUM_SAMPLES;
	
	// go through all the data points
	for(int i = 1, jn = j + xZoom; i < GRID_WIDTH - 1; j=j+xZoom, i++, jn=jn+xZoom)	
	{
		if(jn >= NUM_SAMPLES)
			jn = 0;

		if(j >= NUM_SAMPLES)
			j = 0;

		// erase old line segment 
    if(wavesOld[D3])
    {
      val1 = (bitOld[i] & 0b1000) ? dHeight/config.dsize[D3-1] : 0;
      val2 = (bitOld[i + 1] & 0b1000) ? dHeight/config.dsize[D3-1] : 0;
      // clear the line segment
      transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D3] - val1;
      transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D3] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, fade_color(DG_SIGNAL3)); 
    }

		if(wavesOld[D2])
		{
			val1 = (bitOld[i] & 0b0100) ? dHeight/config.dsize[D2-1] : 0;
			val2 = (bitOld[i + 1] & 0b0100) ? dHeight/config.dsize[D2-1] : 0;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D2] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D2] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, fade_color(DG_SIGNAL2)); 
		}
   
		if(wavesOld[D1])
		{
			val1 = (bitOld[i] & 0b0010) ? dHeight/config.dsize[D1-1] : 0;
			val2 = (bitOld[i + 1] & 0b0010) ? dHeight/config.dsize[D1-1] : 0;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D1] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, fade_color(DG_SIGNAL1)); 
		}

		if(wavesOld[A1])
		{
			val1 = (ch1Old[i] * GRID_HEIGHT)/ADC_2_GRID;
			val2 = (ch1Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[A1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[A1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, fade_color(AN_SIGNAL1)); 
		}
  	
		// draw new segments
		if(wavesSnap[D3])
		{
			val1 = (ch1Capture[j] & 0b10000000) ? dHeight/config.dsize[D3-1] : 0;
			val2 = (ch1Capture[jn] & 0b10000000) ? dHeight/config.dsize[D3-1] : 0;
			bitOld[i] &= 0b0111;
			bitOld[i] |= (ch1Capture[j]>>8) & 0b1000;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D3] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D3] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL3);
		}

    if(wavesSnap[D2])
    {
      val1 = (ch1Capture[j] & 0b01000000) ? dHeight/config.dsize[D2-1] : 0;
      val2 = (ch1Capture[jn] & 0b01000000) ? dHeight/config.dsize[D2-1] : 0;
      bitOld[i] &= 0b10111;
      bitOld[i] |= (ch1Capture[j]>>8) & 0b0100;
      // draw the line segment
      transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D2] - val1;
      transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D2] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL2);
    }    
		if(wavesSnap[D1])
		{
			val1 = (ch1Capture[j] & 0b00100000) ? dHeight/config.dsize[D1-1] : 0;
			val2 = (ch1Capture[jn] & 0b00100000) ? dHeight/config. dsize[D1-1] : 0;
			bitOld[i] &= 0b1101;
			bitOld[i] |= (ch1Capture[j]>>8) & 0b0010;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL1);
		}
		
		if(wavesSnap[A1])
		{
			val1 = (((ch1Capture[j] && 0x0FFF) - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
			val2 = (((ch1Capture[jn] && 0x0FFF) - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
			ch1Old[i] = (ch1Capture[j] && 0x0FFF) - zeroVoltageA1Snap;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[A1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[A1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, AN_SIGNAL1);
		}
	}
	
	// store the drawn parameters to old storage
	wavesOld[A1] = wavesSnap[A1];
	wavesOld[D1] = wavesSnap[D1];
	wavesOld[D2] = wavesSnap[D2];
    wavesOld[D3] = wavesSnap[D3];
  
	yCursorsOld[A1] = yCursorsSnap[A1];
	yCursorsOld[D1] = yCursorsSnap[D1];
	yCursorsOld[D2] = yCursorsSnap[D2];
    yCursorsOld[D3] = yCursorsSnap[D3];

  if ( config.persistence_on && fade_color_clear )
     fade_color_clear = false; 
}


// ------------------------
inline void plotLineSegment(int16_t transposedPt1, int16_t transposedPt2,  int index, uint16_t color)
// ------------------------
{
	// range checks
	if(transposedPt1 > (GRID_HEIGHT + vOffset-2))
		transposedPt1 = GRID_HEIGHT + vOffset-1;
	if(transposedPt1 < vOffset+1)
		transposedPt1 = vOffset+1;
	if(transposedPt2 > (GRID_HEIGHT + vOffset-2))
		transposedPt2 = GRID_HEIGHT + vOffset-2;
	if(transposedPt2 < vOffset+1)
		transposedPt2 = vOffset+1;
  
  // draw the line segments
  //Note.. If Pt1 < Pt2 here it leads to strange drawing artifacts where the 
  //verticval line sign seems to be flipped... Somewhere a bug in the graphics library...
  if (transposedPt1<transposedPt2)
    tft_drawLine(index + hOffset, transposedPt1, index + hOffset, transposedPt2, color);
  else  
	  tft_drawLine(index + hOffset, transposedPt2, index + hOffset, transposedPt1, color);
}


// ------------------------
void drawVCursor(int channel, uint16_t color, bool highlight)
// ------------------------
{
	int cPos = GRID_HEIGHT + vOffset + config.yCursors[channel];
    tft_fillTriangle(0, cPos - 5, hOffset, cPos, 0, cPos + 5, color);
	if(highlight)
		tft_drawRect(0, cPos - 7, hOffset, 14, ILI9341_WHITE);
}


// ------------------------
void drawGrid()
// ------------------------
{
	uint8_t hPacing = GRID_WIDTH / 12;
	uint8_t vPacing = GRID_HEIGHT / 8;

	for(int i = 1; i < 12; i++)
		tft_writeFastVLine(i * hPacing + hOffset, vOffset, GRID_HEIGHT, GRID_COLOR);

	for(int i = 1; i < 8; i++)
		tft_writeFastHLine(hOffset, i * vPacing + vOffset, GRID_WIDTH, GRID_COLOR);

	for(int i = 1; i < 2*8; i++)
		tft_writeFastHLine(hOffset + GRID_WIDTH/2 - 3, i * vPacing/2 + vOffset, 7, GRID_COLOR);

	for(int i = 1; i < 2*12; i++)
		tft_writeFastVLine(i * hPacing/2 + hOffset, vOffset + GRID_HEIGHT/2 - 4, 7, GRID_COLOR);

	tft_drawRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_WHITE);
}


// ------------------------
void drawLabels()
// ------------------------
{
	// draw the static labels around the grid
	// erase top/bottom bar
	tft_writeFillRect(hOffset, 0, TFT_WIDTH, vOffset, ILI9341_BLACK);
	tft_writeFillRect(hOffset + GRID_WIDTH, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);

	// paint run/hold information
	// -----------------
	tft_setCursor(hOffset + 2, 4);

	if(hold)
	{
		tft_setTextColor(ILI9341_WHITE, ILI9341_RED);
		tft_print((char*)"HOLD");
	}
	else
	{
		tft_setTextColor(ILI9341_GREEN, ILI9341_BLACK);
		tft_print((char*)"RUN");
	}

 //Draw Zoom Label
  tft_setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft_setCursor(65, 4);
  if(currentFocus == L_zoom)
    tft_drawRect(60, 0, 25, vOffset, ILI9341_WHITE);
  tft_print((char*)zoomNames[config.zoomFactor]);

	// draw x-window at top, range = 200px
	// -----------------
	int sampleSizePx = 120;
	float lOffset = (TFT_WIDTH - sampleSizePx)/2;
	tft_writeFastVLine(lOffset, 3, vOffset - 6, ILI9341_GREEN);
	tft_writeFastVLine(lOffset + sampleSizePx, 3, vOffset - 6, ILI9341_GREEN);
	tft_writeFastHLine(lOffset, vOffset/2, sampleSizePx, ILI9341_GREEN);

	// where does xCursor lie in this range
	float windowSize = GRID_WIDTH * sampleSizePx/(NUM_SAMPLES/xZoom);
	float xCursorPx = (config.xCursor/xZoom) * sampleSizePx/(NUM_SAMPLES/xZoom) + lOffset;
	if(currentFocus == L_window)
		tft_drawRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_WHITE);
	else
		tft_writeFillRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_GREEN);

	
	// print active wave indicators
	// -----------------
	tft_setCursor(248, 4);
	if(config.waves[A1])
	{
		tft_setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		tft_print((char*)"A1 ");
	}
	else
		tft_print((char*)"   ");

  tft_setCursor(266, 4);
	if(config.waves[D1])
	{
		tft_setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		tft_print((char*)"D1 ");
	}
	else
		tft_print((char*)"   ");

  tft_setCursor(284, 3);
	if(config.waves[D2])
	{
		tft_setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		tft_print((char*)"D2");
	}
  else
    tft_print((char*)"   ");

  tft_setCursor(302, 4);
  if(config.waves[D3])
  {
    tft_setTextColor(DG_SIGNAL3, ILI9341_BLACK);
    tft_print((char*)"D3");
  }
  
  if(currentFocus == L_waves)
		tft_drawRect(227, 0, 92, vOffset, ILI9341_WHITE);

	// erase left side of grid
  tft_writeFillRect(0, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);
	
	// draw new wave cursors
	// -----------------
  if(config.waves[D3])
    drawVCursor(3, DG_SIGNAL3, (currentFocus == L_vPos4));
  if(config.waves[D2])
	drawVCursor(2, DG_SIGNAL2, (currentFocus == L_vPos3));
  if(config.waves[D1])
	drawVCursor(1, DG_SIGNAL1, (currentFocus == L_vPos2));
  if(config.waves[A1])
	drawVCursor(0, AN_SIGNAL1, (currentFocus == L_vPos1));

  // erase bottom bar
  tft_writeFillRect(hOffset, GRID_HEIGHT + vOffset, TFT_WIDTH, vOffset, ILI9341_BLACK);

  // print input switch pos
  // -----------------
  tft_setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft_setCursor(hOffset + 10, GRID_HEIGHT + vOffset + 4);
  tft_print((char*)rngNames[rangePos]);
  if(currentFocus == L_voltagerange)
  tft_drawRect(hOffset + 5, GRID_HEIGHT + vOffset, 34, vOffset, ILI9341_WHITE);

  //Draw coupling
  tft_setCursor(hOffset + 42, GRID_HEIGHT + vOffset + 4);
  tft_print((char*)cplNames[couplingPos]);

  // print new timebase
  // -----------------
  tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft_setCursor(85, GRID_HEIGHT + vOffset + 4);
  if(currentFocus == L_timebase)
      tft_drawRect(80, GRID_HEIGHT + vOffset, 45, vOffset, ILI9341_WHITE);
  tft_print((char*)tbNames[config.currentTimeBase]);

  // print function
  if(currentFocus == L_function)
	  tft_drawRect(140, GRID_HEIGHT + vOffset, 50, vOffset, ILI9341_WHITE);
  tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft_setCursor(145, GRID_HEIGHT + vOffset + 4);
  tft_print((char*)functionNames[config.currentFunction]);

  // print trigger type
  if(currentFocus == L_triggerType)
      tft_drawRect(215, GRID_HEIGHT + vOffset, 35, vOffset, ILI9341_WHITE);
  tft_setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft_setCursor(220, GRID_HEIGHT + vOffset + 4);
  tft_print((char*)trigModeNames[config.triggerType]);

  //Draw trigger source
  if(currentFocus == L_triggerSource)
    tft_drawRect(250,  GRID_HEIGHT + vOffset, 20, vOffset, ILI9341_WHITE);
  tft_setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft_setCursor(255, GRID_HEIGHT + vOffset + 4);
  tft_print((char*)trigSourceNames[config.triggerSource]);

  // draw trigger edge
  // -----------------
  if(currentFocus == L_triggerEdge)
      tft_drawRect(276, GRID_HEIGHT + vOffset, 15, vOffset, ILI9341_WHITE);

  int trigX = 280;

  switch(config.triggerDir)
  {
      case TRIGGER_RISING:
    	 tft_writeFastHLine(trigX, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
         tft_writeFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
         tft_writeFastHLine(trigX + 4, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
         tft_fillTriangle(trigX + 2, 232, trigX + 4, 230, trigX + 6, 232, ILI9341_GREEN);
         break;
      case TRIGGER_FALLING:
    	 tft_writeFastHLine(trigX + 4, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
         tft_writeFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
         tft_writeFastHLine(trigX - 1, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
         tft_fillTriangle(trigX + 2, 231, trigX + 4, 233, trigX + 6, 231, ILI9341_GREEN);
         break;
      case TRIGGER_ALL:
    	 tft_writeFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
         tft_fillTriangle(trigX + 2, 228, trigX + 4, 226, trigX + 6, 228, ILI9341_GREEN);
         tft_fillTriangle(trigX + 2, 234, trigX + 4, 236, trigX + 6, 234, ILI9341_GREEN);
         break;          
  }
	
	// draw trigger level on right side
	// -----------------
  int cPos = ((getTriggerLevel() + 2048 - config.zeroVoltageA1) * GRID_HEIGHT)/ADC_2_GRID;
  cPos = GRID_HEIGHT + vOffset + config.yCursors[A1] - cPos;
  tft_fillTriangle(TFT_WIDTH, cPos - 5, TFT_WIDTH - hOffset, cPos, TFT_WIDTH, cPos + 5, AN_SIGNAL1);
	if(currentFocus == L_triggerLevel)
		tft_drawRect(GRID_WIDTH + hOffset, cPos - 7, hOffset, 14, ILI9341_WHITE);

}


// #define DRAW_TIMEBASE

// ------------------------
void drawStats()
// ------------------------
{
	static uint32_t lastCalcTime = 0;
	bool clearStats = false;
	
	// calculate stats once a while
	if(millis() - lastCalcTime > 300)	{
		lastCalcTime = millis();	
		calculateStats();
		clearStats = true;
	}
	// draw stat labels
	tft_setTextColor(ILI9341_RED, ILI9341_BLACK);

	tft_setCursor(25, 20);
	tft_print((char*)"Freq:");
	tft_setCursor(25, 30);
	tft_print((char*)"Cycle:");
	tft_setCursor(25, 40);
	tft_print((char*)"PW:");
	tft_setCursor(25, 50);
	tft_print((char*)"Duty:");

	tft_setCursor(230, 20);
	tft_print((char*)"Vmax:");
	tft_setCursor(230, 30);
	tft_print((char*)"Vmin:");
	tft_setCursor(230, 40);
	tft_print((char*)"Vavr:");
	tft_setCursor(230, 50);
	tft_print((char*)"Vpp:");
	tft_setCursor(230, 60);
	tft_print((char*)"Vrms:");
	
	// print new stats
	tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);

	if(clearStats)
		tft_writeFillRect(65, 20, 55, 50, ILI9341_BLACK);
	
	if(wStats.pulseValid)	{
		tft_setCursor(65, 20);
		tft_print((int) wStats.freq);tft_print((char*)" Hz");
		tft_setCursor(65, 30);
		tft_print(wStats.cycle); tft_print((char*)" ms");
		tft_setCursor(65, 40);
		tft_print((float)(wStats.avgPW/1000)); tft_print((char*)" ms");
		tft_setCursor(65, 50);
		tft_print(wStats.duty); tft_print((char*)" %");
	}
	
#ifdef DRAW_TIMEBASE
	tft_setCursor(65, 60);
	int timebase = ((double)samplingTime * 25) / NUM_SAMPLES;
	if(timebase > 10000)	{
		tft_print(timebase/1000); tft_print(" ms");
	}
	else	{
		tft_print(timebase); tft_print(" us");
	}
#endif
	
	if(clearStats)
		tft_writeFillRect(265, 20, GRID_WIDTH + hOffset - 265 - 1, 50, ILI9341_BLACK);
	
	drawVoltage(wStats.Vmaxf, 20, wStats.mvPos);
	drawVoltage(wStats.Vminf, 30, wStats.mvPos);
	drawVoltage(wStats.Vavrf, 40, wStats.mvPos);
	drawVoltage(wStats.Vmaxf - wStats.Vminf, 50, wStats.mvPos);
	drawVoltage(wStats.Vrmsf, 60, wStats.mvPos);
}


// ------------------------
void drawVoltageStat()
// ------------------------
{
  calculateSimpleStats();

  // print new stats
  drawVoltageLarge(wStats.Vavrf, 32,ILI9341_WHITE);
}

// ------------------------
void calculateStats()
// ------------------------
{
	// extract waveform stats
	int16_t Vmax = -ADC_MAX_VAL, Vmin = ADC_MAX_VAL;
	int32_t sumSamples = 0;
	int64_t sumSquares = 0;
	int32_t freqSumSamples = 0;
 int16_t val = 0;
 int i = 1;

  //Only look at visisble samples
  // sampling stopped at sIndex - 1
  int j = sIndex + config.xCursor;
  if(j >= NUM_SAMPLES)
    j = j - NUM_SAMPLES;
  
  // go through all the data points
  for(i = 1; i < GRID_WIDTH - 1; j++, i++)  
  {
    if(j == NUM_SAMPLES)
      j = 0;

		val = (ch1Capture[j] && 0x0FFF) - config.zeroVoltageA1;
		if(Vmax < val)
			Vmax = val;
		if(Vmin > val)
			Vmin = val;

		sumSamples += val;
		freqSumSamples += (ch1Capture[j]  && 0x0FFF);
		sumSquares += (val * val);
	}

	// find out frequency
	uint16_t fVavr = freqSumSamples/i;
	bool dnWave = ((ch1Capture[sIndex] && 0x0FFF) < fVavr - 10);
	bool firstOne = true;
	uint16_t cHigh = 0;

	uint16_t sumCW = 0;
	uint16_t sumPW = 0;
	uint16_t numCycles = 0;
	uint16_t numHCycles = 0;

  j = sIndex + config.xCursor;
  if(j >= NUM_SAMPLES)
    j = j - NUM_SAMPLES;
  
  // go through all the data points
  for(i = 1; i < GRID_WIDTH - 1; j++, i++)  
  {
    if(j == NUM_SAMPLES)
      j = 0;

		// mark the points where wave transitions the average value
		if(dnWave && ((ch1Capture[j] && 0x0FFF) > fVavr + 10))
		{
			if(!firstOne)
			{
				sumCW += (i - 1 - cHigh);
				numCycles++;
			}
			else
				firstOne = false;

			dnWave = false;
			cHigh = i-1;
		}

		if(!dnWave && ((ch1Capture[j] && 0x0FFF) < fVavr - 10))
		{
			if(!firstOne)
			{
				sumPW += ( i- 1 - cHigh);
				numHCycles++;
			}

			dnWave = true;
		}
	}

	double tPerSample = ((double)samplingTime) / NUM_SAMPLES;
	double avgCycleWidth = sumCW * tPerSample / numCycles;
	
	wStats.avgPW = sumPW * tPerSample / numHCycles;
	wStats.duty = wStats.avgPW * 100 / avgCycleWidth;
	wStats.freq = 1000000/avgCycleWidth;
	wStats.cycle = avgCycleWidth/1000;
	wStats.pulseValid = (avgCycleWidth != 0) && (wStats.avgPW != 0) && ((Vmax - Vmin) > 20);
	wStats.mvPos = (rangePos == RNG_50mV) || (rangePos == RNG_20mV) || (rangePos == RNG_10mV) || (rangePos == RNG_5mV);
	wStats.Vrmsf = sqrt(sumSquares/i) * config.adcMultiplier[rangePos];
	wStats.Vavrf = sumSamples/i * config.adcMultiplier[rangePos];
	wStats.Vmaxf = Vmax * config.adcMultiplier[rangePos];
	wStats.Vminf = Vmin * config.adcMultiplier[rangePos];
}

// ------------------------
//Only Calculate Vavg,Vmin,Vmax with running average of 10 samples

#define FLOAT_AVG_BUFSIZE 10
void calculateSimpleStats()
// ------------------------
{

  int16_t Vmax = -ADC_MAX_VAL, Vmin = ADC_MAX_VAL;
  int32_t sumSamples = 0;
  int16_t val = 0;
  int i = 0;
  static uint16_t sample_buf[FLOAT_AVG_BUFSIZE] = {0};
  static uint8_t arrayptr = 0;

  //dd last sample to array
  sample_buf[arrayptr] = (ch1Capture[sIndex] && 0x0FFF) - config.zeroVoltageA1;
  arrayptr++;
  if (arrayptr == FLOAT_AVG_BUFSIZE)
	  arrayptr = 0;
  
  //Calulate average

  // go through all the data points
  for(i = 1; i < FLOAT_AVG_BUFSIZE; i++)
  {
    val = sample_buf[i];
    if(Vmax < val)
      Vmax = val;
    if(Vmin > val)
      Vmin = val;
    sumSamples += val;
  }
  
  wStats.Vavrf = ((float)sumSamples/FLOAT_AVG_BUFSIZE) * config.adcMultiplier[rangePos];
  wStats.Vmaxf = (float)Vmax * config.adcMultiplier[rangePos];
  wStats.Vminf = (float)Vmin * config.adcMultiplier[rangePos];
}

// ------------------------
void drawVoltage(float volt, int y, bool mvRange)
// ------------------------
{
	// text is standard 5 px wide
	int numDigits = 1;
	int lVolt = volt;
	
	// is there a negative sign at front
	if(volt < 0)	
	{
		numDigits++;
		lVolt = -lVolt;
	}
	
	// how many digits before 0
	if(lVolt > 999)
		numDigits++;
	if(lVolt > 99)
		numDigits++;
	if(lVolt > 9)
		numDigits++;
	
	// mv range has mV appended at back
	if(mvRange)	
	{
		numDigits += 2;
		int x = GRID_WIDTH + hOffset - 15 - numDigits * 5;
		tft_setCursor(x, y);
		tft_print((int)(volt*1000.0));
		tft_print((char*)"mV");
	}
	else	
	{
		// non mV range has two decimal pos and V appended at back
		numDigits += 4;
		int x = GRID_WIDTH + hOffset - 15 - numDigits * 5;
		tft_setCursor(x, y);
		tft_print(volt);
        tft_print((char*)"V");
	}
}


void draw_markerstats(void)
{
	float v1;
	float v2;
	float vd;
	bool mvPos = (rangePos == RNG_50mV) || (rangePos == RNG_20mV) || (rangePos == RNG_10mV) || (rangePos == RNG_5mV);
	float t1;
	float t2;
	float td;

	int16_t temp;

	//Voltage values
	//--------------
	//Grid to ADC
	temp = (config.marker_y1 * ADC_2_GRID)/GRID_HEIGHT;
	//ADC to Voltage
	v1 = (float)(temp) * config.adcMultiplier[rangePos];

	//Grid to ADC
	temp = (config.marker_y2 * ADC_2_GRID)/GRID_HEIGHT;
	//ADC to Voltage
	v2 = (float)(temp) * config.adcMultiplier[rangePos];
	vd = v2 - v1;

	//Time Values
	//-----------

	//We calculate the offset from the center in ms....
	float tPerSample = ((float)samplingTime) / NUM_SAMPLES;
	temp = config.marker_x1 - (GRID_WIDTH/2);
	t1 = (float)(tPerSample * (float)temp * (float)xZoom)/1000;

	temp = config.marker_x2 - (GRID_WIDTH/2);
	t2 = (float)(tPerSample * (float)temp * (float)xZoom)/1000;
	td = t2 -t1;

	// draw stat labels
	tft_setTextColor(ILI9341_RED, ILI9341_BLACK);
	tft_setCursor(25, 20);
	tft_print((char*)"T1:");
	tft_setCursor(25, 30);
	tft_print((char*)"T2:");
	tft_setCursor(25, 40);
	tft_print((char*)"Td:");

	tft_setCursor(230, 20);
	tft_print((char*)"V1:");
	tft_setCursor(230, 30);
	tft_print((char*)"V2:");
	tft_setCursor(230, 40);
	tft_print((char*)"Vd:");

	//clear stats
	tft_writeFillRect(265, 20, GRID_WIDTH + hOffset - 265 - 1, 50, ILI9341_BLACK);
	tft_writeFillRect(52, 20, 50, 50, ILI9341_BLACK);

	// print new stats
	tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	drawVoltage(v1, 20, mvPos);
	drawVoltage(v2, 30, mvPos);
	drawVoltage(vd, 40, mvPos);


	tft_setCursor(52, 20);
	tft_print(t1);tft_print((char*)" ms");
	tft_setCursor(52, 30);
	tft_print(t2); tft_print((char*)" ms");
	tft_setCursor(52, 40);
	tft_print(td); tft_print((char*)" ms");

}



// ------------------------
void drawVoltageLarge(float volt, int y,uint16_t col)
// ------------------------
{
  // text is standard 20 px wide
  int numDigits = 1;
  int lVolt = volt;
  
  tft_setTextColor(col, ILI9341_BLACK);

  // is there a negative sign at front
  if(volt < 0)  
  {
    numDigits++;
    lVolt = -lVolt;
  }
  
  // how many digits before 0
  if(lVolt > 999)
    numDigits++;
  if(lVolt > 99)
    numDigits++;
  if(lVolt > 9)
    numDigits++;
    
  // mv range has mV appended at back

  if(lVolt<1.0) 
  {
    numDigits += 1;
    int x = GRID_WIDTH + hOffset - 90;
    tft_writeFillRect(x, y-12, GRID_WIDTH + hOffset - x - 1, 22, ILI9341_BLACK);
    tft_setCursor(x, y);
    tft_setFont(&FreeSansBold12pt7b);
    tft_print((int)(volt*1000.0));
    tft_print((char*)"mV");
    tft_setFont(NULL);
  }
  else  
  {
    // non mV range has two decimal pos and V appended at back
    numDigits += 4;
    int x = GRID_WIDTH + hOffset - 90;
    tft_writeFillRect(x, y-12, GRID_WIDTH + hOffset - x - 1, 22, ILI9341_BLACK);
    tft_setCursor(x, y);
    tft_setFont(&FreeSansBold12pt7b);
    tft_print(volt);
    tft_print((char*)"V");
    tft_setFont(NULL);
  }
}


// ------------------------
void clearStats()
// ------------------------
{
	tft_writeFillRect(hOffset, vOffset, GRID_WIDTH, 80, ILI9341_BLACK);
}


// ------------------------
void initDisplay()
// ------------------------
{

  //Contrary to the Schematic for the DSO-150 that lists a
  //S95417 display module which should contain according to the
  //datasheet an ILI9325 IT IS ACTUALLY A ILI9341
  tft_begin();

  tft_setRotation(LANDSCAPE);
  tft_fillScreen(ILI9341_BLACK);
  banner();

  delayMS(3000);

  // and paint o-scope
   clearWaves();
}


// ------------------------
void banner()
// ------------------------
{
tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
//tft_setTextSize(2);
tft_setFont(&FreeSansBold12pt7b);
tft_setCursor(90, 40);
tft_print((char*)FIRMWARE_TARGET);
tft_drawRect(80, 15, 170, 35, ILI9341_WHITE);
tft_setFont(NULL);
tft_setTextSize(1);
tft_setCursor(30, 60);
tft_print((char*)"Digital Storage Oscilloscope");

tft_setTextSize(1);
tft_setCursor(30, 80);
tft_printf("Storage Depth: %d Samples",(int16_t)NUM_SAMPLES);

tft_setCursor(30, 100);
tft_print((char*)"Usage: ");
tft_setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
tft_print((char*)"https://github.com/michar71/Open-DSO-150");

tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
tft_setCursor(30, 120);
tft_print((char*)FIRMWARE_INFO);

tft_setCursor(30, 140);
tft_print((char*)"Firmware version: ");
tft_print((char*)FIRMWARE_VERSION);

tft_setTextSize(1);
tft_setCursor(30, 210);
tft_print((char*)"GNU GENERAL PUBLIC LICENSE Version 3");
}



void drawtextbox(t_tb_data *pTextbox)
{

	  tft_drawRect(pTextbox->x, pTextbox->y, pTextbox->w, pTextbox->h, ILI9341_WHITE);
	  tft_writeFillRect(pTextbox->x+1, pTextbox->y+1, pTextbox->w-2, pTextbox->h-2, ILI9341_BLACK);
}



void showtextbox(t_tb_data *pTextbox,char* text)
{
	t_tb_data t;
	drawtextbox(pTextbox);
	tft_setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	t.x = pTextbox->x + 5;
	t.y = pTextbox->y + 18;
	t.w = pTextbox->w - 10;
	t.h = pTextbox->h - 10;
	tft_setFont(&FreeSans9pt7b);
	tft_box_print(&t,text);
	tft_setFont(NULL);
}

bool waitforOKorCancel()
{
	bool ok_but = false;
	bool trig_but = false;
	do
	{
		ok_but = !HAL_GPIO_ReadPin(DB7_GPIO_Port,DB7_Pin);
		trig_but = !HAL_GPIO_ReadPin(DB6_GPIO_Port,DB6_Pin);
	}
	while ((trig_but == true) || (ok_but == true));
	do
	{
		ok_but = !HAL_GPIO_ReadPin(DB7_GPIO_Port,DB7_Pin);
		trig_but = !HAL_GPIO_ReadPin(DB6_GPIO_Port,DB6_Pin);
	}
	while ((trig_but == false) && (ok_but == false));

	if (trig_but)
		return false;
	else
		return true;
}

