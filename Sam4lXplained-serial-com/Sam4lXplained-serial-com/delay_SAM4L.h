#ifndef _DELAY_SAM4L_H_	// Guards against double calling of headers
#define _DELAY_SAM4L_H_

#include <stddef.h>
#include <stdint.h>
#include "PM_SAM4L.h"	// fMainClk stored here

#define delay_ms(ms)	delay_cycles(((uint64_t)(ms) * (fMainClk) + (uint64_t)(14e3 - 1ul)) / (uint64_t)14e3)
#define delay_us(us)	delay_cycles(((uint64_t)(us) * (fMainClk) + (uint64_t)(14e6 - 1ul)) / (uint64_t)14e6)

// Call delay function
void delay_cycles(uint32_t n);


#endif
