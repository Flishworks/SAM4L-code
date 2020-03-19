#include "TMP116_SAM4L.h"
#include "sam.h"
#include "delay_SAM4L.h"


TMP116_ERROR_Type TPM116_Init(Manager_TMP116_Type* pmTMP, uint8_t numTWIM,
							  Pin* alert, 
							  TMP116_I2C_Address_Type i2cAddress,
							  TMP116_AVG_Type numAvgs, bool activeHI )
							  //uint8_t DMA_TX_chan, uint8_t DMA_RX_chan)
{
	uint16_t rxData;
	TMP116_EEPROM_UNLOCK_Type unlockReg;

	pmTMP->pmTWIM			= &mTWIM[numTWIM];				// Push the I2C manager
	//pmTMP->pDMA_TX		= &mPDCA.chan[DMA_TX_chan];		// Push the DMA manager for TX
	//pmTMP->pDMA_RX		= &mPDCA.chan[DMA_RX_chan];

	pmTMP->pin.ALERT		= *alert;						// Alert pin mapping
	pmTMP->deviceAddress	= i2cAddress;					// I2C address for device


	// At this point, the device will have whatever EEPROM settings were previously programmed and not necessarily factory reset values
	pmTMP->state = TMP116_STATE_UNKNOWN;		
	pmTMP->powerMode = TMP116_POWER_MODE_UNKNOWN;

	pmTMP->state = TMP116_STATE_INIT_REGISTERS;

	// Read device ID to make sure can talk to the chip
	TMP116_readRegister(pmTMP, TMP116_REG_DEVICE_ID_ADDRESS, &rxData);
	if ( (rxData & TMP116_DEVICE_ID_BITMASK) != TMP116_DEVICE_ID)	return TMP116_ERROR_UNKNOWN;

	// Configure registers with EEPROM unlocked to save for resets
	TMP116_writeRegister(pmTMP, TMP116_REG_EEPROM_UNLOCK_ADDRESS, TMP116_EEPROM_UNLOCK_EUN_UNLOCK);
	delay_ms(1);

	// Check to make sure not busy
	do 
	{
		TMP116_readRegister(pmTMP, TMP116_REG_EEPROM_UNLOCK_ADDRESS, &unlockReg.reg);
		//unlockReg.reg = rxData;
	} while (unlockReg.bit.EEPROM_BUSY);
	
	// Configure the sensor
	volatile TMP116_CONFIGURATION_Type correct_config;
	if ( activeHI )		correct_config.reg = TMP116_CONFIGURATION_POL_HI;
	else				correct_config.reg = TMP116_CONFIGURATION_POL_LO;
	correct_config.reg |= ( TMP116_CONFIGURATION_MOD(TMP116_CONTINUOUS_CONVERSION) |
							TMP116_CONFIGURATION_CONV(TMP116_16s) |
							TMP116_CONFIGURATION_AVG(numAvgs) |
							TMP116_CONFIGURATION_DR_ALERT_DR_FLAG );
	TMP116_writeRegister(pmTMP, TMP116_REG_CONFIGURATION_ADDRESS, correct_config.reg);


	// Wait typical time for register to change (7 ms typical) before checking to see if write complete
	delay_ms(10);	
	do 
	{
		delay_ms(1);
		TMP116_readRegister(pmTMP, TMP116_REG_EEPROM_UNLOCK_ADDRESS, &unlockReg.reg);
	} while ( unlockReg.bit.EEPROM_BUSY );

	// Complete general call reset, which also locks the EEPROM (according to program flow but necessary??? how does this affect other devices on the bus?)
	TMP116_generalCallReset(pmTMP);
	delay_ms(2);	// Wait until power-up sequence (1.5 ms typical) is complete to read valid data

	// Read back the config register to check the value to make sure it as configured correctly
	TMP116_CONFIGURATION_Type act_config;
	do
	{
		TMP116_readRegister(pmTMP, TMP116_REG_CONFIGURATION_ADDRESS, &act_config.reg);
	} while (act_config.bit.EEPROM_BUSY == 1);	// Make sure not busy (i.e., still powering up)
	if ( (act_config.reg & TMP116_REG_CONFIGURATION_CONFIG_BITMASK) != (correct_config.reg & TMP116_REG_CONFIGURATION_CONFIG_BITMASK) )	return TMP116_ERROR_UNKNOWN;
	
	// Put in shutdown mode for now
	TMP116_writeRegister(pmTMP, TMP116_REG_CONFIGURATION_ADDRESS, TMP116_CONFIGURATION_MOD(TMP116_SHUTDOWN));
	pmTMP->powerMode = TMP116_POWER_MODE_SHUTDOWN;
	pmTMP->state = TMP116_STATE_SHUTDOWN;

	pmTMP->flags.indv.bInit = true;

	return TMP116_SUCCSESS;
}

TMP116_ERROR_Type TMP116_start(Manager_TMP116_Type* pmTMP)
{
	if (pmTMP->state == TMP116_STATE_NORMAL_RUNNING && pmTMP->powerMode == TMP116_POWER_MODE_ACTIVE) return TMP116_SUCCSESS;				// Want in shutdown until trigger a measurment

	// Put in continuous conversion mode
	TMP116_writeRegister(pmTMP, TMP116_REG_CONFIGURATION_ADDRESS, TMP116_CONFIGURATION_MOD(TMP116_CONTINUOUS_CONVERSION));
	
	pmTMP->powerMode = TMP116_POWER_MODE_ACTIVE;
	pmTMP->state = TMP116_STATE_NORMAL_RUNNING;

	return TMP116_SUCCSESS;
}

TMP116_ERROR_Type TMP116_stop(Manager_TMP116_Type* pmTMP)
{																																																												
	if (pmTMP->state == TMP116_STATE_SHUTDOWN && pmTMP->powerMode == TMP116_POWER_MODE_SHUTDOWN) return TMP116_SUCCSESS;				// Want in shutdown until trigger a measurement

	// Put in shutdown mode
	TMP116_writeRegister(pmTMP, TMP116_REG_CONFIGURATION_ADDRESS, TMP116_CONFIGURATION_MOD(TMP116_SHUTDOWN));
	
	pmTMP->powerMode = TMP116_POWER_MODE_SHUTDOWN;
	pmTMP->state = TMP116_STATE_SHUTDOWN;

	return TMP116_SUCCSESS;
}

TMP116_ERROR_Type TMP116_triggerMeasurement(Manager_TMP116_Type* pmTMP)
{
	// To get simultaneous measurements from multiple temp sensors, trigger a general-call reset (knowing all EEPROMs have been configured previously)
	return TMP116_generalCallReset(pmTMP);
}

TMP116_ERROR_Type TMP116_generalCallReset(Manager_TMP116_Type* pmTMP)
{
	// This function sends a reset command to ALL the TMP116's on the I2C bus

	uint8_t origDevAddress = pmTMP->deviceAddress;

	pmTMP->deviceAddress = TMP116_GENERAL_CALL_ADDRESS;

	// Form the packet to be sent: START | GENERAL_CALL_ADDR | W | ACK | RESET_CMD | ACK | STOP
	uint8_t bytesCMDR[1] = {TMP116_RESET_COMMAND};

	// General-call reset: all devices on the bus will be reset, load config settings from EEPROM, and complete measurement based on settings
	TWIM_setCMDR(pmTMP->pmTWIM->numTWIM, pmTMP->deviceAddress, false, &bytesCMDR[0], 1, true, true, false);
	TWIM_runCMDs(pmTMP->pmTWIM->numTWIM);

	pmTMP->deviceAddress = origDevAddress;

	return TMP116_SUCCSESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// COMMUNCIATIONS FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////
TMP116_ERROR_Type TMP116_writeRegister(Manager_TMP116_Type* pmTMP, uint8_t regAdd, uint16_t regData)
{
	u_16 regData_u16;
	regData_u16.all = regData;

	// Form the packets to be sent: START | SLAVE_ADDR | ACK | W | REG_ADDR_BYTE | ACK | REG_DATA_HI | ACK | REG_DATA_LO | ACK | STOP
	uint8_t bytesCMDR[3] = {regAdd, regData_u16.byte[1], regData_u16.byte[0]};

	TWIM_setCMDR(pmTMP->pmTWIM->numTWIM, pmTMP->deviceAddress, false, &bytesCMDR[0], 3, true, true, false);
	TWIM_runCMDs(pmTMP->pmTWIM->numTWIM);

	return TMP116_SUCCSESS;
}

TMP116_ERROR_Type TMP116_readRegister(Manager_TMP116_Type* pmTMP, uint8_t regAddress, uint16_t* pRegDataRx)
{
	// Form the packet(s) to be sent: : START | SLAVE_ADDR | W | ACK | REG_ADDR_BYTE | ACK | START | SLAVE_ADDR | R | ACK | REG_DATA_HI | ACK | REG_DATA_LO | NACK | STOP
	uint8_t bytesCMDR[1] = {regAddress};
	
	uint8_t rxBuff[2];

	// Setup the command(s) and run them
	TWIM_setCMDR(pmTMP->pmTWIM->numTWIM, pmTMP->deviceAddress, false, &bytesCMDR[0], 1, true, false, false);
	TWIM_setNCMDR(pmTMP->pmTWIM->numTWIM, pmTMP->deviceAddress, true, &rxBuff[0], 2, true, true, false);	
	TWIM_runCMDs(pmTMP->pmTWIM->numTWIM);

	pRegDataRx[0] = (rxBuff[0] << 8 | rxBuff[1]);

	return TMP116_SUCCSESS;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// CONVERSION
///////////////////////////////////////////////////////////////////////////////////////////////////
float TMP116_convertTemp(float temp)
{
	// Returns temperature in deg C

	temp *= (float)(0.0078125);

	return temp;
}


