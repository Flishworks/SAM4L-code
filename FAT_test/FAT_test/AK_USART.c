
//EDGB conected to USART1
//PC26 = Rx
//PC27 = Tx
// periph func A

#include "AK_USART.h"
#include <stdarg.h>

void USART_init(TX, RX){
	//disable GPIO on USART pins
	GPIO->bf.Port[2].bf.GPERC.reg = TX | RX;
	//set GPIO muxing to USART (function A)
	GPIO->bf.Port[2].bf.PMR0C.reg = TX | RX;
	GPIO->bf.Port[2].bf.PMR1C.reg = TX | RX;
	GPIO->bf.Port[2].bf.PMR2C.reg = TX | RX;
	
	//Set up USART in PM (PBA Mask)
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBAMASK_OFFSET);	// Unlock PBA Register
	PM->bf.PBAMASK.reg |= (uint32_t)(0x1ul << PM_PBAMASK_USART1_Pos);	// Enable clock for USART
	
	//Configure USART
	//To start using the USART, the user must perform the following steps:
	//1. Configure the baud rate by writing to the Baud Rate Generator Register (BRGR) as described in “Baud Rate Generator” on page 587
	set_baudrate(115000);
	//2. Select the operating mode by writing to the relevant fields in the Mode Register (MR)
	//USART1->bf.WPMR.bit.WPKEY = 0x555341;
	//USART1->bf.WPMR.bit.WPEN = 0;
	USART1->bf.MR.USART.CHRL = 0b11; //8bit
	//3. Enable the transmitter and/or receiver, by writing a one to CR.TXEN and/or CR.RXEN respectively
	USART1->bf.CR.USART.RSTTX = 1; //reset transmitter
	USART1->bf.CR.USART.TXEN = 1;
	
	//4. Check that CSR.TXRDY and/or CSR.RXRDY is one before writing to THR and/or reading	from RHR respectively
	while (!USART1->bf.CSR.USART.TXRDY);
}


void USART_print(uint8_t *data, uint32_t size){
	for (uint32_t i = 0; i < size; i++) { //data
			USART_write_char(*data);
			data++; //increment address
		}
}

void USART_newline(){
	USART_write_char(10); //line feed
	USART_write_char(13); //carriage return
	//USART_write_char('n');
}

void USART_write_char(uint8_t data){
	//Check that CSR.TXRDY is one before writing to THR
	while (!USART1->bf.CSR.USART.TXRDY); //set when THR is empty.
	USART1->bf.THR.bit.TXCHR = data;
}

void set_baudrate(uint32_t rate){
		//      -BaudRate = SelectedClock/(8(2-over)cd)
		//		-over = oversampling mode (8 or 16)
		//      -cd = clock divider
		//      -see page 587
		uint8_t oversamp;
		//calculate CD
		USART1->bf.MR.USART.OVER = 1;
		if (!USART1->bf.MR.USART.OVER){oversamp = 16;}
		else{oversamp = 8;}
		
		uint32_t clk = clock_speed();
		uint16_t cd = clk/(rate*oversamp);
		if (cd == 0){throw_error();}
		USART1->bf.BRGR.bit.CD = cd;
}


//adapted code to mimic printf function on serial terminal
void  USART_printf (char * str, ...){
	va_list arg_list;
	int i = 0, j=0;
	char buff[100]={0}, tmp[20];
	va_start( arg_list, str );

	while (str && str[i]){
		if(str[i] == '%'){
			i++;
			switch (str[i]) {
				case 'c': {
					buff[j] = (char)va_arg( arg_list, int );
					j++;
					break;
				}
				case 'd': {
					itoa(va_arg( arg_list, int ), tmp, 10);
					strcpy(&buff[j], tmp);
					j += strlen(tmp);
					break;
				}
				case 'x': {
					itoa(va_arg( arg_list, int ), tmp, 16);
					strcpy(&buff[j], tmp);
					j += strlen(tmp);
					break;
				}
			}
			} else {
			buff[j] =str[i];
			j++;
		}
		i++;
	}
	//fwrite(buff, j, 1, stdout);
	USART_print(buff, sizeof(buff));
	va_end(arg_list);
}

