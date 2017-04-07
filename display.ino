
// TFT display constants
#define PORTRAIT 		0
#define LANDSCAPE 		1

#define TFT_WIDTH		  ((uint16_t)320)
#define TFT_HEIGHT		((uint16_t)240)
#define GRID_WIDTH		((uint16_t)300)
#define GRID_HEIGHT		((uint16_t)210)

#define GRID_COLOR		0x4208
#define ADC_MAX_VAL		4096
#define ADC_2_GRID		800


Adafruit_TFTLCD_8bit_STM32 tft;

// rendered waveform data is stored here for erasing
int16_t ch1Old[GRID_WIDTH] = {0};
int16_t ch2Old[GRID_WIDTH] = {0};
int8_t bitOld[GRID_WIDTH] = {0};

// grid variables
uint8_t hOffset = (TFT_WIDTH - GRID_WIDTH)/2;
uint8_t vOffset = (TFT_HEIGHT - GRID_HEIGHT)/2;
uint8_t dHeight = GRID_HEIGHT/8;

// plot variables -- modified by interface section
// controls which section of waveform is displayed on screen
// 0 < xCursor < (NUM_SAMPLES - GRID_WIDTH)
int16_t xCursor;
// controls the vertical positioning of waveform
int16_t yCursors[5];
// controls which waveforms are displayed
boolean waves[5];

// prints waveform statistics on screen
boolean printStats = true;
// repaint the labels on screen in draw loop
boolean paintLabels = false;

// labels around the grid
enum {L_voltagerange,L_timebase,L_function, L_triggerType, L_triggerEdge, L_triggerLevel, L_waves, L_window, L_vPos1, L_vPos2, L_vPos3, L_vPos4,L_vPos5};
uint8_t currentFocus = L_timebase;


void setFocusLabel(uint8_t label)
{
  currentFocus = label;
  repaintLabels();
}

// ------------------------
void focusNextLabel()	{
// ------------------------
	currentFocus++;

  //TODO limit this according to number of channels... 
	if((currentFocus == L_vPos1) && !waves[A1])
		currentFocus++;

	if((currentFocus == L_vPos2) && !waves[A2])
		currentFocus++;

	if((currentFocus == L_vPos3) && !waves[D1])
		currentFocus++;
   
	if((currentFocus == L_vPos4) && !waves[D2])
		currentFocus++;

  if((currentFocus == L_vPos5) && !waves[D3])
    currentFocus++;

#ifdef DSO_150
  if(currentFocus > L_vPos5)
    currentFocus = L_voltagerange;
#else    
	if(currentFocus > L_vPos5)
		currentFocus = L_timebase;
#endif    
}


// ------------------------
void repaintLabels()	{
// ------------------------
	paintLabels = true;
}


// ------------------------
void initDisplay()	{
// ------------------------
  tft.reset();

  //Contrary to the Schematic for the DSO-150 that lists a 
  //S95417 display module which should contain according to the 
  //datasheet an ILI9325 IT IS ACTUALLY A ILI9341 
	tft.begin(0x9341);

	tft.setRotation(LANDSCAPE);
	tft.fillScreen(ILI9341_BLACK);
	banner();

	delay(4000);

	// and paint o-scope
	clearWaves();
}


// ------------------------
void drawWaves()	{
// ------------------------
	static boolean printStatsOld = false;

	if(printStatsOld && !printStats)
		clearStats();
	
	printStatsOld = printStats;

  // draw the grid
  drawGrid();
	
	// clear and draw signal traces
	clearNDrawSignals();
	
	// if requested update the stats
	if(printStats)
		drawStats();

	// if label repaint requested - do so now
	if(paintLabels)	{
		drawLabels();
		paintLabels = false;
	}
}

// ------------------------
void clearWaves()	{

	// clear screen
	tft.fillScreen(ILI9341_BLACK);
	// and paint o-scope
	drawGrid();
	drawLabels();
}


boolean cDisplayed = false;

// ------------------------
void indicateCapturing()	{
// ------------------------
	if((currentTimeBase > T2MS) || (triggerType != TRIGGER_AUTO))	{
		cDisplayed = true;
  
		tft.setTextColor(ILI9341_PINK, ILI9341_BLACK);
		tft.setCursor(140, 20);
		tft.print("Sampling...");
	}
}


// ------------------------
void indicateCapturingDone()	{
// ------------------------
	if(cDisplayed)	{
		tft.fillRect(140, 20, 66, 8, ILI9341_BLACK);
		cDisplayed = false;
	}
}


// local operations below


// 0, 1 Analog channels. 2, 3 ,4 digital channels
// ------------------------
void clearNDrawSignals()	{
// ------------------------
	static boolean wavesOld[5] = {false,};
	static int16_t yCursorsOld[5];
	
	// snap the values to prevent interrupt from changing mid-draw
	int16_t xCursorSnap = xCursor;
	int16_t zeroVoltageA1Snap = zeroVoltageA1;
	int16_t zeroVoltageA2Snap = zeroVoltageA2;
	int16_t yCursorsSnap[5];
	boolean wavesSnap[5];

	yCursorsSnap[A1] = yCursors[A1];
	yCursorsSnap[A2] = yCursors[A2];
	yCursorsSnap[D1] = yCursors[D1];
	yCursorsSnap[D2] = yCursors[D2];
  yCursorsSnap[D3] = yCursors[D3];
	wavesSnap[A1] = waves[A1];
	wavesSnap[A2] = waves[A2];
	wavesSnap[D1] = waves[D1];
	wavesSnap[D2] = waves[D2];
  wavesSnap[D3] = waves[D3];

	// draw the GRID_WIDTH section of the waveform from xCursorSnap
	int16_t val1, val2;
	int16_t transposedPt1, transposedPt2;
	uint8_t shiftedVal;

	// sampling stopped at sIndex - 1
	int j = sIndex + xCursorSnap;
	if(j >= NUM_SAMPLES)
		j = j - NUM_SAMPLES;
	
	// go through all the data points
	for(int i = 1, jn = j + 1; i < GRID_WIDTH - 1; j++, i++, jn++)	
	{
		if(jn == NUM_SAMPLES)
			jn = 0;

		if(j == NUM_SAMPLES)
			j = 0;

		// erase old line segment
#ifdef ADD_D3   
    if(wavesOld[D3]) {
      val1 = (bitOld[i] & 0b10000000) ? dHeight/dsize[D3-2] : 0;
      val2 = (bitOld[i + 1] & 0b10000000) ? dHeight/dsize[D3-2] : 0;
      // clear the line segment
      transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D3] - val1;
      transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D3] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
    }
#endif


		if(wavesOld[D2])	{
			val1 = (bitOld[i] & 0b01000000) ? dHeight/dsize[D2-2] : 0;
			val2 = (bitOld[i + 1] & 0b01000000) ? dHeight/dsize[2-2] : 0;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D2] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D2] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
		}
		if(wavesOld[D1])	{
			val1 = (bitOld[i] & 0b00100000) ? dHeight/dsize[D1-2] : 0;
			val2 = (bitOld[i + 1] & 0b00100000) ? dHeight/dsize[D1-2] : 0;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[D1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[D1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
		}

#ifdef ADD_AN2 
		if(wavesOld[A2])	{
			val1 = (ch2Old[i] * GRID_HEIGHT)/ADC_2_GRID;
			val2 = (ch2Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[A2] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[A2] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
		}
#endif

		if(wavesOld[A1])	{
			val1 = (ch1Old[i] * GRID_HEIGHT)/ADC_2_GRID;
			val2 = (ch1Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
			// clear the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[A1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[A1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
		}
  	
		// draw new segments
#ifdef ADD_D3  
		if(wavesSnap[D3])	{
			shiftedVal = bitStore[j] >> 8;
			val1 = (shiftedVal & 0b10000000) ? dHeight/dsize[D3-2] : 0;
			val2 = ((bitStore[jn] >> 8) & 0b10000000) ? dHeight/dsize[D3-2] : 0;
			bitOld[i] &= 0b01111111;
			bitOld[i] |= shiftedVal & 0b10000000;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D3] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D3] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL3);
		}
#endif

    if(wavesSnap[D2])  {
      shiftedVal = bitStore[j] >> 8;
      val1 = (shiftedVal & 0b01000000) ? dHeight/dsize[D2-2] : 0;
      val2 = ((bitStore[jn] >> 8) & 0b01000000) ? dHeight/dsize[D2-2] : 0;
      bitOld[i] &= 0b10111111;
      bitOld[i] |= shiftedVal & 0b01000000;
      // draw the line segment
      transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D2] - val1;
      transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D2] - val2;
      plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL2);
    }    
		if(wavesSnap[D1])	{
			shiftedVal = bitStore[j] >> 8;
			val1 = (shiftedVal & 0b00100000) ? dHeight/dsize[D1-2] : 0;
			val2 = ((bitStore[jn] >> 8) & 0b00100000) ? dHeight/dsize[D1-2] : 0;
			bitOld[i] &= 0b11011111;
			bitOld[i] |= shiftedVal & 0b00100000;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[D1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[D1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL1);
		}

#ifdef ADD_AN2
		if(wavesSnap[A2])	{
			val1 = ((ch2Capture[j] - zeroVoltageA2Snap) * GRID_HEIGHT)/ADC_2_GRID;
			val2 = ((ch2Capture[jn] - zeroVoltageA2Snap) * GRID_HEIGHT)/ADC_2_GRID;
			ch2Old[i] = ch2Capture[j] - zeroVoltageA2Snap;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[A2] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[A2] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, AN_SIGNAL2);
		}
#endif
		
		if(wavesSnap[A1])	{
			val1 = ((ch1Capture[j] - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
			val2 = ((ch1Capture[jn] - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
			ch1Old[i] = ch1Capture[j] - zeroVoltageA1Snap;
			// draw the line segment
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[A1] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[A1] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, AN_SIGNAL1);
		}
	}
	
	// store the drawn parameters to old storage
	wavesOld[A1] = wavesSnap[A1];
	wavesOld[A2] = wavesSnap[A2];
	wavesOld[D1] = wavesSnap[D1];
	wavesOld[D2] = wavesSnap[D2];
  wavesOld[D3] = wavesSnap[D3];
  
	yCursorsOld[A1] = yCursorsSnap[A1];
	yCursorsOld[A2] = yCursorsSnap[A2];
	yCursorsOld[D1] = yCursorsSnap[D1];
	yCursorsOld[D2] = yCursorsSnap[D2];
  yCursorsOld[D3] = yCursorsSnap[D3];
}


// ------------------------
inline void plotLineSegment(int16_t transposedPt1, int16_t transposedPt2,  int index, uint16_t color)	{
// ------------------------
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
    tft.drawLine(index + hOffset, transposedPt1, index + hOffset, transposedPt2, color);
  else  
	  tft.drawLine(index + hOffset, transposedPt2, index + hOffset, transposedPt1, color);
}


// ------------------------
void drawVCursor(int channel, uint16_t color, boolean highlight)	{
// ------------------------
	int cPos = GRID_HEIGHT + vOffset + yCursors[channel];
    tft.fillTriangle(0, cPos - 5, hOffset, cPos, 0, cPos + 5, color);
	if(highlight)
		tft.drawRect(0, cPos - 7, hOffset, 14, ILI9341_WHITE);
}


// ------------------------
void drawGrid()	{
// ------------------------
	uint8_t hPacing = GRID_WIDTH / 12;
	uint8_t vPacing = GRID_HEIGHT / 8;

	for(int i = 1; i < 12; i++)
		tft.drawFastVLine(i * hPacing + hOffset, vOffset, GRID_HEIGHT, GRID_COLOR);

	for(int i = 1; i < 8; i++)
		tft.drawFastHLine(hOffset, i * vPacing + vOffset, GRID_WIDTH, GRID_COLOR);

	for(int i = 1; i < 2*8; i++)
		tft.drawFastHLine(hOffset + GRID_WIDTH/2 - 3, i * vPacing/2 + vOffset, 7, GRID_COLOR);

	for(int i = 1; i < 2*12; i++)
		tft.drawFastVLine(i * hPacing/2 + hOffset, vOffset + GRID_HEIGHT/2 - 4, 7, GRID_COLOR);

	tft.drawRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_WHITE);
}


// ------------------------
void drawLabels()	{
// ------------------------
	// draw the static labels around the grid
	// erase top/bottom bar
	tft.fillRect(hOffset, 0, TFT_WIDTH, vOffset, ILI9341_BLACK);
	tft.fillRect(hOffset + GRID_WIDTH, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);

	// paint run/hold information
	// -----------------
	tft.setCursor(hOffset + 2, 4);

	if(hold)	{
		tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
		tft.print(" HOLD ");
	}
	else	{
		tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
		tft.print("RUN");
	}

	// draw x-window at top, range = 200px
	// -----------------
	int sampleSizePx = 120;
	float lOffset = (TFT_WIDTH - sampleSizePx)/2;
	tft.drawFastVLine(lOffset, 3, vOffset - 6, ILI9341_GREEN);
	tft.drawFastVLine(lOffset + sampleSizePx, 3, vOffset - 6, ILI9341_GREEN);
	tft.drawFastHLine(lOffset, vOffset/2, sampleSizePx, ILI9341_GREEN);

	// where does xCursor lie in this range
	float windowSize = GRID_WIDTH * sampleSizePx/NUM_SAMPLES;
	float xCursorPx =  xCursor * sampleSizePx/NUM_SAMPLES + lOffset;
	if(currentFocus == L_window)
		tft.drawRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_WHITE);
	else
		tft.fillRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_GREEN);

	
	// print active wave indicators
	// -----------------
	tft.setCursor(230, 4);
	if(waves[A1])	{
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		tft.print("A1 ");
	}
	else
		tft.print("   ");

  tft.setCursor(248, 4);
	if(waves[A2])	{
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		tft.print("A2 ");
	}
	else
		tft.print("   ");

  tft.setCursor(266, 4);
	if(waves[D1])	{
		tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		tft.print("D1 ");
	}
	else
		tft.print("   ");

  tft.setCursor(284, 3);
	if(waves[D2])	{
		tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		tft.print("D2");
	}
  else
    tft.print("   ");

  tft.setCursor(302, 4);
  if(waves[D3]) {
    tft.setTextColor(DG_SIGNAL3, ILI9341_BLACK);
    tft.print("D3");
  }
  
  if(currentFocus == L_waves)
		tft.drawRect(227, 0, 92, vOffset, ILI9341_WHITE);

	// erase left side of grid
	tft.fillRect(0, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);
	
	// draw new wave cursors
	// -----------------
  if(waves[D3])
    drawVCursor(4, DG_SIGNAL3, (currentFocus == L_vPos5));
	if(waves[D2])
		drawVCursor(3, DG_SIGNAL2, (currentFocus == L_vPos4));
	if(waves[D1])
		drawVCursor(2, DG_SIGNAL1, (currentFocus == L_vPos3));
	if(waves[A2])
		drawVCursor(1, AN_SIGNAL2, (currentFocus == L_vPos2));
	if(waves[A1])
		drawVCursor(0, AN_SIGNAL1, (currentFocus == L_vPos1));

	// erase bottom bar
	tft.fillRect(hOffset, GRID_HEIGHT + vOffset, TFT_WIDTH, vOffset, ILI9341_BLACK);

	// print input switch pos
	// -----------------
	tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
	tft.setCursor(hOffset + 10, GRID_HEIGHT + vOffset + 4);
	tft.print(rngNames[rangePos]);
  #ifdef DSO_150
  if(currentFocus == L_voltagerange)
    tft.drawRect(hOffset + 5, GRID_HEIGHT + vOffset, 34, vOffset, ILI9341_WHITE);
  #endif
    
  //Draw coupling
	tft.setCursor(hOffset + 42, GRID_HEIGHT + vOffset + 4);
	tft.print(cplNames[couplingPos]);
	
	// print new timebase
	// -----------------
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setCursor(85, GRID_HEIGHT + vOffset + 4);
	if(currentFocus == L_timebase)
		tft.drawRect(80, GRID_HEIGHT + vOffset, 45, vOffset, ILI9341_WHITE);
	tft.print(getTimebaseLabel());

  // print function
  // -----------------
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(145, GRID_HEIGHT + vOffset + 4);
  if(currentFocus == L_function)
    tft.drawRect(140, GRID_HEIGHT + vOffset, 55, vOffset, ILI9341_WHITE);
  tft.print(functionNames[currentFunction]);
  
	// print trigger type
	// -----------------
	tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
	tft.setCursor(230, GRID_HEIGHT + vOffset + 4);
	if(currentFocus == L_triggerType)
		tft.drawRect(225, GRID_HEIGHT + vOffset, 35, vOffset, ILI9341_WHITE);

	switch(triggerType)	{
		case TRIGGER_AUTO:
			tft.print("AUTO");
			break;
		case TRIGGER_NORM:
			tft.print("NORM");
			break;
		case TRIGGER_SINGLE:
			tft.print("SING");
			break;
	}

	// draw trigger edge
	// -----------------
	if(currentFocus == L_triggerEdge)
		tft.drawRect(266, GRID_HEIGHT + vOffset, 15, vOffset + 4, ILI9341_WHITE);

	int trigX = 270;
	
	if(triggerRising)	{
		tft.drawFastHLine(trigX, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
		tft.drawFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
		tft.drawFastHLine(trigX + 4, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
		tft.fillTriangle(trigX + 2, 232, trigX + 4, 230, trigX + 6, 232, ILI9341_GREEN);
	}
	else	{
		tft.drawFastHLine(trigX + 4, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
		tft.drawFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
		tft.drawFastHLine(trigX - 1, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
		tft.fillTriangle(trigX + 2, 231, trigX + 4, 233, trigX + 6, 231, ILI9341_GREEN);
	}	
	
	
	// draw trigger level on right side
	// -----------------
//	int cPos = GRID_HEIGHT + vOffset + yCursors[0] - getTriggerLevel()/3;
  int cPos = GRID_HEIGHT + yCursors[0] - getTriggerLevel()/3; 
  tft.fillTriangle(TFT_WIDTH, cPos - 5, TFT_WIDTH - hOffset, cPos, TFT_WIDTH, cPos + 5, AN_SIGNAL1);
	if(currentFocus == L_triggerLevel)
		tft.drawRect(GRID_WIDTH + hOffset, cPos - 7, hOffset, 14, ILI9341_WHITE);

}


// #define DRAW_TIMEBASE

// ------------------------
void drawStats()	{
// ------------------------
	static long lastCalcTime = 0;
	boolean clearStats = false;
	
	// calculate stats once a while
	if(millis() - lastCalcTime > 300)	{
		lastCalcTime = millis();	
		calculateStats();
		clearStats = true;
	}
	// draw stat labels
	tft.setTextColor(ILI9341_RED, ILI9341_BLACK);

	tft.setCursor(25, 20);
	tft.print("Freq:");
	tft.setCursor(25, 30);
	tft.print("Cycle:");
	tft.setCursor(25, 40);
	tft.print("PW:");
	tft.setCursor(25, 50);
	tft.print("Duty:");
#ifdef DRAW_TIMEBASE
	tft.setCursor(25, 60);
	tft.print("T/div:");
#endif

	tft.setCursor(240, 20);
	tft.print("Vmax:");
	tft.setCursor(240, 30);
	tft.print("Vmin:");
	tft.setCursor(240, 40);
	tft.print("Vavr:");
	tft.setCursor(240, 50);
	tft.print("Vpp:");
	tft.setCursor(240, 60);
	tft.print("Vrms:");
	
	// print new stats
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

	if(clearStats)
		tft.fillRect(60, 20, 50, 50, ILI9341_BLACK);
	
	if(wStats.pulseValid)	{
		tft.setCursor(60, 20);
		tft.print((int) wStats.freq);tft.print(" Hz");
		tft.setCursor(60, 30);
		tft.print(wStats.cycle); tft.print(" ms");
		tft.setCursor(60, 40);
		tft.print(wStats.avgPW/1000); tft.print(" ms");
		tft.setCursor(60, 50);
		tft.print(wStats.duty); tft.print(" %");
	}
	
#ifdef DRAW_TIMEBASE
	tft.setCursor(60, 60);
	int timebase = ((double)samplingTime * 25) / NUM_SAMPLES;
	if(timebase > 10000)	{
		tft.print(timebase/1000); tft.print(" ms");
	}
	else	{
		tft.print(timebase); tft.print(" us");
	}
#endif
	
	if(clearStats)
		tft.fillRect(270, 20, GRID_WIDTH + hOffset - 270 - 1, 50, ILI9341_BLACK);
	
	drawVoltage(wStats.Vmaxf, 20, wStats.mvPos);
	drawVoltage(wStats.Vminf, 30, wStats.mvPos);
	drawVoltage(wStats.Vavrf, 40, wStats.mvPos);
	drawVoltage(wStats.Vmaxf - wStats.Vminf, 50, wStats.mvPos);
	drawVoltage(wStats.Vrmsf, 60, wStats.mvPos);
}

// ------------------------
void calculateStats()	{
// ------------------------
	// extract waveform stats
	int16_t Vmax = -ADC_MAX_VAL, Vmin = ADC_MAX_VAL;
	int32_t sumSamples = 0;
	int64_t sumSquares = 0;
	int32_t freqSumSamples = 0;
 int16_t val = 0;
 int i = 1;

  //Only look at visisble samples
  // sampling stopped at sIndex - 1
  int j = sIndex + xCursor;
  if(j >= NUM_SAMPLES)
    j = j - NUM_SAMPLES;
  
  // go through all the data points
  for(i = 1; i < GRID_WIDTH - 1; j++, i++)  
  {
    if(j == NUM_SAMPLES)
      j = 0;

		val = ch1Capture[j] - zeroVoltageA1;
		if(Vmax < val)
			Vmax = val;
		if(Vmin > val)
			Vmin = val;

		sumSamples += val;
		freqSumSamples += ch1Capture[j];
		sumSquares += (val * val);
	}

	// find out frequency
	uint16_t fVavr = freqSumSamples/i;
	boolean dnWave = (ch1Capture[sIndex] < fVavr - 10);
	boolean firstOne = true;
	uint16_t cHigh = 0;

	uint16_t sumCW = 0;
	uint16_t sumPW = 0;
	uint16_t numCycles = 0;
	uint16_t numHCycles = 0;

  j = sIndex + xCursor;
  if(j >= NUM_SAMPLES)
    j = j - NUM_SAMPLES;
  
  // go through all the data points
  for(i = 1; i < GRID_WIDTH - 1; j++, i++)  
  {
    if(j == NUM_SAMPLES)
      j = 0;

		// mark the points where wave transitions the average value
		if(dnWave && (ch1Capture[j] > fVavr + 10))	{
			if(!firstOne)	{
				sumCW += (i - 1 - cHigh);
				numCycles++;
			}
			else
				firstOne = false;

			dnWave = false;
			cHigh = i-1;
		}

		if(!dnWave && (ch1Capture[j] < fVavr - 10))	{
			if(!firstOne)	{
				sumPW += ( i- 1 - cHigh);
				numHCycles++;
			}

			dnWave = true;
		}
	}

	double tPerSample = ((double)samplingTime) / NUM_SAMPLES;
	float timePerDiv = tPerSample * 25;
	double avgCycleWidth = sumCW * tPerSample / numCycles;
	
	wStats.avgPW = sumPW * tPerSample / numHCycles;
	wStats.duty = wStats.avgPW * 100 / avgCycleWidth;
	wStats.freq = 1000000/avgCycleWidth;
	wStats.cycle = avgCycleWidth/1000;
	wStats.pulseValid = (avgCycleWidth != 0) && (wStats.avgPW != 0) && ((Vmax - Vmin) > 20);
	
	wStats.mvPos = (rangePos == RNG_50mV) || (rangePos == RNG_20mV) || (rangePos == RNG_10mV) || (rangePos == RNG_5mV);
	wStats.Vrmsf = sqrt(sumSquares/i) * adcMultiplier[rangePos];
	wStats.Vavrf = sumSamples/i * adcMultiplier[rangePos];
	wStats.Vmaxf = Vmax * adcMultiplier[rangePos];
	wStats.Vminf = Vmin * adcMultiplier[rangePos]; 
}

// ------------------------
void drawVoltage(float volt, int y, boolean mvRange)	{
// ------------------------
	// text is standard 5 px wide
	int numDigits = 1;
	int lVolt = volt;
	
	// is there a negative sign at front
	if(volt < 0)	{
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
	if(mvRange)	{
		numDigits += 1;
		int x = GRID_WIDTH + hOffset - 10 - numDigits * 5;
		tft.setCursor(x, y);
		int iVolt = volt;
		tft.print(iVolt);
		tft.print("m");
	}
	else	{
		// non mV range has two decimal pos and V appended at back
		numDigits += 3;
		int x = GRID_WIDTH + hOffset -10 - numDigits * 5;
		tft.setCursor(x, y);
		tft.print(volt);
	}
}


// ------------------------
void clearStats()	{
// ------------------------
	tft.fillRect(hOffset, vOffset, GRID_WIDTH, 80, ILI9341_BLACK);
}


// ------------------------
void banner() {
// ------------------------
tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
tft.setTextSize(2);
tft.setCursor(110, 20);
tft.print(FIRMWARE_TARGET);
tft.drawRect(100, 15, 100, 25, ILI9341_WHITE);

tft.setTextSize(1);
tft.setCursor(30, 60);
tft.print("Digital Stroage Oscilloscope");

tft.setTextSize(1);
tft.setCursor(30, 90);
tft.print("Analog Channels: ");
tft.print(ANALOG_CHANNEL_COUNT);

if (DIGITAL_CHANNEL_COUNT > 0)
{
tft.setCursor(30, 110);
tft.print("Digital Channels: ");
tft.print(DIGITAL_CHANNEL_COUNT); 
}

tft.setCursor(30, 130);
tft.print("Storage Depth:");
tft.print(NUM_SAMPLES);

tft.setCursor(30, 150);
tft.print("Usage: ");
tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
tft.print("https://github.com/ardyesp/DLO-138");

tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
tft.setCursor(30, 170);
tft.print(FIRMWARE_INFO);

tft.setCursor(30, 190);
tft.print("Firmware version: ");
tft.print(FIRMWARE_VERSION);

tft.setTextSize(1);
tft.setCursor(30, 210);
tft.print("GNU GENERAL PUBLIC LICENSE Version 3");
}


