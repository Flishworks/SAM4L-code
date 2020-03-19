#ifndef _AK_SD_H_	// Guards against double calling of headers
#define _AK_SD_H_

#include <stddef.h>
#include "sam.h"
#include "PM_SAM4L.h"
#include "delay_SAM4L.h"

uint32_t SD_transmit_32(uint32_t data_out);
uint8_t SD_send_byte(uint8_t byte_to_send);
uint8_t SD_send_command_R1(uint8_t cmd, uint32_t argument, uint8_t crc);
uint32_t SD_send_command_R3(uint8_t cmd, uint32_t argument, uint8_t crc);
uint8_t SD_send_command_cmd_0(void);
void SD_init();
void SPI_init();

#endif

