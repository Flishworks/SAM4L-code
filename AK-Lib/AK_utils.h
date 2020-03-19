#ifndef _AK_utils_H_	// Guards against double calling of headers
#define _AK_utils_H_

#include <stddef.h>
#include "sam.h"
#include "delay_SAM4L.h"
#include <stdbool.h>


void delay_2(uint32_t delay_time);
void delay_ms_2(uint64_t ms);
void delay_us_2(uint64_t ms);
void throw_error();
void RCFAST_init();
void Assert(bool expr);
uint32_t clock_speed();

#endif