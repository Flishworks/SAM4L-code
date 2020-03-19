#include <stddef.h>
#include <stdint.h>
#include "delay_SAM4L.h"

int s;
__attribute__((optimize(s)))
__attribute__ ((section(".ramfunc")))
// 
void delay_cycles(uint32_t n)
{
	(void)(n);		//Void Cast

//#ifdef _SAM4L_		// SAM4L Series delay function
	__asm (
	"loop: DMB	\n"
	"SUBS R0, R0, #1  \n"
	"BNE.N loop         "
	);
//#elif _SAML21_	//SAML21 Series delay function
	//__asm (
	//"loop: DMB    \n"
	//"SUB r0, r0, #1 \n"
	//"CMP r0, #0  \n"
	//"BNE loop         "
	//);
//#else
	//// Wrong chip.
//#endif
}