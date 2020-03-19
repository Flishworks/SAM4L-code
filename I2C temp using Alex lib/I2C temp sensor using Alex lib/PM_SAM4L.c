#include "PM_SAM4L.h"
#include "sam.h"
#include "SCIF_SAM4L.h"
#include "BSCIF_SAM4L.h"


PM_ERROR_TYPE PM_init(void)
{
	mSCIF.bEnabled.CLK.RCSYS = 1;
	mSCIF.freq.CLK.RCSYS = PM_SYSTEM_CLOCK;
	fMainClk = mSCIF.freq.CLK.RCSYS;
	activeClk = PM_MCSEL_SOURCE_RCSYS;
	return PM_SUCCSESS;
}

PM_ERROR_TYPE PM_switchMCLK(PM_MCSEL_SOURCE_TYPE src)
{
	safeDisableIRQ();

	if (src == PM_MCSEL_SOURCE_RCSYS) // NEED TO FIGURE WAY TO SET ENABLED....
	{
		if (mSCIF.bEnabled.CLK.RCSYS!= 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		fMainClk = mSCIF.freq.CLK.RCSYS;
		activeClk = PM_MCSEL_SOURCE_RCSYS;
	}
	else if (src == PM_MCSEL_SOURCE_OSC0)
	{
		if (mSCIF.bEnabled.CLK.OSC0 != 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		fMainClk = mSCIF.freq.CLK.OSC0;
		activeClk = PM_MCSEL_SOURCE_OSC0;
	}
	else if (src == PM_MCSEL_SOURCE_PLL)
	{
		if (mSCIF.bEnabled.CLK.PLL != 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		fMainClk = mSCIF.freq.CLK.PLL;
		activeClk = PM_MCSEL_SOURCE_PLL;
	}
	else if (src == PM_MCSEL_SOURCE_DFLL)
	{
		if (mSCIF.bEnabled.CLK.DFLL != 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		fMainClk = mSCIF.freq.CLK.DFLL;
		activeClk = PM_MCSEL_SOURCE_DFLL;
	}	
	else if (src == PM_MCSEL_SOURCE_RC80M)
	{
		if (mSCIF.bEnabled.CLK.RC80M != 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		// fMainClk = mSCIF.freq.;
		// SEE NOTE, in PM.MCCTRL, division is required.
		activeClk = PM_MCSEL_SOURCE_RC80M;
	}	
	else if (src == PM_MCSEL_SOURCE_RCFAST)
	{
		if (mSCIF.bEnabled.CLK.RCFAST != 1) return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		fMainClk = mSCIF.freq.CLK.RCFAST;
		activeClk = PM_MCSEL_SOURCE_RCFAST;
	}
	else if (src == PM_MCSEL_SOURCE_RC1M)
	{
		return PM_ERROR_CLK_SOURCE_NOT_ENABLED;
		//if (mBSCIF.bEnabled.clk.RC1M != 1) 
		//fMainClk = mSCIF.freq.;
		activeClk = PM_MCSEL_SOURCE_RC1M;
	}
	else // Somehow we chose a bad source....
	{
		return PM_ERROR_UNKNOWN;
	}
	
	// Switch the clock	
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_MCCTRL_OFFSET);
	PM->bf.MCCTRL.reg = src;

	safeEnableIRQ();
	return PM_SUCCSESS;
}


void PM_enableHighSpeed(void)
{
	// NOTE --------------------------- WE NEED TO CHECK THE BRIDGE IN PM TO SEE IF IT'S ACTIVE (it is by default...)-----------------------------
	
	// Do some stuff to enable picoCache HCACHE?  I think that this is actually HRAMC1 (naming discrepancy?) see section 14.3
	//This is copying what they did.  Not sure why it is being done..............
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_HSBMASK_OFFSET);
	PM->bf.HSBMASK.reg |= PM_HSBMASK_HRAMC1;			// Data
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBBMASK_OFFSET);
	PM->bf.PBBMASK.reg |= PM_PBBMASK_HCACHE;			// Regs
	
	// Enable micro-cache
	HCACHE->reg.HCACHE_CTRL = HCACHE_CTRL_CEN_YES;
	while (!(HCACHE->reg.HCACHE_SR & HCACHE_SR_CSTS_EN));
	
	// Activate high speed interface on the flash controller (need to be able to read program once
	// we switch the mainclock to the pll)
	HFLASHC->reg.FLASHCALW_FCR = (HFLASHC->reg.FLASHCALW_FCR & ~FLASHCALW_FCR_FWS) | FLASHCALW_FCR_FWS_1;
	while (!((HFLASHC->reg.FLASHCALW_FSR & FLASHCALW_FSR_FRDY) != 0));	// block until FLASHCALW is ready for new command
	
	// These two sections enable flash high speed mode and store any errors/wait until the command has cleared.
	// I actually think I don't need to worry about the PAGEN field.. needs more investigation later.
	uint32_t temp, errstatus;
	temp = HFLASHC->reg.FLASHCALW_FCMD;
	temp &= ~FLASHCALW_FCMD_CMD_Msk;
	temp |= (FLASHCALW_FCMD_KEY_KEY | FLASHCALW_FCMD_CMD_HSEN); // Enable High Speed Mode
	HFLASHC->reg.FLASHCALW_FCMD = temp;
	
	// Block? error status
	errstatus = HFLASHC->reg.FLASHCALW_FSR & (FLASHCALW_FSR_LOCKE | FLASHCALW_FSR_PROGE);
	while (! ((HFLASHC->reg.FLASHCALW_FSR & FLASHCALW_FSR_FRDY) != 0));	// block until FLASHCALW is ready for new command
}
void PM_disableHighSpeed(void)
{
	// disable High speed flash mode:
	uint32_t temp, errstatus;
	temp = HFLASHC->reg.FLASHCALW_FCMD;
	temp &= ~FLASHCALW_FCMD_CMD_Msk;
	temp |= (FLASHCALW_FCMD_KEY_KEY | FLASHCALW_FCMD_CMD_HSDIS); // Disable High Speed Mode
	HFLASHC->reg.FLASHCALW_FCMD = temp;
	while (! ((HFLASHC->reg.FLASHCALW_FSR & FLASHCALW_FSR_FRDY) != 0));	// block until FLASHCALW is ready for new command
	
	// disable flash controller???? no... something about wait state.
	HFLASHC->bf.FCR.bit.FWS = 0; // Set flash wait state to 0;
	//HFLASHC->reg.FLASHCALW_FCR = (HFLASHC->reg.FLASHCALW_FCR & ~FLASHCALW_FCR_FWS) | FLASHCALW_FCR_FWS_1;
	while (!((HFLASHC->reg.FLASHCALW_FSR & FLASHCALW_FSR_FRDY) != 0));	// block until FLASHCALW is ready for new command
	
	// Disable micro-cache
	HCACHE->reg.HCACHE_CTRL = HCACHE_CTRL_CEN_NO;
	while (HCACHE->reg.HCACHE_SR & HCACHE_SR_CSTS_EN);

	// Do some stuff to enable picoCache HCACHE?  I think that this is actually HRAMC1 (naming discrepancy)
	//  Not sure why it is being done..............
	PM->bf.UNLOCK.reg	= ADDR_UNLOCK(PM_HSBMASK_OFFSET);
	PM->bf.HSBMASK.reg	&= ~PM_HSBMASK_HRAMC1;			// Data
	PM->bf.UNLOCK.reg	= ADDR_UNLOCK(PM_PBBMASK_OFFSET);
	PM->bf.PBBMASK.reg	&= ~PM_PBBMASK_HCACHE;			// Regs
}