// BSCIF
// James A. Heller
// 2016.10.21
// =============================

#ifndef _BSCIF_SAM4L_H_
#define _BSCIF_SAM4L_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include "sam.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
//#define BSCIF_ALEX_VERSION = 0.01
#define BSCIF_OSCCTRL32_RESETVAL	0x00000004ul // this exists because there is a bug in the default Atmel code for this (added a ;)
#define BSCIF_RC32KCR_RESETVAL		0x00000000ul

#define BSCIF_ADDR_UNLOCK(value)	(0xAA000000ul | (value))	// Unlock PM register (Key | offset address)

typedef enum
{
	BSCIF_STATE_UNKNOWN						= 0,
	BSCIF_STATE_IDLE,
	BSCIF_STATE_BUSY,
	BSCIF_STATE_SETTING_CLOCKS,
	BSCIF_STATE_SETTING_CHANNEL,
} BSCIF_STATE_TYPE;

typedef enum
{
	BSCIF_SUCCSESS						= 0,
	BSCIF_ERROR_UNKNOWN					= 0x1200,	// Error offset for BSCIF
	BSCIF_ERROR_OSC32_ALREADY_INIT,
	
} BSCIF_ERROR_TYPE;

typedef enum
{
	OSC32K_MODE_EXT_CLK					= 0, // External clock connected to XIN32
	
	// Crystal Modes, Crystal is connected to XIN32/XOUT32.
	OSC32K_MODE_XTAL_NORMAL				= 1, // Crystal mode
	OSC32K_MODE_XTAL_AMP_CTRL			= 3, // Crystal mode w/ amplitude controlled mode
	OSC32K_MODE_XTAL_HCURRENT			= 4, // Crystal and high current mode
	OSC32K_MODE_XTAL_HCURRENT_AMP_CTRL	= 5, // Crystal with high current mode and amplitude controlled mode.
} OSC32K_MODE_Type;

typedef enum
{
	OSC32K_OUTPUT_DISABLE	= 0,
	OSC32K_OUTPUT_32K_EN	= BSCIF_OSCCTRL32_EN32K,						// Enable 32Khz signal output.  Default at power up
	OSC32K_OUTPUT_1K_EN		= BSCIF_OSCCTRL32_EN1K,							// Enable 1kHz signal.
	OSC32K_OUTPUT_BOTH_EN	= BSCIF_OSCCTRL32_EN32K | BSCIF_OSCCTRL32_EN1K,	// Enable 32Khz and 1kHz signal outputs.
} OSC32K_OUTPUT_Type;

typedef enum
{
	OSC32K_SELCURR_50nA	= 0,
	OSC32K_SELCURR_75nA,
	OSC32K_SELCURR_100nA,
	OSC32K_SELCURR_125nA,
	OSC32K_SELCURR_150nA,
	OSC32K_SELCURR_175nA,
	OSC32K_SELCURR_200nA,
	OSC32K_SELCURR_225nA,
	OSC32K_SELCURR_250nA,
	OSC32K_SELCURR_275nA,
	OSC32K_SELCURR_300nA,	// Recommended value for 32khz xtal
	OSC32K_SELCURR_325nA,
	OSC32K_SELCURR_350nA,
	OSC32K_SELCURR_375nA,
	OSC32K_SELCURR_400nA,
	OSC32K_SELCURR_425nA,
} OSC32K_SELCURR_Type;

typedef enum
{
	OSC32K_STARTUP_0ms = 0,
	OSC32K_STARTUP_1ms,
	OSC32K_STARTUP_72ms,
	OSC32K_STARTUP_143ms,
	OSC32K_STARTUP_540ms,
	OSC32K_STARTUP_1100ms,	// What is used on the SAM4L8 xPlained Dev board
	OSC32K_STARTUP_2300ms,
	OSC32K_STARTUP_4600ms,
} OSC32K_STARTUP_Type;

typedef enum
{
	RC32K_MODE_OPEN					= 0, // 
	RC32K_MODE_CLOSED				= 1, // 
} RC32K_MODE_Type;

typedef enum
{
	RC32K_OUTPUT_DISABLE	= 0,
	RC32K_OUTPUT_32K_EN		= BSCIF_RC32KCR_EN32K,						// Enable 32Khz signal output.  Default at power up
	RC32K_OUTPUT_1K_EN		= BSCIF_RC32KCR_EN1K,						// Enable 1kHz signal.
	RC32K_OUTPUT_GCLK_EN	= BSCIF_RC32KCR_EN,
} RC32K_OUTPUT_Type;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t				bInit;
	BSCIF_STATE_TYPE	state;
	
	struct
	{
		uint32_t OSC32K;
		uint32_t RC32K;
		uint32_t RC1M;
	} freqClks;

	struct // PROBABLY NEED TO RE-DO THIS IF WE ADD MORE FUNCTIONALITTY IN THE FUTURE
	{
		uint8_t OSC32K:1;
		uint8_t RC32K:1;
		uint8_t RC1M:1;
		uint8_t :5;
	} bClkEn;
	
	
} Manager_BSCIF_Type;

Manager_BSCIF_Type mBSCIF;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////
BSCIF_ERROR_TYPE BSCIF_init(			void);
BSCIF_ERROR_TYPE OSC32k_init(			OSC32K_MODE_Type osc_mode,
										OSC32K_OUTPUT_Type output,
										OSC32K_SELCURR_Type xtalDriveCurrent,
										OSC32K_STARTUP_Type startupTime,
										uint32_t frequency);
BSCIF_ERROR_TYPE OSC32k_disable(		void);
BSCIF_ERROR_TYPE RC32k_init(			RC32K_MODE_Type osc_mode,
										RC32K_OUTPUT_Type output,
										uint32_t frequency);

#endif