/*
 * USB_test.c
 *
 * Created: 11/4/2019 3:46:23 PM
 * Author : NUC11
 */ 


#include "sam.h"
#include "USBC_SAM4L.h"


int main(void)
{
	
    /* Initialize the SAM system */
    SystemInit();
	USB_setup();
    /* Replace with your application code */
	
	// USB Interrupt startup procedure ////////////////////////////////////////////////////////
	USBC_UDD_enable();	// Enable the PLL and stuff
	USBC_UDD_attach();
				
    while (1) 
    {			
		// If there's a job running (data downloading), flash quickly, otherwise USB idle, flash slowly
		if (jobEP[1].bJobEnabled == 1) blink_LED(100);
		else // device is idle
		{
			blink_LED(1000);
		}
    }
}

void USB_setup(void)
{
	Pin USB_VUSB	= {GPIO_PB01C_EIC_EXTINT0, PORTB_NUM, MUX_PB01C_EIC_EXTINT0}; //how does this external interrupt get fired?
	USBC_init(&USB_VUSB, USB_VBUS_INTERRUPT_EIC, USB_VBUS_INTERRUPT_LOW, EXT_INT0_NMI, 1); //initialize the USB interrupt pin and ptorotpe functions
}

// MANAGING SD TRANSFER JOBS
void SD_2_USB_jobStarter(dataHeader_Type* pDataHeader)
{
	// This doesn't actually start the transmit, it just enables job on the EP, so the next
	// transmission from the host on that EP will cause data to transfer.  All of the information
	// generated in here is either defined in globals (buffer size... etc..) or from information in
	// the device header.
	
	// STEP 1 - Load the 1st part of the buffer to be transferred upon receiving an EP1 USB request.
	// Start at the bottom of the buffer, and  set the base address of the file to a global that will be ticked
	// clear out any/all data in data sampling buffer... Don't want spill over
	
	sd_2_usb_address = pDataHeader->firstFile.firstAddress;
	uint64_t numBytes_2_Tx = (uint64_t)pDataHeader->firstFile.numBlocks * 512;
	cleanDataBuffer();
	
	// Fill the bottom half of the buffer with data from SD card
	//SD_readBlock_Byte(&mSD, &dataBuffer[MAIN_BUFFER_BYTE_LENGTH_HALF * bufferPart], sd_2_usb_address, BLOCKS_IN_BUFFER/2, SD_COM_DMA_WHILE_BLOCK);
	//sd_2_usb_address += BLOCKS_IN_BUFFER/2; // tick address to next good location
	
	// Create the job, prime it with information looking at the just loaded data
	// This will be transmitted upon the next EP1 IN transmission
	USB_EP_createJob(1, &dataBuffer[MAIN_BUFFER_BYTE_LENGTH_HALF * bufferPart], numBytes_2_Tx, MAIN_BUFFER_BYTE_LENGTH_HALF);
	
	// Flip the buffer part indicator to upper half and load it with data from SD
	bufferPart = (uint8_t)!bufferPart;
	//SD_readBlock_Byte(&mSD, &dataBuffer[MAIN_BUFFER_BYTE_LENGTH_HALF * bufferPart], sd_2_usb_address, BLOCKS_IN_BUFFER/2, SD_COM_DMA_WHILE_BLOCK);
}

uint8_t SD_2_USB_jobManager(void)
{
	// Load the next buffer location to be transfered
	USB_EP_manageJob(1, &dataBuffer[MAIN_BUFFER_BYTE_LENGTH_HALF * bufferPart]);														//	---------------- Use & to change it instead of multiplication
	
	// If there is still a job and the last transfer is not up next, load more data from into buffer
	if ((jobEP[1].bJobEnabled == 1) && (jobEP[1].bLastTransfer == 0))
	{
		sd_2_usb_address += BLOCKS_IN_BUFFER/2;
		bufferPart = (uint8_t)!bufferPart;
		SD_readBlock_Byte(&mSD, &dataBuffer[MAIN_BUFFER_BYTE_LENGTH_HALF * bufferPart], sd_2_usb_address, BLOCKS_IN_BUFFER/2, SD_COM_DMA_WHILE_BLOCK);
	}
	else // the job is done.
	{
		// put anything required for cleanup here...
	}
	
	return 1;
}

void SD_2_USB_jobAbort(void)
{
	// This should only be called when the job fails for some reason - probably form the USB being pulled out mid transfer
	jobEP[1].bJobEnabled = 0;	// disable job
	jobEP[1].sizeTotalTransfered = 0;
	jobEP[1].sizeTotalPayload = 0;
	jobEP[1].bufferLengthLimit = 0;
	jobEP[1].sizeNextTransfer = 0;
	jobEP[1].bLastTransfer = 0;
	jobEP[1].pBuffer = NULL;
	USB_EP_cleanupTransfer(1);
	
	// Nuke everything in the buffer
	cleanDataBuffer();
}


