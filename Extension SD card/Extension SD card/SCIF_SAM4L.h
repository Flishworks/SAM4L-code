#ifndef	_SCIF_SAM4L_H_	// Guards against double calling of headers
#define _SCIF_SAM4L_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "sam.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	SCIF_STATE_UNKNOWN						= 0,
	SCIF_STATE_IDLE,
	SCIF_STATE_BUSY,
	SCIF_STATE_SETTING_CLOCKS,
} SCIF_STATE_TYPE;

typedef enum
{
	// General
	SCIF_SUCCSESS = 0,
	SCIF_ERROR_UNKNOWN = 0x1300,	// Error offset for BSCIF
	SCIF_ERROR_NOT_EN,				// PM not enabled
	
	// OSC0
	SCIF_ERROR_OSC0_SRC_XTAL_OOR,	// Crystal source for OSC0 out of range
	
	// PLL
	SCIF_ERROR_PLL_NUM_NO_EXIST,	// The PLL number chosen doesn't exist on this device
	SCIF_ERROR_PLLMUL_OOR,
	SCIF_ERROR_PLLDIV_OOR,
	SCIF_ERROR_PLLOPT_OOR,
	SCIF_ERROR_PLLOSC_OOR,
	SCIF_ERROR_PLL_OSC0_SRC_NOT_ENABLED,	// The source OSC0 for the PLL is not enabled
	SCIF_ERROR_PLL_GCLK9_SRC_NOT_ENABLED,	// The source GCLK9 for the PLL is not enabled
	
	// DFLL
	SCIF_ERROR_DFLL_NOT_ENABLED,		// DFLL is not enabled
	SCIF_ERROR_DFLL_REF_FREQ_OOR,		// Output frequency requested too high/low
	SCIF_ERROR_DFLL_OUTPUT_FREQ_OOR,	// Reference frequency provided too high/low
	SCIF_ERROR_DFLL_MUL_OOR,			// The multiplier with is larger than [SCIF.DFLL0MUL.MUL], or is 0
	SCIF_ERROR_DFLL_COARSE_VAL_OOR,		// The COARSE value selected is larger than [SCIF.DFLL0VAL.COARSE]
	SCIF_ERROR_DFLL_FINE_VAL_OOR,		// The FINE value selected is larger than [SCIF.DFLL0VAL.FINE]
	
	// RCSYS
	
	// RCFAST
	SCIF_ERROR_RCFAST_OOR,			// Out of range
	
	// RC80M
	
	// GCLK
	SCIF_ERROR_GCLK_NUM_NO_EXIST,	// The GCLK number chosen doesn't exist on this device
	SCIF_ERROR_GCLK_DIV_ODD,		// GCLKs can't handle odd divisors, everything rounded
	SCIF_ERROR_GCLK_DIV_OOR,		// Value for [SCIF.GCCTRL[#].DIV] too large (11 is special)
	
	
} SCIF_ERROR_TYPE;

typedef enum
{
	SCIF_SRCCLK_RCSYS	= 0,		// Default Clock
	SCIF_SRCCLK_OSC0	= 1,
	SCIF_SRCCLK_DFLL	= 2,
	SCIF_SRCCLK_PLL		= 3,
	SCIF_SRCCLK_RC80M	= 4,
	SCIF_SRCCLK_RCFAST	= 5,
	
} SCIF_SRCCLK_TYPE;


///////////////////////////////////////////////////////////////////////////////////////////////////
// PLL Registers - §13.6.2
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SCIF_PLL_RESET_VAL		(0ul)	// Broken in their code, making my own version

#define PLLMUL_MAX_VAL (0x0F)
#define PLLDIV_MAX_VAL (0x0F)
#define PLLOPT_MAX_VAL (0x0F)
#define PLLOSC_MAX_VAL (0x01)	// NOTE, 2-3 are reserved


typedef enum
{
	SCIF_PLL_CLKSRC_OSC0	= 0,
	SCIF_PLL_CLKSRC_GCLK9	= 1,
	//SCIF_PLL_CLKSRC_2		= 2,	// Reserved
	//SCIF_PLL_CLKSRC_3		= 3		// Reserved
} SCIF_PLL_CLKSRC_TYPE;


///////////////////////////////////////////////////////////////////////////////////////////////////
// OSCCTRLn Registers
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SCIF_OSCTRLn_RESET_VAL		(0ul)	// Broken in their code, making my own version

typedef enum // per Table 13-5
{
											// NUMBER RC CYCLES
	SCIF_OSCCTRLn_STARTUP_0s		= 0,	// 0
	SCIF_OSCCTRLn_STARTUP_35us		= 8,	// 4
	SCIF_OSCCTRLn_STARTUP_70us		= 9,	// 8
	SCIF_OSCCTRLn_STARTUP_139us		= 10,	// 16
	SCIF_OSCCTRLn_STARTUP_278us		= 11,	// 32
	SCIF_OSCCTRLn_STARTUP_550us		= 1,	// 64
	SCIF_OSCCTRLn_STARTUP_1100us	= 2,	// 128
	SCIF_OSCCTRLn_STARTUP_2200us	= 12,	// 256
	SCIF_OSCCTRLn_STARTUP_4500us	= 13,	// 512
	SCIF_OSCCTRLn_STARTUP_8900us	= 14,	// 1024
	SCIF_OSCCTRLn_STARTUP_18ms		= 3,	// 2048
	SCIF_OSCCTRLn_STARTUP_36ms		= 4,	// 4096
	SCIF_OSCCTRLn_STARTUP_71ms		= 5,	// 8192
	SCIF_OSCCTRLn_STARTUP_143ms		= 6,	// 16384
	SCIF_OSCCTRLn_STARTUP_285ms		= 7		// 32768
} SCIF_OSCCTRLn_STARTUP_TYPE;

typedef enum // per Table 13-5
{
	SCIF_OSCCTRLn_GAIN_0			= 0,	// G0 -> XIN from 0.6->2.0MHz
	SCIF_OSCCTRLn_GAIN_1			= 1,	// G1 -> XIN from 2.0->4.0MHz
	SCIF_OSCCTRLn_GAIN_2			= 2,	// G2 -> XIN from 4.0->8.0MHz
	SCIF_OSCCTRLn_GAIN_3			= 3,	// G3 -> XIN from 8.0->16.0MHz
} SCIF_OSCCTRLn_GAIN_TYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////
// DFLL Registers - §13.6.3
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SCIF_DFLLVAL_RESET_VAL		(0ul)	// Broken in their code, making my own version
#define SCIF_DFLLMUL_RESET_VAL		(0ul)	// Broken in their code, making my own version
#define SCIF_DFLLSTEP_RESET_VAL		(0ul)	// Broken in their code, making my own version

#define DFLL_REF_FREQ_MIN			(8e3)
#define DFLL_REF_FREQ_MAX			(150e3)
#define DFLL_OUTPUT_FREQ_MIN		(20e6)
#define DFLL_OUTPUT_FREQ_MAX		(150e6)

#define DFLL_FREQ_RANGE0_MAX		(DFLL_OUTPUT_FREQ_MAX)
#define DFLL_FREQ_RANGE1_MAX		(110e6)
#define DFLL_FREQ_RANGE2_MAX		(55e6)
#define DFLL_FREQ_RANGE3_MAX		(30e6)

#define DFLL_COARSE_MAX				(0x1F)	//
#define DFLL_FINE_MAX				(0xFF)	//



///////////////////////////////////////////////////////////////////////////////////////////////////
// RCFAST Registers - §13.6.5
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SCIF_RCFASTCFG_RESET_VAL		(0ul)	// Broken in their code, making my own version


typedef enum
{
	SCIF_RCFASTCFG_FRANGE_4MHz	= 0,
	SCIF_RCFASTCFG_FRANGE_8MHz	= 1,
	SCIF_RCFASTCFG_FRANGE_12MHz	= 2
} SCIF_RCFASTCFG_FRANGE_Type;



///////////////////////////////////////////////////////////////////////////////////////////////////
// RC80M Registers - §13.6.6
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// Generic Clock Registers - §13.6.7-8
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SCIF_GCCTRL_RESET_VAL		(0ul)	// Broken in their code, making my own version

typedef enum
{
	SCIF_GCLK_SRC_RCSYS		= SCIF_GC_USES_CLK_SLOW,		// 0 - System RC oscillator
	SCIF_GCLK_SRC_OSC32K	= SCIF_GC_USES_CLK_32,			// 1 - 32 kHz oscillator
	SCIF_GCLK_SRC_DFLL		= SCIF_GC_USES_DFLL0,			// 2 - DFLL
	SCIF_GCLK_SRC_OSC0		= SCIF_GC_USES_OSC0,			// 3 - Oscillator 0
	SCIF_GCLK_SRC_RC80M		= SCIF_GC_USES_RC80M,			// 4 - 80 MHz RC oscillator
	SCIF_GCLK_SRC_RCFAST	= SCIF_GC_USES_RCFAST,			// 5 - 4-8-12 MHz RC oscillator
	SCIF_GCLK_SRC_RC1M		= SCIF_GC_USES_RC1M,			// 6 - 1 MHz RC oscillator
	SCIF_GCLK_SRC_CLK_CPU	= SCIF_GC_USES_CLK_CPU,		// 7 - CPU clock
	SCIF_GCLK_SRC_CLK_HSB	= SCIF_GC_USES_CLK_HSB,		// 8 - High Speed Bus clock
	SCIF_GCLK_SRC_CLK_PBA	= SCIF_GC_USES_CLK_PBA,		// 9 - Peripheral Bus A clock
	SCIF_GCLK_SRC_CLK_PBB	= SCIF_GC_USES_CLK_PBB,		// 10 - Peripheral Bus B clock
	SCIF_GCLK_SRC_CLK_PBC	= SCIF_GC_USES_CLK_PBC,		// 11 - Peripheral Bus C clock
	SCIF_GCLK_SRC_CLK_PBD	= SCIF_GC_USES_CLK_PBD,		// 12 - Peripheral Bus D clock
	SCIF_GCLK_SRC_RC32K		= SCIF_GC_USES_RC32K,			// 13 - 32 kHz RC oscillator
	SCIF_GCLK_SRC_CLK_1K	= SCIF_GC_USES_CLK_1K,			// 15 - 1 kHz output from OSC32K
	SCIF_GCLK_SRC_PLL0		= SCIF_GC_USES_PLL0,			// 16 - PLL0
	SCIF_GCLK_SRC_HRPCLK	= SCIF_GC_USES_GCLKPRESC_HRP,	// 17 - High resolution prescaler
	SCIF_GCLK_SRC_FPCLK		= SCIF_GC_USES_GCLKPRESC_FP,	// 18 - Fractional prescaler
	SCIF_GCLK_SRC_GCLKIN0	= SCIF_GC_USES_GCLK_IN0,		// 19 - GCLKIN0
	SCIF_GCLK_SRC_GCLKIN1	= SCIF_GC_USES_GCLK_IN1,		// 20 - GCLKIN1
	SCIF_GCLK_SRC_GCLK11	= SCIF_GC_USES_MASTER,			// 21 - GCLK11
} SCIF_GCLK_SRC_TYPE;

typedef enum
{
	GCLK_0_DFLL_REF_GCLK0_PIN	= SCIF_GCLK_NUM_DFLL_REF,		// 0 - 
	GCLK_1_DFLL_SSG_GCLK1_PIN	= SCIF_GCLK_NUM_DFLL_SSG,		// 1 - 
	GCLK_2_AST_GCLK2_PIN		= SCIF_GCLK_NUM_EXTCLK_2,		// 2 - 
	GCLK_3_CATB_GCLK3_PIN		= SCIF_GCLK_NUM_EXTCLK_3,		// 3 - 
	GCLK_4_AESA					= 4,							// 4 - Also FLO? - SCIF_GCLK_NUM_FLO??
	GCLK_5_GLOC_TC0_RC32K		= SCIF_GCLK_NUM_RC32KIFB_REF,	// 5 - 
	GCLK_6_ABDACB_IISC			= 6,							// 6 - 
	GCLK_7_USBC					= 7,							// 7 - 
	GCLK_8_TC1_PEVC0			= 8,							// 8 - 
	GCLK_9_PLL0_PEVC1			= SCIF_GCLK_NUM_PLL,			// 9 - 
	GCLK_10_ADCIFE				= 10,							// 10 - 
	GCLK_11_MASTER				= SCIF_GCLK_NUM_MASTER,			// 11 - 
} GCLK_NUM_TYPE;

// typedef enum
// {
// 	GENCLK_ALLOC_DFLLIF        = 0,    // System RC oscillator
// 	GENCLK_
// } GENCLK_ALLOCATION_TYPE;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t				bInit;
	SCIF_STATE_TYPE		state;
	//SCIF_ERROR_TYPE errorState;
	
	struct
	{
		struct  
		{
			uint32_t RCSYS;
			uint32_t OSC0;
			uint32_t PLL;
			uint32_t DFLL;
			uint32_t RC80M;
			uint32_t RCFAST;
		} CLK;
		uint32_t GCLK[12];
	} freq;


	struct // PROBABLY NEED TO RE-DO THIS IF WE ADD MORE FUNCTIONALITY IN THE FUTURE
	{
		union
		{
			struct
			{
				uint8_t RCSYS:1;
				uint8_t OSC0:1;
				uint8_t PLL:1;
				uint8_t DFLL:1;
				uint8_t	RC80M:1;
				uint8_t RCFAST:1;
				uint8_t	:2;
			};
			uint8_t allCLKS;
		} CLK;
		union
		{
			struct
			{
				uint8_t GCLK0_DFLL:1;
				uint8_t GCLK1_DFLL:1;
				uint8_t GCLK2_AST:1;
				uint8_t GCLK3_CATB:1;
				uint8_t GCLK4_AESA:1;
				uint8_t	GCLK5_GLOC_TC0_RC32K:1;
				uint8_t	GCLK6_ABDACB_IISC:1;
				uint8_t	GCLK7_USBC:1;
				uint8_t GCLK8_TC1_PEVC0:1;
				uint8_t	GCLK9_PLL0_PEVC1:1;
				uint8_t	GCLK10_ADCIFE:1;
				uint8_t	GCLK11_MASTER_GCLK:1;
				uint8_t	:4;
			};
			uint16_t allGCLKS;
		} GCLK;
	} bEnabled;
	

} Manager_SCIF_Type;

Manager_SCIF_Type mSCIF;



///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////
// General
SCIF_ERROR_TYPE SCIF_init(					void);
// SCIF_ERROR_TYPE SCIF_disable(void); // Not Tested yet...

// OSCO
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_OSC0(	uint32_t fXTAL, SCIF_OSCCTRLn_STARTUP_TYPE startup);
SCIF_ERROR_TYPE SCIF_resetSrcClk_OSC0(		void);

// PLL
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_PLL(	uint8_t numPLL, uint8_t pllMul, uint8_t pllDiv,
											uint8_t pllOpt, SCIF_PLL_CLKSRC_TYPE pllOsc);
SCIF_ERROR_TYPE SCIF_resetSrcClk_PLL(		uint8_t numPLL);

// DFLL
SCIF_ERROR_TYPE SCIF_enableDFLL(			void);
SCIF_ERROR_TYPE SCIF_disableResetDFLL(		void);
SCIF_ERROR_TYPE SCIF_initSrcClk_DFLL_Closed(uint32_t fReference, uint32_t fDFLL, bool bUseBestGuessVAL);
SCIF_ERROR_TYPE SCIF_awaitFineLock_DFLL(	void);

// RCFAST
SCIF_ERROR_TYPE SCIF_setEnableSrcClk_RCFAST(SCIF_RCFASTCFG_FRANGE_Type frange);
SCIF_ERROR_TYPE SCIF_reenableSrcClk_RCFAST(	void);
SCIF_ERROR_TYPE SCIF_disableSrcClk_RCFAST(	void);
SCIF_ERROR_TYPE SCIF_resetSrcClk_RCFAST(	void);

// GCLK
SCIF_ERROR_TYPE SCIF_setEnableGCLK(			GCLK_NUM_TYPE numGCLK, uint32_t fSrc, uint16_t divFactor,
											SCIF_GCLK_SRC_TYPE srcClk);
SCIF_ERROR_TYPE SCIF_resetDisableGCLK(		GCLK_NUM_TYPE numGCLK);

#endif