# DLO-138
An open source firmware for DSO-138 Oscilloscope. 

DSO-138 is an excellent piece of hardware based on ARM Cortex M3 core STM32F103 processor and sufficient for most beginner users. The stock firmware, while quite responsive, can use a few improvements. The main shortcoming which prompted the development of DLO-138 firmware is the inability to get waveform data into a computer for further analysis and the lack of a second channel. Engineers troubleshooting hardware issues need to mark reference points on waveform so having another analog or digital channel can greatly improve analysis. This firmware hopes to improve on these issues.

## Features
1. Two analog channels
2. Two digital logic channels
3. Serial port interface for captured waveform data
4. Trigger source selectable from Analog Channel 1 or Digital Channel
5. Option to use rotary encoder instead of + - and SEL switches

This firmware can be used on stock DSO-138 hardware as well. Select one of the pre-compiled binaries to suit the board. Follow the firmware upgrade instructions for DSO-138. At any time, you can reflash DSO-138 with JYE Tech provided firmware.

# Cost
Extra features come at an additional cost. In the case of DLO-138, it is the loss of lowest timebase. Maximum sampling rate in DLO-138 is 20 µs/div instead of 10 µs/div. In the 20 µs/div range, firmware under-samples ADC channels, often reading same data twice. To use the second analog channel, analog front end has to be duplicated on a daughter board. On a stock hardware, this firmware can be used to provide two digital logic channels.

# Build
The build environment uses Arduino. For help with setting up IDE visit http://www.stm32duino.com

# Hardware
![Mod Schematic](https://github.com/ardyesp/DLO-138/blob/master/pics/HardwareMod.png)

# References
DSO-138 - http://www.jyetech.com/Products/LcdScope/e138.php
STM32Duino - http://www.stm32duino.com
STM32F103 - http://www.st.com/en/microcontrollers/stm32f103.html
 
