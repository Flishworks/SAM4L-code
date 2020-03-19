/*
 * I2C temp sensor using Alex lib.c
 *
 * Created: 7/24/2019 12:19:45 PM
 * Author : NUC11
 */ 

#include "sam.h"
#include "PM_SAM4L.h"
#include "delay_SAM4L.h"
#include "TWIM_SAM4L.h"
//prototypes
void RCFAST_init();
uint16_t get2ByteTemp();
uint8_t getTemp();


uint8_t I2C_Address =  0b1001111; //0b1001000;
Pin LED1 = {GPIO_PC07, PORTC_NUM, 0}; //set as general I/O
	
//SDA - TWIMS0 TWD - Pin PA23/GPIO 23 Function B 
//Pin SDA = {GPIO_PA23, PORTA_NUM, 1}; //busted SDA pin
Pin SDA =	{GPIO_PB14, PORTB_NUM, 2};

//SCL - TWIMS0 TWCK - Pin PA24/GPIO 24 Function B
//Pin SCL = {GPIO_PA24, PORTA_NUM, 1}; 
Pin SCL = 	{GPIO_PB15, PORTB_NUM, 2};

Pin USART = {GPIO_PC27, PORTC_NUM, 0};

uint8_t TWIM_num = 3;

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	PM_init();
	//RCFAST_init();
	 
	wiggle_pin(&USART);
	//wiggle_pin(&SDA);

	
	//try I2C
	volatile uint8_t data = 0b10111000; //data to send
	//initiate TWIM
	TWIM_init(TWIM_num, &SDA, &SCL, TWIM_MODE_STANDARD);
	getTemp();
	
    while (1) 
    {
		flashLEDms(&LED1, 100, 100); //Something weird with delay code
	}
}


uint8_t getTemp(){
	//Read 2 byte temp
	TWIM_setCMDR(TWIM_num, I2C_Address, 1, 0x0, 1, 1, 1, 0); 
	//send command
	TWIM_runCMDs(TWIM_num);
	//mTWIM[TWIM_num].CMD[0].pData;
	return ;
	};
	
uint16_t get2ByteTemp(){
	//Read 2 byte temp
	TWIM_setCMDR(TWIM_num, I2C_Address, 1, 0x0, 1, 1, 1, 1); 
	TWIM_setNCMDR(TWIM_num, I2C_Address, 1, 0x0, 1, 1, 1, 0);
	//send command
	TWIM_runCMDs(TWIM_num);
	//mTWIM[TWIM_num].CMD[0].pData;
	return ;
	};
	
void wiggle_pin(Pin* pin){
		//used to test if pins even work
		GPIO_clearPeriphPin(pin);
		GPIO_forceLow(pin); 
		delay_ms(10);
		GPIO_forceHigh(pin);
		delay_ms(10);
		GPIO_forceLow(pin);
		delay_ms(10);
		GPIO_forceHigh(pin);
		delay_ms(10);
	};


void RCFAST_init(){
	//Enable SCIF, which controls oscillators/clocks.
	SCIF->bf.UNLOCK.reg = 0xAA000000 | SCIF_RCFASTCFG_OFFSET; //unlock RCFASTCFG
	SCIF->bf.RCFASTCFG.bit.EN = 1; // Enable RCFAST
	while (SCIF->bf.RCFASTCFG.bit.EN != 1); // wait for it to turn on
	//set RCFAST as system clock
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_MCCTRL_OFFSET;
	PM->bf.MCCTRL.reg = 0x00000005;
}
