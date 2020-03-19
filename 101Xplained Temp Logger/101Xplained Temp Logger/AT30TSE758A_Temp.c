//AT30TSE758A_Temp.c
//functions for reading temperature from AT30TSE758A over I2C
//A. Kazen
//08-2019

#include "AT30TSE758A_Temp.h"
#include "AK_I2C.h"

uint16_t get_temp_12bit(uint8_t address){
	uint8_t data_read [2];
	I2C_read_bytes(address, data_read, sizeof(data_read));
	uint16_t raw_temp = data_read[0] << 4 | data_read[1] >> 4; //ignore bottom 4 bits
	return raw_temp;
}

void set_config_register(uint8_t address, uint8_t config_val){
	uint8_t data_to_send[2];// = {0x01, config_val};
	data_to_send[0] = 0x01; //pointer to configuration Register
	data_to_send[1] = config_val; 
	I2C_write_bytes(address, data_to_send, sizeof(data_to_send)); //data to send is already pointer (because it is an array), so no & needed
	
	//now set pointer back to temp register
	uint8_t pointer = 0x00; //pointer to temp register
	I2C_write_bytes(address, &pointer, 1);
}

float temp_to_float(uint16_t raw_temp){
	return (float)raw_temp*.0625;
}
