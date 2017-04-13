#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include "src/TFTLib/Adafruit_TFTLCD_8bit_STM32.h"
#include "global.h"
#include "variables.h"

// ------------------------
void setup()	{
// ------------------------
	DBG_INIT(SERIAL_BAUD_RATE);
	DBG_PRINT("DSO Firmware ver: ");
	DBG_PRINTLN(FIRMWARE_VERSION);

  //Disable JTAG Debug Pins so PB can be used for display
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);

	// set digital and analog stuff
	initIO();

#ifdef DSO_150_EEPROM
  //Init external EEPROM
  initExtEEPROM();
#endif

	// load scope config or factory reset to defaults
#ifdef DSO_150
  pinMode(BTN4,INPUT);
#endif   
	loadConfig(digitalRead(BTN4) == LOW);
#ifdef DSO_150 
  pinMode(BTN4,OUTPUT);
#endif  
	// init the IL9341 display
	initDisplay();
}



// ------------------------
void loop()	{
// ------------------------
	controlLoop();
}


