/*
 * Sam4lXplained-serial-com.c
 *
 * Created: 8/12/2019 2:59:14 PM
 * Author : NUC11
 */ 


#include "sam.h"
#include "AK_USART.h"
#include "AK_utils.h"

//EDGB connected to USART1
//PC26 = Rx
//PC27 = Tx

uint32_t Rx = GPIO_PC26;
uint32_t Tx = GPIO_PC27;

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	RCFAST_init();
	USART_init(Tx, Rx);
	func_timer();
	
	throw_error();
}

void func_timer(){
	//used in combo with py script to measure timing 
	uint16_t num_loops = 1000;
	
	USART_write_char(77); //anything to start the count
	for(uint16_t i=0; i<num_loops; i++){
		delay_ms_2(100);
	}
	USART_write_char(77);
}
