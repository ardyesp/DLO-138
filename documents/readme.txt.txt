- For the moment you will have to compile and upload yourself
- all options are in global.h
- DSO-138 codepath is untested (No hardware...)

- DSO-150 has known issues:
   - Encoder use sucks (Pins are multiplexed with Display so IRQ is not an easy option... Working on it...)
   - Various UI strangeness/artifact problems
   - Timebase affects button/encoder polling (See above...)
   
Key Assignment:
- Encoder Left/Right      -> Change Value
- Encoder Press           -> Select Entry field
- V/Div, Sec/Div,Trigger  -> Quick-activate entry fields
- Long Press on encoder button restores default for that field
- OK start/stops hold mode
- Long OK Press activates stats
- Hold OK during power-up to reset settings
   