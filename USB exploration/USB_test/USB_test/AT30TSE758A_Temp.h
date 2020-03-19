#ifndef _AT30TSE758A_Temp_H_	// Guards against double calling of headers
#define _AT30TSE758A_Temp_H_

//prototypes
void TWI_init();
float get_temp_12bit();
void I2C_write_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );
void I2C_read_bytes(uint8_t address, uint8_t* data, uint8_t num_bytes );
void set_config_register(uint8_t config_val);

//SDA - TWIM3 TWD - Pin PB14 Function C
uint32_t SDA = GPIO_PB14;

//SCL - TWIM3 TWCK - Pin PB15  Function C
uint32_t SCL = GPIO_PB15;

uint8_t I2C_Address = 0b1001111; //0b1001000;

#endif