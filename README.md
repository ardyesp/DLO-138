# DLO-150
All documentation below for DLO-138 might be obsolete....Work-In Progress

- For the moment you will have to compile and upload yourself
- all options are in global.h
- DSO-138 codepath is untested (No hardware...)

   
Key Assignment:
- Encoder Left/Right      -> Change Value
- Encoder Press           -> Select Next Entry field
- V/Div, Sec/Div,Trigger  -> Quick-activate entry fields
- Pressing V/Div,Sec/Div/Trigger while field is activated toggles through settings
- Long Press on encoder button restores default for that field or triggers function
     A1/A2 Marker -> Zero Calib
	 Function -> Execute Function
- OK start/stops hold mode
- Long OK Press activates stats
- Hold OK during power-up to reset settings


Missing
-------
- EEPROM Save/Load not implemented yet...
- Trigger on every change not working yet
- Trigger on digital lines not working yet

Libraries required
-------------------
extEERPOM
Adafruit_GFX




# DLO-138
An open source firmware for DSO-138 Oscilloscope. 
![Photo](https://github.com/ardyesp/DLO-138/blob/master/pics/pic4.png)

DSO-138 is an excellent piece of hardware based on ARM Cortex M3 core STM32F103 processor and sufficient for most beginner users. The stock firmware, while quite responsive, can use a few improvements. The main shortcoming which prompted the development of DLO-138 firmware is the inability to get waveform data into a computer for further analysis and the lack of a second channel. Engineers troubleshooting hardware issues need to mark reference points on waveform so having another analog or digital channel can greatly improve analysis. This firmware hopes to improve on these issues.

## Features
- Two analog channels
- Two digital logic channels (SWDIO and SWDIO pins (PA13 and PA14) on board)
- Serial port interface for captured waveform data
- Trigger source selectable from Analog Channel 1 or Digital Channel
- Option to use rotary encoder instead of + - and SEL switches
- 2K sample depth

This firmware can be used on stock DSO-138 hardware as well. Select one of the pre-compiled binaries to suit the board. Follow the firmware upgrade instructions for DSO-138. At any time, you can reflash DSO-138 with JYE Tech provided firmware.

# Cost
Extra features come at an additional cost. In the case of DLO-138, it is the loss of lowest timebase. Maximum sampling rate in DLO-138 is 20 µs/div instead of 10 µs/div. In the 20 µs/div range, firmware under-samples ADC channels, often reading same data twice. To use the second analog channel, analog front end has to be duplicated on a daughter board. On a stock hardware, this firmware can be used to provide two digital logic channels.

# Build
The build environment uses Arduino. For help with setting up IDE visit http://www.stm32duino.com

# Hardware
Following changes can be applied selectively, to get maximum functionality from board. The firmware can be run on unmodified hardware as well.
![Mod Schematic](https://github.com/ardyesp/DLO-138/blob/master/pics/HardwareMod.png)

# Usage:
	Push button in encoder (SEL if using switches) moves focus to next parameter
	Left/Right turn in encoder (+/- if using switches) changes the parameter which is in focus
	Short press OK to HOLD the waveform and output it on serial port
	Long press OK button:
	
		Focus				Action
		Trigger Level		Zero the trigger level to Analog channel 1
		Wave X scrollbar	Center waveform on screen (at trigger point)
		Wave Y cursor		Zero the cursor. If Analog CH1 coupling is GND, waveform reference base is set 
		Other				Toggle on screen Analog CH1 statistics display

	Press and hold OK button at power up to reset settings to default
		

# References
DSO-138 - http://www.jyetech.com/Products/LcdScope/e138.php

STM32Duino - http://www.stm32duino.com

STM32F103 - http://www.st.com/en/microcontrollers/stm32f103.html

Adafruint Graphics Library - https://github.com/adafruit/Adafruit-GFX-Library

Parallel 8 bit ILI9341 library - https://github.com/stevstrong/Adafruit_TFTLCD_8bit_STM32

 


 
 
