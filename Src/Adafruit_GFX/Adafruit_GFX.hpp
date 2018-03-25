#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "gfxfont.h"
#include "../TFTLib/Adafruit_TFTLCD_8bit_STM32.hpp"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSans9pt7b.h"

//textbox structure
typedef struct tb_data{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
}t_tb_data;

void tft_begin(void);

// This MUST be defined by the subclass:
void tft_drawPixel(int16_t x, int16_t y, uint16_t color);

// TRANSACTION API / CORE DRAW API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
void tft_writePixel(int16_t x, int16_t y, uint16_t color);
void tft_writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void tft_writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void tft_writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void tft_writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

// CONTROL API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
void tft_setRotation(uint8_t r);
void tft_invertDisplay(bool i);


void tft_fillScreen(uint16_t color);
// Optional and probably not necessary to change
void tft_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void tft_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// These exist only with Adafruit_GFX (no subclass overrides)

void tft_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void tft_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,uint16_t color);
void tft_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void tft_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,int16_t delta, uint16_t color);
void tft_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint16_t color);
void tft_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint16_t color);
void tft_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,int16_t radius, uint16_t color);
void tft_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,int16_t radius, uint16_t color);
void tft_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,int16_t w, int16_t h, uint16_t color);
void tft_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,int16_t w, int16_t h, uint16_t color, uint16_t bg);
void tft_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,int16_t w, int16_t h, uint16_t color);
void tft_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,int16_t w, int16_t h, uint16_t color, uint16_t bg);
void tft_drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,int16_t w, int16_t h, uint16_t color);
void tft_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,uint16_t bg, uint8_t size);
void tft_setCursor(int16_t x, int16_t y);
void tft_setTextColor(uint16_t c);
void tft_setTextColor(uint16_t c, uint16_t bg);
void tft_setTextSize(uint8_t s);
void tft_setTextWrap(bool w);
void tft_setFont(const GFXfont *f = NULL);
void tft_getTextBounds(char *string, int16_t x, int16_t y,int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

void tft_write(uint8_t);
void tft_print(char* pStr);
void tft_print(int num);
void tft_print(float num);
void tft_printf(const char *fmt, ...);


int16_t tft_height(void);
int16_t tft_width(void);

uint8_t tft_getRotation(void);

// get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
int16_t tft_getCursorX(void);
int16_t tft_getCursorY(void);

void tft_box_write(t_tb_data *box,uint8_t c);
void tft_box_print(t_tb_data *box,char* pStr);

#endif // _ADAFRUIT_GFX_H
