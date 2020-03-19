#include "EIC_SAM4L.h"
#include "GPIO_SAM4L.h"
#include "PM_SAM4L.h"

EIC_ERROR_TYPE EIC_init(void)
{
	// To setup the EIC, several things need to happen in a specific order:
	// 1) The EIC system needs to be initialized (this is required for programming).
	//	  Basically, this just unlocks the clock and stuffs several variables into the manager
	// 2) Set any pin conditions required to get EIC's to fire, such as pin pull ups/downs
	// 3) Program the peripheral controller mux to setup the actual pin(s) in question correctly
	// 4) EIC channel, with triggers, modes, etc.. the last step is to enable EIC interrupt for the channel (IER)
	// 5) The NVIC needs to be setup:
	//	  - To setup the NVIC, the pre-installed dummy handler needs to be overwritten.  This is done
	//		by simply writing another handler with the same name as NVIC handler in question.
	//		This handler can contain the code directly, or a pointer to another function.
	//	  - The NVIC itself needs to be setup, this done by setting the actual NVIC registers and then enabling NVIC interrupts (IRQs)
	// 6) the channel is fully active once we actually connect it to the external interrupt pin (EN).
	// Interrupts will now be called whenever the condition of the interrupt is met.

	mEIC.state = EIC_STATE_INTIALIZING;
	
	// Sysclock should already be enabled by default after a reset, but doesn't hurt to make sure we do it.
	
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBDMASK_OFFSET);
	PM->bf.PBDMASK.reg |= PM_PBDMASK_EIC;
	
	// Stuff all EIC channels and populate information for later
	mEIC.INT[EXT_INT0_NMI].pHandler = NMI_Handler;
	mEIC.INT[EXT_INT1].pHandler = EIC_0_Handler;
	mEIC.INT[EXT_INT2].pHandler = EIC_1_Handler;
	mEIC.INT[EXT_INT3].pHandler = EIC_2_Handler;
	mEIC.INT[EXT_INT4].pHandler = EIC_3_Handler;
	mEIC.INT[EXT_INT5].pHandler = EIC_4_Handler;
	mEIC.INT[EXT_INT6].pHandler = EIC_5_Handler;
	mEIC.INT[EXT_INT7].pHandler = EIC_6_Handler;
	mEIC.INT[EXT_INT8].pHandler = EIC_7_Handler;

	mEIC.INT[EXT_INT0_NMI].numIRQ = NonMaskableInt_IRQn;
	mEIC.INT[EXT_INT1].numIRQ = EIC_0_IRQn;
	mEIC.INT[EXT_INT2].numIRQ = EIC_1_IRQn;
	mEIC.INT[EXT_INT3].numIRQ = EIC_2_IRQn;
	mEIC.INT[EXT_INT4].numIRQ = EIC_3_IRQn;
	mEIC.INT[EXT_INT5].numIRQ = EIC_4_IRQn;
	mEIC.INT[EXT_INT6].numIRQ = EIC_5_IRQn;
	mEIC.INT[EXT_INT7].numIRQ = EIC_6_IRQn;
	mEIC.INT[EXT_INT8].numIRQ = EIC_7_IRQn;
	mEIC.binit = 0x01;	// Initialized
	mEIC.state = EIC_STATE_IDLE;
	
	return EIC_SUCCSESS;
}

// I think there should be a setup (configure function called) which sets the channel.  This should
// only exist within the .c (not global).
// init chan function would get it setup, would be a onetime only function

EIC_ERROR_TYPE EIC_configureChan(EIC_INT_CHAN_NUM chanNum, EIC_MODE_TRIGGER_Type trigMode,
								 EIC_TRIGGER_Type trigger, EIC_SYNCFILTER_Type syncFilter)
{
	uint32_t chan_mask = (1ul << chanNum);
	mEIC.state = EIC_STATE_SETTING_CHANNEL;
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_CONFIGURING;
	
	// Ensure that the interrupt is disabled and cleared before doing anything
	EIC->bf.IDR.reg = chan_mask;	// disable interrupt
	EIC->bf.DIS.reg = chan_mask;	// disable EIC channel
	EIC->bf.ICR.reg = chan_mask;	// clear any pending interrupts
	
	// Check to make sure we have the right kind of trigger
	if ((trigMode == EIC_MODE_EDGE_TRIGGERED) & (trigger == (EIC_TRIGGER_HIGH_LEVEL | EIC_TRIGGER_LOW_LEVEL))) return EIC_ERROR_BAD_TRIGGER_SET;
	else if ((trigMode == EIC_MODE_LEVEL_TRIGGERED) & (trigger == (EIC_TRIGGER_RISING_EDGE | EIC_TRIGGER_FALLING_EDGE))) return EIC_ERROR_BAD_TRIGGER_SET;

	// Set the trigger mode and type
	mEIC.INT[chanNum].param.trigMode	= trigMode;
	mEIC.INT[chanNum].param.trig		= trigger;

	// Determine if Level|Edge Triggered
	if (trigMode == EIC_MODE_LEVEL_TRIGGERED)
	{
		EIC->bf.MODE.reg |= chan_mask;	// Set [EIC.MODE] high - level
		// Set [EIC.LEVEL]
		if (trigger == EIC_TRIGGER_HIGH_LEVEL) EIC->bf.LEVEL.reg |= chan_mask;	// High level
		else EIC->bf.LEVEL.reg &= ~chan_mask;									// Low level
	}
	else // Edge Triggered
	{
		EIC->bf.MODE.reg &= ~chan_mask;	// Set [EIC.MODE] low - edge
		// Set [EIC.EDGE]
		if (trigger == EIC_TRIGGER_RISING_EDGE) EIC->bf.EDGE.reg |= chan_mask;	// Rising edge
		else EIC->bf.EDGE.reg &= ~chan_mask;									// Falling edge
	}

	// Set sync/async
	mEIC.INT[chanNum].param.syncFilter = syncFilter;
	if (syncFilter == EIC_ASYNC_MODE) EIC->bf.ASYNC.reg |= chan_mask;	// if Async, [EIC.ASYNC] high
	else																// else Sync
	{
		EIC->bf.ASYNC.reg &= ~chan_mask;	// [EIC.ASYNC] low
		// Set filter
		if (syncFilter == EIC_SYNC_MODE_FILTER_EN) EIC->bf.FILTER.reg |= chan_mask;
		else EIC->bf.FILTER.reg &= ~chan_mask; // else, not filtered, set chan low
	}
	
	// Channel now configured, but not enabled
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_CONFIGURED;
	mEIC.state = EIC_STATE_IDLE;
	
	return EIC_SUCCSESS;
}



EIC_ERROR_TYPE EIC_initChan(EIC_INT_CHAN_NUM chanNum, Pin* pin, EIC_MODE_TRIGGER_Type trigMode,
							EIC_TRIGGER_Type trigger, EIC_SYNCFILTER_Type syncFilter)
{
	// Notes about how EIC interrupts work:
	//
	// 1) There is a pin state change. In order for that to be fed into the EIC the peripheral pins
	//	  must be set correctly.
	// 2) The external interrupt on the pin EXTINTn will be enabled by writing to [EN], causing
	//    [CTRL] for that pin to read true.  This register acts as a gate allowing the signal to
	//	  pass up or halted here. Writes to [DIS] will prevent signal propagation.
	// 3) It then propagates through [LEVEL], [MODE], [EDGE], [FILTER], [ASYNC] and will generate
	//    an interrupt in [ISR] if it matches the settings in the aforementioned registers. [ISR]
	//	  being set will cause EIC_WAKE to fire at this point.
	// 4) [IMR] (set and cleared by [IER] and [IDR]) will either allow/prevent the interrupt from
	//    generating an IRQn signal (making it up the chain into [NVIC]).
	
	// Notes:
	// - Numbering for EXTINTn's and EIC IRQn's are completely hosed and can lead to confusion.
	
	uint32_t chan_mask = (1ul << chanNum);
	
	// Error Checking
	if (chanNum > EIC_NUM) return EIC_ERROR_CHANNEL_OUT_OF_RANGE;
	if (mEIC.binit != 1u) return EIC_ERROR_MANAGER_NOT_INTIALIZED;
	if ((mEIC.intializedChannels & chan_mask) == chan_mask) return EIC_ERROR_CHANNEL_IN_USE;
	if (EIC->bf.CTRL.reg == chan_mask) return EIC_ERROR_INT_ALREADY_EN;
		
	// Set the pin/peripheral
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_SETTING_PERIPH;
	mEIC.INT[chanNum].pin = pin;
	GPIO_setPeriphPin(pin);

	// Configure the channel settings
	EIC_configureChan(chanNum, trigMode, trigger, syncFilter);
	
	// Enabling the interrupt will allow for propagation of the EIC to the NVIC, however, the interrupt itself
	// will not be called if the interrupt pin itself is not enabled (EN register).
	EIC->bf.ICR.reg = chan_mask;	// Clear pending interrupts
	EIC->bf.IER.reg = chan_mask;	// Enable Interrupt
	
	// Flag channel as init/enabled
	mEIC.intializedChannels |= chan_mask;
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_ENABLED;
	
	return EIC_SUCCSESS;
}

EIC_ERROR_TYPE EIC_reconfigureChan(EIC_INT_CHAN_NUM chanNum, EIC_MODE_TRIGGER_Type trigMode,
								   EIC_TRIGGER_Type trigger, EIC_SYNCFILTER_Type syncFilter)
{
	// Channels need to be de-primed before this function can be called.
	
	uint32_t chan_mask = (1ul << chanNum);
	
	// Error Checking
	if (chanNum > EIC_NUM) return EIC_ERROR_CHANNEL_OUT_OF_RANGE;
	if (mEIC.binit != 1u) return EIC_ERROR_MANAGER_NOT_INTIALIZED;
	if (EIC->bf.CTRL.reg == chan_mask) return EIC_ERROR_INT_ALREADY_EN; // can't change settings while enabled

	// Disable interrupts and set to Configured state
	EIC->bf.IDR.reg = chan_mask;
	mEIC.intializedChannels &= ~chan_mask;
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_CONFIGURED;

	// re-configure the channel settings
	EIC_configureChan(chanNum, trigMode, trigger, syncFilter);

	// Enabling the interrupt will allow for propagation of the EIC to the NVIC, however, the interrupt itself
	// will not be called if the interrupt pin itself is not enabled (EN register).
	EIC->bf.ICR.reg = chan_mask;	// Clear pending interrupts
	EIC->bf.IER.reg = chan_mask;	// Enable Interrupt

	// Flag channel as init/enabled
	mEIC.intializedChannels |= chan_mask;	 
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_ENABLED;

	return EIC_SUCCSESS;
}

EIC_ERROR_TYPE EIC_primeChan(EIC_INT_CHAN_NUM chanNum) // This enables the actual line of the interrupt (the actual pin), last step before interrupts start going)
{
	uint32_t chanMask = (1ul << chanNum);
	
	// Error checking
	if (chanNum > EIC_NUM) return EIC_ERROR_CHANNEL_OUT_OF_RANGE;	// Make sure channel is in range before we do anything
	if (mEIC.binit != 1u) return EIC_ERROR_MANAGER_NOT_INTIALIZED;	// EIC manager hasn't been initialized yet
	if ((mEIC.intializedChannels & chanMask) != chanMask)	return EIC_ERROR_CHANNEL_NOT_INITALIZED;
	
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_PRIMING;
	
	// We just need to connect the EIC for the channel to the actual channel, flipping the EN bit will do this.	
	EIC->bf.ICR.reg = chanMask;	// clear any pending interrupts on the channel
	EIC->bf.EN.reg = chanMask;	// Enables the external interrupt pin (what I'm calling the line),  If this is set and the IMR bit is !set 
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_PRIMED;
	
	return EIC_SUCCSESS;
}
	
EIC_ERROR_TYPE EIC_deprimeChan(EIC_INT_CHAN_NUM chanNum)
{
	uint32_t chan_mask = (1ul << chanNum);
		
	// We just need to connect the EIC for the channel to the actual channel, flipping the EN bit will do this.
	EIC->bf.DIS.reg = chan_mask;	// Disables interrupt pin (the line, this kills INT source)
	EIC->bf.ICR.reg = chan_mask;	// clear any pending interrupts on the channel
	mEIC.INT[chanNum].state = EIC_CHAN_STATE_ENABLED;
		
	return EIC_SUCCSESS;
}
