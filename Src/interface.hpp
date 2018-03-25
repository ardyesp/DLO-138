#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

void resetParam();
void changeDSize(uint8_t wave);
void encoderChanged(int steps);
void incrementTLevel();
void decrementTLevel();
void incrementWaves();
void decrementWaves();
void incrementEdge();
void decrementEdge();
void nextTT();
void incrementTT();
void decrementTT();
void incrementZoom();
void decrementZoom();
void setZoomFactor(uint8_t zoomF);
void incrementTSource();
void decrementTSource();
void incrementFunc();
void decrementFunc();
void nextTimeBase();
void incrementTimeBase();
void decrementTimeBase();
void nextVoltageRange();
void incrementVoltageRange();
void decrementVoltageRange();
void setVoltageRange(uint8_t voltageRange);
void setTimeBase(uint8_t timeBase);
void toggleWave(uint8_t num);
void changeYCursor(uint8_t num, int16_t yPos);
void changeXCursor(int16_t xPos);
void setSamplingRate(uint8_t timeBase);
void setTriggerType(uint8_t tType);
void setTriggerDir(uint8_t tdir);
void setTriggerSource(uint8_t tsource);
void move_marker(uint8_t currentMarker,int8_t steps);

#endif
