#ifndef _GPIO_SAM4L_H_	// Guards against double calling of headers
#define _GPIO_SAM4L_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include "sam.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
#define PORTA_NUM		0
#define PORTB_NUM		1
#define	PORTC_NUM		2

typedef enum
{
	GPIO_SUCCSESS		= 0,
	GPIO_ERROR_UNKNOWN	= 0x1700, // GPIO error offset
} GPIO_ERROR_Type;

typedef enum
{
	GPIO_INTERRUPT_MODE_PIN_CHANGE		= 0,
	GPIO_INTERRUPT_MODE_RISING_EDGE		= 1,
	GPIO_INTERRUPT_MODE_FALLING_EDGE	= 2
} GPIO_INTERRUPT_MODE_TYPE;

typedef enum
{
	GPIO_UNUSED_PIN_LOWPOWER_MODE_INPUT		= 0,
	GPIO_UNUSED_PIN_LOWPOWER_MODE_OUTPUT	= 1
} GPIO_UNUSED_PIN_LOWPOWER_MODE_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	// Word 1
	uint32_t	gpioMask;
	// Word 2
	uint8_t		port;
	uint8_t		periphMux;
} Pin;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////
void GPIO_setPeriphPin(				Pin* targ);
void flashLEDms(					Pin* LED, uint16_t msDelay1, uint16_t msDelay2);
void GPIO_forceLow(					Pin* targ);
void GPIO_forceHigh(				Pin* targ);
void GPIO_releaseAndReset(			Pin* targ);
void GPIO_setupInterrupt(			Pin* pPin, uint8_t bGlitchFilterEn,
									GPIO_INTERRUPT_MODE_TYPE interruptMode, uint32_t NVIC_priority);
void GPIO_setPinLevelDetect(		Pin* pPin);
void GPIO_setUnusedPinLowPowerState(Pin* pPin, GPIO_UNUSED_PIN_LOWPOWER_MODE_TYPE pinMode);
uint8_t GPIO_calcIntNum(			Pin* pPin);
void* GPIO_getHandlerCall(			uint8_t intNum);
IRQn_Type GPIO_getIRQnum(			uint8_t intNum);

#endif


