#include <stdint.h>
#include <string.h>
#include "zconfig.hpp"
#include "global.h"
#include "variables.h"
#include "awrap.hpp"
#include "display.hpp"
#include "interface.hpp"
#include "capture.hpp"
#include "io.hpp"
#include "tiny_printf.h"
#include "EEPROM/eeprom.hpp"

#define PAGE_WAVEFORM_START 111
#define PAGE_WAVEFORM_COUNT 12


extern uint16_t ch1Capture[NUM_SAMPLES];
extern volatile bool hold;
extern I2C_HandleTypeDef hi2c2;  //External EEPROM

t_config config;
t_config oldConfig;

const float adcMultiplierBase[] = {0.2000,0.1000,0.0500, 0.0200, 0.0100, 0.0050, 0.0020, 0.0010, 0.0005, 0.0002, 0.0001,0.00005};

//Rudimentary wear leveling...(Cyclic buffers...)
//- we have multiple pages
//- when we write we increase a counter in the header
//- when we load we look for the last written header and load that
//- Unless one of them is 255 and the next is 0, thne we use the one with 0 (Rollover...)

uint16_t makeVersion(void)
{
	return ((uint16_t)FIRMWARE_VERSION_MAJOR<<8) + (uint16_t)FIRMWARE_VERSION_MINOR;
}


void autoSafe(void)
{
  //Compare if config has changed
  if (0 != memcmp(&config, &oldConfig,sizeof(t_config)))
  {
      formatSaveConfig();
      memcpy(&oldConfig,&config,sizeof(t_config));
  }
}

// ------------------------
void loadConfig(bool reset)
{
// ------------------------
	DBG_PRINT("Loading stored config...\n");


	// read preamble
	loadDefaults();
	if(reset)
	{
		formatSaveConfig();
		memcpy(&oldConfig,&config,sizeof(t_config));
	}
	else
	{
		//Load Defaults
		EE_Reads(0,sizeof(t_config),(uint32_t*)&config);
		//Marker Check
		if ((config.configID[0] != 'D') || (config.configID[1] != 'S') || (config.configID[2] != 'O') || (config.configID[3] != 'S'))
		{
			DBG_PRINT("No Config ID, creating defaults\n");
			//Create defaults
			loadDefaults();
			formatSaveConfig();
		}
		else
		{
			//Check FW version
			if (config.configFWversion != makeVersion())
			{
				DBG_PRINT("Wrong Firmware ID, creating defaults\n");
				//Create defaults
				loadDefaults();
				formatSaveConfig();
			}
		}

		memcpy(&oldConfig,&config,sizeof(t_config));
	}

    //printConfig();

	//Set Parameters
	setTimeBase(config.currentTimeBase);
	setTriggerType(config.triggerType);
	setTriggerDir(config.triggerDir);
	setTriggerLevel(config.trigLevel);
    setPersistence(config.persistence_on);
    setZoomFactor(config.zoomFactor);
    setTriggerSource(config.triggerSource);
    setVoltageRange(config.currentVoltageRange);
}

// ------------------------
void loadDefaults()
// ------------------------
{
    uint16_t ii;
	DBG_PRINT("Loading defaults...\n");

	config.configID[0] = 'D';
	config.configID[1] = 'S';
	config.configID[2] = 'O';
	config.configID[3] = 'S';
	config.configFWversion = makeVersion();

	setTimeBase(T30US);
    setVoltageRange(RNG_5V);
	setTriggerDir(TRIGGER_RISING);
	setTriggerType(TRIGGER_AUTO);
	setTriggerLevel(0);
	
	// set x in the middle
	config.xCursor = (NUM_SAMPLES - GRID_WIDTH)/2;

	config.markerMode = false;
	config.currentMarker = (uint8_t)MARKER_X1;
	config.marker_x1 = GRID_WIDTH / 3;
	config.marker_x2 = 2*(GRID_WIDTH / 3);
	config.marker_y1 = GRID_HEIGHT / 3;
	config.marker_y2 = 2*(GRID_HEIGHT / 3);

	config.old_x1_marker = config.marker_x1;
	config.old_x2_marker = config.marker_x2;
	config.old_y1_marker = config.marker_y1;
	config.old_y2_marker = config.marker_y2;
 
	// set y in the middle
	config.yCursors[0] = -105;
	config.yCursors[1] = -45;
	config.yCursors[2] = -30;
	config.yCursors[3] = -15;
   
	// show all waves
	config.waves[0] = true;
	config.waves[1] = true;
	config.waves[2] = true;
	config.waves[3] = false;
    
	config.printStats = false;
	config.printVoltage = false;
	
	config.zeroVoltageA1 = 1985;

	config.dsize[0]=2;
	config.dsize[1]=2;
	config.dsize[2]=2;

	config.currentFunction = 0;

    setZoomFactor(ZOOM_X1);

    config.triggerSource = 0;

    for (ii=0;ii<RNG_5mV+1;ii++)
    {
    	config.zeroVoltageA1Cal[ii] = 1985;
    }


  for (ii=1;ii<RNG_5mV+1;ii++)
  {
	  config.adcMultiplier[ii] = adcMultiplierBase[ii];
  }

  setPersistence(0);
  oldConfig = config;

}

void printConfig(void)
{
	uint16_t ii;


	printf("Loaded config:\n");
	printf("Timebase: %d\n",config.currentTimeBase);
    printf("Voltage Range: %d\n",config.currentVoltageRange);
    printf("Trigger Dir: %d\n",config.triggerDir);
    printf("Trigger Type: %d\n",config.triggerType);
    printf("X Cursor Pos: %d\n",config.xCursor);

    printf("Y Cursors: %d,%d,%d,%d\n",config.yCursors[0],config.yCursors[1],config.yCursors[2],config.yCursors[3]);
    printf("Waves: %d,%d,%d,%d\n",config.waves[0],config.waves[1],config.waves[2],config.waves[3]);


    //printf("Trigger Level: %d\n",triggerLevel);
    printf("Print Stats: %d\n",config.printStats);
    printf("Print Voltage: %d\n",config.printVoltage);
    printf("Wave Zero: %d\n",config.zeroVoltageA1);

    printf("Digital Waveform Size: %d,%d,%d\n",config.dsize[0],config.dsize[1],config.dsize[2]);


    printf("Function: %d\n",config.currentFunction);
    printf("Zoom: %d\n",config.zoomFactor);

    printf("Trigger Source: %d\n",config.triggerSource);

    printf("Display Persistence: %d\n",config.persistence_on);

    printf("A1 Zerocal ");
    for (ii=0;ii<(RNG_5mV+1);ii++)
    {
    	printf("%d,",config.zeroVoltageA1Cal[ii]);
    }

    printf("\n");

    printf("A1 Gaincal ");
    for (ii=0;ii<(RNG_5mV+1);ii++)
    {
    	printf("%d,",(int)(config.adcMultiplier[ii]*1000.0));
    }

    printf("\n");
}

// ------------------------
void formatSaveConfig()
// ------------------------
{
	DBG_PRINT("Saving all config params....\n");
	EE_Writes(0,256,(uint32_t*)&config);
}



#if 0

//External EEPROM Wave Save. For some reason this doesn't seem to work so we just save to the internal flash
//Plenty of room there...



#define EEPROM_ADDRESS            200
#define EEPROM_MAXPKT             32              //(page size)
#define EEPROM_WRITE              10              //time to wait in ms
#define EEPROM_TIMEOUT            5*EEPROM_WRITE  //timeout while writing
#define EEPROM_SECTIONSIZE		  64




HAL_StatusTypeDef readEEPROM(uint8_t i2cAddr,uint16_t address, uint8_t* MemTarget,
		uint16_t Size)
{
	uint16_t Counter = 0;
	HAL_StatusTypeDef Result = HAL_OK;
	while (Counter < Size && Result == HAL_OK)
	{
		uint16_t Diff = Size - Counter;

		if (Diff >= EEPROM_MAXPKT)
		{
			//Multi-Byte
			Result = HAL_I2C_Mem_Read(&hi2c2, i2cAddr,
					address + Counter, I2C_MEMADD_SIZE_16BIT,
					&MemTarget[Counter], EEPROM_MAXPKT, EEPROM_TIMEOUT);
			Counter += EEPROM_MAXPKT;
		}
		else
		{
			//and the remaining ones...low packet size
			Result = HAL_I2C_Mem_Read(&hi2c2, i2cAddr,
					address + Counter, I2C_MEMADD_SIZE_16BIT,
					&MemTarget[Counter], Diff, EEPROM_TIMEOUT);
			Counter += Diff;
		}
		HAL_Delay(EEPROM_WRITE / 2);
	}
	return Result;
}

HAL_StatusTypeDef writeEEPROM(uint8_t i2cAddr,uint16_t address, uint8_t* MemTarget,
		uint16_t Size)
{
	uint16_t Counter = 0;
	HAL_StatusTypeDef Result = HAL_OK;
	while (Counter < Size && Result == HAL_OK)
	{
		uint16_t Diff = Size - Counter;

		if (Diff >= EEPROM_MAXPKT)
		{
			//Multi-Byte
			Result = HAL_I2C_Mem_Write(&hi2c2, i2cAddr,
					address + Counter, I2C_MEMADD_SIZE_16BIT,
					&MemTarget[Counter], EEPROM_MAXPKT, EEPROM_TIMEOUT);
			Counter += EEPROM_MAXPKT;
		}
		else
		{
			//and the remaining ones...low packet size
			Result = HAL_I2C_Mem_Write(&hi2c2, i2cAddr,
					address + Counter, I2C_MEMADD_SIZE_16BIT,
					&MemTarget[Counter], Diff, EEPROM_TIMEOUT);
			Counter += Diff;
		}
		HAL_Delay(EEPROM_WRITE);
	}
	return Result;
}

void loadWaveform()
{

  //go into hold-mode
  hold = true;
  
  //read analog data
  if (HAL_OK !=  readEEPROM(EEPROM_ADDRESS,0,(uint8_t*)ch1Capture,sizeof(ch1Capture)))
	  printf("Error reading Analog data from EEPROM\n");
  else
	  printf("Analog data read from EEPROM\n");


  //read digital data
  if (HAL_OK !=  readEEPROM(EEPROM_ADDRESS,sizeof(ch1Capture),(uint8_t*)bitStore,sizeof(bitStore)))
	  printf("Error reading Digital data from EEPROM\n");
  else
	  printf("Digital data read from EEPROM\n");

  //Force display refresh
  drawWaves();
}

void saveWaveform(void)
{
  //go into hold mode
  hold = true;
  

  //write analog data
  if (HAL_OK !=  writeEEPROM(EEPROM_ADDRESS,0,(uint8_t*)ch1Capture,sizeof(ch1Capture)))
	  printf("Error writing Analog data to EEPROM\n");
  else
	  printf("Analog data writtem to EEPROM\n");


  //write digital data
  if (HAL_OK !=  writeEEPROM(EEPROM_ADDRESS,sizeof(ch1Capture),(uint8_t*)bitStore,sizeof(bitStore)))
	  printf("Error writing Digital data to EEPROM\n");
  else
	  printf("Digital data writtem to EEPROM\n");
}

#endif


void loadWaveform()
{

  //go into hold-mode
  hold = true;

  if (false == EE_LL_Read(PAGE_WAVEFORM_START,0,sizeof(ch1Capture),(uint8_t*)ch1Capture))
    printf("Error reading Analog data from EEPROM\n");
  //Force display refresh
  drawWaves();
}


void saveWaveform(void)
{
  //go into hold mode
  hold = true;

  if (false == EE_LL_Write(PAGE_WAVEFORM_START,0,sizeof(ch1Capture),(uint8_t*)ch1Capture))
    printf("Error writing Analog data to EEPROM\n");

}

