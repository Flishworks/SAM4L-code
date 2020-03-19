/*
 * 101Xplained Temp Logger.c
 *
 * Created: 8/6/2019 9:31:47 AM
 * Author : AKazen
 */ 

#define _debug_
#ifdef _debug_
#include "AK_USART.h"
#endif

#include "sam.h"
#include "PM_SAM4L.h"
#include "delay_SAM4L.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "AK_utils.h"
#include "AT30TSE758A_Temp.h"
#include "AK_SD.h"
#include "AK_I2C.h"
#include "fat_filelib.h"

//SDA - TWIM3 TWD - Pin PB14 Function C
uint32_t SDA = GPIO_PB14;
//SCL - TWIM3 TWCK - Pin PB15  Function C
uint32_t SCL = GPIO_PB15;
uint8_t I2C_Address = 0b1001111; //0b1001000;

int media_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count);
int media_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count);

int main(void){
//****INIT****//
	SystemInit();
	PM_init();
	RCFAST_init();

#ifdef _debug_
	//usart init for readout
	uint32_t Rx = GPIO_PC26;
	uint32_t Tx = GPIO_PC27;
	USART_init(Tx, Rx);
	USART_newline();
	USART_printf("System initialization...");
	USART_newline();
#endif

	TWI_init(SCL, SDA);
	SPI_init();
	SD_init();
	
	//fat filesystem init
	FL_FILE *file;
	fl_init();
	if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
	{
		#ifdef _debug_
		USART_printf("ERROR: Media attach failed");
		USART_newline();
		#endif
		throw_error();
	}
	//temp sensor init
	uint8_t config = 0b11 << 5; //conversion resolution to 12 bits in config register
	set_config_register(I2C_Address, config);

//****LOG DATA****//
	while(1){
		file = fl_fopen("/temp_log.csv", "a"); //open log file in append mode
		if (!file){
			#ifdef _debug_
			USART_printf("ERROR: Failed to open file");
			USART_newline();
			#endif
			delay_ms_2(1000);
			continue; //if fail, delay then try again
		}
		//read temp
		volatile uint16_t  temp_reading = get_temp_12bit(I2C_Address);

		//print to screen
		volatile float float_temp = (float)temp_reading*.0625;
		volatile int ipart = (int)float_temp; 
		// Extract floating part 
		volatile float fpart = float_temp - (float)ipart; 
		fpart = fpart*1000;
		USART_printf("%d.%d",ipart,(int)fpart);
		USART_newline();
	
		//write to SD card
		uint8_t j =0;
		uint8_t packet_size = 7;
		char data_packet[packet_size];
		char tmp[2];
		itoa(ipart, tmp, 10);
		strcpy(&data_packet[j], tmp);
		j += 2;
		data_packet[j] =  46; //period
		j++;
		itoa((int)fpart, tmp, 10);
		strcpy(&data_packet[j], tmp);
		j += 2;
		//data_packet[j] =  44; //comma
		data_packet[j] = 13; //carriage return
		j++;
		data_packet[j] = 10; //line feed
		//USART_printf(data_packet);
		if (fl_fwrite(data_packet, 1, sizeof(data_packet), file) != sizeof(data_packet)){
			#ifdef _debug_
			USART_printf("ERROR: Write file failed\n");
			USART_newline();
			#endif
		}
		fl_fclose(file);
		delay_ms_2(1000);
	}
	throw_error();
}

int media_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count){
	unsigned long i;
	for (i=0;i<sector_count;i++){
		SD_read_single_block(sector, buffer);
		sector ++;
		buffer += 512;
	}
	return 1;
}

int media_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count){
	unsigned long i;
	for (i=0;i<sector_count;i++){
		SD_write_single_block(sector, buffer);
		sector ++;
		buffer += 512;
	}
	return 1;
}