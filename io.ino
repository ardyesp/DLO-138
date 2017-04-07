int16_t trigLevel = 0;


// ------------------------
void initIO()	{
// ------------------------
	// set pin I/O direction
	pinMode(BOARD_LED, OUTPUT);
	pinMode(AN_CH1, INPUT_ANALOG);

  #ifdef ADD_AN2
	pinMode(AN_CH2, INPUT_ANALOG);
  #endif

	pinMode(DG_CH1, INPUT_PULLDOWN);
	pinMode(DG_CH2, INPUT_PULLDOWN);

  #ifdef ADD_D3
  pinMode(DG_CH3, INPUT_PULLDOWN);
  #endif

  pinMode(CPLSEL,INPUT_ANALOG);
  
	pinMode(TRIGGER_IN, INPUT_PULLUP);
	
	// calibrate the ADC channels at startup
	adc_calibrate(ADC1);
	adc_calibrate(ADC2);
  
	setADC();

	// start 1KHz square wave
	pinMode(TEST_WAVE_PIN, PWM);
	Timer3.setPeriod(1000);
	pwmWrite(TEST_WAVE_PIN, 17850);
	DBG_PRINTLN("Test square wave started");

 #ifdef DSO_150  
  pinMode(VSENSSEL0, OUTPUT);
  pinMode(VSENSSEL1, OUTPUT);
  pinMode(VSENSSEL2, OUTPUT);
  pinMode(VSENSSEL3, OUTPUT);    

  attachInterrupt(ENCODER_A, readEncoderISR, CHANGE);
  attachInterrupt(ENCODER_B, readEncoderISR, CHANGE);  
 #else
	// input button and encoder
	pinMode(ENCODER_SW, INPUT_PULLUP);
	pinMode(ENCODER_A, INPUT_PULLUP);
	pinMode(ENCODER_B, INPUT_PULLUP);
	pinMode(BTN4, INPUT_PULLUP);

	attachInterrupt(ENCODER_SW, readESwitchISR, FALLING);
	attachInterrupt(BTN4, btn4ISR, CHANGE);
	
#ifdef USE_ENCODER
	attachInterrupt(ENCODER_A, readEncoderISR, CHANGE);
	attachInterrupt(ENCODER_B, readEncoderISR, CHANGE);
#else
	attachInterrupt(ENCODER_A, readASwitchISR, FALLING);
	attachInterrupt(ENCODER_B, readBSwitchISR, FALLING);
#endif	
#endif

	// init trigger level PWM
	// start 20KHz square wave on trigger out reference and negative v gen
	Timer4.setPeriod(50);
	pinMode(TRIGGER_LEVEL, PWM);
  
 #ifndef DSO_150 
	pinMode(VGEN, PWM);
	pwmWrite(VGEN, 700);
#endif
	
	blinkLED();
	
	// init scan timeout timer
	initScanTimeout();
}


// ------------------------
void setADC()	{
// ------------------------
	int pinMapADCin1 = PIN_MAP[AN_CH1].adc_channel;
	int pinMapADCin2 = PIN_MAP[AN_CH2].adc_channel;

	// opamp is low impedance, set fastest sampling 
	adc_set_sample_rate(ADC1, ADC_SMPR_1_5);
	adc_set_sample_rate(ADC2, ADC_SMPR_1_5);

	adc_set_reg_seqlen(ADC1, 1);
	ADC1->regs->SQR3 = pinMapADCin1;
	// set ADC1 continuous mode
	ADC1->regs->CR2 |= ADC_CR2_CONT; 	
	// set ADC1 in regular simultaneous mode
	ADC1->regs->CR1 |= 0x60000; 		
	ADC1->regs->CR2 |= ADC_CR2_SWSTART;

	// set ADC2 continuous mode
	ADC2->regs->CR2 |= ADC_CR2_CONT; 	
	ADC2->regs->SQR3 = pinMapADCin2;
}


// ------------------------
void blinkLED()	{
// ------------------------
	LED_ON;
	delay(10);
	LED_OFF;
}


// ------------------------
void initScanTimeout()	{
// ------------------------
	Timer2.setChannel1Mode(TIMER_OUTPUTCOMPARE);
	Timer2.pause();
	Timer2.setCompare1(1);
	Timer2.attachCompare1Interrupt(scanTimeoutISR);
}


// ------------------------
int16_t getTriggerLevel()	{
// ------------------------
	return trigLevel;
}


// ------------------------
void setTriggerLevel(int16_t tLvl)	{
// ------------------------
	// 600 = 20% duty
	// 1800 = 50%
	trigLevel = tLvl;
	pwmWrite(TRIGGER_LEVEL, 1800 + trigLevel);
}

// ------------------------
void setVRange(uint8_t voltageRange) 
// ------------------------
{
  unsigned char val = tbBitval[voltageRange];
  
  digitalWrite(VSENSSEL0, (val>>0) & 0x01);
  digitalWrite(VSENSSEL1, (val>>1) & 0x01);
  digitalWrite(VSENSSEL2, (val>>2) & 0x01);
  digitalWrite(VSENSSEL3, (val>>3) & 0x01);
}

// ------------------------
void readInpSwitches()	{
// ------------------------
	static uint8_t couplingOld, rangeOld;
  uint16_t cpl, pos1, pos2;
  adc_reg_map *ADC1regs = ADC1->regs;

  // ADC1 and ADC2 are free running at max speed
#ifndef DSO_150
	// change to switch 1 
	ADC1regs->SQR3 = PIN_MAP[VSENSSEL2].adc_channel;
	delayMicroseconds(100);
	pos2 = (uint16_t) (ADC1regs->DR & ADC_DR_DATA);
	
	ADC1regs->SQR3 = PIN_MAP[VSENSSEL1].adc_channel;
	delayMicroseconds(100);
	pos1 = (uint16_t) (ADC1regs->DR & ADC_DR_DATA);
#endif
	ADC1regs->SQR3 = PIN_MAP[CPLSEL].adc_channel;
	delayMicroseconds(100);
	cpl = (uint16_t) (ADC1regs->DR & ADC_DR_DATA);

#ifdef DSO_150
	if(cpl < 400)
		couplingPos = CPL_GND;
	else if(cpl < 2000)
		couplingPos = CPL_DC;
	else
		couplingPos = CPL_AC;
#else
  if(cpl < 400)
    couplingPos = CPL_GND;
  else if(cpl < 2000)
    couplingPos = CPL_DC;
  else
    couplingPos = CPL_AC;
#endif

  #ifndef DSO_150
	if(pos1 < 400)
		rangePos = RNG_1V;
	else if(pos1 < 2000)
		rangePos = RNG_0_1V;
	else
		rangePos = RNG_10mV;
	
	if(pos2 < 400)
		rangePos -= 2;
	else if(pos2 < 2000)
		rangePos -= 1;
	else
		rangePos -= 0;
	#endif

  
	// check if switch position changed from previous snap
	if(couplingPos != couplingOld)	{
		couplingOld = couplingPos;
    //Auto Zero-Cal
 //   if (couplingPos == CPL_GND)
 //   {
 //     calculateTraceZero(0);
 //     changeYCursor(A1, -GRID_HEIGHT/2 - 1);
 //   }
		repaintLabels();
	}

  #ifndef DSO_150
	if(rangePos != rangeOld)	{
		rangeOld = rangePos;
		repaintLabels();
	}
	#endif
	// read the negative voltage generator
	// ***

	// switch ADC1 back to capture channel
	ADC1regs->SQR3 = PIN_MAP[AN_CH1].adc_channel;
	delayMicroseconds(100);
}

