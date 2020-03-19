#include "PM_SAM4L.h"
#include "TWIM_SAM4L.h"

Twim* TWIM_Instances[TWIM_INST_NUM] = TWIM_INSTS;


TWIM_ERROR_TYPE TWIM_clearCMD(uint8_t numTWIM, uint8_t numCMD);


TWIM_ERROR_TYPE TWIM_init(uint8_t numTWIM, Pin* TWD, Pin* TWCK, TWIM_MODE_TYPE modeTWIM)
{
	if (numTWIM >= TWIM_INST_NUM) return TWIM_ERROR_UNKNOWN;	// if we are out of range for the USART, thrown an error
	// need to throw error if we have an odd number divider
	
	mTWIM[numTWIM].regTWIM = TWIM_Instances[numTWIM];	// Push a base address of the TWIM register structure.
	mTWIM[numTWIM].numTWIM = numTWIM;					// Use this for TWIM write read commands externally
	
	// Clear out the commands
	TWIM_clearCMD(numTWIM, 0);
	TWIM_clearCMD(numTWIM, 1);
	
	// Set PM structure, perhaps if we divide here we should look into this part more
	mTWIM[numTWIM].state = TWIM_STATE_INITIALIZING_CLOCK;
	uint8_t pmOffset;
	switch (numTWIM)
	{
		case 0: pmOffset = PM_PBAMASK_TWIM0_Pos; break;
		case 1: pmOffset = PM_PBAMASK_TWIM1_Pos; break;
		case 2: pmOffset = PM_PBAMASK_TWIM2_Pos; break;
		case 3: pmOffset = PM_PBAMASK_TWIM3_Pos; break;
		default: break;
	}
	
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBAMASK_OFFSET);	// Unlock PBA Register
	PM->bf.PBAMASK.reg |= (uint32_t)(0x1ul << pmOffset);	// Unlock TWIM
	
	// Push the pins into mUSPI and set peripherals
	mTWIM[numTWIM].pin.TWD	= *TWD;
	mTWIM[numTWIM].pin.TWCK	= *TWCK;
	
	mTWIM[numTWIM].state = TWIM_STATE_SETTING_PERIPHERALS;
	GPIO_setPeriphPin(TWD);
	GPIO_setPeriphPin(TWCK);
	
	mTWIM[numTWIM].bFlags.pinsInit = 0x01;
	
	// SOMETHING ABOUT SETTING THESE TO OPEN DRAIN? SEE SECTION 27.7.1
	mTWIM[numTWIM].mode = modeTWIM;
	
	// Enable the master mode and reset
	mTWIM[numTWIM].regTWIM->bf.CR.reg = TWIM_CR_MEN;
	mTWIM[numTWIM].regTWIM->bf.CR.reg = TWIM_CR_SWRST;
	mTWIM[numTWIM].regTWIM->bf.SCR.reg = TWIM_SCR_MASK;	// reset all status fields
	
	// The purpose of the loop below is to take the PM.TWIM_CLK & desired output clock (based on
	// I2C mode) and determine a value for EXP and fPrescaled that fit within the register field sizes
	uint8_t exp = 0;
	uint32_t fPrescaled = (fMainClk/(uint32_t)modeTWIM)/(2+exp);	
	while (fPrescaled > UINT8_MAX)
	{
		exp++;	// tick exp
		fPrescaled = (fMainClk/modeTWIM)/(2+exp); // re-evaluate fPrescaled
	}
	
	if (exp > 0x7) return TWIM_ERROR_UNKNOWN; // ensure not overflowing [CWGR.EXP]
	
	// Setup characteristics and filters
	if ((modeTWIM == TWIM_MODE_STANDARD) | (modeTWIM == TWIM_MODE_FAST) | (modeTWIM == TWIM_MODE_FASTPLUS))
	{
		mTWIM[numTWIM].regTWIM->bf.CWGR.reg =
		TWIM_CWGR_LOW((uint8_t)(fPrescaled/2))
		| TWIM_CWGR_HIGH((uint8_t)(fPrescaled/2))
		| TWIM_CWGR_STASTO((uint8_t)fPrescaled)
		| TWIM_CWGR_DATA(0)
		| TWIM_CWGR_EXP(exp);
		
		// Set slew rate register --- not complete, only setting filter for now
		if (modeTWIM == TWIM_MODE_FASTPLUS)
		{
			mTWIM[numTWIM].regTWIM->bf.SRR.reg = TWIM_SRR_FILTER(0x3);
		}
		else
		{
			mTWIM[numTWIM].regTWIM->bf.SRR.reg = TWIM_SRR_FILTER(0x2);
		}
		
	}
	
	if (modeTWIM == TWIM_MODE_HIGHSPEED)
	{
		mTWIM[numTWIM].regTWIM->bf.HSCWGR.reg =
		TWIM_HSCWGR_LOW((uint8_t)(fPrescaled/2))
		| TWIM_HSCWGR_HIGH((uint8_t)(fPrescaled/2))
		| TWIM_HSCWGR_STASTO((uint8_t)fPrescaled)
		| TWIM_HSCWGR_DATA(0)
		| TWIM_HSCWGR_EXP(exp);
		
		// Set HS slew rate register --- not complete, only setting filter for now
		mTWIM[numTWIM].regTWIM->bf.HSSRR.reg = TWIM_SRR_FILTER(0x1);
		
	}
	
	mTWIM[numTWIM].bFlags.devInit = 0x01;
	mTWIM[numTWIM].state = TWIM_STATE_IDLE;
	
	//// need to use divided clock first for setup.
	//mUSPI[numUSART].regUSART->bf.BRGR.reg = US_BRGR_CD(USPI_MIN_BAUDRATE_DIVIDER_DATASHEET);
	//mUSPI[numUSART].mainClkUSART = fMainClk / USPI_MIN_BAUDRATE_DIVIDER_DATASHEET;
	//
	//mUSPI[numUSART].regUSART->bf.CR.reg = US_CR_SPI_MASTER_RSTRX | US_CR_SPI_MASTER_RSTTX;	// Reset the transmitter and receiver
	//// Set the USART properties
	////phUSPI->USART_reg->bf.MR.reg = US_MR_SPI_MODE_SPI_MASTER; // This also automatically disables parity
	//
	//mUSPI[numUSART].bFlags.devInit = 0x01;
	//mUSPI[numUSART].state = USPI_STATE_IDLE;
	
	return TWIM_SUCCESS;
}

TWIM_ERROR_TYPE TWIM_setCMDR(uint8_t numTWIM, uint8_t slaveAddress, bool bRead, uint8_t* pData, uint8_t numBytes, bool bStart, bool bStop, bool bACKLAST)
{
	// error checks:
	// check to make sure that the TWIM we're using is initialized
	
	// This function is used to set CMDR
	uint32_t reg = TWIM_CMDR_NBYTES(numBytes) | TWIM_CMDR_SADR(slaveAddress) | TWIM_CMDR_VALID;
	
	if (bStart) reg |= TWIM_CMDR_START;
	if (bStop) reg |= TWIM_CMDR_STOP;
	if (bRead) reg |= TWIM_CMDR_READ;
	if (bACKLAST) reg |= TWIM_CMDR_ACKLAST;
	
	// set the CMD object
	mTWIM[numTWIM].CMD[1].slaveAddress = slaveAddress;
	mTWIM[numTWIM].CMD[0].pData = pData;
	mTWIM[numTWIM].CMD[0].numBytes = numBytes;
	mTWIM[numTWIM].CMD[0].bRead = bRead;
	mTWIM[numTWIM].CMD[0].bReady = 1;
	
	// Push values to the register
	mTWIM[numTWIM].regTWIM->bf.CMDR.reg = reg;
	
	return TWIM_SUCCESS;
}

TWIM_ERROR_TYPE TWIM_setNCMDR(uint8_t numTWIM, uint8_t slaveAddress, bool bRead, uint8_t* pData, uint8_t numBytes, bool bStart, bool bStop, bool bACKLAST)
{
	// error checks:
	// check to make sure that the TWIM we're using is initialized
	
	// This function is used to set CMDR
	uint32_t reg = TWIM_NCMDR_NBYTES(numBytes) | TWIM_NCMDR_SADR(slaveAddress) | TWIM_NCMDR_VALID;
	
	if (bStart) reg |= TWIM_NCMDR_START;
	if (bStop) reg |= TWIM_NCMDR_STOP;
	if (bRead) reg |= TWIM_NCMDR_READ;
	if (bACKLAST) reg |= TWIM_NCMDR_ACKLAST;
	
	// set the CMD object
	mTWIM[numTWIM].CMD[1].slaveAddress = slaveAddress;
	mTWIM[numTWIM].CMD[1].pData = pData;
	mTWIM[numTWIM].CMD[1].numBytes = numBytes;
	mTWIM[numTWIM].CMD[1].bRead = bRead;
	mTWIM[numTWIM].CMD[1].bReady = 1;
	
	// Push values to the register
	mTWIM[numTWIM].regTWIM->bf.NCMDR.reg = reg;
	
	return TWIM_SUCCESS;
}

TWIM_ERROR_TYPE TWIM_runCMDs(uint8_t numTWIM)
{
	// This will need to deal with error conditions (what happens if we die in between CMDR & NCMDR
	
	mTWIM[numTWIM].regTWIM->bf.CR.reg = TWIM_CR_MEN;
	
	// if the first command is loaded
	if (mTWIM[numTWIM].CMD[0].bReady)
	{
		// keep pushing data until done?
		if (!mTWIM[numTWIM].CMD[0].bRead) // if Writing
		{
			// Cycle through the count of data and pull it all
			for (uint8_t i = 0; i < mTWIM[numTWIM].CMD[0].numBytes; i++)
			{
				// block until ready to TX
				while (!(mTWIM[numTWIM].regTWIM->bf.SR.reg & TWIM_SR_TXRDY));
				mTWIM[numTWIM].regTWIM->bf.THR.reg = mTWIM[numTWIM].CMD[0].pData[i];
			}
		}
		else // we're reading
		{
			for (uint8_t i = 0; i < mTWIM[numTWIM].CMD[0].numBytes; i++)
			{
				// block until ready to RX
				while (!(mTWIM[numTWIM].regTWIM->bf.SR.reg & TWIM_SR_RXRDY));
				mTWIM[numTWIM].CMD[0].pData[i] = (uint8_t)mTWIM[numTWIM].regTWIM->bf.RHR.reg;
			}
		}
		
		TWIM_clearCMD(numTWIM, 0); // Clear out the CMD struct if we got here, get ready for the next one
	}
	
	// after the first command is sent, check to make sure it was successful and then nuke the contents of the transmission to get ready for the next reload
	
	
	// if the next command is loaded
	if (mTWIM[numTWIM].CMD[1].bReady)
	{
		// repeat again for NMCDR info.
		// keep pushing data until done?
		if (!mTWIM[numTWIM].CMD[1].bRead) // if Writing
		{
			// Cycle through the count of data and pull it all
			for (uint8_t i = 0; i < mTWIM[numTWIM].CMD[1].numBytes; i++)
			{
				// block until ready to TX
				while (!(mTWIM[numTWIM].regTWIM->bf.SR.reg & TWIM_SR_TXRDY));
				mTWIM[numTWIM].regTWIM->bf.THR.reg = mTWIM[numTWIM].CMD[1].pData[i];
			}
		}
		else // we're reading
		{
			for (uint8_t i = 0; i < mTWIM[numTWIM].CMD[1].numBytes; i++)
			{
				// block until ready to RX
				while (!(mTWIM[numTWIM].regTWIM->bf.SR.reg & TWIM_SR_RXRDY));
				mTWIM[numTWIM].CMD[1].pData[i] = (uint8_t)mTWIM[numTWIM].regTWIM->bf.RHR.reg;
			}
		}
		
		TWIM_clearCMD(numTWIM, 1); // Clear out the CMD struct if we got here, get ready for the next one
	}
	
	
	// Wait until the bus is free & and the peripheral is ready to handle more commands
	while ( (mTWIM[numTWIM].regTWIM->bf.SR.reg & (TWIM_SR_CRDY | TWIM_SR_BUSFREE)) != (TWIM_SR_CRDY | TWIM_SR_BUSFREE));
		
	// Conversation over, clear [SCR] to get ready for next one
	mTWIM[numTWIM].regTWIM->bf.SCR.reg = TWIM_SCR_MASK;
	
	mTWIM[numTWIM].regTWIM->bf.CR.reg = TWIM_CR_MDIS;
	
	
	return TWIM_SUCCESS;
}

TWIM_ERROR_TYPE TWIM_clearCMD(uint8_t numTWIM, uint8_t numCMD)
{
	mTWIM[numTWIM].CMD[numCMD].bReady = false;
	mTWIM[numTWIM].CMD[numCMD].pData = NULL;
	mTWIM[numTWIM].CMD[numCMD].numBytes = 0;
	mTWIM[numTWIM].CMD[numCMD].bRead = false;
	mTWIM[numTWIM].CMD[numCMD].slaveAddress = 0;
		
	return TWIM_SUCCESS;
}