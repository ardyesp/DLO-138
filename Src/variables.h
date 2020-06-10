// global capture variables

#ifndef _VARIABLES_H_
#define _VARIABLES_H_

#include <stdint.h>
#include "global.h"

enum {TRIGGER_RISING, TRIGGER_FALLING, TRIGGER_ALL};
enum { TRIGGER_AUTO,TRIGGER_NORM,TRIGGER_SINGLE};
enum {CPL_GND, CPL_AC, CPL_DC};
enum {ZOOM_X1, ZOOM_X2, ZOOM_X4,ZOOM_X8,ZOOM_X16};
enum {TRIGSRC_A1, TRIGSRC_D1, TRIGSRC_D2,TRIGSRC_D3};
enum {FUNC_SERIAL, FUNC_LOAD, FUNC_SAVE,FUNC_AUTOCAL};
enum {RNG_20V,RNG_10V,RNG_5V, RNG_2V, RNG_1V, RNG_0_5V, RNG_0_2V, RNG_0_1V, RNG_50mV, RNG_20mV, RNG_10mV,RNG_5mV};
enum {T20US,T30US,T50US,T0_1MS,T0_2MS,T0_5MS,T1MS,T2MS,T5MS,T10MS,T20MS,T50MS,LOOP};
const unsigned char tbBitval[] = {3,5,0,7,6,4,11,13,8,15,14,12};
enum {TESTMODE_GND,TESTMODE_PULSE,TESTMODE_3V3,TESTMODE_0V1};
enum {MARKER_X1,MARKER_X2,MARKER_Y1,MARKER_Y2};

typedef struct{
	uint8_t configID[4];   //Config ID has to be DSOS
	uint16_t configFWversion;
	uint8_t currentTimeBase;
	uint8_t currentVoltageRange;
	uint8_t triggerType;
	uint8_t triggerDir;
	int16_t xCursor;
	int16_t yCursors[4];
	bool waves[4];
	int16_t trigLevel;
	bool printStats;
	bool printVoltage;
	uint8_t zoomFactor;
	int16_t zeroVoltageA1;
	uint8_t dsize[3];
	uint8_t currentFunction;
	uint8_t triggerSource;
	uint8_t persistence_on;
	float adcMultiplier[RNG_5mV+1];
	int16_t zeroVoltageA1Cal[RNG_5mV+1];
	bool    markerMode;
	uint8_t currentMarker;
	int16_t marker_x1;
	int16_t marker_x2;
	int16_t marker_y1;
	int16_t marker_y2;

	int16_t old_x1_marker;
	int16_t old_x2_marker;
	int16_t old_y1_marker;
	int16_t old_y2_marker;
}t_config;



// waveform calculated statistics
typedef struct Stats {
	bool pulseValid;
	double avgPW;
	float duty;
	float freq;
	float cycle;
	
	bool mvPos;
	float Vrmsf;
	float Vavrf;
	float Vmaxf;
	float Vminf;
} t_Stats;


#endif




