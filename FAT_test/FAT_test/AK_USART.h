#ifndef _AK_USART_	// Guards against double calling of headers
#define _AK_USART_

#include "sam.h"
#include "PM_SAM4L.h"


//prototypes
void USART_write_char(uint8_t data);
void USART_init(TX, RX);
void set_baudrate(uint32_t rate);
void USART_print(uint8_t *data, uint32_t size);
void USART_newline();
void USART_printf (char * str, ...);


#endif