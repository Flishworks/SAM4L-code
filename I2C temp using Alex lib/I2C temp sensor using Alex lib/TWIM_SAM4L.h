#ifndef _TWIM_SAM4L_H_	// Guards against double calling of headers
#define _TWIM_SAM4L_H_


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "GPIO_SAM4L.h"
#include "sam.h"

typedef enum
{
	TWIM_SUCCESS		= 0,
	TWIM_ERROR_UNKNOWN	= 0x1B00,	// TWIM ERROR OFFSET
	
} TWIM_ERROR_TYPE;

typedef enum
{
	TWIM_STATE_UNKNOWN						= 0,
	TWIM_STATE_IDLE,
	TWIM_STATE_BUSY,
	TWIM_STATE_INITIALIZING_CLOCK,
	TWIM_STATE_SETTING_PERIPHERALS,
	TWIM_STATE_INITIALIZING,
	TWIM_STATE_CONFIGURING,
	TWIM_STATE_ENABLED,
} TWIM_STATE_TYPE;

typedef enum
{
	TWIM_MODE_STANDARD	= 100000ul,		// 100kb/s
	TWIM_MODE_FAST		= 400000ul,		// 400kb/s
	TWIM_MODE_FASTPLUS	= 1000000ul,	// 1Mb/s
	TWIM_MODE_HIGHSPEED	= 3400000ul		// 3.4Mb/s
}TWIM_MODE_TYPE;




typedef struct
{
	struct
	{
		uint8_t	devInit:1;
		uint8_t pinsInit:1;
		uint8_t chanEnabled:1;
		uint8_t :5;
	} bFlags;
	
	TWIM_STATE_TYPE		state;
	TWIM_MODE_TYPE		mode;
	uint8_t				numTWIM;
	Twim*				regTWIM;
	uint32_t			fmainCLK;
	
	TWIM_ERROR_TYPE		error;
	
	union
	{
		Pin	num[2];
		struct
		{
			Pin	TWD;
			Pin	TWCK;
		};
	} pin;
	
	struct  
	{
		uint8_t status;
		bool bReady;
		// empty
		// ready
		// incomplete
		// running
		uint8_t slaveAddress;
		uint8_t* pData;
		uint8_t numBytes;
		bool bRead;
		
		//enum  ... command status;
		
		
	} CMD[2];
	
	
	
} Manager_TWIM_Type;

Manager_TWIM_Type mTWIM[TWIM_INST_NUM];	// 4 global instance of this structure, will allow for manipulation everywhere

TWIM_ERROR_TYPE TWIM_init(		uint8_t numTWIM,
								Pin* TWD,
								Pin* TWCK,
								TWIM_MODE_TYPE modeTWIM);
TWIM_ERROR_TYPE TWIM_setCMDR(	uint8_t numTWIM,
								uint8_t slaveAddress,
								bool bRead,
								uint8_t* pData,
								uint8_t numBytes,
								bool bStart,
								bool bStop,
								bool bACKLAST);
TWIM_ERROR_TYPE TWIM_setNCMDR(	uint8_t numTWIM,
								uint8_t slaveAddress,
								bool bRead,
								uint8_t* pData,
								uint8_t numBytes,
								bool bStart,
								bool bStop,
								bool bACKLAST);
TWIM_ERROR_TYPE TWIM_runCMDs(	uint8_t numTWIM);



#endif