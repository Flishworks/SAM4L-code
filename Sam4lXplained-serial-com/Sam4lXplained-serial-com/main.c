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
	test_com();

}

void test_com(){
	//endless stream of numbers to test a com port connection
	char data[] = "Testing. Check 1 2. Is this thing on?";
	USART_print(data, sizeof(data)/sizeof(data[1]));
	uint32_t x = 0;
    while (1) 
    {
		USART_newline();
		USART_write_char((x++%10)+48);
		delay_ms_2(500);
    }
}
