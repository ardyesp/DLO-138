#ifndef DISPLAY_H
#define DISPLAY_H

#include "Adafruit_GFX/Adafruit_GFX.hpp"
#include "global.h"


// TFT display constants
#define PORTRAIT 		0
#define LANDSCAPE 		1

#define TFT_WIDTH   	((uint16_t)320)
#define TFT_HEIGHT		((uint16_t)240)
#define GRID_WIDTH		((uint16_t)300)
#define GRID_HEIGHT		((uint16_t)210)

#define GRID_COLOR		0x4208
#define ADC_MAX_VAL		4096
#define ADC_2_GRID		800



// labels around the grid
enum {L_voltagerange,L_timebase,L_function, L_triggerType, L_triggerSource, L_triggerEdge, L_triggerLevel, L_waves, L_window,L_zoom, L_vPos1, L_vPos2, L_vPos3, L_vPos4};

void setFocusLabel(uint8_t label);
void focusNextLabel();
void repaintLabels();
uint16_t initDisplay();
void draw_markers();
void drawWaves();
void clearWaves();

void setTriggerTimer(void);
uint16_t fade_color(uint16_t color);
void clearPersistence();
void setPersistence(uint8_t pers);
void togglePersistence();

void indicateCapturing();
void indicateCapturingDone();
void clearNDrawSignals();
void plotLineSegment(int16_t transposedPt1, int16_t transposedPt2,  int index, uint16_t color);
void drawVCursor(int channel, uint16_t color, bool highlight);
void drawGrid();
void drawLabels();
void draw_markerstats(void);


void drawStats();
void drawVoltageStat();
void calculateStats();
void calculateSimpleStats();
void drawVoltage(float volt, int y, bool mvRange);
void drawVoltageLarge(float volt, int y,uint16_t col);
void clearStats();

void banner();
void drawtextbox(t_tb_data *pTextbox);
void showtextbox(t_tb_data *pTextbox,char* text);
bool waitforOKorCancel();

#endif
