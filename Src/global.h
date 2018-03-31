//Global Setup
//============

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdint.h>

#define FIRMWARE_VERSION "3.0"
#define FIRMWARE_TARGET "C++ DSO-150"
#define FIRMWARE_INFO "DSO-150 Hardware by JYE-Tech"

//General Defenitions
//===================

// analog and digital samples storage depth
// 
#define NUM_SAMPLES   3072 

// display colours
#define AN_SIGNAL1    ILI9341_YELLOW
#define DG_SIGNAL1    ILI9341_RED
#define DG_SIGNAL2    ILI9341_BLUE
#define DG_SIGNAL3    ILI9341_GREEN


// number of pixels waveform moves left/right or up/down
#define XCURSOR_STEP	25
#define YCURSOR_STEP	5
#define XCURSOR_STEP_COARSE  75
#define YCURSOR_STEP_COARSE  15

#define BTN_LONG_PRESS    700
#define BTN_DEBOUNCE_TIME 15

#define A1   0
#define D1   1
#define D2   2
#define D3   3

#define ANALOG_CHANNEL_COUNT 1
#define DIGITAL_CHANNEL_COUNT 3

#endif
