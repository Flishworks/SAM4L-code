
#include "SCIF_SAM4L.h"
#include "PM_SAM4L.h"

#define SCIF_ADDR_UNLOCK(value)	(0xAA000000 | (value))	// Unlock PM register (Key | offset address)

///////////////////////////////////////////////////////////////////////////////////////////////////
// General Control
///////////////////////////////////////////////////////////////////////////////////////////////////
SCIF_ERROR_TYPE SCIF_init(void)
{
	// If not enabled, turn it on
	if (!(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF))
	{
		PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBCMASK_OFFSET);
		PM->bf.PBCMASK.reg |= PM_PBCMASK_SCIF;
	}
		
	// Mark some settings
	mSCIF.bInit = 1;
	mSCIF.state = SCIF_STATE_IDLE;
		
	return SCIF_SUCCSESS;
}

// NOT TESTED YET
SCIF_ERROR_TYPE SCIF_disable(void)
{
	// If not enabled, turn it on
	
	// check to make sure all clocks controlled by this are disabled first.
	
	if (PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)
	{
		PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBCMASK_OFFSET);
		PM->bf.PBCMASK.reg &= ~PM_PBCMASK_SCIF;
	}
	
	// Mark some settings
	mSCIF.bInit = 0;
	mSCIF.state = SCIF_STATE_UNKNOWN;
	
	return SCIF_SUCCSESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OSC0 Control/Operation - §13.6.1
///////////////////////////////////////////////////////////////////////////////////////////////////
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_OSC0(uint32_t fXTAL, SCIF_OSCCTRLn_STARTUP_TYPE startup)
{
	// add error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	// change range calc... <6e5 and >16e6 OOR
	
	// Figure gain and range
	SCIF_OSCCTRLn_GAIN_TYPE gain;
	if ((fXTAL >= 6e5) && (fXTAL <= 2e6))		gain = SCIF_OSCCTRLn_GAIN_0;
	else if ((fXTAL > 2e6) && (fXTAL <= 4e6))	gain = SCIF_OSCCTRLn_GAIN_1;
	else if ((fXTAL > 4e6) && (fXTAL <= 8e6))	gain = SCIF_OSCCTRLn_GAIN_2;
	else if ((fXTAL > 8e6) && (fXTAL <= 16e6))	gain = SCIF_OSCCTRLn_GAIN_3;
	else return SCIF_ERROR_OSC0_SRC_XTAL_OOR;

	// Set external oscillator (OSC0) - this is the source for the PLL, this should be in an SCIF_activateClkSource function
	SCIF->bf.UNLOCK.reg		= SCIF_ADDR_UNLOCK(SCIF_OSCCTRL0_OFFSET);
	SCIF->bf.OSCCTRL0.reg	= SCIF_OSCCTRL0_STARTUP(startup)
							| SCIF_OSCCTRL0_GAIN(gain)
							| SCIF_OSCCTRL0_MODE
							| SCIF_OSCCTRL0_OSCEN; // These are based on setting defined in example, board settings.
	while (!(SCIF->bf.PCLKSR.bit.OSC0RDY)); // Block until SCIF is ready
	
	mSCIF.bEnabled.CLK.OSC0 = 1;
	mSCIF.freq.CLK.OSC0		= fXTAL;
	
	return SCIF_SUCCSESS;
}
SCIF_ERROR_TYPE SCIF_resetSrcClk_OSC0(void)
{
	// add error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	
	// Set and Enable external oscillator (OSC0) - this is the source for the PLL, this should be in an SCIF_activateClkSource function
	SCIF->bf.UNLOCK.reg	 = SCIF_ADDR_UNLOCK(SCIF_OSCCTRL0_OFFSET);
	SCIF->bf.OSCCTRL0.reg = SCIF_OSCTRLn_RESET_VAL; // These are based on setting defined in example, board settings.
	
	mSCIF.bEnabled.CLK.OSC0 = 0;
	mSCIF.freq.CLK.OSC0		= 0;
	
	return SCIF_SUCCSESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PLL Control/Operation - §13.6.2
///////////////////////////////////////////////////////////////////////////////////////////////////
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_PLL(uint8_t numPLL, uint8_t pllMul, uint8_t pllDiv,
										 uint8_t pllOpt, SCIF_PLL_CLKSRC_TYPE pllOsc)
{
	// add error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numPLL < SCIF_PLL_NUM)) return SCIF_ERROR_PLL_NUM_NO_EXIST;
	if (pllMul > PLLMUL_MAX_VAL) return SCIF_ERROR_PLLMUL_OOR;
	if (pllDiv > PLLDIV_MAX_VAL) return SCIF_ERROR_PLLDIV_OOR;
	if (pllOpt > PLLOPT_MAX_VAL) return SCIF_ERROR_PLLOPT_OOR;
	
	switch (pllOsc)
	{
	case SCIF_PLL_CLKSRC_OSC0:
		if (!mSCIF.bEnabled.CLK.OSC0 || !SCIF->bf.OSCCTRL0.bit.OSCEN) return SCIF_ERROR_PLL_OSC0_SRC_NOT_ENABLED;
		break;
	case SCIF_PLL_CLKSRC_GCLK9:
		if (!mSCIF.bEnabled.GCLK.GCLK9_PLL0_PEVC1 || !SCIF->bf.Gcctrl[9].bf.GCCTRL.bit.CEN) return SCIF_ERROR_PLL_GCLK9_SRC_NOT_ENABLED;
		break;
	default :
		return SCIF_ERROR_PLLOSC_OOR;
	}
	
	uint32_t fREF;	// Temp variable for holding the calculate frequency
	// Figure what the source frequency is - Also (need to check that these are both enabled....
	if (pllOsc == SCIF_PLL_CLKSRC_OSC0) fREF = mSCIF.freq.CLK.OSC0;
	else if (pllOsc == SCIF_PLL_CLKSRC_GCLK9) fREF = 0;			// NEED TO FIGURE WHAT TO DO AT SOME POINT
	
	// Determine the output frequency ----------- NOTE: THIS DOESN'T ACCOUNT FOR PLLOPT[1]
	if (pllDiv == 0) mSCIF.freq.CLK.PLL = (2 * (pllMul + 1) * fREF);
	else mSCIF.freq.CLK.PLL = (((pllMul + 1)/pllDiv) * fREF);
	
	SCIF->bf.UNLOCK.reg	 = SCIF_ADDR_UNLOCK(SCIF_PLL_OFFSET);
	SCIF->bf.Pll[numPLL].bf.PLL.reg = SCIF_PLL_PLLCOUNT_Msk
									| SCIF_PLL_PLLMUL(pllMul)
									| SCIF_PLL_PLLDIV(pllDiv)
									| SCIF_PLL_PLLOPT(pllOpt)		// don't do anything to PLLOPT
									| SCIF_PLL_PLLOSC(pllOsc);		// Set PLL source, OSC0
	
	SCIF->bf.UNLOCK.reg	 = SCIF_ADDR_UNLOCK(SCIF_PLL_OFFSET);
	SCIF->bf.Pll[numPLL].bf.PLL.reg |= SCIF_PLL_PLLEN;				// Enable
	
	while (!(SCIF->bf.PCLKSR.bit.PLL0LOCK)); // Block until PLL is locked
	
	mSCIF.bEnabled.CLK.PLL = 1;
	
	return SCIF_SUCCSESS;
}
SCIF_ERROR_TYPE SCIF_resetSrcClk_PLL(uint8_t numPLL)
{
	// add error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numPLL < SCIF_PLL_NUM)) return SCIF_ERROR_PLL_NUM_NO_EXIST;
	
	//if (SCIF->bf.Pll[numPLL].bf.PLL.reg & SCIF_PLL_PLLEN)
	//{
		//
	//}
	//else
	//{
		//
	//}
	SCIF->bf.UNLOCK.reg	 = SCIF_ADDR_UNLOCK(SCIF_PLL_OFFSET);
	SCIF->bf.Pll[numPLL].bf.PLL.reg &= ~SCIF_PLL_PLLEN;				// Disable

	SCIF->bf.UNLOCK.reg	 = SCIF_ADDR_UNLOCK(SCIF_PLL_OFFSET);	
	SCIF->bf.Pll[numPLL].bf.PLL.reg = SCIF_PLL_RESET_VAL;			// Clear register
	
	mSCIF.bEnabled.CLK.PLL	= 0;
	mSCIF.freq.CLK.PLL		= 0;
	while (!(SCIF->bf.PCLKSR.bit.PLL0LOCKLOST)); // Block until PLL is lock is lost
	
	return SCIF_SUCCSESS;
}

// ADD A PURE DISABLE

///////////////////////////////////////////////////////////////////////////////////////////////////
// DFLL Control/Operation - §13.6.3
///////////////////////////////////////////////////////////////////////////////////////////////////

// The DFLL takes an input clock, and is able to multiply it get a high speed clock in the range of
// 20 - 150MHz

// fDFLL output freq is defined by [DFLLxMUL.MUL] & [GCCTRL0.DIV] (remember, all integers):
// If [GCCTRL0.DIVEN] = 0
//		fDFLL = fRef * [DFLLxMUL.MUL]
// If [GCCTRL0.DIVEN] = 1
//		fDFLL = (fRef / 2*([GCCTRL0.DIV] + 1)) * [DFLLxMUL.MUL]

// There are two main modes of operation for this module:
// - Open Loop - Figure some basic parameters and let fly, it's tuned once and goes
// - Closed Loop - Feedback to compensate for drift caused by temp and voltage changes, will
//   continue to tune itself

// It is recommended that basically, even if you want to use open loop, to set up as a closed loop
// system first. This way [DFLLxVAL.COARSE] and [DFLLxVAL.FINE] can be set to something better than
// just dead reckoning.

// ***************** Note *******************
// There is a lot of oddness with [DFLLxVAL.COARSE] and [DFLLxVAL.FINE] - there is no formula
// listed in the data sheet, the on line ASF help doesn't appear to be quite right. I've gotten
// close some reckoning of my own (COARSE Seems to be on point, still trying to figure FINE)

// This solely turns the DFLL on, it won't start outputting a clock until a on zero value is in
// [DFLLxMUL.MUL] & ( [DFLLxSTEP.COARSE] | [DFLLxSTEP.COARSE] )
SCIF_ERROR_TYPE SCIF_enableDFLL(void)
{
	// add error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	
	// Block until DFLL ready to take commands and get the most recent settings of [DFLLxCONF]
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	
	// Enable if not already
	if (!(SCIF->bf.DFLL0CONF.reg & SCIF_DFLL0CONF_EN))
	{
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
		SCIF->bf.DFLL0CONF.reg |= SCIF_DFLL0CONF_EN;
		
		// SYNC
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	}
	
	mSCIF.bEnabled.CLK.DFLL = 1;	// system enabled, but not calibrated at all.
	
	return SCIF_SUCCSESS;
}

// This disables the DFLL, and resets all the parameters
SCIF_ERROR_TYPE SCIF_disableResetDFLL(void)
{
	// TEST ME
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;

	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	
	// Disable if not already
	if (SCIF->bf.DFLL0CONF.reg & SCIF_DFLL0CONF_EN)
	{
		// Switch to Open Loop Mode & Sync (Should loose a lock here)
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
		SCIF->bf.DFLL0CONF.reg &= ~SCIF_DFLL0CONF_MODE;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));

		// Disable & Sync
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
		SCIF->bf.DFLL0CONF.reg &= ~SCIF_DFLL0CONF_EN;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
		
		// Clear [SCIF.DFLL0CONF] back to reset values (but make sure not to touch [CALIB] and [FCD])
		uint32_t tempCONF = SCIF->bf.DFLL0CONF.reg;
		tempCONF &= (uint32_t)(SCIF_DFLL0CONF_CALIB_Msk | SCIF_DFLL0CONF_FCD);
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
		SCIF->bf.DFLL0CONF.reg = tempCONF;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
		
		// Clear & Sync [SCIF.STEP]
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0STEP_OFFSET);
		SCIF->bf.DFLL0STEP.reg = SCIF_DFLLSTEP_RESET_VAL;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
		
		// Clear & Sync [SCIF.MUL]
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0MUL_OFFSET);
		SCIF->bf.DFLL0MUL.reg = SCIF_DFLLMUL_RESET_VAL;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
		
		// Clear & Sync [SCIF.VAL]
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0VAL_OFFSET);
		SCIF->bf.DFLL0VAL.reg = SCIF_DFLLVAL_RESET_VAL;
		SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
		while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	}
	
	mSCIF.bEnabled.CLK.DFLL = 0;
	mSCIF.freq.CLK.DFLL		= 0;
	while (SCIF->bf.PCLKSR.bit.DFLL0LOCKC || SCIF->bf.PCLKSR.bit.DFLL0LOCKF); // Block until DFLL is lock lost
	
	return SCIF_SUCCSESS;
}

// This function was not discussed anywhere on the data sheet, but was offered on-line by ASF help.
// However, that doesn't even appear to be correct for COARSE values. I've discovered there's
// basically a fudge factor of 4x in there. The formula's listed below are my best guess right now.
//
// Briefly, this function figures some initial values for [DFLL0VAL.COARSE] and [DFLLVAL.FINE].
// this isn't necessarily required for closed loop operation (it is if going in blind with Open
// loop however) but it should help drastically speed up the tuning process by giving the feedback
// loop a good initial guess instead of just starting at the default starting values.
//
// The formula is from the following: http://asf.atmel.com/docs/latest/sam.drivers.twis.twis_example.sam4l_ek/html/group__dfll__group.html
SCIF_ERROR_TYPE SCIF_setInitalTuningDFLL(uint32_t fDFLL)
{
	// **** Note: One has to be careful when doing math here, no floating point. All integer based
	
	uint64_t temp;
	uint64_t fCoarse;
	
	// The formula for calculating [DFLLxVAL.COARSE] is as follows (same as online, but wit
	// multiply 4 which makes the result of this match what we see the closed loop converge on:
	// [COARSE] = (4 * COARSE_MAX * (fDFLL-fDFLL_MIN)) / (fDFLL_MAX - fDFLL_MIN)
	
	uint32_t coarse;
	temp = 4 * DFLL_COARSE_MAX * ((uint64_t)fDFLL - DFLL_OUTPUT_FREQ_MIN); 
	coarse = (uint32_t)(temp / (DFLL_OUTPUT_FREQ_MAX - DFLL_OUTPUT_FREQ_MIN));
	temp = 0;
	
	if (coarse > DFLL_COARSE_MAX) return SCIF_ERROR_DFLL_COARSE_VAL_OOR;
	
	// Need to calculate fCoarse now, so that we can figure how frequency [FINE] has to deal with:
	// fCOARSE = fDFLL_MIN + ((fDFLL_MAX-fDFLL_MIN)*COARSE / 4) / COARSE_MAX
	fCoarse = DFLL_OUTPUT_FREQ_MIN + ( ((DFLL_OUTPUT_FREQ_MAX - DFLL_OUTPUT_FREQ_MIN) * (uint64_t)coarse) / 4 ) / DFLL_COARSE_MAX;
	
	// The formula for calculating [DFLL0VAL.FINE] seems odd, and I can't really figure out what
	// they are doing. The on line ASF example and the ASF Clocks example 3 don't agree. The Clock
	// Example 3 seems to be closer? I'm not understanding how the [DFLLxCONF.CALIB] plays in here
	// as well.
	
	// This is the formula from ASF clock example 3
	// [FINE] = ((((10 * FINE_MAX * (fDFLL - fCOARSE)) / fCOARSE) + FINE_MAX/2) / 4
	uint32_t fine;
	temp = (10 * DFLL_FINE_MAX * (uint64_t)(fDFLL - fCoarse)) / (uint64_t)fCoarse;
	temp += DFLL_FINE_MAX/2;
	temp /= 4;
	fine = (uint32_t)temp;
	temp = 0;
	
	if (fine > DFLL_FINE_MAX) return SCIF_ERROR_DFLL_FINE_VAL_OOR;
		
	// WRITE REGISTERS
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0VAL_OFFSET);
	SCIF->bf.DFLL0VAL.reg = SCIF_DFLL0VAL_FINE((uint8_t)fine) | SCIF_DFLL0VAL_COARSE((uint8_t)coarse); 
	
	// Sync to make sure we have most recent values (for debugging purposes only)
	SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	
	return SCIF_SUCCSESS;
}

// Some notes about STEP:
// - Small steps will ensure small overshoot, but results in longer lock times
// - Large step can have large overshoot, but lock faster
// - 
static inline void SCIF_setStepDFLL(uint8_t cstep, uint8_t fstep)
{
	// Error checks:
	//if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	// [DFLLxSTEP] [COARSE] & [FINE] must be < 50% of [DFLLxVAL] [COARSE] & [FINE] max value 
	if ((cstep > DFLL_COARSE_MAX/2) || (fstep > DFLL_FINE_MAX/2)) return;
	
	// Check and set
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.DFLL0STEP.reg = SCIF_DFLL0STEP_CSTEP(cstep) | SCIF_DFLL0STEP_FSTEP(fstep);
}

SCIF_ERROR_TYPE SCIF_initSrcClk_DFLL_Closed(uint32_t fReference, uint32_t fDFLL, bool bUseBestGuessVAL)
{
	SCIF_ERROR_TYPE err;
	// Error checks
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!mSCIF.bEnabled.CLK.DFLL || !SCIF->bf.DFLL0CONF.bit.EN) return SCIF_ERROR_DFLL_NOT_ENABLED;
	if ((fReference < DFLL_REF_FREQ_MIN) || (fReference > DFLL_REF_FREQ_MAX)) return SCIF_ERROR_DFLL_REF_FREQ_OOR;
	if ((fDFLL < DFLL_OUTPUT_FREQ_MIN) || (fDFLL > DFLL_OUTPUT_FREQ_MAX)) return SCIF_ERROR_DFLL_OUTPUT_FREQ_OOR;
	
	// Figure [SCIF.DFLL0MUL.MUL] - check to make sure not larger than register size
	uint32_t mul = fDFLL/fReference;
	if ((mul > UINT16_MAX) || (mul == 0)) return SCIF_ERROR_DFLL_MUL_OOR;
	mSCIF.freq.CLK.DFLL = fDFLL;
	
	SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	
	// Pull in the current value for [SCIF.DFLL0CONF], figure Range, and set
	uint32_t temp_CONF = SCIF->reg.SCIF_DFLL0CONF;
	temp_CONF &= ~SCIF_DFLL0CONF_RANGE_Msk;

	if (fDFLL < DFLL_FREQ_RANGE3_MAX) temp_CONF |= SCIF_DFLL0CONF_RANGE(3);
	else if (fDFLL < DFLL_FREQ_RANGE2_MAX) temp_CONF |= SCIF_DFLL0CONF_RANGE(2);
	else if (fDFLL < DFLL_FREQ_RANGE1_MAX) temp_CONF |= SCIF_DFLL0CONF_RANGE(1);
	else temp_CONF |= SCIF_DFLL0CONF_RANGE(0);
	
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
	SCIF->bf.DFLL0CONF.reg = temp_CONF;
	
	// SET [SCIF.DFLL0MUL.MUL]
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0MUL_OFFSET);
	SCIF->bf.DFLL0MUL.reg = SCIF_DFLL0MUL_MUL((uint16_t)mul);
	
	// By default in closed loop mode, it is not necessary to set [DFLLxSTEP] [COARSE] & [FINE]
	// That being said, one can speed up the locking speed by giving a good initial guess, but as
	// as result, we need to use a smaller step size.
	
	if (bUseBestGuessVAL == true)
	{
		err = SCIF_setInitalTuningDFLL(fDFLL);
		if (err) return err;
		// since we tuned this, we should limit the maximum to use really small steps to hone in on the final frequency
		SCIF_setStepDFLL(1,4);
	}
	else // Don't adjust [VALs] closed loop should take care of it choose values carefully.
	{
		// if this step is skipped the following happens per step 5 of 13.6.3.5:
		// [DFLL0VAL.COARSE] - starts at it's current value (at reset this is 0)
		// [DFLLVAL.FINE] - starts at half it's max value? this is either 0x80 or 0x7F?????
		
		// figure a step size that works for this, per step 4 of 13.6.3.5, In the example code they use a value of 4 for each
		SCIF_setStepDFLL(4,4);

	}
	
	// Switch to closed loop mode
	temp_CONF = SCIF->reg.SCIF_DFLL0CONF;
	temp_CONF |= SCIF_DFLL0CONF_MODE;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_DFLL0CONF_OFFSET);
	SCIF->bf.DFLL0CONF.reg = temp_CONF;
	
	SCIF->bf.DFLL0SYNC.reg = SCIF_DFLL0SYNC_SYNC;
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0RDY));

	
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_awaitFineLock_DFLL(void)
{
	// Error checks
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!mSCIF.bEnabled.CLK.DFLL || !SCIF->bf.DFLL0CONF.bit.EN) return SCIF_ERROR_DFLL_NOT_ENABLED;
	
	// wait for Fine Lock
	while (!(SCIF->bf.PCLKSR.reg & SCIF_PCLKSR_DFLL0LOCKF));
	
	return SCIF_SUCCSESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RCSYS Control/Operation - §13.6.4
///////////////////////////////////////////////////////////////////////////////////////////////////

// Put some stuff here eventually

///////////////////////////////////////////////////////////////////////////////////////////////////
// RCFAST Control/Operation - §13.6.5
///////////////////////////////////////////////////////////////////////////////////////////////////
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_RCFAST(SCIF_RCFASTCFG_FRANGE_Type fRange) // Sets and Enables RCFAST
{
	// Error Check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (fRange > SCIF_RCFASTCFG_FRANGE_12MHz) return SCIF_ERROR_RCFAST_OOR;
	
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_RCFASTCFG_OFFSET);	// Unlock the control register for RCFAST
	SCIF->bf.RCFASTCFG.reg = SCIF_RCFASTCFG_EN						// Enable the Clock and set to
							| SCIF_RCFASTCFG_FRANGE(fRange)
							| (~(SCIF_RCFASTCFG_EN | SCIF_RCFASTCFG_FRANGE_Msk) & SCIF->bf.RCFASTCFG.reg);
	while (SCIF->bf.RCFASTCFG.bit.EN == 0);							// Wait for the Clock to enable before proceeding
	mSCIF.bEnabled.CLK.RCFAST = 1;										// Flag as enabled
	
	// Set frequency in the manager
	if (fRange == SCIF_RCFASTCFG_FRANGE_4MHz)		mSCIF.freq.CLK.RCFAST = 4e6;
	else if (fRange == SCIF_RCFASTCFG_FRANGE_8MHz)	mSCIF.freq.CLK.RCFAST = 8e6;
	else if (fRange == SCIF_RCFASTCFG_FRANGE_12MHz) mSCIF.freq.CLK.RCFAST = 12e6;
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_reenableSrcClk_RCFAST(void)	// Enables RCFAST with existing settings
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	
	SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_RCFASTCFG_OFFSET);
	SCIF->bf.RCFASTCFG.bit.EN = 1;
	while (SCIF->bf.RCFASTCFG.bit.EN == 0);							// Wait for the Clock to enable before proceeding
	mSCIF.bEnabled.CLK.RCFAST = 1;									// Flag as enabled
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_disableSrcClk_RCFAST(void) // Disables RCFAST and keeps existing settings
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	
	// Only disable if flagged as enabled somehow, otherwise, unnecessary
	if (mSCIF.bEnabled.CLK.RCFAST || SCIF->bf.RCFASTCFG.bit.EN)
	{
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_RCFASTCFG_OFFSET);
		SCIF->bf.RCFASTCFG.bit.EN = 0;
		while (SCIF->bf.RCFASTCFG.bit.EN == 1);
	}
	
	mSCIF.bEnabled.CLK.RCFAST = 0;

	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_resetSrcClk_RCFAST(void) // Clears settings and Disables RCFAST
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	
	// // Only disable | reset if flagged as enabled, or ref isn't reset value, otherwise, unnecessary
	if (mSCIF.bEnabled.CLK.RCFAST || SCIF->bf.RCFASTCFG.bit.EN || (SCIF->bf.RCFASTCFG.reg != SCIF_RCFASTCFG_RESET_VAL))
	{	
		SCIF->bf.UNLOCK.reg = SCIF_ADDR_UNLOCK(SCIF_RCFASTCFG_OFFSET);
		SCIF->bf.RCFASTCFG.reg = SCIF_RCFASTCFG_RESET_VAL;
		while (SCIF->bf.RCFASTCFG.bit.EN == 1);
	}
	
	mSCIF.bEnabled.CLK.RCFAST = 0;
	mSCIF.freq.CLK.RCFAST = 0;
	
	return SCIF_SUCCSESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RC80M Control/Operation - §13.6.6
///////////////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////////////////////////
// Generic Clock Control/Operation - §13.6.7-8
///////////////////////////////////////////////////////////////////////////////////////////////////

SCIF_ERROR_TYPE SCIF_setEnableGCLK(GCLK_NUM_TYPE numGCLK, uint32_t fSrc, uint16_t divFactor, SCIF_GCLK_SRC_TYPE srcClk)
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numGCLK < SCIF_GCLK_NUM)) return SCIF_ERROR_GCLK_NUM_NO_EXIST;
	if ((divFactor > 1) && (divFactor % 2)) return SCIF_ERROR_GCLK_DIV_ODD;
	if ((numGCLK < SCIF_GCLK_NUM) && (divFactor > UINT8_MAX)) return SCIF_ERROR_GCLK_DIV_OOR;			
	else if ((numGCLK == SCIF_GCLK_NUM_MASTER) && (divFactor > UINT16_MAX)) return SCIF_ERROR_GCLK_DIV_OOR;	// GCLK_11 is special

	// NOTE: divFactor is the actual division factor want to divide source by and not DIV in defined in equation in §13.6.8.1
	// This allows for divFactor to be used stand-alone (i.e., no separate boolean to determine if division is actually completed)
	// divFactor = 0 or 1 indicates no division occurs (and is not enabled) while still allowing for divFactor > 1 math to work out
	if (divFactor > 1)
	{
		SCIF->bf.Gcctrl[numGCLK].bf.GCCTRL.reg = SCIF_GCCTRL_OSCSEL(srcClk) | SCIF_GCCTRL_CEN |
												 SCIF_GCCTRL_DIVEN |
												 SCIF_GCCTRL_DIV(((divFactor + 1) >> 1) - 1);
		mSCIF.freq.GCLK[numGCLK] = (uint32_t)(fSrc/divFactor);
	}
	else
	{
		SCIF->bf.Gcctrl[numGCLK].bf.GCCTRL.reg = SCIF_GCCTRL_OSCSEL(srcClk) | SCIF_GCCTRL_CEN;
		mSCIF.freq.GCLK[numGCLK] = fSrc;
	}
	
	mSCIF.bEnabled.GCLK.allGCLKS |= (uint16_t)(1 << numGCLK);
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_resetDisableGCLK(GCLK_NUM_TYPE numGCLK)
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numGCLK < SCIF_GCLK_NUM)) return SCIF_ERROR_GCLK_NUM_NO_EXIST;
	
	// NEEDS MORE WORK
	SCIF->bf.Gcctrl[numGCLK].bf.GCCTRL.reg = SCIF_GCCTRL_RESET_VAL;
	
	mSCIF.freq.GCLK[numGCLK] = 0;
	mSCIF.bEnabled.GCLK.allGCLKS &= ~((uint16_t)(1 << numGCLK));
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_enableGCLK(GCLK_NUM_TYPE numGCLK)
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numGCLK < SCIF_GCLK_NUM)) return SCIF_ERROR_GCLK_NUM_NO_EXIST;
	
	// NEEDS MORE WORK
	SCIF->bf.Gcctrl[numGCLK].bf.GCCTRL.bit.CEN = 1;
	
	return SCIF_SUCCSESS;
}

SCIF_ERROR_TYPE SCIF_disableGCLK(GCLK_NUM_TYPE numGCLK)
{
	// Error check
	if ((mSCIF.bInit == 0) || !(PM->bf.PBCMASK.reg & PM_PBCMASK_SCIF)) return SCIF_ERROR_NOT_EN;
	if (!(numGCLK < SCIF_GCLK_NUM)) return SCIF_ERROR_GCLK_NUM_NO_EXIST;
	
	// NEEDS MORE WORK
	SCIF->bf.Gcctrl[numGCLK].bf.GCCTRL.bit.CEN = 0;
	
	return SCIF_SUCCSESS;
}
