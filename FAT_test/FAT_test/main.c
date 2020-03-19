/*
 * FAT_test.c
 *
 * Created: 8/27/2019 10:16:27 AM
 * Author : NUC11
 */ 


#define _debug_
#ifdef _debug_
#include "AK_USART.h"
#endif

#include "sam.h"
#include "fat_filelib.h"
#include "AK_SD.h"
#include "AK_utils.h"



uint32_t Rx = GPIO_PC26;
uint32_t Tx = GPIO_PC27;

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

int main()
{
	/* Initialize the SAM system */
	SystemInit();
	//PM_init();
	RCFAST_init();
	SPI_init();
	
	#ifdef _debug_
	USART_init(Tx, Rx);
	USART_newline();
	USART_printf("System init begin");
	USART_newline();
	#endif
	
	FL_FILE *file;
	FL_FILE *file2;

	// Initialise media
	SD_init();

	// Initialise File IO Library
	fl_init();

	// Attach media access functions to library
	if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
	{
		#ifdef _debug_
        USART_printf("ERROR: Media attach failed\n");
		#endif
	}
	
	
	 //List root directory
	//fl_listdirectory("/");

	 //Create File
	file = fl_fopen("/test.txt", "w");
	if (file){
		// Write some data
		volatile char data[] = "Once upon a time there was a small file that lived on a SD card. This file was composed of two sentences.";
		if (fl_fwrite(data, 1, sizeof(data), file) != sizeof(data)){
#ifdef _debug_
			USART_printf("ERROR: Write file failed\n");
#endif
		}
	}
	else{
#ifdef _debug_
		USART_printf("ERROR: Create file failed\n");
#endif
	}
			
	// Close file
	fl_fclose(file);

	
	//open file
	file2 = fl_fopen("/test.txt", "r");
	if (file2){
		print_file(file2); //only in debug mode
	}
	else{
#ifdef _debug_
		USART_printf("ERROR: open file failed\n");
#endif
	}
	
	
	
	
	// Delete File
	//if (fl_remove("/file.bin") < 0){
    //  #ifdef _debug_
	//	USART_printf("ERROR: Delete file failed\n");
	//  #endif
	//}
		
	// List root directory
	
	fl_shutdown();
	throw_error();
}

#ifdef _debug_
void print_file(FL_FILE *file){
	char read_data[file->filelength] ;
	fl_fread(read_data, 1, file->filelength, file);
	USART_newline();
	for (uint8_t i = 0; i < file->filelength; i++){
		USART_printf("%c",read_data[i]);
	}
}
#endif