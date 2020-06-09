// IMPORTANT: SEE COMMENTS @ LINE 15 REGARDING SHIELD VS BREAKOUT BOARD USAGE.

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#ifndef _ADAFRUIT_TFTLCD_8BIT_STM32_H_
#define _ADAFRUIT_TFTLCD_8BIT_STM32_H_

#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))


#include <stdint.h>
#include "../Adafruit_GFX/Adafruit_GFX.hpp"
#include "stm32f1xx_hal.h"
#include "../awrap.hpp"

/*****************************************************************************/
#define TFTWIDTH   320
#define TFTHEIGHT  320		// HACK: Should be 240, but text print gets clipped at 240 width in landscape mode
#define TFTHEIGHTREAL 240
// Initialization command tables for different LCD controllers
#define TFTLCD_DELAY 0xFF

// For compatibility with sketches written for older versions of library.
// Color function name was changed to 'color565' for parity with 2.2" LCD
// library.
#define Color565 color565

/*****************************************************************************/
// Define pins and Output Data Registers
/*****************************************************************************/

//static unsigned int intReg;

#define TFT_DATA       GPIOB
// Port data bits D0..D7:
// enable only one from below lines corresponding to your HW setup:
#define TFT_DATA_NIBBLE	0 // take the lower 8 bits: 0..7
//#define TFT_DATA_NIBBLE	8 // take the higher 8 bits: 8..15

//Control pins |RD |WR |RS |CS |RST|
//#define TFT_RD         PA6
//#define TFT_WR         PC15
//#define TFT_RS         PC14
//#define TFT_CS         PC13
//#define TFT_RST        PB9

#define TFT_RD_MASK    (0x0001 << 6)
#define TFT_WR_MASK    (0x0001 << 15)
#define TFT_RS_MASK    (0x0001 << 14)
#define TFT_CS_MASK    (0x0001 << 13)
#define TFT_RST_MASK   (0x0001 << 9)

// use fast bit toggling, very fast speed!
#define RD_ACTIVE    { GPIOA->BRR  = TFT_RD_MASK; }
#define RD_IDLE      { GPIOA->BSRR = TFT_RD_MASK; }
#define WR_ACTIVE    { GPIOC->BRR  = TFT_WR_MASK; }
#define WR_IDLE      { GPIOC->BSRR = TFT_WR_MASK; }
#define CD_COMMAND   { GPIOC->BRR  = TFT_RS_MASK; }
#define CD_DATA      { GPIOC->BSRR = TFT_RS_MASK; }
#define RST_HIGH     { GPIOB->BSRR = TFT_RST_MASK; }
#define RST_LOW      { GPIOB->BRR  = TFT_RST_MASK; }





//Set pins to the 8 bit number
#define write8(c) { TFT_DATA->BSRR = (((c^0xFF)<<16) | (c))<<TFT_DATA_NIBBLE; WR_STROBE; }
#define read8(x) ( x = read8_() )

// set the pins to output mode
// not required to mask and assign, because all pins of bus are set together
//each pin is configured by four bits, and 0b0011 or 0x3 means output mode (same as pinmode())
#if TFT_DATA_NIBBLE>0
	#define setWriteDir() ( TFT_DATA->CRH = 0x33333333 )	// set the lower 8 bits as output
#else
	#define setWriteDir() ( TFT_DATA->CRL = 0x33333333 )	// set the lower 8 bits as output
#endif

// set the pins to input mode
// not required to mask and assign, because all pins of bus are set together
// 8 in hex is 0b1000, which means input, same as pinmode()
#if TFT_DATA_NIBBLE>0
  #define setReadDir() ( TFT_DATA->CRH = 0x88888888 )	// set the upper 8 bits as input
#else
  #define setReadDir() ( TFT_DATA->CRL = 0x88888888 )	// set the lower 8 bits as input
#endif


/*****************************************************************************/

#define swap(a, b) { int16_t t = a; a = b; b = t; }


// Color definitions
#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_DARKRED     0x8800      /* 128,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F


// Register names
#define ILI9341_SOFTRESET          0x01
#define ILI9341_SLEEPIN            0x10
#define ILI9341_SLEEPOUT           0x11
#define ILI9341_NORMALDISP         0x13
#define ILI9341_INVERTOFF          0x20
#define ILI9341_INVERTON           0x21
#define ILI9341_GAMMASET           0x26
#define ILI9341_DISPLAYOFF         0x28
#define ILI9341_DISPLAYON          0x29
#define ILI9341_COLADDRSET         0x2A
#define ILI9341_PAGEADDRSET        0x2B
#define ILI9341_MEMORYWRITE        0x2C
#define ILI9341_PIXELFORMAT        0x3A
#define ILI9341_FRAMECONTROL       0xB1
#define ILI9341_DISPLAYFUNC        0xB6
#define ILI9341_ENTRYMODE          0xB7
#define ILI9341_POWERCONTROL1      0xC0
#define ILI9341_POWERCONTROL2      0xC1
#define ILI9341_VCOMCONTROL1      	0xC5
#define ILI9341_VCOMCONTROL2      	0xC7
#define ILI9341_MADCTL  			0x36

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

//Borrowed registers from other chipsets...
#define HX8357D_SETC 0xB9

#define HX8347G_COLADDRSTART_HI    0x02
#define HX8347G_COLADDRSTART_LO    0x03
#define HX8347G_COLADDREND_HI      0x04
#define HX8347G_COLADDREND_LO      0x05
#define HX8347G_ROWADDRSTART_HI    0x06
#define HX8347G_ROWADDRSTART_LO    0x07
#define HX8347G_ROWADDREND_HI      0x08
#define HX8347G_ROWADDREND_LO      0x09
#define HX8347G_MEMACCESS          0x16


uint16_t ili9341_begin(void);
void     ili9341_reset(void);
void     ili9341_setLR(void);
void     drawPixel(int16_t x, int16_t y, uint16_t color);
void     drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
void     drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
void     fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
void     fillScreen(uint16_t color);
void     setRotation(uint8_t x);
void     setAddrWindow(int x1, int y1, int x2, int y2);
void     pushColors(uint16_t *data, uint8_t len, bool first);
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void     flood(uint16_t color, uint32_t len);


extern uint8_t  read8_(void);
extern void writeRegister8(uint8_t a, uint8_t d);
extern void writeRegister16(uint16_t a, uint16_t d);
extern void writeRegister32(uint8_t a, uint32_t d);
extern void writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d);

#endif
