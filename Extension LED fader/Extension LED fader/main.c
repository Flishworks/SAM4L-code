/*
 * Extension LED fader.c
 *
 * Created: 6/27/2019 1:49:42 PM
 * Author : NUC11
 */ 

#include "sam.h"
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
	
	//set RCFAST as system clock
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_MCCTRL_OFFSET;
	PM->bf.MCCTRL.reg = 0x00000005; 
	
	//enable GPIO PA08
	GPIO->bf.Port[0].bf.GPERS.reg = 0x00000100;
	//GPIO->bf.Port[2].GPERS.reg = 0b00000000000000000000000100000000;
	//GPIO->bf.Port[2].GPERS.reg = GPIO_GPER_P8;
	//GPIO->bf.Port[2].GPERS.bit.P8 = 1;
	
	//set PA08 as output
	GPIO->bf.Port[0].bf.ODERS.reg = GPIO_ODER_P8;
	
	
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
		GPIO->bf.Port[0].bf.OVRC.reg = GPIO_OVRC_P8;
		delay_ms_2(delay_time);

		//turn off
		delay_time = pwm_period*(float)(1.0-duty);
		GPIO->bf.Port[0].bf.OVRS.reg = GPIO_OVRS_P8;
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




