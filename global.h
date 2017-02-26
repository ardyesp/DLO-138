// comment out following line to use DSO push buttons instead of encoder
#define USE_ENCODER

// serial print macros
#define DBG_INIT(...) 		{ Serial.begin(__VA_ARGS__); 	}
#define DBG_PRINT(...) 		{ Serial.print(__VA_ARGS__); 	}
#define DBG_PRINTLN(...) 	{ Serial.println(__VA_ARGS__); }

#define SERIAL_BAUD_RATE	115200

// analog and digital samples storage depth
#define NUM_SAMPLES 	2048	

// display colours
#define AN_SIGNAL1 		ILI9341_YELLOW
#define AN_SIGNAL2 		ILI9341_MAGENTA
#define DG_SIGNAL1 		ILI9341_RED
#define DG_SIGNAL2 		ILI9341_BLUE

// pin definitions (DSO138)
#define BOARD_LED 		PA15
#define TEST_WAVE_PIN 	PA7     // 1KHz square wave output
#define TRIGGER_IN		PA8
#define TRIGGER_LEVEL	PB8
#define VGEN			PB9		// used to generate negative voltage in DSO138

// captured inputs
#define AN_CH1 			PA0		// analog channel 1
#define AN_CH2 			PA4		// analog channel 2
#define DG_CH1 			PA13	// digital channel 1 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define DG_CH2 			PA14	// digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin

// misc analog inputs
#define VSENSSEL1 		PA2
#define VSENSSEL2		PA1
#define CPLSEL			PA3

// switches
#define ENCODER_SW		PB12
#define ENCODER_A		PB13
#define ENCODER_B		PB14
#define BTN4 			PB15

// TFT pins are hard coded in Adafruit_TFTLCD_8bit_STM32.h file
// TFT_RD         PB10
// TFT_WR         PC15
// TFT_RS         PC14
// TFT_CS         PC13
// TFT_RST        PB11

// FLASH memory address defines
#define PARAM_PREAMBLE	0
#define PARAM_TIMEBASE	1
#define PARAM_TRIGTYPE	2
#define PARAM_TRIGDIR	3
#define PARAM_XCURSOR	4
#define PARAM_YCURSOR	5	// 5,6,7,8 - 4 params
#define PARAM_WAVES		9	// 9,10,11,12 - 4 params
#define PARAM_TLEVEL	13
#define PARAM_STATS		14
#define PARAM_ZERO1		15
#define PARAM_ZERO2		16

#define LED_ON	digitalWrite(BOARD_LED, LOW)
#define LED_OFF	digitalWrite(BOARD_LED, HIGH)

// number of pixels waveform moves left/right or up/down
#define XCURSOR_STEP	25
#define YCURSOR_STEP	5


#define BTN_DEBOUNCE_TIME	350
