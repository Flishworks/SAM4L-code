/*
 * Extension SD card.c
 * Description: Library for initializing and reading/writing to SD card using SAM4L8XPlained and 101Xplained SD extension board
 * Created: 6/27/2019 3:03:03 PM
 * Author : Avi Kazen
 */ 


#include "sam.h"
#include "PM_SAM4L.h"
#include "delay_SAM4L.h"
volatile uint8_t x = 0;
volatile uint8_t byteTest = 0;
volatile uint32_t wordTest = 0;

uint32_t SD_transmit_32(uint32_t data_out);
uint8_t SD_send_byte(uint8_t byte_to_send);
uint8_t SD_send_command_R1(uint8_t cmd, uint32_t argument, uint8_t crc);
uint32_t SD_send_command_R3(uint8_t cmd, uint32_t argument, uint8_t crc);
uint8_t SD_send_command_cmd_0(void);
//void SD_init();
//void SPI_init();
//void RCFAST_init();
//void throw_error();

int main(void)
{
    // Initialize the system
    SystemInit();
	PM_init();
	RCFAST_init();
	SPI_init();
	SD_init();
	uint8_t r1_response;
	
    //generate data to store on card
	uint8_t  data_to_send[1024];
	uint16_t size = sizeof(data_to_send);
   	for(uint16_t i=0; i < size; i++){ //data
		data_to_send[i] = 64;
    }
	
	//initialize variable to store data read back from card
	volatile uint8_t data_read[size]; 
	
	//write block
	//SD_write_single_block(2000, data_to_send); //data must be 512 bytes
	
	//write multiple blocks
	SD_write_multi_block(2000, data_to_send, size);
	
	//read block
	//SD_read_single_block(2000, data_read);
	
	//read mulitple blocks
	SD_read_multi_block(2000, data_read, size);
	
    while (1) 
    {
		x++;
    }
}

void SD_read_multi_block(uint32_t address, uint8_t *data, uint32_t size){	
	//cmd 18
	uint8_t r1_response = SD_send_command_R1(18, address, 0xFF); //read from address 
	if (r1_response != 0){throw_error();};
	uint32_t end_address = data + size;
	while (data < end_address){
		//data packet
		while (SD_send_byte(0xFF)!=0b11111110){}; //wait for data token
		for(uint16_t i=0; i < 512; i++){
			*data = SD_send_byte(0xFF); //store value at variable address
			data++; //increment address
		}
		SD_send_byte(0xFF);//receive CRC
	    SD_send_byte(0xFF);
	}
	SD_send_command_R1(12, 0, 0xFF); //terminate transfer. Ignore response
	r1_response = 0xFF;
	while (r1_response == 0xFF) r1_response = SD_send_byte(0xFF); //wait for CMD response
	if (r1_response != 0){throw_error();};
	while(SD_send_byte(0xFF) != 0xFF); //wait for card to not be busy
}

void SD_write_multi_block(uint32_t address, uint8_t *data, uint32_t size){
	//CMD25
	uint8_t r1_response = SD_send_command_R1(25, address, 0xFF); //send cmd 25 with argument specifying address
	if (r1_response != 0){throw_error();}; //get cmd response
	SD_send_byte(0xFF); //send dummy variable to give the card some time
	uint32_t end_address = data + size;
	while (data < end_address){
		//data packet
		SD_send_byte(0b11111100); //send data token
		for (uint16_t i = 0; i < 512; i++) { //data
			SD_send_byte(*data); //send value at address
			data++; //increment address
		}
		SD_send_byte(0);// send  two byte CRC
		SD_send_byte(0xFF); //
		uint8_t response = SD_send_byte(0xFF); //send dummy variable to get response
		while(SD_send_byte(0xFF) != 0xFF); //wait for card to not be busy
		if ((response & 0b00001111) != 0b00000101){throw_error();};
	}
	SD_send_byte(0b11111101); //send stop tran
	SD_send_byte(0xFF);
	while(SD_send_byte(0xFF) != 0xFF); //wait for card to not be busy
}

void SD_write_single_block(uint32_t address, uint8_t *data){
	//data must be 512 bytes
	//CMD24 
	uint8_t r1_response = SD_send_command_R1(24, address, 0xFF); //send cmd 24 with argument specifying address
	if (r1_response != 0){throw_error();};
	SD_send_byte(0xFF); //send dummy variable to give the card some time
	//data packet
	SD_send_byte(0b11111110); //send data token
	for (uint16_t i = 0; i < 512; i++) { //data
		SD_send_byte(*data); //send value at address
		data++; //increment address
	}
	SD_send_byte(0);// send  two byte CRC
	SD_send_byte(0xFF); // send CRC
	uint8_t response = SD_send_byte(0xFF); //send dummy variable to get response
	while(SD_send_byte(0xFF) != 0xFF); //wait for card to not be busy
	if ((response & 0b00001111) != 0b00000101){throw_error();};
}

void SD_read_single_block(uint32_t address, uint8_t *data){	
	uint8_t r1_response = SD_send_command_R1(17, address, 0xFF); //read from address 
	if (r1_response != 0){throw_error();};
	while (SD_send_byte(0xFF)!=0b11111110){}; //wait for data token
	for(uint16_t i=0; i < 512; i++){
		*data = SD_send_byte(0xFF); //store value at variable address
		data++; //increment address
	}
	SD_send_byte(0xFF);//receive CRC
	SD_send_byte(0xFF);
}



void delay_2(volatile uint32_t delay_time) //delays by t program loops.
{
	uint32_t t = 0;
	while (t < delay_time){
		t ++; //__NOP();
	}
}

void throw_error(){
	//blinks to indicate and issue
	//enable GPIO PC7
	GPIO->bf.Port[2].bf.GPERS.reg = 0x00000080;
	//set PC7 as output
	GPIO->bf.Port[2].bf.ODERS.reg = GPIO_ODER_P7;
    while (1) 
    { 
		//turn on
		GPIO->bf.Port[2].bf.OVRC.reg = GPIO_OVRC_P7;
		//delay_us(100000);
		delay_2(500000);
		//turn off
		GPIO->bf.Port[2].bf.OVRS.reg = GPIO_OVRS_P7;
		//delay_us(100000);
		delay_2(500000);
    }
}



void SD_init(){
	//wait 1 ms after powerup
	//to be implemented
	
	volatile uint8_t return_byte;
	volatile uint32_t return_data;
	//set SPI clock to 375khz for initialization
	SPI->bf.CR.bit.SPIDIS = 1;
	SPI->bf.CSR[2].bit.SCBR = 32; //set clock division to 32 (~375 khz)
	SPI->bf.CR.bit.SPIEN = 1;
	
	//drive CS pin high for several cycles for initialization
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	GPIO->bf.Port[1].bf.ODERS.reg = (uint32_t)(1) << 11; //set GPIO output to output
	GPIO->bf.Port[1].bf.GPERS.reg = (uint32_t)(1) << 11; // enable GPIO
	for (uint8_t i=0; i<12; i++)
	{
		SD_send_byte(0xFF);
	}	
	
	SPI->bf.CR.bit.SPIDIS = 1;
	SPI->bf.CSR[2].bit.SCBR = 8; //set clock division to 8 (1.5mhz?)
	SPI->bf.CR.bit.SPIEN = 1;
	
	//GPIO->bf.Port[1].bf.GPERC.reg = (uint32_t)(1) << 11; // disable GPIO (reenabling SPI)
	
	//send command zero
	while(SD_send_command_cmd_0() != 0x01);

	//send command 8 with argument of 0x000001AA - check voltage range (verif card version)
	wordTest = SD_send_command_R3(8, 0x000001AA, 0x87);
	if(wordTest != 0x000001AA){throw_error();};
	
	return_byte = 0x01;
	while(return_byte != 0){
		//ACMD41
		//send command 55
		SD_send_command_R1(55,0,0x01);
		//send command 41
		return_byte = SD_send_command_R1(41,1<<30,0x01);
		}
	
	//send CMD 58 to read OCR register and check CCS flag (bit 30). When it is set, the card is a high-capacity card
	return_data = SD_send_command_R3(58, 0, 1);
	delay_us(10); //for breakpoint
}


uint8_t SD_send_command_cmd_0(void){
	for (uint8_t i=0; i<10; i++){SD_send_byte(0xFF);} //send some dummy codes
	uint8_t return_byte = 0;
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	return_byte = SD_send_byte(0b01000000);
	SD_transmit_32(0ul);
	return_byte = SD_send_byte(0x95); //CRC field
	return_byte = 0xFF;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(10);
	return return_byte;
}

uint8_t SD_send_command_R1(uint8_t cmd, uint32_t argument, uint8_t crc){
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD ready to receive command
	SD_send_byte(cmd | 0b01000000);
	SD_transmit_32(argument);
	SD_send_byte(crc); //CRC field
	volatile uint8_t return_byte = 0xFF;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(10);
	return return_byte;
}

uint32_t SD_send_command_R3(uint8_t cmd, uint32_t argument, uint8_t crc){
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD read to receive command
	SD_send_byte(cmd | 0b01000000);
	SD_transmit_32(argument);
	SD_send_byte(crc); //CRC field
	volatile uint8_t return_byte = 0xFF;
	volatile uint32_t return_data = 0;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	return_data = SD_transmit_32(0xFFFFFFFF);
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(10);
	return return_data;
}


uint8_t SD_send_byte(uint8_t byte_to_send){
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	//send first byte
	while(SPI->bf.SR.bit.TDRE == 0); //is transmit register empty? //while((SPI->reg.SPI_SR & SPI_SR_TDRE) == 0);
	SPI->bf.TDR.reg = byte_to_send; //load Transmit Data Register
	//read data returned
	while (SPI->bf.SR.bit.RDRF == 0); //If byte is present in RDR, will read high
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(10);
	return (uint8_t)SPI->bf.RDR.reg; //to receive data (Receive Data reg)
}

uint32_t SD_transmit_32(uint32_t data_out)
{
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	uint32_t return_data;
	//for (uint8_t i=0; i<4; i++)
	//{
		////uint8_t byte_out = (uint8_t)data_out;
		////data_out = uint8_t(data_out >> 8);
		//uint32_t d_out_temp = data_out;
		//return_data |= (uint32_t)SD_send_byte((uint8_t)(d_out_temp >> (24-(8*i)))) << (24-(8*i));
	//}
	//return return_data;
	
	uint8_t byte0 = SD_send_byte(data_out >> (24-(8*0)));
	uint8_t byte1 = SD_send_byte(data_out >> (24-(8*1)));
	uint8_t byte2 = SD_send_byte(data_out >> (24-(8*2)));
	uint8_t byte3 = SD_send_byte(data_out >> (24-(8*3)));
	return byte0<<(24-(8*0))|byte1<<(24-(8*1))|byte2<<(24-(8*2))|byte3<<(24-(8*3));
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(10);
}

void SPI_init()
{
	//Disable GPIO on pins used for SPI
	GPIO->bf.Port[0].bf.GPERC.reg = (uint32_t)(1) << 21;
	GPIO->bf.Port[0].bf.GPERC.reg = (uint32_t)(1) << 22;
	GPIO->bf.Port[1].bf.GPERC.reg = (uint32_t)(1) << 11;
	GPIO->bf.Port[2].bf.GPERC.reg = (uint32_t)(1) << 30;
	//GPIO->bf.Port[0].bf.GPERC.reg = 0x00200000;
	//GPIO->bf.Port[0].bf.GPERC.reg = GPIO_PA21;
	//GPIO->bf.Port[0].bf.GPERC.bit.P21 = 1; //has a slight performance hit
	
    //Configure peripheral Muxing registers (PMR 0-2) to specify that the peripheral function of these GPIO pins is SPI (peripheral function B)
	GPIO->bf.Port[0].bf.PMR0C.reg = GPIO_PA21 | GPIO_PA22;
	GPIO->bf.Port[0].bf.PMR1C.reg = GPIO_PA21 | GPIO_PA22;
	GPIO->bf.Port[0].bf.PMR2C.reg = GPIO_PA21 | GPIO_PA22;
	
	GPIO->bf.Port[1].bf.PMR0S.reg = GPIO_PB11B_SPI_NPCS2;
	GPIO->bf.Port[1].bf.PMR1C.reg = GPIO_PB11;
	GPIO->bf.Port[1].bf.PMR2C.reg = GPIO_PB11;
	
	GPIO->bf.Port[2].bf.PMR0S.reg = GPIO_PC30;
	GPIO->bf.Port[2].bf.PMR1C.reg = GPIO_PC30;
	GPIO->bf.Port[2].bf.PMR2C.reg = GPIO_PC30;
	
	//enable system clock to be used for SPI peripheral
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_PBAMASK_OFFSET;
	PM->bf.PBAMASK.reg |= PM_PBAMASK_SPI;
	
	//configure SPI
	SPI->bf.MR.bit.MSTR = 1; //set mode to master (in mode register);
	SPI->bf.MR.bit.MODFDIS = 1;//set mode fault detection to off
	SPI->bf.MR.bit.PS = 0;  //set as fixed peripheral (will need to change this if talking to more than one device)
	SPI->bf.MR.bit.PCSDEC = 0;//use CS lines directly not in decode mode)
	SPI->bf.MR.bit.PCS = 0b1011; //peripheral chip select.
    
	SPI->bf.CSR[2].bit.CPOL = 0; //set clock polarity (see reference table)
	SPI->bf.CSR[2].bit.NCPHA = 1; //set clock phase (see reference table)
	SPI->bf.CSR[2].bit.CSAAT=1; //something with timing
	SPI->bf.CSR[2].bit.CSNAAT=0;
	SPI->bf.CSR[2].bit.BITS = 0; //Configure 8 bit package size
	SPI->bf.CSR[2].bit.SCBR =32; //set clock division to 32 (~375 khz)
	SPI->bf.CSR[2].bit.DLYBS = 0; //other timing stuff
	SPI->bf.CSR[2].bit.DLYBCT = 0;
	//or...
	//SPI->bf.CSR[2].reg = SPI_CSR_CPOL_0|SPI_CSR_NCPHA_1|SPI_CSR_CSAAT_1|SPI_CSR_BITS(0)|SPI_CSR_SCBR(1)|SPI_CSR_DLYBS(0)|SPI_CSR_DLYBCT(0);
	
	SPI->bf.CR.reg = SPI_CR_SPIEN; //open the gates for communication (enable SPI)
	//SPI->bf.CR.reg = SPI_CR_SWRST; //reset device
}

void RCFAST_init(){
	//Enable SCIF, which controls oscillators/clocks.
	SCIF->bf.UNLOCK.reg = 0xAA000000 | SCIF_RCFASTCFG_OFFSET; //unlock RCFASTCFG
	SCIF->bf.RCFASTCFG.bit.EN = 1; // Enable RCFAST
	while (SCIF->bf.RCFASTCFG.bit.EN != 1); // wait for it to turn on
	//set RCFAST as system clock
	PM->bf.UNLOCK.reg =  0xAA000000 | PM_MCCTRL_OFFSET;
	PM->bf.MCCTRL.reg = 0x00000005;
}