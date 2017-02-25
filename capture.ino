
// sampling delay table in quarter-microseconds
const int16_t samplingDelay[] =   {-1,  0, 14, 38, 86, 229, 468, 948, 2385, 4776, 9570, 23940};
const uint16_t timeoutDelayMs[] = {50, 50, 50, 100, 100, 100, 150, 250, 500, 1000, 2000, 4500};

int16_t sDly, tDly;
boolean minSamplesAcquired;
boolean triggerRising;
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



// ------------------------
void setTriggerRising(boolean rising)	{
// ------------------------
	// trigger changed, break out from previous sampling loop
	keepSampling = false;
	triggerRising = rising;
	// attach interrupt to trigger pin
	detachInterrupt(TRIGGER_IN);
	if(rising)
		attachInterrupt(TRIGGER_IN, triggerISR, RISING);
	else
		attachInterrupt(TRIGGER_IN, triggerISR, FALLING);
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
	
	// clear all pending 5-9 interrupts
    EXTI_BASE->PR =  0x03E0;
	
	asm volatile(
		"	cbnz %[triggered], fin_trig		\n\t"	// if(!triggered)
		"	cbnz %[minSamples], validCond 	\n\t"	// if(!minSamplesAcquired)
		"	cmp %[sIndex], %[halfSamples]	\n\t"	// if(sIndex < NUM_SAMPLES/2)
		"	bcc fin_trig					\n\t"
	
		"validCond:							\n\t"
		"	mov %[tIndex], %[sIndex]		\n\t"	// tIndex = sIndex;
		"	mov %[triggered], #1			\n\t"	// triggered = true;
		
		"fin_trig:							\n\t"
		
		: [triggered] "+r" (triggered), [tIndex] "+r" (tIndex)
		: [sIndex] "r" (sIndex), [minSamples] "r" (minSamplesAcquired), [halfSamples] "I" (NUM_SAMPLES/2)
		: "cc"
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
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"

			"	ldr r1, =0x40010800				\n\t"	// load GPIOA address
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save GPIOA IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_1				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;

			"	stmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"

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
				[ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
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

			"waitADC2_2:						\n\t"
			"	ldr r0, [r1, #0x400]			\n\t"	// ADC2 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC2_2					\n\t"

			"	ldr r0, [r1, #0x4C]				\n\t"	// get and save ADC1 DR
			"	strh r0, [%[ch1], r9, lsl #1]	\n\t"
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"

			"	ldr r1, =0x40010800				\n\t"	// load GPIOA address
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save GPIOA IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_2				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;

			"	stmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr]}	\n\t"

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
				[ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
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

			"waitADC2_3:						\n\t"
			"	ldr r0, [r1, #0x400]			\n\t"	// ADC2 SR
			"	lsls r0, r0, #30				\n\t"	// get to EOC bit
			"	bpl	waitADC2_3					\n\t"

			"	ldr r0, [r1, #0x4C]				\n\t"	// get and save ADC1 DR
			"	strh r0, [%[ch1], r9, lsl #1]	\n\t"
			"	ldr r0, [r1, #0x44C]			\n\t"	// get and save ADC2 DR
			"	strh r0, [%[ch2], r9, lsl #1]	\n\t"

			"	ldr r1, =0x40010800				\n\t"	// load GPIOA address
			"	ldr r0, [r1, #0x08]				\n\t"	// get and save GPIOA IDR
			"	strh r0, [%[dCH], r9, lsl #1]	\n\t"

			"	adds r9, #1						\n\t"	// increment sIndex
			"	cmp r9, %[nSamp]				\n\t"	// if(sIndex == NUM_SAMPLES)
			"	bne notOverflowed_3				\n\t"
			"	mov r9, #0						\n\t"	// sIndex = 0;

			"	stmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr], %[tDelay]}	\n\t"
			"	bl %[snapMicros]				\n\t"	// micros() - r0 contains the 32bit result
			"	ldmfd sp!,{r0, r1, r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[ch2], %[dCH], %[lCtr], %[tDelay]}	\n\t"

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
				[ch1] "r" (ch1Capture), [ch2] "r" (ch2Capture), [dCH] "r" (bitStore), [lCtr] "r" (lCtr),
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
	DBG_PRINTLN("CH2 Coupling: --, Range: +-2048");

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
	DBG_PRINTLN("Time\tCH1\tCH2\tD_CH1\tD_CH2");
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
	DBG_PRINT(ch2Capture[k] - zeroVoltageA2);
	DBG_PRINT("\t");
	DBG_PRINT((bitStore[k] & 0x2000) ? 1 : 0);
	DBG_PRINT("\t");
	DBG_PRINT((bitStore[k] & 0x4000) ? 1 : 0);
	
	if(triggered && (tIndex == k))
		DBG_PRINT("\t<--TRIG");
	
	DBG_PRINTLN();
}

