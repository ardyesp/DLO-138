// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.  SEE RELEVANT COMMENTS IN Adafruit_TFTLCD_8bit_STM32.h

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#include "Adafruit_TFTLCD_8bit_STM32.hpp"
#include "../global.h"

//These macros enable/disable external interrupts so we can use the display together with buttons/encoder on the same lines...
uint32_t intReg;
uint32_t opReg;
uint16_t dispControllerId = 0;
#define CS_ACTIVE  { intReg = EXTI->IMR;opReg = TFT_DATA->ODR;EXTI->IMR = 0 ; TFT_DATA->CRL = 0x33333333 ;GPIOC->BRR  = TFT_CS_MASK; }
#define CS_IDLE    { TFT_DATA->ODR = opReg;TFT_DATA->CRL = 0x88888888; GPIOC->BSRR = TFT_CS_MASK ; EXTI->IMR = intReg; }

#define WR_STROBE    { WR_ACTIVE; WR_IDLE; }

extern  void delayUS(uint32_t us); // ?


uint32_t readReg32(uint8_t r);
uint16_t readReg(uint8_t r);


/*****************************************************************************/
uint16_t ili9341_begin(void)
{
  //Set command lines as output
  //Do this in General Setup...

  CS_IDLE; // Set all control bits to HIGH (idle)
  CD_DATA; // Signals are ACTIVE LOW
  WR_IDLE;
  RD_IDLE;

  ili9341_reset();
  setWriteDir();

  writeRegister8(ILI9341_SOFTRESET, 0);
  delayMS(50);


  //TODO readback doesn't seem to work...
  dispControllerId = (uint16_t)(readReg32(0x04) & 0xffff);

  writeRegister8(ILI9341_DISPLAYOFF, 0);

  writeRegister8(ILI9341_POWERCONTROL1, 0x23);
  writeRegister8(ILI9341_POWERCONTROL2, 0x10);
  writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
  writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
  writeRegister8(ILI9341_MADCTL , ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
  writeRegister8(ILI9341_PIXELFORMAT, 0x55);
  writeRegister16(ILI9341_FRAMECONTROL, 0x001B);

  writeRegister8(ILI9341_ENTRYMODE, 0x07);

  writeRegister8(ILI9341_SLEEPOUT, 0);
  delayMS(120);
  writeRegister8(ILI9341_DISPLAYON, 0);
  setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);

  return dispControllerId;
}


void ili9341_setLR(void)
{
  writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, TFTWIDTH  - 1);
  writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, TFTHEIGHT - 1);
}

/*****************************************************************************/
void ili9341_reset(void)
{
  // toggle RST low to reset
	RST_HIGH;
    delayMS(10);
	RST_LOW;
    delayMS(10);
	RST_HIGH;
    delayMS(100);
}

/*****************************************************************************/
// Sets the LCD address window (and address counter, on 932X).
// Relevant to rect/screen fills and H/V lines.  Input coordinates are
// assumed pre-sorted (e.g. x2 >= x1).
/*****************************************************************************/
void setAddrWindow(int x1, int y1, int x2, int y2)
{
    uint32_t t;

    t = x1;
    t <<= 16;
    t |= x2;
    writeRegister32(ILI9341_COLADDRSET, t);  // HX8357D uses same registers!
    t = y1;
    t <<= 16;
    t |= y2;
    writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!
}

/*****************************************************************************/
// Fast block fill operation for fillScreen, fillRect, H/V line, etc.
// Requires setAddrWindow() has previously been called to set the fill
// bounds.  'len' is inclusive, MUST be >= 1.
/*****************************************************************************/
void flood(uint16_t color, uint32_t len)
{
  uint16_t blocks;
  uint8_t  i, hi = color >> 8,
              lo = color;

  CS_ACTIVE;
  CD_COMMAND;
  write8(ILI9341_MEMORYWRITE);

  // Write first pixel normally, decrement counter by 1
  CD_DATA;
  write8(hi);
  write8(lo);
  len--;

  blocks = (uint16_t)(len / 64); // 64 pixels/block
  if(hi == lo) {
    // High and low bytes are identical.  Leave prior data
    // on the port(s) and just toggle the write strobe.
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
      } while(--i);
    }
    // Fill any remaining pixels (1 to 64)
    for(i = (uint8_t)len & 63; i--; ) {
      WR_STROBE;
      WR_STROBE;
    }
  } else {
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        write8(hi); write8(lo); write8(hi); write8(lo);
        write8(hi); write8(lo); write8(hi); write8(lo);
      } while(--i);
    }
    for(i = (uint8_t)len & 63; i--; ) {
      write8(hi);
      write8(lo);
    }
  }
  CS_IDLE;
}

/*****************************************************************************/
void drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
  int16_t x2;

  // Initial off-screen clipping
  if((length <= 0     ) ||
     (y      <  0     ) || ( y                  >= TFTHEIGHTREAL) ||
     (x      >= TFTWIDTH) || ((x2 = (x+length-1)) <  0      )) return;

  if(x < 0) {        // Clip left
    length += x;
    x       = 0;
  }
  if(x2 >= TFTWIDTH) { // Clip right
    x2      = TFTWIDTH - 1;
    length  = x2 - x + 1;
  }

  setAddrWindow(x, y, x2, y);
  flood(color, length);
  ili9341_setLR();
}

/*****************************************************************************/
void drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
  int16_t y2;
	
  // Initial off-screen clipping
  if((length <= 0      ) ||
     (x      <  0      ) || ( x                  >= TFTWIDTH) ||
     (y      >= TFTHEIGHTREAL) || ((y2 = (y+length-1)) <  0     )) return;
  if(y < 0) {         // Clip top
    length += y;
    y       = 0;
  }
  if(y2 >= TFTHEIGHTREAL) { // Clip bottom
    y2      = TFTHEIGHTREAL - 1;
    length  = y2 - y + 1;
  }

  setAddrWindow(x, y, x, y2);
  flood(color, length);
  ili9341_setLR();
}

/*****************************************************************************/
void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor)
{
  int16_t  x2, y2;

  // Initial off-screen clipping
  if( (w            <= 0     ) ||  (h             <= 0      ) ||
      (x1           >= TFTWIDTH) ||  (y1            >= TFTHEIGHTREAL) ||
     ((x2 = x1+w-1) <  0     ) || ((y2  = y1+h-1) <  0      )) return;
  if(x1 < 0) { // Clip left
    w += x1;
    x1 = 0;
  }
  if(y1 < 0) { // Clip top
    h += y1;
    y1 = 0;
  }
  if(x2 >= TFTWIDTH) { // Clip right
    x2 = TFTWIDTH - 1;
    w  = x2 - x1 + 1;
  }
  if(y2 >= TFTHEIGHTREAL) { // Clip bottom
    y2 = TFTHEIGHTREAL - 1;
    h  = y2 - y1 + 1;
  }

  setAddrWindow(x1, y1, x2, y2);
  flood(fillcolor, (uint32_t)w * (uint32_t)h);
  ili9341_setLR();
}

/*****************************************************************************/
void fillScreen(uint16_t color)
{
  setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHTREAL - 1);
  flood(color, (uint32_t)TFTWIDTH * (uint32_t)TFTHEIGHTREAL);
}

/*****************************************************************************/
void drawPixel(int16_t x, int16_t y, uint16_t color)
{
  // Clip
  if((x < 0) || (y < 0) || (x >= TFTWIDTH) || (y >= TFTHEIGHTREAL)) return;

  setAddrWindow(x, y, TFTWIDTH-1, TFTHEIGHTREAL-1);
  writeRegister16(0x2C, color);
}

/*****************************************************************************/
// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
/*****************************************************************************/
void pushColors(uint16_t *data, uint8_t len, bool first)
{
  uint16_t color;
  uint8_t  hi, lo;
  CS_ACTIVE;
  if(first == true)
  { // Issue GRAM write command only on first call
    CD_COMMAND;
    write8(0x2C);
  }
  CD_DATA;
  while(len--)
  {
    color = *data++;
    hi    = color >> 8; // Don't simplify or merge these
    lo    = color;      // lines, there's macro shenanigans
    write8(hi);         // going on.
    write8(lo);
  }
  CS_IDLE;
}

/*****************************************************************************/
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
/*****************************************************************************/
uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


/*****************************************************************************/
void setRotation(uint8_t x)
{
  //perform hardware-specific rotation operations...
   uint16_t t = 0;


   if ( dispControllerId==0x8552)
   {
	   x = (x+1) % 4; // Landscape & portrait are inverted compared to ILI
   }


//#ifdef IS_ST7789
//   x = (x+1) % 4; // Landscape & portrait are inverted compared to ILI
//#endif
 

   switch (x)
   {
   case 2:
     t = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
     break;
   case 3:
     t = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
     break;
  case 0:
    t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
    break;
   case 1:
     t = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
     break;
   }
   writeRegister8(ILI9341_MADCTL, t ); // MADCTL
   // For 9341, init default full-screen address window:
   setAddrWindow(0, 0, TFTWIDTH - 1, TFTHEIGHT - 1); // CS_IDLE happens here
}



// speed optimization
//static void writeCommand(uint8_t c) __attribute__((always_inline));
/*****************************************************************************/
static void writeCommand(uint8_t c)
{
	CS_ACTIVE;
	CD_COMMAND;
	write8(0);
	write8(c);
}


/*****************************************************************************/
uint8_t read8_(void)
{
  RD_ACTIVE;
  delayUS(10);
  uint8_t temp = ( (TFT_DATA->IDR>>TFT_DATA_NIBBLE) & 0x00FF);
  delayUS(1);
  RD_IDLE;
  delayUS(1);
  return temp;
}



/*****************************************************************************/
uint32_t readReg32(uint8_t r)
{
  uint32_t id;
  uint8_t x;

  // try reading register #4
  writeCommand(r);
  setReadDir();  // Set up LCD data port(s) for READ operations
  CD_DATA;
  delayUS(50);
  read8(x);
  id = x;          // Do not merge or otherwise simplify
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id  |= x;        // shenanigans that are going on.
  CS_IDLE;
  setWriteDir();  // Restore LCD data port(s) to WRITE configuration
  return id;
}
/*****************************************************************************/
uint16_t readReg(uint8_t r)
{
  uint16_t id;
  uint8_t x;

  writeCommand(r);
  setReadDir();  // Set up LCD data port(s) for READ operations
  CD_DATA;
  delayUS(50);
  read8(x);
  id = x;          // Do not merge or otherwise simplify
  id <<= 8;              // these lines.  It's an unfortunate
  read8(x);
  id |= x;        // shenanigans that are going on.
  CS_IDLE;
  setWriteDir();  // Restore LCD data port(s) to WRITE configuration

  //Serial.print("Read $"); Serial.print(r, HEX);
  //Serial.print(":\t0x"); Serial.println(id, HEX);
  return id;
}


/*****************************************************************************/
void writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d)
{
  writeRegister8(aH, d>>8);
  writeRegister8(aL, d);
}

/*****************************************************************************/
void writeRegister8(uint8_t a, uint8_t d)
{
  writeCommand(a);
  CD_DATA;
  write8(d);
  CS_IDLE;
}

/*****************************************************************************/
void writeRegister16(uint16_t a, uint16_t d)
{
  writeCommand(a);
  CD_DATA;
  write8(d>>8);
  write8(d);
  CS_IDLE;
}

/*****************************************************************************/
void writeRegister32(uint8_t r, uint32_t d)
{
  writeCommand(r);
  CD_DATA;
  write8(d >> 24);
  write8(d >> 16);
  write8(d >> 8);
  write8(d);
  CS_IDLE;
}



