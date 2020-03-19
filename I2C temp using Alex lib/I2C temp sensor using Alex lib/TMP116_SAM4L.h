#ifndef _TMP116_SAM4L_H_
#define _TMP116_SAM4L_H_

#include <stddef.h>
#include <stdint.h>
#include "TWIM_SAM4L.h"
#include "datashuffle_Alex.h"
//#include "PDCA_SMA4L.h"
#include "GPIO_SAM4L.h"


// http://www.ti.com/product/TMP116					- Product website
// http://www.ti.com/lit/ds/symlink/tmp116.pdf		- Datasheet

typedef enum
{
	TMP116_STATE_UNKNOWN					= 0,
	TMP116_STATE_RESET,
	TMP116_STATE_INIT_REGISTERS,
	TMP116_STATE_NORMAL_RUNNING,
	TMP116_STATE_PULL_DATA,
	TMP116_STATE_STANDBY,
	TMP116_STATE_SHUTDOWN,
} TMP116_STATE_Type;

typedef enum
{
	TMP116_POWER_MODE_ACTIVE				= 0,
	TMP116_POWER_MODE_STANDBY,
	TMP116_POWER_MODE_SHUTDOWN,
	TMP116_POWER_MODE_UNKNOWN,
} TMP116_POWER_MODE_Type;

typedef enum
{
	TMP116_SUCCSESS							= 0,
	TMP116_ERROR_UNKNOWN					= 0x6900ul,	// Error offset for TMP116 TODO: edit this number
} TMP116_ERROR_Type;

typedef enum
{
	// 7-bit address = LSB bits here
	TMP116_I2C_ADDRESS_GND		= 0b01001000,
	TMP116_I2C_ADDRESS_VPLUS	= 0b01001001,	
	TMP116_I2C_ADDRESS_SDA		= 0b01001010,
	TMP116_I2C_ADDRESS_SCL		= 0b01001011,
} TMP116_I2C_Address_Type;

#define TMP116_WRITE_REG(value)		(uint8_t)(value)
#define TMP116_READ_REG(value)		(uint8_t)((value) | 0x01)

#define TMP116_GENERAL_CALL_ADDRESS					0x00
#define TMP116_RESET_COMMAND						0x06

///////////////////////////////////////////////////////////////////////////////
// TEMPERATURE: READ ONLY DATA REGISTER
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_TEMPERATURE_ADDRESS				0x00
#define TMP116_REG_TEMPERATURE_RESET_VALUE			0x8000	// Read only

///////////////////////////////////////////////////////////////////////////////
// DEVICE_ID: READ ONLY REGISTER
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_DEVICE_ID_ADDRESS				0x0F
#define TMP116_DEVICE_ID							0x0116	// Read only (reset = x116h => <11:0> = device ID)
#define TMP116_DEVICE_ID_BITMASK					0x0FFF

///////////////////////////////////////////////////////////////////////////////
// CONFIGURATION: READ ONLY DATA REGISTER
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_CONFIGURATION_ADDRESS			0x01
#define TMP116_REG_CONFIGURATION_RESET_VALUE		0x0220
#define TMP116_REG_CONFIGURATION_CONFIG_BITMASK		0x0FFF

#define TMP116_CONFIGURATION_DR_ALERT_pos			2
#define TMP116_CONFIGURATION_DR_ALERT				(0x1u << TMP116_CONFIGURATION_DR_ALERT_pos)
#define TMP116_CONFIGURATION_DR_ALERT_ALERT_FLAG	(0x0u << TMP116_CONFIGURATION_DR_ALERT_pos)
#define TMP116_CONFIGURATION_DR_ALERT_DR_FLAG		(0x1u << TMP116_CONFIGURATION_DR_ALERT_pos)
#define TMP116_CONFIGURATION_POL_pos				3
#define TMP116_CONFIGURATION_POL					(0x1u << TMP116_CONFIGURATION_POL_pos)
#define TMP116_CONFIGURATION_POL_LO					(0x0u << TMP116_CONFIGURATION_POL_pos)
#define TMP116_CONFIGURATION_POL_HI					(0x1u << TMP116_CONFIGURATION_POL_pos)
#define TMP116_CONFIGURATION_T_NA_pos				4
#define TMP116_CONFIGURATION_T_NA					(0x1u << TMP116_CONFIGURATION_T_NA_pos)
#define TMP116_CONFIGURATION_T_NA_ALERT				(0x0u << TMP116_CONFIGURATION_T_NA_pos)
#define TMP116_CONFIGURATION_T_NA_THERM				(0x1u << TMP116_CONFIGURATION_T_NA_pos)
#define TMP116_CONFIGURATION_AVG_pos				5
#define TMP116_CONFIGURATION_AVG(value)				((value) << TMP116_CONFIGURATION_AVG_pos)
#define TMP116_CONFIGURATION_CONV_pos				7
#define TMP116_CONFIGURATION_CONV(value)			((value) << TMP116_CONFIGURATION_CONV_pos)
#define TMP116_CONFIGURATION_MOD_pos				10
#define TMP116_CONFIGURATION_MOD(value)				((value) << TMP116_CONFIGURATION_MOD_pos)
#define TMP116_CONFIGURATION_EEPROM_BUSY_pos		12
#define TMP116_CONFIGURATION_DATA_READY_pos			13
#define TMP116_CONFIGURATION_LOW_ALRET_pos			14
#define TMP116_CONFIGURATION_HIGH_ALERT_pos			15

typedef enum
{
	TMP116_NO_AVG	=	0x00,
	TMP116_8_AVG	=	0x01,
	TMP116_32_AVG	=	0x02,
	TMP116_64_AVG	=	0x03,
} TMP116_AVG_Type;

typedef enum
{
	TMP116_15p5ms	=	0x00,
	TMP116_125ms	=	0x01,
	TMP116_250ms	=	0x02,
	TMP116_500ms	=	0x03,
	TMP116_1s		=	0x04,
	TMP116_4s		=	0x05,
	TMP116_8s		=	0x06,
	TMP116_16s		=	0x07,
} TMP116_CONV_Type;

typedef enum
{
	TMP116_CONTINUOUS_CONVERSION			=	0x00,
	TMP116_SHUTDOWN							=	0x01,
	TMP116_CONTINUOUS_CONVERSION_READBACK00	=	0x02,
	TMP116_ONE_SHOT_CONVERSION				=	0x03,
} TMP116_MOD_Type;

COMPILER_TIGHT_PACK()	// Bit pack because CONV on byte boundary
typedef union {
	struct {
		uint16_t :2;						// [1:0] - <R>	- Reserved, reads as <00>
		uint16_t DR_ALERT:1;				// [2]	 - <R>	- Data ready, Alert flag select: <0> = status of the alert flags, <1> = status of the data ready flag
		uint16_t POL:1;						// [3]   - <R/W>- Alert polarity: <0> = active LO, <1> = active high
		uint16_t T_NA:1;					// [4]   - <R/W>- Therm <1>/Alert <0> mode selection
		TMP116_AVG_Type AVG:2;				// [6:5] - <R/W>- Conversion averaging mode
		TMP116_CONV_Type CONV:3;			// [9:7] - <R/W>- conversion cycle
		TMP116_MOD_Type MOD:2;				// [11:10]-<R/W>- Conversion mode
		uint16_t EEPROM_BUSY:1;				// [12]	 - <R>	- EEPROM busy flag
		uint16_t DATA_READY:1;				// [13]	 - <R>	- Data ready flag
		uint16_t LOW_ALERT:1;				// [14]	 - <R>	- Alert mode conversion result <
		uint16_t HIGH_ALERT:1;				// [15]	 - <R>	- Alert mode conversion result >; therm mode conversion result >
	} bit;
	uint16_t reg;
} TMP116_CONFIGURATION_Type;
COMPILER_RESET_PACK()

///////////////////////////////////////////////////////////////////////////////
// HIGH_LIMIT: High limit for comparison with temperature result
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_HIGH_LIMIT_ADDRESS				0x02
#define TMP116_REG_HIGH_LIMIT_RESET_VALUE			0x6000

#define TMP116_HIGH_LIMIT(value)					(value)

typedef union {
	struct {
		uint16_t H:16;						// [15:0] - <R/W>- High limit for comparison with the temperature result
	} bit;
	uint16_t reg;
} TMP116_HIGH_LIMIT_Type;

///////////////////////////////////////////////////////////////////////////////
// LOW_LIMIT: Low limit for comparison with temperature result
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_LOW_LIMIT_ADDRESS				0x03
#define TMP116_REG_LOW_LIMIT_RESET_VALUE			0x8000

#define TMP116_LOW_LIMIT(value)						(value)

typedef union {
	struct {
		uint16_t L:16;						// [15:0] - <R/W>- Low limit for comparison with the temperature result
	} bit;
	uint16_t reg;
} TMP116_LOW_LIMIT_Type;

///////////////////////////////////////////////////////////////////////////////
// EEPROM_UNLOCK: Unlock EEPROM and check status
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_EEPROM_UNLOCK_ADDRESS			0x04
#define TMP116_REG_EEPROM_UNLOCK_RESET_VALUE		0x0000

#define TMP116_EEPROM_UNLOCK_EEPROM_BUSY_pos		14
#define TMP116_EEPROM_UNLOCK_EUN_pos				15
#define TMP116_EEPROM_UNLOCK_EUN					(0x1u << TMP116_EEPROM_UNLOCK_EUN_pos)
#define TMP116_EEPROM_UNLOCK_EUN_LOCK				(0x0u << TMP116_EEPROM_UNLOCK_EUN_pos)
#define TMP116_EEPROM_UNLOCK_EUN_UNLOCK				(0x1u << TMP116_EEPROM_UNLOCK_EUN_pos)

typedef union {
	struct {
		uint16_t :14;						// [13:0] - <R>  - Reserved
		uint16_t EEPROM_BUSY:1;				// [14]	  - <R>	 - EEPROM busy flag
		uint16_t EUN:1;						// [15]	  - <R/W>- <0> = locked for programming; <1> = unlocked for programming
	} bit;
	uint16_t reg;
} TMP116_EEPROM_UNLOCK_Type;

///////////////////////////////////////////////////////////////////////////////
// EEPROM1: General purpose data storage (scratch pad)
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_EEPROM1_ADDRESS					0x05
#define TMP116_REG_EEPROM1_RESET_VALUE				0x0000

#define TMP116_EEPROM1_D(value)						(value)

typedef union {
	struct {
		uint16_t D:16;						// [15:0] - <R/W>- Scratch pad
	} bit;
	uint16_t reg;
} TMP116_EEPROM1_Type;

///////////////////////////////////////////////////////////////////////////////
// EEPROM2: General purpose data storage (scratch pad)
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_EEPROM2_ADDRESS					0x06
#define TMP116_REG_EEPROM2_RESET_VALUE				0x0000

#define TMP116_EEPROM2_D(value)						(value)

typedef union {
	struct {
		uint16_t D:16;						// [15:0] - <R/W>- Scratch pad
	} bit;
	uint16_t reg;
} TMP116_EEPROM2_Type;

///////////////////////////////////////////////////////////////////////////////
// EEPROM3: General purpose data storage (scratch pad)
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_EEPROM3_ADDRESS					0x07
#define TMP116_REG_EEPROM3_RESET_VALUE				0x0000

#define TMP116_EEPROM3_D(value)						(value)

typedef union {
	struct {
		uint16_t D:16;						// [15:0] - <R/W>- Scratch pad
	} bit;
	uint16_t reg;
} TMP116_EEPROM3_Type;

///////////////////////////////////////////////////////////////////////////////
// EEPROM4: General purpose data storage (scratch pad)
///////////////////////////////////////////////////////////////////////////////
#define TMP116_REG_EEPROM4_ADDRESS					0x08
#define TMP116_REG_EEPROM4_RESET_VALUE				0x0000 // (xxxxh)

#define TMP116_EEPROM4_D(value)						(value)

typedef union {
	struct {
		uint16_t D:16;						// [15:0] - <R/W>- Scratch pad
	} bit;
	uint16_t reg;
} TMP116_EEPROM4_Type;




typedef struct
{
	TMP116_ERROR_Type		error_flags;	// not sure what to do with this....
	TMP116_STATE_Type		state;
	TMP116_POWER_MODE_Type	powerMode;		// keeps track of the power mode device is in.
	
	
	union
	{
		struct
		{
			uint8_t	bInit:1;
			uint8_t bSTART:1;
			uint8_t :6;
		} indv;
		uint8_t all;
	} flags;
	
	struct
	{
		Pin ALERT;						// Over-temperature or data-ready signal (open drain)
	} pin;

	uint8_t deviceAddress;				// I2C address for the device (4 options possible)
	
	// Communication info
	Manager_TWIM_Type*			pmTWIM;		// I2C manager
	//Manager_PDCA_Chan_Type*	pDMA_TX;	// this holds the transmit data
	//Manager_PDCA_Chan_Type*	pDMA_RX;	// this holds the receive data
	
	// Command array for DMA transfers
	//uint16_t	getData[2];

	
} Manager_TMP116_Type;



TMP116_ERROR_Type TPM116_Init					(Manager_TMP116_Type* pmTMP, uint8_t numTWIM, Pin* alert, TMP116_I2C_Address_Type i2cAddress, TMP116_AVG_Type numAvgs, bool activeHI);
TMP116_ERROR_Type TMP116_start					(Manager_TMP116_Type* pmTMP);
TMP116_ERROR_Type TMP116_stop					(Manager_TMP116_Type* pmTMP);
TMP116_ERROR_Type TMP116_triggerMeasurement		(Manager_TMP116_Type* pmTMP);
TMP116_ERROR_Type TMP116_generalCallReset		(Manager_TMP116_Type* pmTMP);
TMP116_ERROR_Type TMP116_writeRegister			(Manager_TMP116_Type* pmTMP, uint8_t regAdd, uint16_t regData);
TMP116_ERROR_Type TMP116_readRegister			(Manager_TMP116_Type* pmTMP, uint8_t regAddress, uint16_t* pRegDataRx);
float			  TMP116_convertTemp			(float result);
#endif