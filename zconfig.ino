// zconfig: since we are referencing variables defined in other files 

#ifdef DSO_150_EEPROM
#include <extEEPROM.h>
#endif

#define PREAMBLE_VALUE	2859


// ------------------------
void loadConfig(boolean reset)	{
// ------------------------
	DBG_PRINTLN("Loading stored config...");

	if(EEPROM.init() != EEPROM_OK)	{
		loadDefaults();
		return;
	}
	
	// read preamble
	if(reset || (EEPROM.read(PARAM_PREAMBLE) != PREAMBLE_VALUE))	{
		loadDefaults();
		formatSaveConfig();
		return;
	}	
	
	// load all the parameters from EEPROM
	uint16_t data;
	
	data = EEPROM.read(PARAM_TIMEBASE);
	setTimeBase(data);
	
	// trigger type is not persisted
	setTriggerType(TRIGGER_AUTO);

	data = EEPROM.read(PARAM_TRIGDIR);
	setTriggerDir(data);
	
	data = EEPROM.read(PARAM_XCURSOR);
	xCursor = data;
	
	data = EEPROM.read(PARAM_YCURSOR);
	yCursors[0] = data;
	
	data = EEPROM.read(PARAM_YCURSOR + 1);
	yCursors[1] = data;
	
	data = EEPROM.read(PARAM_YCURSOR + 2);
	yCursors[2] = data;

	data = EEPROM.read(PARAM_YCURSOR + 3);
	yCursors[3] = data;
 
  data = EEPROM.read(PARAM_YCURSOR + 4);
  yCursors[4] = data;
  
	data = EEPROM.read(PARAM_WAVES);
	waves[0] = data;
	
	data = EEPROM.read(PARAM_WAVES + 1);
	waves[1] = data;
	
	data = EEPROM.read(PARAM_WAVES + 2);
	waves[2] = data;
	
	data = EEPROM.read(PARAM_WAVES + 3);
	waves[3] = data;

  data = EEPROM.read(PARAM_WAVES + 4);
  waves[4] = data;
	
	data = EEPROM.read(PARAM_TLEVEL);
	setTriggerLevel(data);
	
	printStats = EEPROM.read(PARAM_STATS);
	zeroVoltageA1 = EEPROM.read(PARAM_ZERO1);
	zeroVoltageA2 = EEPROM.read(PARAM_ZERO2);

  data = EEPROM.read(PARAM_VRANGE);
#ifdef DSO_150   
  setVoltageRange(data);
#endif
	
  data = EEPROM.read(PARAM_DSIZE);
  dsize[0] = data;
  data = EEPROM.read(PARAM_DSIZE + 1);
  dsize[1] = data;
  data = EEPROM.read(PARAM_DSIZE + 2);
  dsize[2] = data;

  data = EEPROM.read(PARAM_FUNC);
  currentFunction = data;

  data = EEPROM.read(PARAM_ZOOM);
  setZoomFactor(data);

  data = EEPROM.read(PARAM_TSOURCE);
  setTriggerSource(data);
  
  
	DBG_PRINTLN("Loaded config:");
	DBG_PRINT("Timebase: ");DBG_PRINTLN(currentTimeBase);
  DBG_PRINT("Voltage Range: ");DBG_PRINTLN(currentVoltageRange);
	DBG_PRINT("Trigger Dir: ");DBG_PRINTLN(triggerDir);
	DBG_PRINT("Trigger Type: ");DBG_PRINTLN(triggerType);
	DBG_PRINT("X Cursor Pos: ");DBG_PRINTLN(xCursor);
	
	DBG_PRINT("Y Cursors: ");
	DBG_PRINT(yCursors[0]);
	DBG_PRINT(", ");
	DBG_PRINT(yCursors[1]);
	DBG_PRINT(", ");
	DBG_PRINT(yCursors[2]);
	DBG_PRINT(", ");
	DBG_PRINTLN(yCursors[3]);
	DBG_PRINT(", ");
  DBG_PRINTLN(yCursors[4]);
  
	DBG_PRINT("Waves: ");
	DBG_PRINT(waves[0]);
	DBG_PRINT(", ");
	DBG_PRINT(waves[1]);
	DBG_PRINT(", ");
	DBG_PRINT(waves[2]);
	DBG_PRINT(", ");
	DBG_PRINT(waves[3]);
  DBG_PRINT(", ");
  DBG_PRINTLN(waves[4]);
  
	DBG_PRINT("Trigger Level: ");DBG_PRINTLN(data);
	DBG_PRINT("Print Stats: ");DBG_PRINTLN(printStats);
	DBG_PRINT("Wave1 Zero: ");DBG_PRINTLN(zeroVoltageA1);
	DBG_PRINT("Wave2 Zero: ");DBG_PRINTLN(zeroVoltageA2);

  DBG_PRINT("Digital Waveform Size: ");
  DBG_PRINT(dsize[0]);
  DBG_PRINT(", ");
  DBG_PRINTLN(dsize[1]);
  DBG_PRINT(", ");
  DBG_PRINTLN(dsize[2]); 
	
	DBG_PRINT("Function: ");
  DBG_PRINTLN(currentFunction); 

  DBG_PRINT("Zoom: ");
  DBG_PRINTLN(zoomFactor); 

  DBG_PRINT("Trigger Source: ");
  DBG_PRINTLN(triggerSource); 
  

	// check if EEPROM left enough space, or else invoke formatSaveConfig
}

// ------------------------
void loadDefaults()	{
// ------------------------
	DBG_PRINTLN("Loading defaults");

	setTimeBase(T30US);
#ifdef DSO_150 
  setVoltageRange(RNG_5V);
#endif  
	setTriggerDir(TRIGGER_RISING);
	setTriggerType(TRIGGER_AUTO);
	setTriggerLevel(0);
	
	// set x in the middle
	xCursor = (NUM_SAMPLES - GRID_WIDTH)/2;
 
	// set y in the middle
	yCursors[0] = -105;
	yCursors[1] = -152;
	yCursors[2] = -45;
	yCursors[3] = -30;
	yCursors[3] = -15;
   
	// show all waves
	waves[0] = true;
	waves[1] = false;
	waves[2] = true;
	waves[3] = true;
	waves[4] = false;
    
	printStats = false;
	
	zeroVoltageA1 = 1985;
	zeroVoltageA2 = 1985;

  dsize[0]=2;
  dsize[1]=2;
  dsize[2]=2;

  currentFunction = 0;

  setZoomFactor(ZOOM_X1);

  triggerSource = 0;

}

// ------------------------
void formatSaveConfig()	{
// ------------------------
	DBG_PRINTLN("Formatting EEPROM");
	EEPROM.format();
	DBG_PRINTLN("Saving all config params....");
	
	saveParameter(PARAM_PREAMBLE, PREAMBLE_VALUE);
	
	saveParameter(PARAM_TIMEBASE, currentTimeBase);
	saveParameter(PARAM_TRIGDIR, triggerDir);
	saveParameter(PARAM_XCURSOR, xCursor);
	saveParameter(PARAM_YCURSOR, yCursors[0]);
	saveParameter(PARAM_YCURSOR + 1, yCursors[1]);
	saveParameter(PARAM_YCURSOR + 2, yCursors[2]);
	saveParameter(PARAM_YCURSOR + 3, yCursors[3]);
	saveParameter(PARAM_YCURSOR + 4, yCursors[4]);
  
	saveParameter(PARAM_WAVES, waves[0]);
	saveParameter(PARAM_WAVES + 1, waves[1]);
	saveParameter(PARAM_WAVES + 2, waves[2]);
	saveParameter(PARAM_WAVES + 3, waves[3]);
  saveParameter(PARAM_WAVES + 4, waves[4]);
  
	saveParameter(PARAM_TLEVEL, trigLevel);
 	saveParameter(PARAM_STATS, printStats);
	
	saveParameter(PARAM_ZERO1, zeroVoltageA1);
	saveParameter(PARAM_ZERO2, zeroVoltageA2);
  saveParameter(PARAM_VRANGE, currentVoltageRange);  

  saveParameter(PARAM_DSIZE, dsize[0]);
  saveParameter(PARAM_DSIZE + 1, dsize[1]);
  saveParameter(PARAM_DSIZE + 2, dsize[2]);

  saveParameter(PARAM_FUNC,currentFunction);
  saveParameter(PARAM_ZOOM,zoomFactor);
  saveParameter(PARAM_TSOURCE,triggerSource);
}

// ------------------------
void saveParameter(uint16_t param, uint16_t data)	{
// ------------------------
	uint16 status = EEPROM.write(param, data);
	if(status != EEPROM_OK)	{
		DBG_PRINT("Unable to save param in EEPROM, code: ");DBG_PRINTLN(status);
	}
}


#ifdef DSO_150_EEPROM
#define OFFSET_MARKER 0
#define LENGTH_MARKER 4
#define OFFSET_DATA_D (OFFSET_MARKER+LENGTH_MARKER)
#define LENGTH_DATA_D (NUM_SAMPLES * 2)
#define OFFSET_DATA_A1 (OFFSET_DATA_D+LENGTH_DATA_D)
#define LENGTH_DATA_A1 (NUM_SAMPLES * 2)
#define OFFSET_DATA_A2 (OFFSET_DATA_A1+LENGTH_DATA_A1)
#define LENGTH_DATA_A2 (NUM_SAMPLES * 2)


#define SDA I2C_SDA
#define SCL I2C_SCL

const byte marker[LENGTH_MARKER] = {0x44,0x88,0x22,0xAA};
extEEPROM myEEPROM(kbits_128, 1, 64);

void initExtEEPROM()
{
  //Set I2C Pins?
  wire.begin();
  byte i2cStat = myEEPROM.begin();
  if ( i2cStat != 0 ) 
  {
  //there was a problem
    DBG_PRINTLN("Failed to initalize external EEPROM!");
  }
}

void loadWaveform()
{
  byte checkheader[LENGTH_MARKER];
  //Load first 4 bytes
byte i2cStat = myEEPROM.read(OFFSET_MARKER, &checkheader, LENGTH_MARKER);
if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) {
      DBG_PRINTLN("Failed to read Marker. Bad Address!");
  }
  else {
        DBG_PRINTLN("Failed to read Marker!");
  }
}
  //compare against markers
  if ((checkheader[0] != marker[0])||(checkheader[1] != marker[1])||(checkheader[2] != marker[2])||(checkheader[3] != marker[3]))
  {
      DBG_PRINTLN("Marker does not Match!");
      return;
  }
 
  //go into hold-mode
  hold = true;
  
  //load waveforms
 i2cStat = myEEPROM.read(OFFSET_DATA_D, &bitStore[0], LENGTH_DATA_D);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to read Data D. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to read Data D!");
  }  

  i2cStat = myEEPROM.write(OFFSET_DATA_A1, &ch1Capture[0], LENGTH_DATA_A1);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to read Data A1. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to read Data A1!");
  }  

#ifdef ADD_AN2  
  i2cStat = myEEPROM.read(OFFSET_DATA_A2, &ch2Capture[0], LENGTH_DATA_A2);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to read Data A2. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to read Data A2!");
  }    
#endif

  //Force display refresh
  drawWaves();
}



void saveWaveform()
{
  //go into hold mode
  hold = true;
  
  //Write markers
  byte i2cStat = myEEPROM.write(OFFSET_MARKER, &marker[0], LENGTH_MARKER);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) {
        DBG_PRINTLN("Failed to write Marker. Bad Address!");
  }
  else {
        DBG_PRINTLN("Failed to write Marker!");
  }  

  //write data
  i2cStat = myEEPROM.write(OFFSET_DATA_D, &bitStore[0], LENGTH_DATA_D);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to write Data D. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to write Data D!");
  }  

  i2cStat = myEEPROM.write(OFFSET_DATA_A1, &ch1Capture[0], LENGTH_DATA_A1);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to write Data A1. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to write Data A1!");
  }  

#ifdef ADD_AN2  
  i2cStat = myEEPROM.write(OFFSET_DATA_A2, &ch2Capture[0], LENGTH_DATA_A2);
  if ( i2cStat != 0 ) {
  //there was a problem
  if ( i2cStat == EEPROM_ADDR_ERR) 
  {
        DBG_PRINTLN("Failed to write Data A2. Bad Address!");
  }
  else 
  {
        DBG_PRINTLN("Failed to write Data A2!");
  }    
#endif
}

#endif

