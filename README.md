# Open DSO-150

## Complete rewrite of DSO-150 Firmware

- No longer DSO-138 compatible (Sorry...)
- Rewritten for Atollic trueSTUDIO Yea... Real debugging)
- Now using internal digital trigger through ADC IRQ (So trigger level can now be correlated to real voltage)
- Lots of new features

## Features
- Support 1 Analog and up to 3 Digital Channels
- Trigger on rising/falling/both edges
- Trigger on Analog, Digital 1,2,3 Signal
- Single/Norm/Auto Trigger Mode
- Exact trigger voltage
- Serial Data Dump
- Automatic Zero-Level Cal for all gain-stages
- Voltmeter mode (Averaged over last 10 samples)
- Signal Statistics Display
- Cursors for Analog Signal
- Load/Store Waveform to/from Flash
- Zoom Out Display (See more data with full sampling Rate)
- "Persistence" Mode
- Variable Signal Size for Digital Signal Waveforms
- "loop" mode to scroll current input-values
- AC/DC mode
- 3K Sampling Depth

## Images

### Features
![Features](/Pics/features.JPG)

### Stats Display
![Stats](/Pics/stats.JPG)

### Voltage Display
![Voltage](/Pics/voltage.JPG)

### Zoom
![Zoom](/Pics/zoom.JPG)

### Digital
![Digital](/Pics/digital.JPG)

### Cursors
![Cursors](/Pics/cursors.JPG)

## Key Assignment
- Encoder Left/Right -> Change Value

- Encoder Button
  Short Press -> Select next field
              -> (In Cursor Mode select next cursor)
  Long Press -> Activate Function
     -> A1 -> Enable Cursor Mode
	 -> D1,D2,D3 -> Change Waveform Size
	 -> Function -> Execute selected function
	 -> Others -> Restore Defaults

- OK Button
  Short Press -> hold/run mode
  Long Press -> Show Analog Signal Stats
  
 - Trigger Button
   Short Press -> Activate Trigger Level
   Long Press -> Enable/Disable Trace Persistence

- Sec/Div Button
  Short Press -> Select Time Base Selection

- V/Div Button
  Short Press -> Select Voltage Range Selection
  Long Press -> Enable/Disable Voltage Meter Mode

- Pressing V/Div,Sec/Div/Trigger while field is activated toggles through settings
- Hold OK button at power-up -> Reset all values to defaults

### Cursor Mode
There are some limitations in cursor mode. While it is possible to use cursors both on signals in hold mode and running mode only the functions available via the buttons are available to modify.
Short-Press on the encoder button returns back to cursor mode, long press on encoder-button disables cursor mode. 

### Loop Mode
In Loop mode samples will be added on the right side of the waveform. The buffer is limited to the width of the screen. (Works espcially well with voltage display enabled as voltmeter with history...)

## Building Open DSO-150
Open DSO-150 should build directly after opening the project in the free STM32 version of Atollic trueSTUDIO.
https://atollic.com/truestudio/

It should also compile under the System Workbench for STM32 after creating a new project and importing the soruce files but I haven't tried that...

With an STLink V2 probe it is very easy to both program and debug the scope via the DebugWire Link.
(I haven't tried uploading hex-files through the serial port with the STM32 Bootloader but there's no reason it shouldn't work...)

## Libraries Used
The code for the graphics library is based on a heavily modified/optimized version of the Adafruit_GFX/Adafruit_TFT libraries. 	

## TODO
- If somebody has a working example of using a STM32 timer to trigger the ADC and then have the ADC trigger an IRQ let me know.... Should work but I had no luck getting that to work...

## References
- Adafruit Graphics Library - https://github.com/adafruit/Adafruit-GFX-Library
- Parallel 8 bit ILI9341 library - https://github.com/stevstrong/Adafruit_TFTLCD_8bit_STM32

 


 
 
