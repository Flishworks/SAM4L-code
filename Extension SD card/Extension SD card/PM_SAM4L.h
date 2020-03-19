#ifndef	_PM_SAM4L_H_	// Guards against double calling of headers
#define _PM_SAM4L_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include "sam.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ADDR_UNLOCK(value)	(0xAA000000 | (value))	// Unlock PM register (Key | offset address) -------------- SHOULD THIS BE MOVED TO THE .c?


typedef enum
{
	PM_SUCCSESS						= 0,
	PM_ERROR_UNKNOWN				= 0x0A00ul,	// Error offset for SCIF
	PM_ERROR_CLK_SOURCE_NOT_ENABLED,
	PM_ERROR_CLK_MASK_ALREADY_EN,
} PM_ERROR_TYPE;

typedef enum
{
	PM_MASK_OFFSET_CPU		= PM_CPUMASK_OFFSET,	// CPU offset
	PM_MASK_OFFSET_HSB		= PM_HSBMASK_OFFSET,	// High Speed Bus (HSB) offset
	PM_MASK_OFFSET_PBA		= PM_PBAMASK_OFFSET,	// Peripheral Bus A (PBA) offset
	PM_MASK_OFFSET_PBB		= PM_PBBMASK_OFFSET,	// Peripheral Bus B (PBB) offset
	PM_MASK_OFFSET_PBC		= PM_PBCMASK_OFFSET,	// Peripheral Bus C (PBC) offset
	PM_MASK_OFFSET_PBD		= PM_PBCMASK_OFFSET,	// Peripheral Bus D (PBD) offset
} PM_MASK_OFFSET_TYPE;

typedef enum
{
	PM_MCSEL_SOURCE_RCSYS	= PM_MCCTRL_MCSEL_SLOW,		// 0 - System RC oscillator
	PM_MCSEL_SOURCE_OSC0	= PM_MCCTRL_MCSEL_OSC0,		// 1 - Oscillator0 (OSC0)
	PM_MCSEL_SOURCE_PLL		= PM_MCCTRL_MCSEL_PLL0,		// 2 - PLL
	PM_MCSEL_SOURCE_DFLL	= PM_MCCTRL_MCSEL_DFLL0,	// 3 - 3 - DFLL
	PM_MCSEL_SOURCE_RC80M	= PM_MCCTRL_MCSEL_RC80M,	// 4 - 80MHz RC oscillator
	PM_MCSEL_SOURCE_RCFAST	= PM_MCCTRL_MCSEL_RCFAST,	// 5 - 4/8/12 MHz RC oscillator
	PM_MCSEL_SOURCE_RC1M	= PM_MCCTRL_MCSEL_RC1M,		// 6 - 1 MHz RC oscillator
	//PM_MCSEL_SOURCE_FLO		= PM_MCCTRL_MCSEL_FLO,	// 7 - not sure what this is...
} PM_MCSEL_SOURCE_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t				fMainClk; // default clock on this system is RCSYS
PM_MCSEL_SOURCE_TYPE	activeClk;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////

PM_ERROR_TYPE PM_init(			void);
PM_ERROR_TYPE PM_switchMCLK(	PM_MCSEL_SOURCE_TYPE src);

// Need for switching the MCLK into highspeed mode -- honestly, need to investigate what this stuff does...
void PM_enableHighSpeed(		void);
void PM_disableHighSpeed(		void);



#endif