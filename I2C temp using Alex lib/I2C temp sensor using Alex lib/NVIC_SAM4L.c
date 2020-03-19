#include "NVIC_SAM4L.h"

uint8_t bIRQ_wasEnabled;	// used to keep track on interrupt states.
uint8_t bIRQ_Enabled;

void NVIC_initEnable(IRQn_Type numIRQ, uint32_t int_piority)
{
	// need to set the actual function we're going to be doing before this.  need to shove a pointer to that into where the handler points to.
		
	NVIC_ClearPendingIRQ(numIRQ);			// clear any pending interrupts the line
	NVIC_SetPriority(numIRQ, int_piority);	// set the priority
	NVIC_EnableIRQ(numIRQ);					// enable the line
}

void NVIC_disable(IRQn_Type numIRQ)
{
	NVIC_DisableIRQ(numIRQ);
	NVIC_ClearPendingIRQ(numIRQ);
}

//*************************************************************************************************
//********************* NOTE: CODE BELOW HAS NOT BEEN TESTED - ALEX 7/6/2017 ********************** // i think it has been I just never updated this...
//*************************************************************************************************
void forceEnableIRQ(void)
{
	__DMB();
	__enable_irq();
	bIRQ_Enabled = 1;
}
void forceDisableIRQ(void)
{
	bIRQ_Enabled = 0;
	__disable_irq();
	__DMB();
}

// These next two functions work in tandem. There are two cases:
// 1) Global IRQ is enabled going in, we want to disable, run some code, then re-enable
// 2) Global IRQ is disabled going in, no need to disable, run some code, and DON'T re-enable
// 
// This is accomplished by detecting the interrupt state going in, setting a global, then only
// re-enabling if 
void safeDisableIRQ(void)
{
	// determine current IRQ state
	if (__get_PRIMASK() == 0) bIRQ_wasEnabled = 1;	// __get_PRIMASK() = 0 when IRQ enabled
	else bIRQ_wasEnabled = 0;						// __get_PRIMASK() = 1 when IRQ disabled
	forceDisableIRQ();									// disable
}
void safeEnableIRQ(void)
{
	// only re-enable if it was enabled before last Disable call;
	if (bIRQ_wasEnabled == 1)
	{
		bIRQ_wasEnabled = 0;	// clear the previous state
		forceEnableIRQ();
	}
	else
	{
		// do nothing,
	}
}
//*************************************************************************************************
//*************************************************************************************************