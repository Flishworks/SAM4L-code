/*
 * I2C-temp.c
 *
 * Created: 7/18/2019 3:22:16 PM
 * Author : NUC11
 */ 


#include "sam.h"
#include "PM_SAM4L.h"
#include "delay_SAM4L.h"
#include <stdbool.h>

//prototypes
void TWI_init();
void delay_2(volatile uint32_t delay_time);
void throw_error();
void RCFAST_init();
uint8_t get_temp();
uint16_t get_temp_2_byte();

float get_temp_12bit();
void I2C_write_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );
void I2C_read_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );
void set_config_register(uint8_t config_val);

//SDA - TWIM3 TWD - Pin PB14 Function C
uint32_t SDA = GPIO_PB14;

//SCL - TWIM3 TWCK - Pin PB15  Function C
uint32_t SCL = GPIO_PB15;

uint8_t I2C_Address = 0b1001111; //0b1001000;
  
int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	PM_init();
	RCFAST_init();
	TWI_init();
	
	//temp sensor
	uint8_t config = 0b11 << 5; //conversion resolution to m12 bits in config register
	set_config_register(config);
	
	volatile uint16_t delay = 1; //milliseconds of delay. Will be set dynamically to maximum speed possible
	volatile float 
	
	
	 = get_temp_12bit();
	delay_ms(delay);
	
    while (1) 
    {
		temp = get_temp_12bit();
		delay_ms(delay);
		if (temp == 0) { //readings were too fast
			//delay++;
		}
	}
}

float get_temp_12bit(){
	uint8_t data_read [2];
	I2C_read_bytes(I2C_Address, data_read, sizeof(data_read));
	uint16_t raw_temp = data_read[0] << 4 | data_read[1] >> 4; //ignore bottom 4 bits
	float temp = raw_temp*.0625;
	return temp;
}

void set_config_register(uint8_t config_val){
	uint8_t data_to_send[2];// = {0x01, config_val};
	data_to_send[0] = 0x01; //pointer to configuration Register
	data_to_send[1] = config_val; 
	I2C_write_bytes(I2C_Address, data_to_send, sizeof(data_to_send)); //data to send is already pointer (because it is an array), so no & needed
	
	//now set pointer back to temp register
	uint8_t pointer = 0x00; //pointer to temp register
	I2C_write_bytes(I2C_Address, &pointer, 1);
}

void I2C_read_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes){
	//set up CMDR
	uint32_t CMDR_reg = TWIM_CMDR_SADR(I2C_Address) | TWIM_CMDR_NBYTES(num_bytes);
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
	uint32_t CMDR_reg = TWIM_CMDR_SADR(I2C_Address) | TWIM_CMDR_NBYTES(num_bytes);
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

void TWI_init(){
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

void delay_2(volatile uint32_t delay_time) //delays by delay_time program loops.
{
	uint32_t t = 0;
	while (t < delay_time){
		t ++; //__NOP(); 
	}
}

void throw_error(){
	//blinks to indicate and issue
	//enable GPIO PC7
	GPIO->bf.Port[2].bf.GPERS.reg = 0x00000080;
	//set PC7 as output
	GPIO->bf.Port[2].bf.ODERS.reg = GPIO_ODER_P7;
	while (1)
	{
		//turn on
		GPIO->bf.Port[2].bf.OVRC.reg = GPIO_OVRC_P7;
		//delay_us(100000);
		delay_2(5000);
		//turn off
		GPIO->bf.Port[2].bf.OVRS.reg = GPIO_OVRS_P7;
		//delay_us(100000);
		delay_2(5000);
	}
}

void RCFAST_init(){
	//Enable SCIF, which controls oscillators/clocks.
	SCIF->bf.UNLOCK.reg = 0xAA000000 | SCIF_RCFASTCFG_OFFSET; //unlock RCFASTCFG
	SCIF->bf.RCFASTCFG.bit.EN = 1; // Enable RCFAST
	while (SCIF->bf.RCFASTCFG.bit.EN != 1); // wait for it to turn on
	//set RCFAST as system clock
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_MCCTRL_OFFSET;
	PM->bf.MCCTRL.reg = 0x00000005;
}


//old
uint8_t get_temp(){
	//set up CMDR
	//TWIM3 -> bf.SCR.reg = TWIM_SCR_MASK;	// reset all status fields
	uint32_t CMDR_reg = TWIM_CMDR_SADR(I2C_Address) | TWIM_CMDR_NBYTES(1);
	CMDR_reg |= TWIM_CMDR_VALID; //son of a bitch, this has to be set manually!
	CMDR_reg |= TWIM_CMDR_READ;
	CMDR_reg |= TWIM_CMDR_START;
	CMDR_reg |= TWIM_CMDR_STOP;
	TWIM3 -> bf.CMDR.reg = CMDR_reg;
	TWIM3 -> bf.CR.reg = TWIM_CR_MEN; //this will trigger the send
	
	// block until ready to RX
	while (TWIM3->bf.SR.bit.RXRDY == 0);
	return TWIM3->bf.RHR.reg;
}

uint16_t get_temp_2_byte(){
	//set up CMDR
	TWIM3 -> bf.SCR.reg = TWIM_SCR_MASK;	// reset all status fields
	uint32_t CMDR_reg = TWIM_CMDR_SADR(I2C_Address) | TWIM_CMDR_NBYTES(2);
	CMDR_reg |= TWIM_CMDR_VALID;
	CMDR_reg |= TWIM_CMDR_READ;
	CMDR_reg |= TWIM_CMDR_START;
	CMDR_reg |= TWIM_CMDR_STOP;
	TWIM3 -> bf.CMDR.reg = CMDR_reg;
	TWIM3 -> bf.CR.reg = TWIM_CR_MEN; //this will trigger the send
	
	// block until ready to RX
	while (TWIM3->bf.SR.bit.RXRDY == 0);
	volatile uint16_t temp = TWIM3->bf.RHR.reg;
	//TWIM3->bf.SR.bit.RXRDY = 0; //reset RCRDY flag. Is this necessary?
	while (TWIM3->bf.SR.bit.RXRDY == 0);
	temp = temp << 8 | TWIM3->bf.RHR.reg;
	return temp;
}
