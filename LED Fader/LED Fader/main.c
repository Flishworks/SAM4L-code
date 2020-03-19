/*
 * LED Fader.c
 *
 * Created: 6/24/2019 11:16:35 AM
 * Author : NUC11
 */ 


#include "sam.h"
#include "delay_SAM4L.h"
#include "PM_SAM4L.h"
#include <math.h>
uint32_t t;

void delay_ms_2(uint32_t delay_time);

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	PM_init();
	
	//SCIF controls oscilators/clocks.
	SCIF->bf.UNLOCK.reg = 0xAA000000 | SCIF_RCFASTCFG_OFFSET; //unlock RCFASTCFG
	SCIF->bf.RCFASTCFG.bit.EN = 1; // Enable RCFAST
	while (SCIF->bf.RCFASTCFG.bit.EN != 1)
	{
		__NOP();
	}
		
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_MCCTRL_OFFSET;
	PM->bf.MCCTRL.reg = 0x00000005; 
	
	//enable GPIO PC7
	GPIO->bf.Port[2].bf.GPERS.reg = 0x00000080;
	//GPIO->bf.Port[2].GPERS.reg = 0b00000000000000000000000010000000;
	//GPIO->bf.Port[2].GPERS.reg = GPIO_GPER_P7;
	//GPIO->bf.Port[2].GPERS.bit.P7 = 1;
	
	//set PC7 as output
	GPIO->bf.Port[2].bf.ODERS.reg = GPIO_ODER_P7;
	
	
	volatile float duty = 0;
	uint32_t fader_period = 500000; //time for one complete LED fade
	uint32_t pwm_period = 500; //time for one complete PWM cycle. As a result, this also controlls resolution (higher number, greater fade resolution). too high and it will look blinky.
	volatile uint32_t delay_time = 0;
	t = 1;
	
    /* Replace with your application code */
    while (1) 
    {
		t += 1;
		duty = (float)((sin(((float)t/(float)fader_period))+1)/2);
		if (duty>1 || duty<0){ //oh no, something went wrong in the sine function!
			break;
		}
		
		//turn on
		delay_time = duty*pwm_period;
		GPIO->bf.Port[2].bf.OVRC.reg = GPIO_OVRC_P7;
		delay_ms_2(delay_time);

		//turn off
		delay_time = pwm_period*(float)(1.0-duty);
		GPIO->bf.Port[2].bf.OVRS.reg = GPIO_OVRS_P7;
		delay_ms_2(delay_time);
    }
}


void delay_ms_2(volatile uint32_t delay_time) //delays by t program loops.
{
	uint32_t then = t;
	while (t < then + delay_time){
		t += 1; //__NOP();
	}
}




