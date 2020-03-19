#ifndef	_EIC_SAM4L_H_	// Guards against double calling of headers
#define _EIC_SAM4L_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include "sam.h"
#include "GPIO_SAM4L.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
#define EIC_NUM		(EIC_STD_NUM + 1) 	// 8 standard EICs + 1 Non-maskable one 

//	External Interrupt lines
// THe SAM4LC8C only has 9 EICs, including NMI (see section 21.8, table 21-3)
typedef enum
{
	EXT_INT0_NMI	= 0,	// This is NonMaskableInt_IRQn, NMI_Handler
	EXT_INT1		= 1,	// This is EIC_0_IRQn, EIC_0_Handler
	EXT_INT2		= 2,	// This is EIC_1_IRQn, EIC_1_Handler
	EXT_INT3		= 3,	// This is EIC_2_IRQn, EIC_2_Handler
	EXT_INT4		= 4,	// This is EIC_3_IRQn, EIC_3_Handler
	EXT_INT5		= 5,	// This is EIC_4_IRQn, EIC_4_Handler
	EXT_INT6		= 6,	// This is EIC_5_IRQn, EIC_5_Handler
	EXT_INT7		= 7,	// This is EIC_6_IRQn, EIC_6_Handler
	EXT_INT8		= 8,	// This is EIC_7_IRQn, EIC_7_Handler
} EIC_INT_CHAN_NUM;

typedef enum {
	EIC_MODE_EDGE_TRIGGERED		= 0,
	EIC_MODE_LEVEL_TRIGGERED	= 1,
}EIC_MODE_TRIGGER_Type;

typedef enum {
	EIC_TRIGGER_FALLING_EDGE	= 0,
	EIC_TRIGGER_RISING_EDGE		= 1,
	EIC_TRIGGER_LOW_LEVEL		= 2,
	EIC_TRIGGER_HIGH_LEVEL		= 3,	
}EIC_TRIGGER_Type;


typedef enum {
	EIC_SYNC_MODE_FILTER_DIS	= 0,
	EIC_SYNC_MODE_FILTER_EN		= 1,
	EIC_ASYNC_MODE				= 2,	// Filter is automatically disabled in ASYNC MODE
}EIC_SYNCFILTER_Type;

typedef enum
{
	EIC_SUCCSESS						= 0,
	EIC_ERROR_UKNOWN					= 0x1500ul,	// Error offset for PDCA
	EIC_ERROR_GENERAL,
	EIC_ERROR_MANAGER_NOT_INTIALIZED,
	EIC_ERROR_CHANNEL_NOT_INITALIZED,
	EIC_ERROR_CHANNEL_IN_USE,
	EIC_ERROR_INT_ALREADY_EN,
	EIC_ERROR_CHANNEL_OUT_OF_RANGE,
	EIC_ERROR_BAD_TRIGGER_SET,
} EIC_ERROR_TYPE;

typedef enum
{
	EIC_STATE_UNKNOWN						= 0,
	EIC_STATE_INTIALIZING,
	EIC_STATE_IDLE,
	EIC_STATE_SETTING_CHANNEL,
} EIC_STATE_TYPE;

typedef enum
{
	EIC_CHAN_STATE_UNKNOWN = 0,

	EIC_CHAN_STATE_BUSY,
	EIC_CHAN_STATE_SETTING_PERIPH,
	EIC_CHAN_STATE_CONFIGURING,
	EIC_CHAN_STATE_CONFIGURED,
	EIC_CHAN_STATE_ENABLED,				// Channel enabled but interrupt not enabled.  Won't actually do anything...
	EIC_CHAN_STATE_PRIMING,
	EIC_CHAN_STATE_PRIMED,				// Channel enabled and interrupt enabled, will throw to interrupt vector as soon as interrupt occurs
	//EIC_CHAN_STATE_INTERRUPT_QUEUED,
	//EIC_CHAN_STATE_EXECUTING_INTERRUPT,
} EIC_CHAN_STATE_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t		binit;				// initialized?
	union
	{
		uint8_t all;
		struct
		{
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
			uint8_t:1;
		} single;
	} error_flags;
	EIC_STATE_TYPE	state;
	uint32_t		intializedChannels;	// Flag which channels are open/active
	
	struct		// structure holding all the info for every EIC channel
	{
		Pin*	pin;
		EIC_CHAN_STATE_TYPE state;
		EIC_ERROR_TYPE error;
		// Interrupt Specific functionality
		void (*pHandler)(void);						// Pointer to the interrupt handler function
		IRQn_Type numIRQ;							// for holding IRQ numbers
		union // for holding all the info about the trigger characteristics of the interrupt
		{
			uint8_t all;
			struct
			{
				EIC_MODE_TRIGGER_Type	trigMode:1;
				EIC_TRIGGER_Type		trig:2;
				EIC_SYNCFILTER_Type		syncFilter:2;
				uint8_t					:3;
			};
		} param;
	} INT[EIC_NUM];
}Manager_EIC_type;

// Create a global manager that can be passed around
Manager_EIC_type mEIC;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////
EIC_ERROR_TYPE EIC_init(				void);
EIC_ERROR_TYPE EIC_initChan(			EIC_INT_CHAN_NUM chanNum, Pin* pin, EIC_MODE_TRIGGER_Type trigMode,
										EIC_TRIGGER_Type trigger, EIC_SYNCFILTER_Type syncFilter);
EIC_ERROR_TYPE EIC_reconfigureChan(		EIC_INT_CHAN_NUM chanNum, EIC_MODE_TRIGGER_Type trigMode,
										EIC_TRIGGER_Type trigger, EIC_SYNCFILTER_Type syncFilter);
EIC_ERROR_TYPE EIC_primeChan(			EIC_INT_CHAN_NUM chanNum);
EIC_ERROR_TYPE EIC_deprimeChan(			EIC_INT_CHAN_NUM chanNum);

#endif