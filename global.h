//Global Setup
//============

#define FIRMWARE_VERSION "1.2"

//If defined build for DSO-150 instead of DSO-138
#define DSO_150

//Add Second analog channel option (Pin varies between DSO-138 and DSO-150, see defenition below)
//#define ADD_AN2


//Add Digital Channels. DSO-138 only supports D0/D1
#define ADD_D3


//DSO-138 Specific
//================

// comment out following line to use DSO push buttons instead of encoder
// DSO-150 has encoder already build in...
#define USE_ENCODER


//Product-Specific Board Pinout Defines
//=====================================

#ifdef DSO_150

// pin definitions (DSO150)
#define BOARD_LED     PA15
#define TEST_WAVE_PIN PA7     // 1KHz square wave output
#define TEST_AMPSEL   PB12    // Testpin amplituted control
#define TRIGGER_IN    PA8
#define TRIGGER_LEVEL PB8

// captured inputs
#define AN_CH1      PA0   // analog channel 1
#define AN_CH2      PA7   // analog channel 2 (Either analog input or test output... TBD....)
#define DG_CH1      PB13  // digital channel 1 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define DG_CH2      PB14  // digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define DG_CH3      PB15  // digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin

// misc analog inputs
#define CPLSEL      PA5

//Misc digital outputs
#define VSENSSEL0   PA1
#define VSENSSEL1   PA2
#define VSENSSEL2   PA3
#define VSENSSEL3   PA4

// switches
#define ENCODER_SW  PB3
#define ENCODER_A   PB0
#define ENCODER_B   PB1

#define BTN1        PB4   //V-DIV
#define BTN2        PB5   //SEC-DIV
#define BTN3        PB6   //TRIGGER
#define BTN4        PB7   //OK

#define I2C_SCL     PB10
#define I2C_SDA     PB11

// TFT pins are hard coded in Adafruit_TFTLCD_8bit_STM32.h file
// TFT_RD         PA6
// TFT_WR         PC15
// TFT_RS         PC14
// TFT_CS         PC13
// TFT_RST        PB9

#define FIRMWARE_TARGET "DSO-150"
#define FIRMWARE_INFO "DSO-150 Hardware by JYE-Tech"

#else
// pin definitions (DSO138)
#define BOARD_LED 		PA15
#define TEST_WAVE_PIN PA7     // 1KHz square wave output
#define TRIGGER_IN		PA8
#define TRIGGER_LEVEL	PB8
#define VGEN			    PB9		// used to generate negative voltage in DSO138

// captured inputs
#define AN_CH1 			PA0		// analog channel 1
#define AN_CH2 			PA4		// analog channel 2
#define DG_CH1 			PA13	// digital channel 1 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define DG_CH2 			PA14	// digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin

// misc analog inputs
#define VSENSSEL1 	PA2
#define VSENSSEL2		PA1
#define CPLSEL			PA3

// switches
#define ENCODER_SW	PB12
#define ENCODER_A		PB13
#define ENCODER_B		PB14
#define BTN4 			  PB15

// TFT pins are hard coded in Adafruit_TFTLCD_8bit_STM32.h file
// TFT_RD         PB10
// TFT_WR         PC15
// TFT_RS         PC14
// TFT_CS         PC13
// TFT_RST        PB11


#define FIRMWARE_TARGET "DSO-138"
#define FIRMWARE_INFO "DSO-138 Hardware by JYE-Tech"
#endif

//General Defenitions
//===================

// serial print macros
#define DBG_INIT(...)     { Serial1.begin(__VA_ARGS__);  }
#define DBG_PRINT(...)    { Serial1.print(__VA_ARGS__);  }
#define DBG_PRINTLN(...)  { Serial1.println(__VA_ARGS__); }

#define SERIAL_BAUD_RATE  115200

// analog and digital samples storage depth
// 
#ifdef ADD_AN2
#define NUM_SAMPLES   2048  
#else
#define NUM_SAMPLES   3072 
#endif

// display colours
#define AN_SIGNAL1    ILI9341_YELLOW
#define AN_SIGNAL2    ILI9341_MAGENTA
#define DG_SIGNAL1    ILI9341_RED
#define DG_SIGNAL2    ILI9341_BLUE
#define DG_SIGNAL3    ILI9341_GREEN

// FLASH memory address defines
#define PARAM_PREAMBLE	0
#define PARAM_TIMEBASE	1
#define PARAM_TRIGTYPE	2
#define PARAM_TRIGDIR	3
#define PARAM_XCURSOR	4
#define PARAM_YCURSOR	5	// 5,6,7,8,9 - 5 params
#define PARAM_WAVES		10	// 10,11,12,13,14 - 5 params
#define PARAM_TLEVEL	15
#define PARAM_STATS		16
#define PARAM_ZERO1		17
#define PARAM_ZERO2		18
#define PARAM_VRANGE  19
#define PARAM_DSIZE   20  //20,21,22 - 3 params
#define PARAM_FUNC    23
#define PARAM_ZOOM    24
#define PARAM_TSOURCE 25
#define PARAM_ZERO1CAL 26 //26 - 37 12 params
#define PARAM_GAIN1CAL 38 //38 - 49 12 params

#define LED_ON	digitalWrite(BOARD_LED, LOW)
#define LED_OFF	digitalWrite(BOARD_LED, HIGH)

// number of pixels waveform moves left/right or up/down
#define XCURSOR_STEP	25
#define YCURSOR_STEP	5
#define XCURSOR_STEP_COARSE  75
#define YCURSOR_STEP_COARSE  15

#define BTN_DEBOUNCE_TIME	50
#define BTN_LONG_PRESS    800

#define A1   0
#define A2   1
#define D1   2
#define D2   3
#define D3   4

#ifdef ADD_AN2
#define ANALOG_CHANNEL_COUNT 2
#else
#define ANALOG_CHANNEL_COUNT 1
#endif

#define DIGITAL_CHANNEL_COUNT 0
#ifdef ADD_D1
#define DIGITAL_CHANNEL_COUNT 1
#endif
#ifdef ADD_D2
#define DIGITAL_CHANNEL_COUNT 2
#endif
#ifdef ADD_D3
#define DIGITAL_CHANNEL_COUNT 3
#endif
