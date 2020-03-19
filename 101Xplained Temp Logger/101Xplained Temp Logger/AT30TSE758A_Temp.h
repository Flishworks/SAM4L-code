#ifndef _AT30TSE758A_Temp_H_	// Guards against double calling of headers
#define _AT30TSE758A_Temp_H_

#include <stddef.h>
#include "sam.h"
#include "PM_SAM4L.h"

//prototypes
uint16_t get_temp_12bit(uint8_t address);
void set_config_register(uint8_t address, uint8_t config_val);
float temp_to_float(uint16_t raw_temp);
#endif