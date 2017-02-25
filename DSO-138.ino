#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include "src/TFTLib/Adafruit_TFTLCD_8bit_STM32.h"
#include "global.h"
#include "variables.h"


// ------------------------
void setup()	{
// ------------------------
	DBG_INIT(SERIAL_BAUD_RATE);
	DBG_PRINTLN("Dual channel O Scope");

	// set digital and analog stuff
	initIO();
	
	// load scope config or factory reset to defaults
	loadConfig(digitalRead(BTN4) == LOW);
	
	// init the IL9341 display
	initDisplay();
}



// ------------------------
void loop()	{
// ------------------------
	controlLoop();
}


