#include "Open-DSO-150.hpp"
#include "global.h"
#include "variables.h"
#include "stm32f1xx_hal.h"
#include "awrap.hpp"
#include "zconfig.hpp"
#include "interface.hpp"
#include "display.hpp"
#include "io.hpp"
#include "control.hpp"

// ------------------------
void dsosetup()
// ------------------------
{
	uint16_t disp;

	DBG_PRINT(FIRMWARE_TARGET);
	DBG_PRINT("Ver:%d.%d\n",(uint16_t)FIRMWARE_VERSION_MAJOR,(uint16_t)FIRMWARE_VERSION_MINOR);


	// set digital and analog stuff
	initIO();

	// load scope config or factory reset to defaults

  setPinMode(DB7_GPIO_Port,DB7_Pin,GPIO_MODE_INPUT,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);
  bool pinval = !HAL_GPIO_ReadPin(DB7_GPIO_Port, DB7_Pin);
  loadConfig(pinval);
  setPinMode(DB7_GPIO_Port,DB7_Pin,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_LOW);

  // init the IL9341 display
  disp = initDisplay();
  DBG_PRINT("Display ID: 0x%x\n",disp);
}


void dsoloop()
{
	controlLoop();
}
