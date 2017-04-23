
// sampling delay table in quarter-microseconds
const int16_t samplingDelay[] =   {-1,  0, 14, 38, 86, 229, 468, 948, 2385, 4776, 9570, 23940};
const uint16_t timeoutDelayMs[] = {50, 50, 50, 100, 100, 100, 150, 250, 500, 1000, 2000, 4500};

int16_t sDly, tDly;
boolean minSamplesAcquired;
long prevTime = 0;

// hold pointer references for updating variables in memory
uint16_t *sIndexPtr = &sIndex;
volatile boolean *keepSamplingPtr = &keepSampling;
volatile boolean *triggeredPtr = &triggered;

// ------------------------
void setSamplingRate(uint8_t timeBase)	{
// ------------------------
	sDly = samplingDelay[timeBase];
	tDly = timeoutDelayMs[timeBase];
	// sampling rate changed, break out from previous sampling loop
	keepSampling = false;
	// disable scan timeout timer
	Timer2.pause();
}

void setTriggerDir(uint8_t tdir)
{
  setTriggerSourceAndDir(triggerSource,tdir);
}

void setTriggerSource(uint8_t tsource)
{
  setTriggerSourceAndDir(tsource,triggerDir);
}

void setTriggerSourceAndDir(uint8_t source,uint8_t dir)
{
  // trigger changed, break out from previous sampling loop
  keepSampling = false;

  //Detach old source
  switch(triggerSource)
  {
    case TRIGSRC_A1:
       detachInterrupt(TRIGGER_IN);
       break;
    case TRIGSRC_D1:
       detachInterrupt(DG_CH1);
       break;     
    case TRIGSRC_D2:
       detachInterrupt(DG_CH2);
       break;   
#ifdef DSO_150          
#ifdef ADD_D3       
    case TRIGSRC_D3:
       detachInterrupt(DG_CH3);
       break;
#endif  
#endif                  
  }

  //Set new Trigger Source
  switch (source)
  {
    case TRIGSRC_A1:
        switch(dir)
        {
          case TRIGGER_RISING:
             attachInterrupt(TRIGGER_IN, triggerISR, RISING);
             break;
          case TRIGGER_FALLING:
             attachInterrupt(TRIGGER_IN, triggerISR, FALLING);
             break; 
          case TRIGGER_ALL:
             attachInterrupt(TRIGGER_IN, triggerISR, CHANGE);
             break;
        }
       break;
    case TRIGSRC_D1:
        switch(dir)
        {
          case TRIGGER_RISING:
             attachInterrupt(DG_CH1, triggerISR, RISING);
             break;
          case TRIGGER_FALLING:
             attachInterrupt(DG_CH1, triggerISR, FALLING);
             break; 
          case TRIGGER_ALL:
             attachInterrupt(DG_CH1, triggerISR, CHANGE);
             break;
        }   
       break;     

    case TRIGSRC_D2:
        switch(dir)
        {
          case TRIGGER_RISING:
             attachInterrupt(DG_CH2, triggerISR, RISING);
             break;
          case TRIGGER_FALLING:
             attachInterrupt(DG_CH2, triggerISR, FALLING);
             break; 
          case TRIGGER_ALL:
             attachInterrupt(DG_CH2, triggerISR, CHANGE);
             break;
        }
       break; 
#ifdef DSO_150  
#ifdef ADD_D3        
    case TRIGSRC_D3:
        switch(dir)
        {
          case TRIGGER_RISING:
             attachInterrupt(DG_CH3, triggerISR, RISING);
             break;
          case TRIGGER_FALLING:
             attachInterrupt(DG_CH3, triggerISR, FALLING);
             break; 
          case TRIGGER_ALL:
             attachInterrupt(DG_CH3, triggerISR, CHANGE);
             break;
        }
       break;    
#endif
#endif
  }
  //Store Vars
  triggerDir = dir;
  triggerSource = source;    
}

// ------------------------
void sampleWaves(boolean wTimeout)	{
// ------------------------
	if(wTimeout)
		// setup timed interrupt to terminate scanning if trigger not found
		startScanTimeout(tDly);

	// start sampling loop - until timeout or trigger
	startSampling(sDly);
	// disable scan timeout timer
	Timer2.pause();
}


// local operations below

// ------------------------
void startScanTimeout(int16_t mSec)	{
// ------------------------
	// interrupt triggers at 1
	Timer2.setCount(2);
	Timer2.setPeriod(mSec * 1000);
	Timer2.resume();
}

/*
Custom handler for Trigger line interrupt (EXTI8). This avoids the 
overhead of demux'ing interrupt line and calling handler, and is faster.
Arduino Core - exti.c has to be modified. Comment out the function __irq_exti9_5
defined there, or add __weak qualifier.
See: 
	http://www.stm32duino.com/viewtopic.php?f=3&t=1816
	https://github.com/leaflabs/libmaple/blob/master/notes/interrupts.txt
	
// ------------------------
extern "C" void __irq_exti9_5(void) {
// ------------------------
	// custom interrupt handler for exti 5 to 9
	// since we have only one interrupt always assume it is Trigger -> exti 8
	
	asm volatile(
		"	cbnz %[triggered], fin_trig		\n\t"	// if(!triggered)
		"	cbnz %[minSamples], validCond 	\n\t"	// if(!minSamplesAcquired)
		"	cmp %[sIndex], %[halfSamples]	\n\t"	// if(sIndex < NUM_SAMPLES/2)
		"	bcc fin_trig					\n\t"
	
		"validCond:							\n\t"
		"	mov %[tIndex], %[sIndex]		\n\t"	// tIndex = sIndex;
		"	mov %[triggered], #1			\n\t"	// triggered = true;
		
		"fin_trig:							\n\t"
		"	ldr r1, =0x40010400				\n\t"	// load EXTI base address
		"	mov r0, #0x03E0					\n\t"	// clear all 5-9 interrupts 
		"	str r0, [r1, #20]				\n\t"	// into EXTI_PR
		"	nop								\n\t"
		"	nop								\n\t"
		
		: [triggered] "+r" (triggered), [tIndex] "+r" (tIndex)
		: [sIndex] "r" (sIndex), [minSamples] "r" (minSamplesAcquired), [halfSamples] "I" (NUM_SAMPLES/2)
		: "r0", "r1", "cc"
	);
}
*/


// ------------------------
void triggerISR(void) {
// ------------------------
	if(!triggered)	{
		// skip this trigger if min samples not acquired
		if(!minSamplesAcquired && (sIndex < NUM_SAMPLES/2))
			return;
		
		// snap the position where trigger occurred
		tIndex = sIndex;
		// avoid multiple triggering
		triggered = true;
	}
} 

// ------------------------
void scanTimeoutISR(void) {
// ------------------------
	keepSampling = false;
	// disable scan timeout timer
	Timer2.pause();
} 

// ------------------------
void startSampling(int16_t lDelay)	{
// ------------------------
	keepSampling = true;
	minSamplesAcquired = false;
	uint16_t lCtr = 0;
	
	// clear old dataset
	samplingTime = 0;
	triggered = false;
	sIndex = 0;
	
	prevTime = micros();
	
	if(lDelay < 0)	{

		asm volatile(
			"	ldrh r9, [%[sIndex]]			\n\t"	// load sIndex value

			"top_1:								\n\t"
			"	ldrb r0, [%[keepSampling]]		\n\t"	// while(keepSampling)
			"	cbz r0, finished_1				\n\t"

			"	ldr r1, =0x40012400				\n\t"	// load ADC1 base address, ADC2 = +0x400

			"	ldr r0, [r1, #0x4C]				\n\t"	// get and save ADC1 DR
			"	strh r0, [%[ch1], r9, lsl #1]	\n\t"
#ifdef ADD_AN2       
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"
#endif

#ifdef DSO_150
      " ldr r1, =0x40010C00       \n\t" // load GPIOB address
#else
			"	ldr r1, =0x40010800				\n\t"	// load GPIOA address
#endif     
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_1				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;
#ifdef ADD_AN2   
			"	stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
#else
      " stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr]} \n\t"
      " bl %[snapMicros]        \n\t" // micros() - r0 contains the 32bit result
      " ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr]} \n\t"
#endif
			"notOverflowed_1:					\n\t"
			"	strh r9, [%[sIndex]]			\n\t"	// save sIndex

			"	ldrb r0, [%[triggered]]			\n\t"	// if(triggered)
			"	cbz r0, notTriggered_1			\n\t"

			"	adds %[lCtr], #1				\n\t"	// lCtr++
			"	cmp %[lCtr], %[halfSamples]		\n\t"	// if(lCtr == NUM_SAMPLES/2)
			"	beq finished_1					\n\t"

			"notTriggered_1:					\n\t"
			"	b top_1							\n\t"
			"finished_1:						\n\t"

			: 
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr), 
#ifdef ADD_AN2       
				[ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#else
        [ch1] "r" (ch1Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#endif
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);		
	
	}
	else if(lDelay == 0)	{
		
		asm volatile(
			"	ldrh r9, [%[sIndex]]			\n\t"	// load sIndex value

			"top_2:								\n\t"
			"	ldrb r0, [%[keepSampling]]		\n\t"	// while(keepSampling)
			"	cbz r0, finished_2				\n\t"

			"	ldr r1, =0x40012400				\n\t"	// load ADC1 base address, ADC2 = +0x400

			"waitADC1_2:						\n\t"
			"	ldr r0, [r1, #0]				\n\t"	// ADC1 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC1_2					\n\t"
#ifdef ADD_AN2  
			"waitADC2_2:						\n\t"
			"	ldr r0, [r1, #0x400]			\n\t"	// ADC2 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC2_2					\n\t"
#endif
			"	ldr r0, [r1, #0x4C]				\n\t"	// get and save ADC1 DR
			"	strh r0, [%[ch1], r9, lsl #1]	\n\t"
#ifdef ADD_AN2       
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"
#endif

#ifdef DSO_150
      " ldr r1, =0x40010C00       \n\t" // load GPIOB address
#else
      " ldr r1, =0x40010800       \n\t" // load GPIOA address
#endif     
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_2				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;

#ifdef ADD_AN2   
			"	stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
#else
      " stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr]} \n\t"
      " bl %[snapMicros]        \n\t" // micros() - r0 contains the 32bit result
      " ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr]} \n\t"
#endif

			"notOverflowed_2:					\n\t"
			"	strh r9, [%[sIndex]]			\n\t"	// save sIndex

			"	ldrb r0, [%[triggered]]			\n\t"	// if(triggered)
			"	cbz r0, notTriggered_2			\n\t"

			"	adds %[lCtr], #1				\n\t"	// lCtr++
			"	cmp %[lCtr], %[halfSamples]		\n\t"	// if(lCtr == NUM_SAMPLES/2)
			"	beq finished_2					\n\t"

			"notTriggered_2:					\n\t"
			"	b top_2							\n\t"
			"finished_2:						\n\t"

			: 
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr), 
#ifdef ADD_AN2       
        [ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#else
        [ch1] "r" (ch1Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#endif        
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);		
		
	}
	else	{
		
		asm volatile(
			"	ldrh r9, [%[sIndex]]			\n\t"	// load sIndex value

			"top_3:								\n\t"
			"	ldrb r0, [%[keepSampling]]		\n\t"	// while(keepSampling)
			"	cbz r0, finished_3				\n\t"

			"	ldr r1, =0x40012400				\n\t"	// load ADC1 base address, ADC2 = +0x400

			"waitADC1_3:						\n\t"
			"	ldr r0, [r1, #0]				\n\t"	// ADC1 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC1_3					\n\t"
#ifdef ADD_AN2  
			"waitADC2_3:						\n\t"
			"	ldr r0, [r1, #0x400]			\n\t"	// ADC2 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC2_3					\n\t"
#endif
			"	ldr r0, [r1, #0x4C]				\n\t"	// get and save ADC1 DR
			"	strh r0, [%[ch1], r9, lsl #1]	\n\t"
#ifdef ADD_AN2
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"
#endif
#ifdef DSO_150
      " ldr r1, =0x40010C00       \n\t" // load GPIOB address
#else
      " ldr r1, =0x40010800       \n\t" // load GPIOA address
#endif     
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_3				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;

#ifdef ADD_AN2  
			"	stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr], %[tDelay]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr], %[tDelay]}	\n\t"
#else
      " stmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr], %[tDelay]}  \n\t"
      " bl %[snapMicros]        \n\t" // micros() - r0 contains the 32bit result
      " ldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[dCH], %[lCtr], %[tDelay]}  \n\t"
#endif

			"notOverflowed_3:					\n\t"
			"	strh r9, [%[sIndex]]			\n\t"	// save sIndex

			"	ldrb r0, [%[triggered]]			\n\t"	// if(triggered)
			"	cbz r0, notTriggered_3			\n\t"

			"	adds %[lCtr], #1				\n\t"	// lCtr++
			"	cmp %[lCtr], %[halfSamples]		\n\t"	// if(lCtr == NUM_SAMPLES/2)
			"	beq finished_3					\n\t"

			"notTriggered_3:					\n\t"
			"	mov r0, %[tDelay]      			\n\t"	// inter sample delay
			"1:           						\n\t"
			"	subs r0, #1            			\n\t"
			"	bhi 1b                 			\n\t"

			"	b top_3							\n\t"
			"finished_3:						\n\t"

			: 
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr), 
#ifdef ADD_AN2       
        [ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#else
        [ch1] "r" (ch1Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
#endif        
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2), [tDelay] "r" (lDelay),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);		
	}
}


// ------------------------
inline void snapMicros()	{
// ------------------------
	samplingTime = micros() - prevTime;
	prevTime = micros();
	minSamplesAcquired = true;
}


// ------------------------
void dumpSamples()	{
// ------------------------
	float timePerSample = ((float)samplingTime) / NUM_SAMPLES;
	DBG_PRINT("Net sampling time (us): "); DBG_PRINTLN(samplingTime);
	DBG_PRINT("Per Sample (us): "); DBG_PRINTLN(timePerSample);
	DBG_PRINT("Timebase: "); DBG_PRINT(getTimebaseLabel()); DBG_PRINTLN("/div");
	DBG_PRINT("Actual Timebase (us): "); DBG_PRINTLN(timePerSample * 25);
	DBG_PRINT("CH1 Coupling: "); DBG_PRINT(cplNames[couplingPos]); DBG_PRINT(", Range: "); DBG_PRINT(rngNames[rangePos]); DBG_PRINTLN("/div");
#ifdef ADD_AN2  
	DBG_PRINTLN("CH2 Coupling: --, Range: +-2048");
#endif

	DBG_PRINT("Triggered: "); 
	if(triggered)	{
		DBG_PRINTLN("YES"); 
	}
	else {
		DBG_PRINTLN("NO"); 
	}

	// calculate stats on this sample set
	calculateStats();
	
	DBG_PRINT("CH1 Stats");
	if(wStats.mvPos)	{
		DBG_PRINTLN(" (mV):");
	}
	else	{
		DBG_PRINTLN(" (V):");
	}
		
	DBG_PRINT("\tVmax: "); DBG_PRINT(wStats.Vmaxf);
	DBG_PRINT(", Vmin: "); DBG_PRINT(wStats.Vminf);
	DBG_PRINT(", Vavr: "); DBG_PRINT(wStats.Vavrf);
	DBG_PRINT(", Vpp: "); DBG_PRINT(wStats.Vmaxf - wStats.Vminf);
	DBG_PRINT(", Vrms: "); DBG_PRINTLN(wStats.Vrmsf);

	if(wStats.pulseValid)	{
		DBG_PRINT("\tFreq: "); DBG_PRINT(wStats.freq);
		DBG_PRINT(", Cycle: "); DBG_PRINT(wStats.cycle); DBG_PRINT(" ms");
		DBG_PRINT(", PW: "); DBG_PRINT(wStats.avgPW/1000); DBG_PRINT(" ms");
		DBG_PRINT(", Duty: "); DBG_PRINT(wStats.duty); DBG_PRINTLN(" %");
	}
	else	{
		DBG_PRINT("Freq: "); DBG_PRINT("--");
		DBG_PRINT(", Cycle: "); DBG_PRINT("--");
		DBG_PRINT(", PW: "); DBG_PRINT("--");
		DBG_PRINT(", Duty: "); DBG_PRINTLN("--");
	}
	
	DBG_PRINTLN("");

	DBG_PRINT("Time\tCH1");

#ifdef ADD_AN2  
  DBG_PRINT("\tCH2");
#endif
	 
  DBG_PRINT("\tD_CH1");
  DBG_PRINT("\tD_CH2");


#ifdef ADD_D3
  DBG_PRINT("\tD_CH3");
#endif

    DBG_PRINTLN("");
    
	uint16_t idx = 0;
	
	// sampling stopped at sIndex - 1
	for(uint16_t k = sIndex; k < NUM_SAMPLES; k++)
		printSample(k, timePerSample * idx++);
	
	for(uint16_t k = 0; k < sIndex; k++)
		printSample(k, timePerSample * idx++);
	
	DBG_PRINTLN("");
}


// ------------------------
void printSample(uint16_t k, float timeStamp) {
// ------------------------
	DBG_PRINT(timeStamp);
	DBG_PRINT("\t");
	DBG_PRINT((ch1Capture[k] - zeroVoltageA1) * adcMultiplier[rangePos]);
	DBG_PRINT("\t");
#ifdef ADD_AN2    
	DBG_PRINT(ch2Capture[k] - zeroVoltageA2);
	DBG_PRINT("\t");
#endif

#ifdef DSO_150
	DBG_PRINT((bitStore[k] & 0x2000) ? 1 : 0);
	DBG_PRINT("\t");
  DBG_PRINT((bitStore[k] & 0x4000) ? 1 : 0);
  DBG_PRINT("\t");
#ifdef ADD_D3  
	DBG_PRINT((bitStore[k] & 0x8000) ? 1 : 0);
#endif
#else
  DBG_PRINT((bitStore[k] & 0x2000) ? 1 : 0);
  DBG_PRINT("\t");
  DBG_PRINT((bitStore[k] & 0x4000) ? 1 : 0);
  DBG_PRINT("\t");
#endif
	
	if(triggered && (tIndex == k))
		DBG_PRINT("\t<--TRIG");
	
	DBG_PRINTLN();
}


// ------------------------
void calculateTraceZero(int waveID)    {
// ------------------------
  // calculate zero only if switch is in GND position
  if(couplingPos != CPL_GND)
    return;

#ifdef ADD_AN2 
  if(waveID > 1)
    return;
  uint16_t *wave = (waveID == 0)? ch1Capture : ch2Capture;

#else
  if(waveID > 0)
    return;
    
uint16_t *wave =  ch1Capture;
#endif
  
  // zero the trace
  int32_t sumSamples = 0;

  for(uint16_t k = 0; k < NUM_SAMPLES; k++) {
    sumSamples += wave[k];
  }

  uint16_t Vavr = sumSamples/NUM_SAMPLES;

  if(waveID == 0) {
    zeroVoltageA1 = Vavr;
    saveParameter(PARAM_ZERO1, zeroVoltageA1);
  }
  else  {
    zeroVoltageA2 = Vavr;
    saveParameter(PARAM_ZERO2, zeroVoltageA2);
  }
}


enum {TESTMODE_GND,TESTMODE_PULSE,TESTMODE_3V3,TESTMODE_0V1};

void setTestPin(uint8_t mode)
{
  switch (mode)
  {
      case TESTMODE_GND:
          pinMode(TEST_AMPSEL,INPUT_ANALOG);
          pinMode(TEST_WAVE_PIN, OUTPUT);
          digitalWrite(TEST_WAVE_PIN,LOW);
      break;         
      case TESTMODE_3V3:
          pinMode(TEST_AMPSEL,INPUT_PULLUP);
          pinMode(TEST_WAVE_PIN, OUTPUT);
          digitalWrite(TEST_WAVE_PIN,HIGH);
      break;  
      case TESTMODE_0V1:
          pinMode(TEST_AMPSEL,OUTPUT);
          digitalWrite(TEST_AMPSEL,LOW);
          pinMode(TEST_WAVE_PIN, OUTPUT);
          digitalWrite(TEST_WAVE_PIN,HIGH);
      break;              
      case TESTMODE_PULSE:
          pinMode(TEST_AMPSEL,INPUT_ANALOG);
          // start 1KHz square wave
          pinMode(TEST_WAVE_PIN, PWM);
          Timer3.setPeriod(1000);
          pwmWrite(TEST_WAVE_PIN, 17850);
      break;
  }  
}

void loopThroughRange(uint8_t rangeStart,uint8_t rangecount,uint16_t *pResults)
{
  uint16_t *wave =  ch1Capture;
  int32_t sumSamples = 0;
  for(uint8_t cnt= rangeStart;cnt<rangeStart+rangecount;cnt++)
  {
      //Set Range
      setVoltageRange(cnt);
      //Wait for new Data
      sampleWaves(true);
      delay(400);
      //Calc Average
      sumSamples = 0; 
      for(uint16_t k = 0; k < NUM_SAMPLES; k++) 
        sumSamples += wave[k];
      pResults[cnt] = sumSamples/NUM_SAMPLES;
      DBG_PRINT(rngNames[cnt]);
      DBG_PRINT(" Sum: ");
      DBG_PRINTLN(sumSamples);    
      
  }
}

void autoCal(void)
{
    //We need to store calibration data for each voltage range
    uint16_t zeroOffset[RNG_5mV+1];
    uint16_t val3V3[RNG_5mV+1];
    uint16_t val0V1[RNG_5mV+1];
    uint16_t tVal3V3[RNG_5mV+1];
    uint16_t tVal0V1[RNG_5mV+1];
    float mulVal[RNG_5mV+1];
    float highcal =3.3;
    float lowcal=0.14;
    const float rangeMultiplier[] = {0.05,0.1,0.2,0.5,1,2,5,10,20,50,100,200}; 
    
    uint8_t ii=0;
    
    for(ii=0;ii!=RNG_5mV;ii++)
    {
      zeroOffset[ii] = 0;
      val3V3[ii] = 0;  
      val0V1[ii] = 0;
      tVal3V3[ii] = 0;   
      tVal0V1[ii] = 0;
    }
    
    
    //Set Trigger to NORM
    setTriggerType(TRIGGER_AUTO);
    //Set timebase to 50uS
    setTimeBase(T50US);
    //Start capturing
    hold = false;

    //Disable Wavweform output
    setTestPin(TESTMODE_GND);
    //Capture full range for Zero Cal
    loopThroughRange(RNG_20V,RNG_5mV+1,zeroOffset);

    //Set 3.3 V
    setTestPin(TESTMODE_3V3);
    //Capture first part of Range
    loopThroughRange(RNG_20V,RNG_5mV+1,val3V3);
    
    //Set 0.1 V
    setTestPin(TESTMODE_0V1);
    //Capture first part of Range
    loopThroughRange(RNG_20V,RNG_5mV+1,val0V1);

    //Subtract zero offsets
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        val3V3[ii]= val3V3[ii]- zeroOffset[ii];
        val0V1[ii]= val0V1[ii]- zeroOffset[ii];
    }

    //Time to do some calulation...
    //The only value we  can reasonably rely on is the 3.3V reference value
    //So the first task is to calulate the voltage for the low reference (~140mV)
    //We use the value measured at the x2 range as it has ther biggest overlap
    lowcal = (float)val0V1[5]/(float)val3V3[5]*highcal;
    DBG_PRINT("HighCal: "); 
    DBG_PRINTLN(highcal); 
    DBG_PRINT("LowCal: "); 
    DBG_PRINTLN(lowcal); 

    //Now we can calculate the multipliers based on these values
    mulVal[0] = highcal/((float)val3V3[0]);
    mulVal[1] = highcal/((float)val3V3[1]);
    mulVal[2] = highcal/((float)val3V3[2]);     
    mulVal[3] = highcal/((float)val3V3[3]);
    mulVal[4] = highcal/((float)val3V3[4]);
    
    mulVal[5] = highcal/((float)val3V3[5]);    
    mulVal[5] = mulVal[5] + (lowcal/((float)val0V1[5]));  
    mulVal[5] = mulVal[5]/2;
    
    mulVal[6] = highcal/((float)val3V3[6]); 
    mulVal[6] = mulVal[6] + (lowcal/((float)val0V1[6]));  
    mulVal[6] = mulVal[6]/2;
    
    mulVal[7] = lowcal/((float)val0V1[7]);  
    mulVal[8] = lowcal/((float)val0V1[8]);  
    mulVal[9] = lowcal/((float)val0V1[9]);  
    mulVal[10] = lowcal/((float)val0V1[10]);              
    mulVal[11] = lowcal/((float)val0V1[11]);  
    
    //Set Trigger to 0
    //Set Trigger mode to Single
    //Set Range
    //set Test output to Low

    //While not triggered
        //Increase Trigger
        //Toggle output to High
        //Wait a little bit
        //Toggle output to Low    


    //Print results
    DBG_PRINT("Zero Point:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT(zeroOffset[ii]); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");     

    DBG_PRINT("3.3V:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT(val3V3[ii]); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");   
        
    DBG_PRINT("0.1V:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT(val0V1[ii]); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");   

    DBG_PRINT("CalMultipliers:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT(mulVal[ii]*100000.0); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");   


    DBG_PRINT("CalCheck1:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT((float)val3V3[ii]*mulVal[ii]); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");   

    DBG_PRINT("CalCheck2:");
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
        DBG_PRINT((float)val0V1[ii]*mulVal[ii]); 
        DBG_PRINT(","); 
    }
    DBG_PRINTLN("");  
    
    setTestPin(TESTMODE_PULSE);   

    //Store Values
    for(ii=0;ii<RNG_5mV+1;ii++)
    {
      adcMultiplier[ii] = mulVal[ii];
      zeroVoltageA1Cal[ii] = zeroOffset[ii];
    }
    formatSaveConfig();
    
}

