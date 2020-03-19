#include "AK_utils.h"
#include "delay_SAM4L.h"

uint32_t clock_speed(){
	switch((uint8_t)PM->bf.MCCTRL.bit.MCSEL) {
		case 0 :
			//System RC oscillator (RCSYS)
			return(115000);
		case 1 :
			//Oscillator0 (OSC0)
			//not implemented
			throw_error();
			return(0);
		case 4 :
			//80MHz RC oscillator (RC80M)()
			return(80000000);
		case 5 :
			//4/8/12 MHz RC oscillator (RCFAST)
			if (SCIF->bf.RCFASTCFG.bit.EN == 1){
				switch((uint8_t)SCIF->bf.RCFASTCFG.bit.FRANGE){
					case 0 : return(4000000);
					case 1 : return(8000000);
					case 2 : return(12000000);
				}
			}
			else{
				//rcfast selected but not turned on
				throw_error();
				return(0);
			}
		case 6 :
			//1 MHz RC oscillator (RC1M)
			return(1000000);
		default :
			throw_error();
			return(0);
	};
}
	
void delay_ms_2(uint64_t ms){
	//Alex's delay function combined with m clock speed function to support multiple clock speeds
	delay_cycles(((uint64_t)(ms) * clock_speed() + (uint64_t)(14e3 - 1ul)) / (uint64_t)14e3);
}


void delay_us_2(uint64_t us){
	//Alex's delay function combined with m clock speed function to support multiple clock speeds
	delay_cycles(((uint64_t)(us) * clock_speed() + (uint64_t)(14e6 - 1ul)) / (uint64_t)14e6);
}

void Assert(bool expr){
		if (!(expr)) throw_error();
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
		delay_ms_2(1000);
		//delay_2(5000);
		//turn off
		GPIO->bf.Port[2].bf.OVRS.reg = GPIO_OVRS_P7;
		delay_ms_2(1000);
		//delay_2(5000);
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

void blink_LED(uint64_t duration){
	//blinks LED once
	//enable GPIO PC7
	GPIO->bf.Port[2].bf.GPERS.reg = 0x00000080;
	//set PC7 as output
	GPIO->bf.Port[2].bf.ODERS.reg = GPIO_ODER_P7;
	//turn on
	GPIO->bf.Port[2].bf.OVRC.reg = GPIO_OVRC_P7;
	delay_ms_2(duration);
	//turn off
	GPIO->bf.Port[2].bf.OVRS.reg = GPIO_OVRS_P7;
	delay_ms_2(duration);
};