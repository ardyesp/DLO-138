
enum { TRIGGER_AUTO, TRIGGER_NORM, TRIGGER_SINGLE };
uint8_t triggerType;


// ------------------------
void setTriggerType(uint8_t tType)	{
// ------------------------
	triggerType = tType;
	// break any running capture loop
	keepSampling = false;
}




// ------------------------
void controlLoop()	{
// ------------------------
	// start by reading the state of analog system
	readInpSwitches();

	if(triggerType == TRIGGER_AUTO)	{
		captureDisplayCycle(true);
	}
	
	else if(triggerType == TRIGGER_NORM)	{
		captureDisplayCycle(false);
	}
	
	else	{
		// single trigger
		clearWaves();
		indicateCapturing();
		// blocking call - until trigger
		sampleWaves(false);
		indicateCapturingDone();
		hold = true;
		// request repainting of screen labels in next draw cycle
		repaintLabels();
		// draw the waveform
		drawWaves();
		blinkLED();
		// dump captured data on serial port
		dumpSamples();

		// freeze display
		while(hold);
		
		// update display indicating hold released
		drawLabels();
	}

	// process any long pending operations which cannot be serviced in ISR
}




// ------------------------
void captureDisplayCycle(boolean wTimeOut)	{
// ------------------------
	indicateCapturing();
	// blocking call - until timeout or trigger
	sampleWaves(wTimeOut);
	// draw the waveform
	indicateCapturingDone();
	drawWaves();
	// inter wait before next sampling
	if(triggered)
		blinkLED();
	
	if(hold)	{
		// update UI labels
		drawLabels();
		// dump captured data on serial port
		dumpSamples();
	}
	
	// freeze display if requested
	while(hold);
}
