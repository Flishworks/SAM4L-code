#include "GPIO_SAM4L.h"
#include <stddef.h>
#include "sam.h"
#include "delay_SAM4L.h"
#include "NVIC_SAM4L.h"

void GPIO_setPeriphPin(Pin* targ) // Set pin to a desired state
{
	// WE basically need to take a peripheral Mux number were given and figure a way to set PMR# appropriately
	
	// if we have a hit on bit[0], we need to set PMR0, if not, we need to clear it.
	if ((targ->periphMux & 0x01) == 0x01) GPIO->bf.Port[targ->port].bf.PMR0S.reg = targ->gpioMask;
	else GPIO->bf.Port[targ->port].bf.PMR0C.reg = targ->gpioMask;
	
	// if we have a hit on bit[1], we need to set PMR1, if not, we need to clear it.
	if ((targ->periphMux & 0x02) == 0x02) GPIO->bf.Port[targ->port].bf.PMR1S.reg = targ->gpioMask;
	else GPIO->bf.Port[targ->port].bf.PMR1C.reg = targ->gpioMask;
	
	// if we have a hit on bit[2], we need to set PMR2, if not, we need to clear it.
	if ((targ->periphMux & 0x04) == 0x04) GPIO->bf.Port[targ->port].bf.PMR2S.reg = targ->gpioMask;
	else GPIO->bf.Port[targ->port].bf.PMR2C.reg = targ->gpioMask;
	
	// We need to clear GPIO control of the target
	GPIO->bf.Port[targ->port].bf.GPERC.reg = targ->gpioMask;
	
	return;
}

void GPIO_clearPeriphPin(Pin* targ)	// Releases pin to default states and gives GPIO control
{
	GPIO->bf.Port[targ->port].bf.PMR0C.reg = targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.PMR1C.reg = targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.PMR2C.reg = targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.GPERC.reg = targ->gpioMask; // shouldn't this be GPERS?
	return;
}

void GPIO_forceLow(Pin* targ)	// Sets the line low, doesn't care about existing state of GPIO line.
{
	GPIO->bf.Port[targ->port].bf.OVRC.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.ODERS.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.GPERS.reg	= targ->gpioMask;
}

void GPIO_forceHigh(Pin* targ)	// Sets the line high, doesn't care about existing state of GPIO line.
{
	GPIO->bf.Port[targ->port].bf.OVRS.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.ODERS.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.GPERS.reg	= targ->gpioMask;
}

// Releases GPIO control of the line and reset GPIO settings to default, line will float to whatever is driving
// it, be it a pullup or peripheral setting
void GPIO_releaseAndReset(Pin* targ)
{
	GPIO->bf.Port[targ->port].bf.GPERC.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.ODERC.reg	= targ->gpioMask;
	GPIO->bf.Port[targ->port].bf.OVRC.reg	= targ->gpioMask;
}

void disable_GPIO_high(Pin* targ)
{
	
}

void GPIO_setPinLevelDetect(Pin* pPin)
{
	GPIO->bf.Port[pPin->port].bf.ODERC.reg = pPin->gpioMask;	// Disable Output
	GPIO->bf.Port[pPin->port].bf.STERS.reg = pPin->gpioMask;	// enable Schmitt trigger
}

uint8_t GPIO_testPinState(Pin* pPin)
{
	uint32_t testmask = GPIO->reg.GPIO_PORT[pPin->port].reg.GPIO_PVR & pPin->gpioMask;
	
	if (testmask == pPin->gpioMask) return 1;	// high state
	else return 0;								// low state
}

void flashLEDms(Pin* LED, uint16_t msDelay1, uint16_t msDelay2)
{
	GPIO->reg.GPIO_PORT[LED->port].bf.OVRC.reg = LED->gpioMask;
	delay_ms(msDelay1);
	GPIO->reg.GPIO_PORT[LED->port].bf.OVRS.reg = LED->gpioMask;
	delay_ms(msDelay2);
}

void GPIO_setupInterrupt(Pin* pPin, uint8_t bGlitchFilterEn,
						 GPIO_INTERRUPT_MODE_TYPE interruptMode, uint32_t NVIC_priority)
{
	GPIO_setPinLevelDetect(pPin);
	GPIO->bf.Port[pPin->port].bf.IERC.reg = pPin->gpioMask;		// Ensure Interrupt disabled before changing settings
	
	// Enable or disable Glitch filter (rarely disabled)
	if (bGlitchFilterEn == 1) GPIO->bf.Port[pPin->port].bf.GFERS.reg = pPin->gpioMask;
	else GPIO->bf.Port[pPin->port].bf.GFERC.reg = pPin->gpioMask;
	
	if (interruptMode == GPIO_INTERRUPT_MODE_PIN_CHANGE)
	{
		GPIO->bf.Port[pPin->port].bf.IMR0C.reg = pPin->gpioMask;
		GPIO->bf.Port[pPin->port].bf.IMR1C.reg = pPin->gpioMask;			
	}
	else if (interruptMode == GPIO_INTERRUPT_MODE_RISING_EDGE)
	{
		GPIO->bf.Port[pPin->port].bf.IMR0S.reg = pPin->gpioMask;
		GPIO->bf.Port[pPin->port].bf.IMR1C.reg = pPin->gpioMask;				
	}
	else if (interruptMode == GPIO_INTERRUPT_MODE_FALLING_EDGE)
	{
		GPIO->bf.Port[pPin->port].bf.IMR0C.reg = pPin->gpioMask;
		GPIO->bf.Port[pPin->port].bf.IMR1S.reg = pPin->gpioMask;				
	}
	else return; // we have an error

	// Determine IRQn offset, there are 4 per GPIO port
	//uint8_t temp = 32u - (__CLZ(pPin->gpioMask) + 1u);
	//uint8_t lineOffset = (uint8_t)GPIO_0_IRQn + (pPin->port * 4u) + (uint8_t)(temp / 8);
	//IRQn_Type irqOffset = (IRQn_Type)lineOffset;
	
	IRQn_Type irqOffset = GPIO_getIRQnum(GPIO_calcIntNum(pPin));
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	// NOTE: this isn't complete, as the NVIC for this interrupt may already be enabled... need
	// to check for crash and act accordingly.................
	
	// If not active, enable the NVIC GPIO group for this GPIO pin 
	if (NVIC_GetActive(irqOffset) == 0) NVIC_initEnable(irqOffset, NVIC_priority);
	else {} // If not active maybe nothing?... just enabling the local [GPIO.IERS] sufficient?
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	GPIO->bf.Port[pPin->port].bf.IERS.reg = pPin->gpioMask;	// enable the interrupt on the pin
}

uint8_t GPIO_calcIntNum(Pin* pPin)
{
	uint8_t temp = 32u - (__CLZ(pPin->gpioMask) + 1u);
	uint8_t intNum = pPin->port * 4u + (uint8_t)(temp / 8);
	
	return intNum;
}

void* GPIO_getHandlerCall(uint8_t intNum)
{
	if (intNum == 0) return GPIO_0_Handler;
	else if (intNum == 1) return GPIO_1_Handler;
	else if (intNum == 2) return GPIO_2_Handler;
	else if (intNum == 3) return GPIO_3_Handler;
	else if (intNum == 4) return GPIO_4_Handler;
	else if (intNum == 5) return GPIO_5_Handler;
	else if (intNum == 6) return GPIO_6_Handler;
	else if (intNum == 7) return GPIO_7_Handler;
	else if (intNum == 8) return GPIO_8_Handler;
	else if (intNum == 9) return GPIO_9_Handler;
	else if (intNum == 10) return GPIO_10_Handler;
	else if (intNum == 11) return GPIO_11_Handler;
	else {} // had an error
}

IRQn_Type GPIO_getIRQnum(uint8_t intNum)
{
	uint8_t offset = (uint8_t)GPIO_0_IRQn + intNum;
	IRQn_Type irqOffset = (IRQn_Type)offset;
	return irqOffset;
}


void GPIO_setUnusedPinLowPowerState(Pin* pPin, GPIO_UNUSED_PIN_LOWPOWER_MODE_TYPE pinMode)
{
	// Per the section 9.2 SAM4L schematic checklist, all unused pins should be set as I/O with the
	// following settings:
	// - Input with Internal pull-up enabled
	// - Output (ODER =1) and driven to low (OVR = 0) with internal pull-up disabled (PUER = 0).
	
	// Set the Pin as GPIO controlled
	GPIO->bf.Port[pPin->port].bf.GPERS.reg = pPin->gpioMask;
	
	if (pinMode == GPIO_UNUSED_PIN_LOWPOWER_MODE_INPUT)
	{
		GPIO->bf.Port[pPin->port].bf.ODERC.reg = pPin->gpioMask;	// Disable Output
		GPIO->bf.Port[pPin->port].bf.PUERS.reg = pPin->gpioMask;	// Enable Pullup
	}
	else if (pinMode == GPIO_UNUSED_PIN_LOWPOWER_MODE_OUTPUT)
	{
		GPIO->bf.Port[pPin->port].bf.PUERC.reg = pPin->gpioMask;	// Disable Pullup
		GPIO->bf.Port[pPin->port].bf.ODERS.reg = pPin->gpioMask;	// Enable Output
		GPIO->bf.Port[pPin->port].bf.OVRS.reg = pPin->gpioMask;		// Drive Low
	}
	else return;	// we had an undefined state entered
}
