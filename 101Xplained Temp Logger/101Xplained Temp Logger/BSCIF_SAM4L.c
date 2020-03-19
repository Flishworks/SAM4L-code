// BSCIF
// James A. Heller
// 2016.10.21
// =============================

#include "BSCIF_SAM4L.h"
#include "PM_SAM4L.h"

// BSCIF is enabled by default, this will only set the register if it's not already on. Will also
// set some default settings
BSCIF_ERROR_TYPE BSCIF_init(void)
{
	// if not enabled, turn it on
	if (!(PM->bf.PBDMASK.reg & PM_PBDMASK_BSCIF))
	{
		PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBDMASK_OFFSET);
		PM->bf.PBDMASK.reg |= PM_PBDMASK_BSCIF;
	}
	
	// Mark some settings
	mBSCIF.bInit = 1;
	mBSCIF.state = BSCIF_STATE_IDLE;
		
	return BSCIF_SUCCSESS;
}

// NOTE: OUTPUT IS NOT REQUIRED IF YOU ARE JUST DRIVING THE AST
BSCIF_ERROR_TYPE OSC32k_init(OSC32K_MODE_Type osc_mode, OSC32K_OUTPUT_Type output,
							 OSC32K_SELCURR_Type xtalDriveCurrent, OSC32K_STARTUP_Type startupTime,
							 uint32_t frequency)
{
	// Error check, if already initialized, kick out already enabled error (but not an issue)
	//if (BSCIF->bf.OSCCTRL32.bit.OSC32EN == 1) return BSCIF_ERROR_OSC32_ALREADY_INIT;
	
	//__disable_irq();																					/// - put a check in here to disable/ re-enable only if necessary
	
	
	// push settings for the oscillator
	BSCIF->bf.OSCCTRL32.reg		= output
								| BSCIF_OSCCTRL32_MODE(osc_mode)													//	--------------- NEED TO WORRY ABOUT [STARTUP] AT SOME POINT.
								| BSCIF_OSCCTRL32_SELCURR(xtalDriveCurrent)
								| BSCIF_OSCCTRL32_STARTUP(startupTime);
			
	BSCIF->bf.OSCCTRL32.reg	|=	BSCIF_OSCCTRL32_OSC32EN;
	while (BSCIF->bf.PCLKSR.bit.OSC32RDY == 0);	// Block until ready
	
	// Mark as enabled
	mBSCIF.bClkEn.OSC32K = 1;
	mBSCIF.freqClks.OSC32K = frequency;
	
	//__enable_irq();																					/// - put a check in here to disable/ re-enable only if necessary
	
	return BSCIF_SUCCSESS;
}

// NOTE: OUTPUT IS NOT REQUIRED IF YOU ARE JUST DRIVING THE AST
BSCIF_ERROR_TYPE OSC32k_disable(void)
{
	// Error check, if already disabled, kick out already enabled error (but not an issue)
	//if (BSCIF->bf.OSCCTRL32.bit.OSC32EN == 1) return BSCIF_ERROR_OSC32_ALREADY_INIT;
	
	// error check to make sure nothing is using it?

	// Reset BSCIF
	BSCIF->bf.OSCCTRL32.bit.OSC32EN = 0;					// Disable OSC32kHz
	BSCIF->bf.OSCCTRL32.reg = BSCIF_OSCCTRL32_RESETVAL;		// Reset register to default values	

	//while (BSCIF->bf.PCLKSR.bit.OSC32RDY == 0);	// Block until ready
	
	// Mark as disabled
	mBSCIF.bClkEn.OSC32K = 0;
	mBSCIF.freqClks.OSC32K = 0;
	
	return BSCIF_SUCCSESS;
}

BSCIF_ERROR_TYPE RC32k_init(RC32K_MODE_Type osc_mode, RC32K_OUTPUT_Type output, uint32_t frequency)
{
	// Error check, if already initialized, kick out already enabled error (but not an issue)
	//if (BSCIF->bf.OSCCTRL32.bit.OSC32EN == 1) return BSCIF_ERROR_OSC32_ALREADY_INIT;
	
	//__disable_irq();																					/// - put a check in here to disable/ re-enable only if necessary
	
	uint32_t RC32KCR_temp = BSCIF->bf.RC32KCR.reg;
	
	// push settings for the oscillator
	if (osc_mode == RC32K_MODE_OPEN)
	{
		RC32KCR_temp |= osc_mode << BSCIF_RC32KCR_MODE_Pos;
		RC32KCR_temp |= output;
	}
	else if (osc_mode == RC32K_MODE_CLOSED)
	{
		// PUT STUFF HERE EVENTUALLY
	}
	else return BSCIF_ERROR_UNKNOWN; // mode out of range...
	
	BSCIF->bf.UNLOCK.reg = BSCIF_ADDR_UNLOCK(BSCIF_RC32KCR_OFFSET);
	BSCIF->bf.RC32KCR.reg = RC32KCR_temp;
	while (BSCIF->bf.PCLKSR.bit.RC32KRDY == 0);	// Block until ready
	
	// Mark as enabled
	mBSCIF.bClkEn.RC32K = 1;
	mBSCIF.freqClks.RC32K = frequency;
	
	//__enable_irq();																					/// - put a check in here to disable/ re-enable only if necessary
	
	return BSCIF_SUCCSESS;
}

// NOTE: OUTPUT IS NOT REQUIRED IF YOU ARE JUST DRIVING THE AST
BSCIF_ERROR_TYPE RC32k_disable(void)
{

	// Mark as disabled
	mBSCIF.bClkEn.RC32K = 0;
	mBSCIF.freqClks.RC32K = 0;
	
	return BSCIF_SUCCSESS;
}


BSCIF_ERROR_TYPE RC32k_Calibrate(void)
{
	return BSCIF_SUCCSESS;
}


