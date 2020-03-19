#include "AK_I2C.h"

void I2C_read_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes){
	//set up CMDR
	uint32_t CMDR_reg = TWIM_CMDR_SADR(address) | TWIM_CMDR_NBYTES(num_bytes);
	CMDR_reg |= TWIM_CMDR_READ;
	CMDR_reg |= TWIM_CMDR_VALID;
	CMDR_reg |= TWIM_CMDR_START;
	CMDR_reg |= TWIM_CMDR_STOP;
	//while (TWIM3->bf.SR.bit.CRDY == 0); // block until ready for command
	TWIM3->bf.SCR.reg = TWIM_SCR_CCOMP; //reset command complete flag
	TWIM3 -> bf.CMDR.reg = CMDR_reg; //this will trigger the read
	//TWIM3 -> bf.CR.reg = TWIM_CR_MEN;
	for (uint8_t byte=0; byte < num_bytes; byte++){
		while (TWIM3->bf.SR.bit.RXRDY == 0); // block until ready to RX
		//while (is_I2C_free() == 0); // block until ready
		*data = TWIM3->bf.RHR.reg; //load byte
		data++;
	}
	while(!TWIM3->bf.SR.bit.CCOMP); // block until command complete
}

void I2C_write_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes ){
	//set up CMDR
	uint32_t CMDR_reg = TWIM_CMDR_SADR(address) | TWIM_CMDR_NBYTES(num_bytes);
	CMDR_reg |= TWIM_CMDR_VALID;
	CMDR_reg |= TWIM_CMDR_START;
	CMDR_reg |= TWIM_CMDR_STOP;
	while (TWIM3->bf.SR.bit.CRDY == 0); // block until ready for command
	TWIM3->bf.SCR.reg = TWIM_SCR_CCOMP; //reset command complete flag
	TWIM3 -> bf.CMDR.reg = CMDR_reg; //this will trigger the send
	//TWIM3 -> bf.CR.reg = TWIM_CR_MEN; 
	for (uint8_t byte=0; byte < num_bytes; byte++){
		while (!(TWIM3->bf.SR.reg & TWIM_SR_TXRDY));
		//load data to be transmitted into THR
		TWIM3 -> bf.THR.reg = *data;
		data++;
	}
	while(!TWIM3->bf.SR.bit.CCOMP); //// block until command complete
}

void TWI_init(SCL, SDA){
	//disable GPIO on TWIM pins
	GPIO->bf.Port[1].bf.GPERC.reg = SCL | SDA; //GPIO_PA23 | GPIO_PA24;
	//set GPIO muxing to TWIM (function C)
	GPIO->bf.Port[1].bf.PMR0C.reg = SCL | SDA; //GPIO_PA23 | GPIO_PA24;
	GPIO->bf.Port[1].bf.PMR1S.reg = SCL | SDA; //GPIO_PA23 | GPIO_PA24;
	GPIO->bf.Port[1].bf.PMR2C.reg = SCL | SDA; // GPIO_PA23 | GPIO_PA24;
	
	//Set up TWIM3 in PM (PBA Mask)
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBAMASK_OFFSET);	// Unlock PBA Register
	PM->bf.PBAMASK.reg |= (uint32_t)(0x1ul << PM_PBAMASK_TWIM3_Pos);	// Enable clock for TWIM
	
	//set up TWI
	TWIM3->bf.CR.reg = TWIM_CR_MDIS;
	TWIM3->bf.CR.reg = TWIM_CR_SWRST;
	TWIM3->bf.SCR.reg = TWIM_SCR_MASK;	// reset all status fields
	TWIM3->bf.CR.reg = TWIM_CR_MEN;
	
	//set clock stuff.. this is all pretty much just a guess
	TWIM3 -> bf.CWGR.bit.EXP = 2;
	TWIM3 -> bf.CWGR.bit.LOW = 1;
	TWIM3 -> bf.CWGR.bit.HIGH = 1;
	TWIM3 -> bf.CWGR.bit.STASTO = 1;
	TWIM3 -> bf.CWGR.bit.DATA = 1;
}