#ifndef _AK_I2C_	// Guards against double calling of headers
#define _AK_I2C_

#include <stddef.h>
#include "sam.h"
#include "PM_SAM4L.h"

//prototypes
void TWI_init();
void I2C_write_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );
void I2C_read_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );

#endif