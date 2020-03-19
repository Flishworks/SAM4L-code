/*
 * Extension SD card.c
 *
 * Created: 6/27/2019 3:03:03 PM
 * Author : NUC11
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

void SD_init();
void SPI_init();
void RCFAST_init();

int main(void)
{
    // Initialize the system
    SystemInit();
	PM_init();
	RCFAST_init();
	SPI_init();
	SD_init();
	
	//volatile uint8_t return_byte = 0xFF;
	//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF);
	
    while (1) 
    {
		x++;
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
	
	//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD read to receive command
	//for (uint8_t i = 0; i < 5; i++) SD_send_byte(0xFF);//send a couple more dummy bytes
	//SD_send_byte(0b01000000); //command 0 - software reset
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0x95); //CRC field
	//return_byte = 0xFF;
	//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	
	//return_byte = SD_send_command_R1(8, 0x000001AA, 0x87);
	//wordTest = ((uint32_t)SD_send_byte(0xFF))<<24;
	//wordTest |= ((uint32_t)SD_send_byte(0xFF))<<16;
	//wordTest |= ((uint32_t)SD_send_byte(0xFF))<<8;
	//wordTest |= (uint32_t)SD_send_byte(0xFF);
	
	wordTest = SD_send_command_R3(8, 0x000001AA, 0x87);
	
	//send command 8 with argument of 0x000001AA - check voltage range (verif card version)
	//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high
	//for (uint8_t i = 0; i < 10; i++) SD_send_byte(0xFF);//send a couple more dummy bytes
	//SD_send_byte(0b01001000);
	//SD_transmit_32(0x000001AA);
	//SD_send_byte(0x87); //CRC field
	//return_byte = 0xFF;
	//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF);
	//return_data = SD_transmit_32(0xFFFFFFFF);
	
	return_byte = 0x01;
	while(return_byte != 0){
		//ACMD41
		//send command 55
		SD_send_command_R1(55,0,0x01);
		//send command 41
		return_byte = SD_send_command_R1(41,1<<30,0x01);
		
		//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high
		//SD_send_byte(0b01110111); //command 55
		//SD_send_byte(0);
		//SD_send_byte(0);
		//SD_send_byte(0);
		//SD_send_byte(0);
		//SD_send_byte(0x01);
		//return_byte = 0xFF;
		//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF);
		////send command 41
		//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high
		//SD_send_byte(0b01101001); //command 41
		//SD_send_byte(0b01000000);
		//SD_send_byte(0);
		//SD_send_byte(0);
		//SD_send_byte(0);
		//SD_send_byte(0x01);
		//return_byte = 0xFF;
		//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF);
		}
	
	//send CMD 58 to read OCR register and check CCS flag (bit 30). When it is set, the card is a high-capacity card
	return_data = SD_send_command_R3(58, 0, 1);
	return_data = SD_send_command_R3(58, 0, 1);
	//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD read to receive command
	//SD_send_byte(0b01111010); //command 58
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0);
	//SD_send_byte(0x01); //CRC field
	//return_byte = 0xFF;
	//while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	
	
	//send CMD 16 to force 512 byte block size
	//return_byte = SD_send_command_R1(0x10, 0x00000200, 0xFF);
	
}


uint8_t SD_send_command_cmd_0(void){
	//while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD ready to receive command
	//for (uint8_t i = 0; i < 2; i++) SD_send_byte(0xFF);//send a couple more dummy bytes to avoid problems
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	delay_us(10);
	SD_send_byte(0b01000000);
	SD_transmit_32(0ul);
	SD_send_byte(0x95); //CRC field
	volatile uint8_t return_byte = 0xFF;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	//GPIO->bf.Port[1].bf.GPERC.reg = (uint32_t)(1) << 11; 
	delay_us(10);
	return return_byte;
}

uint8_t SD_send_command_R1(uint8_t cmd, uint32_t argument, uint8_t crc){
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	//delay_us(10);
	while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD ready to receive command
	//for (uint8_t i = 0; i < 2; i++) SD_send_byte(0xFF);//send a couple more dummy bytes to avoid problems
	SD_send_byte(cmd | 0b01000000);
	SD_transmit_32(argument);
	SD_send_byte(crc); //CRC field
	volatile uint8_t return_byte = 0xFF;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(1);
	return return_byte;
}

uint32_t SD_send_command_R3(uint8_t cmd, uint32_t argument, uint8_t crc){
	GPIO->bf.Port[1].bf.OVRC.reg = (uint32_t)(1) << 11; //set GPIO output to low
	while (SD_send_byte(0xFF) != 0xFF); //wait till DO goes high, indicating SD read to receive command
	SD_send_byte(cmd | 0b01000000);
	SD_transmit_32(argument);
	SD_send_byte(crc); //CRC field
	volatile uint8_t return_byte = 0xFF;
	volatile uint32_t return_data = 0;
	while (return_byte == 0xFF) return_byte = SD_send_byte(0xFF); //send 0xFF while reading in data from SD.
	return_data = SD_transmit_32(0xFFFFFFFF);
	GPIO->bf.Port[1].bf.OVRS.reg = (uint32_t)(1) << 11; //set GPIO output to high
	delay_us(1);
	return return_data;
}


uint8_t SD_send_byte(uint8_t byte_to_send){
	//send first byte
	while(SPI->bf.SR.bit.TDRE == 0); //is transmit register empty? //while((SPI->reg.SPI_SR & SPI_SR_TDRE) == 0);
	SPI->bf.TDR.reg = byte_to_send; //load Transmit Data Register
	//read data returned
	while (SPI->bf.SR.bit.RDRF == 0); //If byte is present in RDR, will read high
	return (uint8_t)SPI->bf.RDR.reg; //to receive data (Receive Data reg)
}

uint32_t SD_transmit_32(uint32_t data_out)
{
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