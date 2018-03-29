#include "variables.h"
#include "control.hpp"
#include "display.hpp"
#include "io.hpp"
#include "encoder.hpp"
#include "capture.hpp"
#include "zconfig.hpp"

#define SAVE_COUNTER 60

extern t_config config;
extern volatile bool triggered;
extern volatile bool hold;
static uint32_t cnt = SAVE_COUNTER;
// ------------------------
void controlLoop()
// ------------------------
{
	// start by reading the state of analog system
	readInpSwitches();

    pollControlSwitches();

	if(config.triggerType == TRIGGER_AUTO)
	{
		captureDisplayCycle(true);
	}
	
	else if(config.triggerType == TRIGGER_NORM)
	{
		captureDisplayCycle(false);
	}
	else	
	{
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

		// freeze display
		while(hold)
		{
          if (pollControlSwitches())
          {
            clearWaves();
            repaintLabels();
            drawWaves();
          }
		}
   
		// update display indicating hold released
		drawLabels();
	}

	// process any long pending operations which cannot be serviced in ISR

	cnt--;
	if (cnt==0)
	{
	  autoSafe();
	  cnt = SAVE_COUNTER;
	}
}

// ------------------------
void captureDisplayCycle(bool wTimeOut)
// ------------------------
{
	indicateCapturing();
	// blocking call - until timeout or trigger
	sampleWaves(wTimeOut);
    pollControlSwitches();
  
	// draw the waveform
	indicateCapturingDone();
	drawWaves();
	// inter wait before next sampling
	if(triggered)
		blinkLED();
	
	if(hold)	
	{
      // update UI labels
      drawLabels();
	}
	
	// freeze display if requested
    while(hold)
    {
      if (pollControlSwitches())
      {        
         clearWaves();
         repaintLabels();
         drawWaves();
      }
    }
}
