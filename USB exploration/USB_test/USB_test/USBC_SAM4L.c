// USB driver

#include "sam.h"
#include "PM_SAM4L.h"
#include "NVIC_SAM4L.h"
#include "GPIO_SAM4L.h"
#include "SCIF_SAM4L.h"
#include "BSCIF_SAM4L.h"
#include "USB_Protocol_SAM4L.h"
#include "USB_MS_OS_Protocol_SAM4L.h"
#include "USBC_SAM4L.h"
#include "string.h"						// used for memcpy, there is probably a better way to do this...
#include "EIC_SAM4L.h"
#include "AST_SAM4L.h"

COMPILER_ALIGNED(4) USB_DESCRIPTOR_DEVICE_Type devDescriptor = {
	.bLength			= 18,
	.bDescriptorType	= USB_DESCRIPTOR_DEVICE,
	.bcdUSB				= USB_V2_0,					// 
	.bDeviceClass		= USB_DEVICE_CLASS_VENDOR_SPECIFIC,
	.bDeviceSubClass	= 0x00,
	.bDeviceProtocol	= 0x00,
	.bMaxPacketSize0	= USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE, // 64 bytes
	.idVendor			= 0xFFFF,
	.idProduct			= 0x0001,
	.bcdDevice			= 0x0001,
	.iManufacturer		= 1,		// Points to String index 1
	.iProduct			= 2,
	.iSerialNumber		= 0,
	.bNumConfigurations	= 1,
};

// This structure holds the combined configuration response.
// The correct organization of this structure is:
// Config
//	|-->Interface1
//	|	|->EP1
//	|	|->EP2
//	|
//	|-->Interface2
//		|->EP3
//		|->EP4
COMPILER_TIGHT_PACK()
typedef struct
{
	USB_DESCRIPTOR_CONIGURATION_Type	config;
	USB_DESCRIPTOR_INTERFACE_Type		iface;
	USB_DESCRIPTOR_ENDPOINT_Type		EP1;
	//USB_DESCRIPTOR_ENDPOINT_Type		EP2;
} configResp_Type;
COMPILER_RESET_PACK()

// DOn't think this needs to be aligned, as 
COMPILER_ALIGNED(4) configResp_Type configResp = {
	.config.bLength				= 9,	//
	.config.bDescriptorType		= USB_DESCRIPTOR_CONFIGURATION,	// CONFIGURATION Descriptor Type (CONSTANT)
	.config.wTotalLength		= sizeof(configResp_Type),	//																========================
	.config.bNumInterfaces		= 1,	//
	.config.bConfigurationValue	= 1,	//
	.config.iConfiguration		= 0,	//
	.config.bmAttributes		= 0xC0,	// NOTE - THIS NEEDS  TO BE BROKEN OUT........, need by status request and stuff----------------------------------------
	.config.bMaxPower			= 50,	// 100mA
	// Interface Descriptor
	.iface.bLength				= 9,	//
	.iface.bDescriptorType		= USB_DESCRIPTOR_INTERFACE,	// INTERFACE Descriptor Type (CONSTANT)
	.iface.bInterfaceNumber		= 0,	//
	.iface.bAlternatingSetting	= 0,	//
	.iface.bNumEndpoints		= 1,	//
	.iface.bInterfaceClass		= USB_IFACE_CLASS_VENDOR_SPECIFIC,	// Vendor Specific
	.iface.bInterfaceSubClass	= 0,	//
	.iface.bInterfaceProtocol	= 0,	//
	.iface.iInterface			= 3,	//
	// Endpoint 1 Descriptor
	.EP1.bLength				= 7,	//
	.EP1.bDescriptorType		= USB_DESCRIPTOR_ENDPOINT,	// Endpoint Descriptor Type (CONSTANT)
	.EP1.bEndpointAddress.all	= 0x81,	// [7] IN Direction, [3:0] EP1
	.EP1.bmAttributes			= 0x02,	// Bulk Transfer type
	.EP1.wMaxPacketSize			= UINT16_C(USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE),	// 64 Bytes
	.EP1.bInterval				= 0,	//
	// Endpoint 2 Descriptor
// 	.EP2.bLength				= 7,	//
// 	.EP2.bDescriptorType		= USB_DESCRIPTOR_ENDPOINT,	// Endpoint Descriptor Type (CONSTANT)
// 	.EP2.bEndpointAddress.all	= 0x02,	// [7] OUT Direction, [3:0] EP2
// 	.EP2.bmAttributes			= 0x02,	// Bulk Transfer type
// 	.EP2.wMaxPacketSize			= UINT16_C(USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE),
// 	.EP2.bInterval				= 0,	//
};

static const uint8_t stringTestArray[4]= {0x04, USB_DESCRIPTOR_STRING,		// bLength (4 Bytes), String Descriptor
									   0x09, 0x04};							// English language ID
static const uint8_t manfName[8]	= {0x08, USB_DESCRIPTOR_STRING,			// bLength (8 Bytes), String Descriptor							// Length of 
									   0x4e, 0x00, 0x4d, 0x00, 0x48, 0x00};	// "NMH" - I don't think this is called yet
static const uint8_t devName[22]	= {0x16, USB_DESCRIPTOR_STRING,			// bLength (22 Bytes), String Descriptor
									   0x48, 0x00, 0x65, 0x00, 0x61, 0x00,	// "HeartPulse"
									   0x72, 0x00, 0x74, 0x00, 0x50, 0x00,
									   0x75, 0x00, 0x6C, 0x00, 0x73, 0x00,
									   0x65, 0x00};
static const uint8_t int0Name[18]	= {0x12, USB_DESCRIPTOR_STRING,			// bLength (18 Bytes), String Descriptor
									   0x44, 0x00, 0x61, 0x00, 0x74, 0x00,	// "Data I/O"
									   0x61, 0x00, 0x20, 0x00, 0x49, 0x00,
									   0x2F, 0x00, 0x4F, 0x00};			   

//uint16_t get_status = 0x0000;	// Variable used to pull status - this should probably be in mUSBC as status and be of type USB_STATUS_Type -----------------------

uint8_t	HostTime[8];
uint8_t testDatasize[4];

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////

// For setting up CTRL EP
static void USBC_UDD_CTRL_EP_reset(void);	// Reset control endpoint - Called after a USB line reset or when UDD is enabled
static void USBC_UDD_CTRL_init(void);		//  Reset control endpoint management - Called after a USB line reset or at the end of SETUP request (after ZLP)

// Routines for managing the CTRL EP
static USBC_ERROR_TYPE USBC_UDD_CTRL_SETUP_Rx(void);
static USBC_ERROR_TYPE USBC_UDD_CTRL_OUT_Rx(void);
static USBC_ERROR_TYPE USBC_UDD_CTRL_IN_Tx(void);
static void USBC_UDD_CTRL_IN_ZLP_Tx(void);
static void USBC_UDD_CTRL_OUT_ZLP_Rx(void);
//static void UDD_CTRL_overflow(void);
//static void UDD_CTRL_underflow(void);
void USB_resetConfiguration(void);

void USBC_UDD_CTRL_STALL_data(void);


// Request Processing
static USBC_ERROR_TYPE USBC_UDC_SETUP_process(USB_SETUP_REQUEST_Type* pReq);
// Standard Requests
static USBC_ERROR_TYPE USBC_UDC_processStdReq(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USBC_UDC_stdReq_getStatus(USB_SETUP_REQUEST_Type* pReq);

static USBC_ERROR_TYPE USBC_UDC_stdReq_getConfig(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USBC_UDC_stdReq_getDesc(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USBC_UDC_stdReq_setAddr(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USBC_UDC_stdReq_setConfig(USB_SETUP_REQUEST_Type* pReq);
// Vendor Requests
static USBC_ERROR_TYPE USBC_UDC_processVendorReq(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USB_MSReq_getDevDesc(USB_SETUP_REQUEST_Type* pReq);
static USBC_ERROR_TYPE USB_MSReq_getIntDesc(USB_SETUP_REQUEST_Type* pReq);

USBC_ERROR_TYPE USB_CTRL_bufferWrite(void);
uint16_t computePayloadLength(USB_SETUP_REQUEST_Type* pReq, uint16_t desiredLength);

void USB_setAddress(void);
void (*pFunction)(void);			// Function pointer, used for running functions a later times...


// Other Endpoint standard functionality
void USB_EP_abort(uint8_t numEP);
USBC_ERROR_TYPE USB_EP_allocate(USB_DESCRIPTOR_ENDPOINT_Type* pDescEP);
static uint8_t USB_EP_interrupt(void);
//static uint8_t USB_EP_interrupt(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
// USBC Peripheral Initialization/Enabling/Disabling
///////////////////////////////////////////////////////////////////////////////////////////////////
void USBC_init(Pin* VUSB,	// Pin information
			   USB_VBUS_INTERRUPT_TYPE type,		// EIC/GPIO interrupt
			   USB_VBUS_INTERRUPT_LEVEL trigLvl,	// High/Low level trigger
			   EIC_INT_CHAN_NUM EIC_intChanNum,		// ONLY FOR EIC interrupts, GPIO calculated internally
			   uint8_t bUseDFLL)					// use DFLL for USBC clock
{

	// Enable the PLL and set it to the system clock... Maybe only need to do this upon actual communication? something to investigate-------
	mUSBC.state.usbc = USBC_STATE_INITIALIZING;
	
	uint32_t testmask;	// for determining pin state later
	
	// Push the pins to their respective holders so we can push pull them later.
	mUSBC.pin.VUSB = *VUSB;
	
	// Set info critical to functionality
	mUSBC.VUSB.intType = type;
	mUSBC.VUSB.intLvl = trigLvl;
	mUSBC.bUseDFLL = bUseDFLL;

	// Setup pins, DM & DP are hard wired to the same pins on every SAM4L model
	Pin USB_DM = {GPIO_PA25A_USBC_DM, PORTA_NUM, MUX_PA25A_USBC_DM};
	Pin USB_DP = {GPIO_PA26A_USBC_DP, PORTA_NUM, MUX_PA26A_USBC_DP};
	GPIO_setPeriphPin(&USB_DM);
	GPIO_setPeriphPin(&USB_DP);
	
	// Based on interrupt type and level, setup things accordingly
	if (type == USB_VBUS_INTERRUPT_GPIO)
	{
		mUSBC.VUSB.intChanNum = GPIO_calcIntNum(VUSB);
		mUSBC.VUSB.numIRQ = GPIO_getIRQnum(mUSBC.VUSB.intChanNum);
		mUSBC.VUSB.pHandler = GPIO_getHandlerCall(mUSBC.VUSB.intChanNum);
		
		GPIO_setupInterrupt(&mUSBC.pin.VUSB, 1, GPIO_INTERRUPT_MODE_PIN_CHANGE, 5);

		// check if interrupt is triggered (USB already plugged in) and run GPIO handler if true
		//testmask = GPIO->bf.Port[mUSBC.pin.VbusSense.port].bf.PVR.reg & mUSBC.pin.VbusSense.gpioMask;
		//if (testmask == mUSBC.pin.VbusSense.gpioMask) GPIO_0_Handler();	
	}
	else if (type == USB_VBUS_INTERRUPT_EIC)
	{
		// This interrupt is structured oddly.  Since this is an ASYNC interrupt, it's only
		// triggered on certain levels.  So, when USB is NOT plugged in, it needs to be triggered
		// on high levels.  When the USB is plugged in, it needs to be set for low levels.  When
		// the interrupt is actually triggered (in either case) we need to switch the interrupt
		// trigger to the opposite condition.
		
		// Firstly, push information about the interrupt and it's handler over for simplified processing later
		mUSBC.VUSB.intChanNum = EIC_intChanNum;
		mUSBC.VUSB.numIRQ = mEIC.INT[EIC_intChanNum].numIRQ;
		mUSBC.VUSB.pHandler = mEIC.INT[EIC_intChanNum].pHandler;
				
		// Based on whether the device triggers on high or low setup the EIC channel 
		if (trigLvl == USB_VBUS_INTERRUPT_HIGH) EIC_initChan(EIC_intChanNum, &mUSBC.pin.VUSB, EIC_MODE_LEVEL_TRIGGERED, EIC_TRIGGER_HIGH_LEVEL, EIC_ASYNC_MODE);
		else EIC_initChan(EIC_intChanNum, &mUSBC.pin.VUSB, EIC_MODE_LEVEL_TRIGGERED, EIC_TRIGGER_LOW_LEVEL, EIC_ASYNC_MODE);	
		
		NVIC_initEnable(mUSBC.VUSB.numIRQ, 5);
		EIC_primeChan(EIC_intChanNum);
		
		//testmask = GPIO->bf.Port[mUSBC.pin.VbusSense.port].bf.PVR.reg & mUSBC.pin.VbusSense.gpioMask;
		//if (testmask == mUSBC.pin.VbusSense.gpioMask) EIC_0_Handler();
		
	}
	else return;	// we hit some error;
	
	// Add functionality here to check and insure that the interrupt isn't already called
	//if lvl == high 
	//{
		//// check if interrupt is triggered (USB already plugged in) and run GPIO/EIC handler if true
		//testmask = GPIO->bf.Port[mUSBC.pin.VbusSense.port].bf.PVR.reg & mUSBC.pin.VbusSense.gpioMask;
		//if (testmask == mUSBC.pin.VbusSense.gpioMask) mUSBC.pIntFunc();
	//}
	/// ----- I should think about making a callback in the manager... that way we don't get locked into replacing this every time we switch the VBUS pin.
}
void USBC_UDD_enable(void)
{
	safeDisable  IRQ();
	
	// Setup the clocks needed to run the USB
	mUSBC.state.usbc = USBC_STATE_INITIALIZING_CLOCK;
		
	// Setup DFLL or PLL for feeding mainCLK and for feeding the USB GCLK7
	if (mUSBC.bUseDFLL == 1)
	{
		// if OSC32K not enabled, enable
		// if GLCK not enabled, enable
		// if DFLL not enabled, enabled
		// Switch MCLK to DFLL - be aware of all those other weird flash states we need to change as well
		// enable the USB bus
		// enabled the GCLK7 for USB
		
		SCIF_enableDFLL();
		//SCIF_setEnableGCLK(GCLK_0_DFLL_REF_GCLK0_PIN, mBSCIF.freqClks.RC32K, 1, SCIF_GCLK_SRC_RC32K);	// This is temporary - o
		SCIF_setEnableGCLK(GCLK_0_DFLL_REF_GCLK0_PIN, mBSCIF.freqClks.OSC32K, 1, SCIF_GCLK_SRC_OSC32K);
		SCIF_initSrcClk_DFLL_Closed(mSCIF.freq.GCLK[GCLK_0_DFLL_REF_GCLK0_PIN], 48e6, true);
		SCIF_awaitFineLock_DFLL();
			
		PM_enableHighSpeed();
		PM_switchMCLK(PM_MCSEL_SOURCE_DFLL);
	}
	else
	{
		SCIF_setEnableSrcClk_OSC0(12e6, SCIF_OSCCTRLn_STARTUP_18ms); // Enable source clock for the PLL
		#define PLL0_SOURCE			0	// Oscillator 0
		#define PLL0_DIVIDER		4	// Divide by 4
		SCIF_setEnableSrcClk_PLL(0, 15, PLL0_DIVIDER, PLL0_SOURCE, SCIF_PLL_CLKSRC_OSC0);

		PM_enableHighSpeed();
		PM_switchMCLK(PM_MCSEL_SOURCE_PLL); // Switch the main clock over
	}
	SCIF_disableSrcClk_RCFAST();		// kill the RCFAST until USB is quit.
	
	// Enable PBB and HSB for USBC interfacing
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBBMASK_OFFSET);
	PM->bf.PBBMASK.reg |= PM_PBBMASK_USBC;
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_HSBMASK_OFFSET);
	PM->bf.HSBMASK.reg |= PM_HSBMASK_USBC;
	
	// 4) Setup the GCLK[7] and enable
	if (mUSBC.bUseDFLL == 1) SCIF_setEnableGCLK(GCLK_7_USBC, mSCIF.freq.CLK.DFLL, 0, SCIF_GCLK_SRC_DFLL);
	else SCIF_setEnableGCLK(GCLK_7_USBC, mSCIF.freq.CLK.PLL, 0, SCIF_GCLK_SRC_PLL0);
	
	// Setup USBC NVIC
	NVIC_initEnable(USBC_IRQn, 5);		// arbitrary low priority (5) .... copying what they described in ATMEL....
	
	// Enable USBC ASYNC wakeup interrupt
	//PM->bf.AWEN.reg |= (1u << PM_AWEN_USBC);
	// Enable fast wakeup in the BPM
	//BPM->bf.UNLOCK.reg = ADDR_UNLOCK(BPM_PMCON_OFFSET);	//	THESE TWO MAY BE SUPER SENSITIVE
	//BPM->bf.PMCON.reg |= BPM_PMCON_FASTWKUP;				//	MAYBE CAN ONLY ACCPET A DIRECT = WRITE, delay from | is too much time?
		
	//Set [USBCON]
	USBC->bf.USBCON.bit.UIMOD	= 1; // Set to device mode manually	--- may be able to detect this with ID pin?
	USBC->bf.USBCON.bit.USBE	= 1; // Enable USBC
	USBC->bf.USBCON.bit.FRZCLK	= 0; // unfreeze-clock
	
	//// Setup the device description table.
	//// They set all the memory of the description table = 0
	//// Then push the address of the endpoint table to the USB descriptor....
	
	// Set the base address of the ATMEL USB descriptors, are they doing 4 endpoints with 2 banks? or 8 endpoints 1 bank? (pg 351)-----
	USBC->bf.UDESC.reg = (uint32_t)&EP_Table;
	
	// clear out the busy flags on the job table? Seems that this is an artificial construct of theirs.
		
	USBC->bf.UDCON.bit.LS = 0;		// Set device to full speed operation
	USBC->bf.USBCON.bit.FRZCLK = 1; // Freeze Clock

	mUSBC.state.usbc = USBC_STATE_ENABLED;
	mUSBC.bFlag.initUSBC = 1;
	
	safeEnableIRQ();
}
void USBC_UDD_disable(void)
{
	safeDisableIRQ();
	
	mUSBC.state.usbc = USBC_STATE_DISABLING;
	mUSBC.bFlag.initUSBC = 0;
	
	// Detach the device
	USBC_UDD_detach();
	USBC->bf.USBCON.bit.USBE	= 0;	// Disable USBC
	
	NVIC_DisableIRQ(USBC_IRQn);			// Clear and disable USB NVIC interrupt - WHY DIDN'T IS USE NVIC_disable() HERE?????????????????????

	SCIF_resetDisableGCLK(GCLK_7_USBC);			// Disable the GCLK feeding the USBC
	
	// Disable PBB and HSB used for USBC interface
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_PBBMASK_OFFSET);
	PM->bf.PBBMASK.reg &= ~PM_PBBMASK_USBC;
	PM->bf.UNLOCK.reg = ADDR_UNLOCK(PM_HSBMASK_OFFSET);
	PM->bf.HSBMASK.reg &= ~PM_HSBMASK_USBC;
	
	// Now we need to disable the USB clocks and hand it over to something else,
	// lets default back to RCFAST.. this should probably be RCSYS in the future...
	SCIF_reenableSrcClk_RCFAST();
	PM_switchMCLK(PM_MCSEL_SOURCE_RCFAST);
	
	PM_disableHighSpeed();
	
	// Kill the source(s) of the clock driving the UBSC module
	if (mUSBC.bUseDFLL)
	{
		SCIF_disableResetDFLL();
		SCIF_resetDisableGCLK(GCLK_0_DFLL_REF_GCLK0_PIN);
	} 
	else
	{
		SCIF_resetSrcClk_PLL(0);
		SCIF_resetSrcClk_OSC0();
	}
	
	mUSBC.state.usbc = USBC_STATE_UNKNOWN;
	safeEnableIRQ();
}
void USBC_UDD_attach(void)
{	
	safeDisableIRQ();

	// Unfreeze clock and block until clock is usable
	USBC->bf.USBCON.bit.FRZCLK = 0;
	while (!(USBC->bf.USBSTA.bit.CLKUSABLE));
	
	// Authorize attach - USB communication with host begins here
	USBC->bf.UDCON.bit.DETACH = 0;
	mUSBC.bFlag.devAttached = 1;
	
	// Enable USB line Interrupts events
	USBC->bf.UDINTESET.reg	= USBC_UDINTESET_EORSTES | USBC_UDINTESET_SUSPES
							| USBC_UDINTESET_WAKEUPES;// | USBC_UDINTESET_SOFES;
	
	// Clear End of Reset (EOR) and SOF interrupts
	USBC->bf.UDINTCLR.reg = USBC_UDINTCLR_EORSTC | USBC_UDINTCLR_SOFC;
	
	// First Suspend interrupt must be forced
	USBC->bf.UDINTSET.bit.SUSPS		= 1; // Set Suspend interrupt
	USBC->bf.UDINTCLR.bit.WAKEUPC	= 1; // Clear Wakeup interrupt
	USBC->bf.USBCON.bit.FRZCLK		= 1; // Freeze Clock
	
	safeEnableIRQ();
}
void USBC_UDD_detach(void)
{
	USBC->bf.USBCON.bit.FRZCLK	= 0;	// unfreeze-clock
	USBC->bf.UDCON.bit.DETACH	= 1;	// Detach from the USB bus
	mUSBC.bFlag.devAttached		= 0;
	USBC->bf.USBCON.bit.FRZCLK	= 1;	// freeze the clock	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Core USB Functionality
///////////////////////////////////////////////////////////////////////////////////////////////////
void USBC_Handler(void)
{
	// UDD SOF INTERRUPT///////////////////////////////////////////////////////////////////////////
	if (USBC->bf.UDINT.bit.SOF)	// If we have a start of frame interrupt do whatever is in here		-- this is probably not necessary....
	{
		USBC->bf.UDINTCLR.bit.SOFC = 1;	// Clear SOF interrupt
		// SOF EVENT ACTION HERE
	}
	
	// UDD EP0 (CTRL) INTERRUPT ///////////////////////////////////////////////////////////////////	
	else if (USBC->bf.UDINT.bit.EP0INT == 1)
	{
		// By default disable the over/underflow interrupts ------------------  not really sure how NAK in/out relates to underflow
		USBC->bf.UECON0CLR.reg = USBC_UECON0CLR_NAKINEC | USBC_UECON0CLR_NAKOUTEC;
		
		// Determine CTRL interrupt type:
		if (USBC->bf.UESTA0.bit.RXSTPI == 1) USBC_UDD_CTRL_SETUP_Rx();			// SETUP Packet Received
		else if (USBC->bf.UESTA0.bit.RXOUTI == 1) USBC_UDD_CTRL_OUT_Rx();		// OUT Packet Received
		else if (USBC->bf.UESTA0.bit.TXINI && USBC->bf.UECON0.bit.TXINE) USBC_UDD_CTRL_IN_Tx();	// IN Packet Sent
		else if (USBC->bf.UESTA0.bit.NAKOUTI == 1)	// NAK OUT Packet Received - Overflow on OUT Packet
		{
			USBC->bf.UESTA0CLR.bit.NAKOUTIC = 1;	// Clear NAK OUT interrupt
			// FUNCTION HERE -----------------------------------------------------------------------------------------------------
		}
		// NAK IN Packet Received - Overflow on IN Packet
		else if (USBC->bf.UESTA0.bit.NAKINI == 1)
		{
			USBC->bf.UESTA0CLR.bit.NAKINIC = 1;		// Clear NAK IN interrupt
			// FUNCTION HERE -----------------------------------------------------------------------------------------------------
		}
		else {} // do nothing...
	}
	// UDD EPn INTERRUPT //////////////////////////////////////////////////////////////////////////
	else if (USB_EP_interrupt() == 1)
	{
		// if it returns true we break and go to the next USB interrupt
		// if false, it goes onto the next if statement
		
		// IN the future we will add whatever code required after running an EP interrupt here
	}

	// UDD RESET INTERRUPT ////////////////////////////////////////////////////////////////////////
	else if (USBC->bf.UDINT.bit.EORST == 1)
	{
		USBC->bf.UDINTCLR.bit.EORSTC = 1;			// Clear Interrupt
				
		// If we had a reset, wipe any existing configuration set and put the device back into
		// default state of not having one.  Immediately kill any endpoints associated with it to
		// prevent erroneous behavior.  Sets the device back to have a default status.
		USB_resetConfiguration();
		
		USBC_UDD_CTRL_EP_reset(); // Reset CTRL Endpoint (EP0)
		
		// Reset Endpoint Control Management
		USBC_UDD_CTRL_init(); // UDD Initialize CTRL
		mUSBC.bFlag.devDefault = 1;
		mUSBC.state.dev = USB_DEVICE_STATE_DEFAULT;
		
	}
	
	// UDD SUSPEND INTERRUPT //////////////////////////////////////////////////////////////////////
	else if (USBC->bf.UDINTE.bit.SUSPE && USBC->bf.UDINT.bit.SUSP)
	{
		 mUSBC.state.dev = USB_DEVICE_STATE_SUSPENDED;
		 USBC->bf.UDINTCLR.bit.SUSPC = 1;			// Clear Interrupt
		 USBC->bf.USBCON.bit.FRZCLK = 0;			// Unfreeze Clock
		 USBC->bf.UDINTECLR.bit.SUSPEC = 1;			// Disable Suspend Interrupt
		 USBC->bf.UDINTCLR.bit.WAKEUPC = 1;			// Clear Wakeup Interrupt
		 USBC->bf.UDINTESET.bit.WAKEUPES = 1;		// Enable Wakeup Interrupt
		 USBC->bf.USBCON.bit.FRZCLK = 1;			// Freeze Clock

		 // WAKEUP EVENT ACTION HERE
	}
	
	// UDD WAKEUP INTERRUPT ///////////////////////////////////////////////////////////////////////
	else if (USBC->bf.UDINTE.bit.WAKEUPE && USBC->bf.UDINT.bit.WAKEUP)
	{
		USBC->bf.UDINTCLR.bit.WAKEUPC = 1;			// Clear Interrupt
		USBC->bf.USBCON.bit.FRZCLK = 0;				// Unfreeze Clock
		while (!(USBC->bf.USBSTA.bit.CLKUSABLE));	// Block until clock usable
		USBC->bf.UDINTECLR.bit.WAKEUPEC = 1;		// Disable Wakeup Interrupt
		USBC->bf.UDINTCLR.bit.SUSPC = 1;			// Clear Suspend Interrupt
		USBC->bf.UDINTESET.bit.SUSPES = 1;			// Enable Suspend Interrupt
		
		// RESUME EVENT ACTION HERE
	}
	else {} // do nothing, no interrupts of interest triggered
	
	
	// Maybe put cleanup here?
	// This is called after the interrupt has run, a flag for setting 
	//if (mUSBC.state.trans == USB_TRANSACTION_STAGE_CLEANUP)
	//{
		//
	//}
	
	__DMB();
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EP interrupt handler(s)
///////////////////////////////////////////////////////////////////////////////////////////////////

//// NOTE........ IN THE CURRENT STATE THIS WONT WORK... NEED TO FIGURE OUT HOW TO ENBALE/DISABLE STUFF
// CORRECTLY.... I THINK THE TRICK IS THAT WE NEEDS TO ADD MORE ON TO THE END OF THE ALLOCATE FUNCTION
// TO GET EVERYTHIN INITALIZED CORRECTLY.

/// Random other note. data out (device -> host) EP interrupt should be disabled until the successful
// receipt of a data IN command instructing a data transfer for the device

uint8_t USB_EP_cleanupTransfer(uint8_t numEP);

// For multiblock transfer, this only occurs after a succsful transfer of bytes.
static uint8_t USB_EP_interrupt(void)
{
	uint8_t numEP;
	uint32_t* pUECFGn, pUECONn, pUECONnSET;
	
	// Sweep through all the endpoints to see if it was the one that was hit, if so, it needs to be processed
	for (uint8_t numEP = 1; numEP <= numEPmax; numEP++)
	{
		// If the interrupt is not enabled, then we need to move to next EP
		if (!(USBC->bf.UDINTE.reg & USBC_UDINT_EP_MASK(numEP))) continue;
		// Or, the interrupt is not currently triggered, move to next EP
		if (!(USBC->bf.UDINT.reg & USBC_UDINT_EP_MASK(numEP))) continue;

		// Disable the EP interrupt to prevent firing while meddling
		safeDisableIRQ();
		USBC->bf.UDINTECLR.reg = USBC_UDINT_EP_MASK(numEP); //
		safeEnableIRQ();	
		
		// Maybe in the future think about putting a pFunction here to do anything post interrupt?
		
		
		// This should flip-flop between SD buffers when shooting data across
		//return USB_EP_manageJob(numEP, bufferEP1Large);
		return SD_2_USB_jobManager();
	}
	// We got here two ways: a) there wasn't an EP interrupt b) something was wrong with it?
	return 0;
}


// Naming convention
// 

/*
// See section 8.5.2. for details on Bulk Transfer.

// Due the limitation of ATMEL's hardware, the maximum size of a USB transfer is maxEPsize
// (UINT16_MAX >> 1), which is due to the width of EP_TABLE.EPn_PCKSIZE_BK.MULTI_PACKET_SIZE.
// to handle transactions larger than this a construct called a job is created. Jobs automatically
// manage and reload the transfer buffer based on the characteristics of the data being transfered.
// Jobs can be thought of as a way to hold/track information status between individual transfers

// Jobs hold:
// - The length of the data to be transferred (of up to UINT32_MAX Bytes)
// - pointer to a smaller transfer buffer, reloaded upon each transfer completion
// - boolean indication of job enabled already or not
// - other variables for tracking where in the job they are.

// Based on the amount of data being transfered one (or more) transfers will be created. As
// described previously, these can be up to maxEPsize (or an artificial buffer limit) or less.
// Transfers are controlled by correctly loading data into the EP_Table. Due to the multi-packet
// controls of the hardware, loading/transferring data is very simple.

// The USB 2.0 specification only allows for transfers up to certain size (§5.5-8) based on the 
// type of EP (multi-packet only applies to ISO, BULK, and INT type interrupts). If the data
// transmitted is less than limits described by the spec, only one packet will be sent, otherwise,
// multiple will. These limits are configured automatically during EP allocation process (based
// on EP type) and are defined by [UECFGn.EPSIZE]. If the packet of data to be transfered is greater
// than that that limit each transaction will be broken up in to chunks and handled automatically,
//  without CPU intervention. This will continue until one of three end conditions is met (this
// should also applies to normal, non-multi-packet, transfers). Upon reaching one of the end
// conditions, the values stored int EP_TABLE[EPn] will be automatically cleared by hardware,
// disabling any further transactions until set again by the user.  These three conditions are:
//
// - A short (less than length limits described by the USB spec) packet was transmitted, this
//   indicates there was insufficient data to fill the buffer completely, meaning we are at the
//   very end of the data buffer
// - A packet has been successfully transmitted, EP_TABLE.MULTI_PACKET_SIZE = EP_TABLE.BYTE_COUNT
//   (and EP_TABLE.AUTO_ZLP = 0).  Meaning all the data request has been transfered out.
// - A ZLP has been auto sent for the last transfer, occurs when EP_TABLE.BYTE_COUNT is multiple
//   of [UECFGn.EPSIZE] and EP_TABLE.AUTO_ZLP = 1. - See section 8.5.3.2 for a description of this?
//   (still a bit unclear as the conditions for this to be used).

// IAW section 17.6.2..13 of the data sheet, the difference between Multi-packet and single-packet
// is that the value set in EP_TABLE.BYTE_COUNT is either greater than or less than the value of
// [UECFGn.EPSIZE].

// For Multi-PACKET Describe for in transaction packet increments upwards
// also how it increments downwards for OUT

// The following functions are used to manage/control transfers:
// createJob - Creates a job, which controls how much data is passed and gets everything setup for correct management
// manageJob - Manages job in between transfers.  Determines where in the job you are, and loads the packet accordingly, and cleansup when done with job.
// createTransfer - function pushes the transfers into the registers gets it setup
// cleanupTransfer - function resets the transfer back to default
// NOTE* interrupts and registers themselves are only handled at the transfer/interrupt level.
*/

uint8_t USB_EP_createTransfer(uint8_t numEP, uint8_t* pBuffer, uint16_t lenTransferBuffer);

// This code only sets up the first transfer, everything else is managed by the job reloader
// buffLengthLimit is used to artificially limit the size of the each transaction
// (for memory limitation concerns)
uint8_t USB_EP_createJob(uint8_t numEP, uint8_t* pBuffer, uint64_t lengthJob, uint16_t bufferLengthLimit)
{
	uint32_t* pUECONn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0, numEP));
	uint32_t* pUECFGn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
	
	// Error check
	if (numEP > numEPmax) return 0;
	if (numEP > numEPused) return 0;
	if ((USBC->reg.USBC_UERST & USBC_UERST_EPENn(numEP)) != USBC_UERST_EPENn(numEP)) return 0;
	if ((*pUECONn & USBC_UECON0_STALLRQ) == USBC_UECON0_STALLRQ) return 0;
	if ((*pUECFGn & USBC_UECFG0_EPTYPE_Msk) == USBC_UECFG0_EPTYPE_CONTROL) return 0;
	if (jobEP[numEP].bJobEnabled == 1) return 0;	// **NOTE** they disable globals for this...
		
	uint16_t numTransFirst;
	EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.all = 0ul; // Ensure all previous job info wiped
	
	// Adjust the buffer to be not more than maxEPsize (max allowed by hardware)
	if (bufferLengthLimit > maxEPsize) bufferLengthLimit = maxEPsize;
	
	// Check if job is multi-transfer, otherwise only single transfer required
	if (lengthJob > ((uint64_t)bufferLengthLimit))
	{
		// Need to adjust the length of the transfer down to a round integer multiple of
		// [UECFGn.EPSIZE], if not, the final packet of this transfer will be seen as a short
		// packet and hardware will automatically wipe the job before completed.
// 		uint16_t ep_size = (8 << ((*pUECFGn & USBC_UECFG0_EPSIZE_Msk) >> USBC_UECFG0_EPSIZE_Pos));
// 		jobEP[numEP].sizeNextTransfer = bufferLengthLimit - (bufferLengthLimit % ep_size);
		jobEP[numEP].sizeNextTransfer = bufferLengthLimit;
		jobEP[numEP].bLastTransfer = 0;	// More transfers to come
	}
	// If the job <= bufferLengthLimit no meddling required, set next transfer length
	else 
	{
		jobEP[numEP].sizeNextTransfer = (uint16_t)lengthJob;
		jobEP[numEP].bLastTransfer = 1;
	}
	// We are done determining buffer characteristics, load next (first) transfer into job manager
	jobEP[numEP].sizeTotalTransfered = 0;				// Starting a job, nothing transfered yet
	jobEP[numEP].sizeTotalPayload = lengthJob;			// push the total job length
	jobEP[numEP].bufferLengthLimit = bufferLengthLimit;	// save the max buffer size
	jobEP[numEP].pBuffer = pBuffer;						// push address of buffer head
	jobEP[numEP].bJobEnabled = 1;						// Enable job
	
	// Create the first transfer - job will be managed by manageJob function from now on
	if (USB_EP_createTransfer(numEP, pBuffer, jobEP[numEP].sizeNextTransfer) == 1) return 1;	
	
	return 0;	// if we got here, means something went wrong
}

// This function will manage job's - help reload and cleanup completed jobs as necessary
// will only hit this function AFTER an IN/OUT interrupt has fired.
uint8_t USB_EP_manageJob(uint8_t numEP, uint8_t* pBufferNew)
{
	uint32_t* pUECFGn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
	
	uint32_t* pUECONnSET	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0SET, numEP));
	uint32_t* pUECONnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0CLR, numEP));
	uint32_t* pUESTAnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UESTA0CLR, numEP));
	
	// Error check
	if (numEP > numEPmax) return 0;		// EP num greater than num available on device, major error
	if (numEP > numEPused) return 0;	// EP num greater than number available on interface
	if (jobEP[numEP].bJobEnabled == 0) return 0;	// if there is not job, we can't manage it
	// ADD MORE CHECK'S HERE
	
	// Increment the total number of Bytes transfered by number transfered in last transfer
	jobEP[numEP].sizeTotalTransfered += (uint64_t)jobEP[numEP].sizeNextTransfer;
	
	// If job done (Total amount transfered == total job size)
	if (jobEP[numEP].sizeTotalTransfered == jobEP[numEP].sizeTotalPayload)
	{
		// Reset the job manager
		jobEP[numEP].bJobEnabled = 0;	// disable job
		jobEP[numEP].sizeTotalTransfered = 0;
		jobEP[numEP].sizeTotalPayload = 0;
		jobEP[numEP].bufferLengthLimit = 0;
		jobEP[numEP].sizeNextTransfer = 0;
		jobEP[numEP].bLastTransfer = 0;
		jobEP[numEP].pBuffer = NULL;
		
		// Reset EP to default state
		return USB_EP_cleanupTransfer(numEP);
	}
	else // Job continues, at least one transfer remaining
	{
		uint64_t bytesRemaining = jobEP[numEP].sizeTotalPayload - jobEP[numEP].sizeTotalTransfered;
		
		if (bytesRemaining > ((uint64_t)jobEP[numEP].bufferLengthLimit)) // job still has >= 1 transfer left
		{
			// Need to adjust the length of the transfer down to a round integer multiple of
			// [UECFGn.EPSIZE], if not, the final packet of this transfer will be seen as a short
			// packet and hardware will automatically wipe the job before completed.
// 			uint16_t ep_size = (8 << ((*pUECFGn & USBC_UECFG0_EPSIZE_Msk) >> USBC_UECFG0_EPSIZE_Pos));
// 			jobEP[numEP].sizeNextTransfer = jobEP[numEP].bufferLengthLimit - (jobEP[numEP].bufferLengthLimit % ep_size);
			jobEP[numEP].sizeNextTransfer = jobEP[numEP].bufferLengthLimit;
			jobEP[numEP].bLastTransfer = 0; // More transfers to come
		}
		// If the job <= bufferLengthLimit no meddling required, set next transfer length, this is the last transfer before cleanup
		else
		{
			jobEP[numEP].sizeNextTransfer = (uint16_t)bytesRemaining;
			jobEP[numEP].bLastTransfer = 1;
		}
		// Create the next transfer
		return USB_EP_createTransfer(numEP, pBufferNew, jobEP[numEP].sizeNextTransfer);		
	}
}

uint8_t USB_EP_createTransfer(uint8_t numEP, uint8_t* pBuffer, uint16_t lenTransferBuffer)
{
	uint32_t* pUECONn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0, numEP));
	uint32_t* pUECFGn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
	
	uint32_t* pUECONnSET	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0SET, numEP));
	uint32_t* pUECONnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0CLR, numEP));
	uint32_t* pUESTAnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UESTA0CLR, numEP));
	
// 	// First, calculate the register of the EP
// 	uint32_t* pUESTAn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UESTA0, numEP));
	
	// Error check
	if (numEP > numEPmax) return 0;		// EP num greater than num available on device, major error
	if (numEP > numEPused) return 0;	// EP num greater than number available on interface
	if ((USBC->reg.USBC_UERST & USBC_UERST_EPENn(numEP)) != USBC_UERST_EPENn(numEP)) return 0;	// If EP !EN
	if ((*pUECONn & USBC_UECON0_STALLRQ) == USBC_UECON0_STALLRQ) return 0;						// If stall requested?
	if ((*pUECFGn & USBC_UECFG0_EPTYPE_Msk) == USBC_UECFG0_EPTYPE_CONTROL) return 0; // if endpoint is of CTRL type this function will not handle this.
	if (lenTransferBuffer > maxEPsize) return 0;				// individual transfer can't be bigger than this
	
	// Disable the EP interrupt (don't want to be called during this period.
	safeDisableIRQ();
	USBC->bf.UDINTECLR.reg = USBC_UDINT_EP_MASK(numEP);
	safeEnableIRQ();

	// I THINK THAT THIS SHOULD BE REMOVED.  JOBS SHOULD ONLY BE CREATED FOR HUGE (bigger than maxEP size items)
	// They should be managed at the job level, once a transfer is initialized (for smaller than jobs, nothing should mess it up).
	//jobEP[numEP].bJobEnabled = 1;
	
	// Clear out any existing information in the banks for this EP,
	// info will be set later based on the type of EP and transaction size.
	EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.all = 0ul;
	
	// Stop transfer by enabling the bank busy flag in the EP's [UECONn]
	// (they call this "locking emission of new packet") - will AUTO-NAK everything on this EP
	*pUECONnSET = USBC_UECON0SET_BUSY0S;
	
	// Determine EP direction and load data accordingly
	if ((*pUECFGn & USBC_UECFG0_EPDIR) == USBC_UECFG0_EPDIR_IN) // IN EP
	{
		
		*pUESTAnCLR = USBC_UESTA0CLR_TXINIC;	// Acknowledge the IN interrupt by clearing it			--- I THINK THESE SHOULD PROBABLY BE FLIPPED
		*pUECONnCLR = USBC_UECON0CLR_TXINEC;	// Disable IN Tx interrupt
		
		// TO BE ADDED - IF USING MULTI-BANK (PING-PONG) need to check value of [UESTAn.CURRBK]
		// to see which current bank is.
		
		// Determine type of interrupt -- populate for correct response
		if (((*pUECFGn & USBC_UECFG0_EPTYPE_Msk) == USBC_UECFG0_EPTYPE_BULK)
			|| ((*pUECFGn & USBC_UECFG0_EPTYPE_Msk) == USBC_UECFG0_EPTYPE_INTERRUPT))
		{
			// Regardless of multi-packet or not, data length set EP table as following for IN EP
			EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.MULTI_PACKET_SIZE = 0;
			EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = lenTransferBuffer;
			EP_Table[numEP].Bank[0].EPn_ADDRESS = pBuffer;
			
			if (lenTransferBuffer == 0) EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.AUTO_ZLP = 1;		 // I am not sure in which case this happens, but functionality is here to deal with it.
			
			// Clearing [FIFCON] allows controller to send bank contents, also Disable the busy bank
			// (AUTO-NAK disabled)
			// How does FIFOCON relate to Multi-packet and ping-pong?
			*pUECONnCLR = USBC_UECON0CLR_FIFOCONC | USBC_UECON0CLR_BUSY0C;
			
		}
		else if((*pUECFGn & USBC_UECFG0_EPTYPE_Msk) == USBC_UECFG0_EPTYPE_ISOCHRONOUS)
		{
			// Check on ISOCHRONOUS IN types of EPs only -- worry about later, see ATMEL's check
			return 0; // RETURN 1 when CORRECT
		}
		
		// Enable EP interrupts
		safeDisableIRQ();
		*pUECONnSET = USBC_UECON0SET_TXINES;				// Enable IN Tx interrupt
		USBC->bf.UDINTESET.reg = USBC_UDINT_EP_MASK(numEP);	// Enable EP interrupt
		safeEnableIRQ();
		
		return 1;		
	}
	else // OUT EP - TO BE FILLED LATER
	{
		/////////////////////////////////////////
		// THIS CODE HAS NOT BEEN TESTED
		/////////////////////////////////////////
		if (1 == 0) // for now don't let this code run, just some filler
		{
			*pUESTAnCLR = USBC_UESTA0CLR_RXOUTIC;	// Acknowledge the OUT interrupt by clearing it
			*pUECONnCLR = USBC_UECON0CLR_FIFOCONC;	// Clearing [FIFOCON] frees the current bank and switches to the next
			
			// STUFF GOES HERE
			
			
			// Enable EP interrupts
			safeDisableIRQ();
			*pUECONnSET = USBC_UECON0SET_RXOUTES;				// Enable OUT Rx interrupt
			USBC->bf.UDINTESET.reg = USBC_UDINT_EP_MASK(numEP);	// Enable EP interrupt
			safeEnableIRQ();
			
			return 1;
		}
		
		
		return 0;
		// if the EP direction is OUT, we need to enable the interrupt now?
	}
}

uint8_t USB_EP_cleanupTransfer(uint8_t numEP)
{
	uint32_t* pUECFGn		= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
	uint32_t* pUECONnSET	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0SET, numEP));
	uint32_t* pUECONnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0CLR, numEP));
	uint32_t* pUESTAnCLR	= (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UESTA0CLR, numEP));
	// Disable the EP interrupt (don't want to be called during this period, or until the next call from a CMD

	// Disable the EP interrupt (don't want to be called during this period.
	safeDisableIRQ();
	USBC->bf.UDINTECLR.reg = USBC_UDINT_EP_MASK(numEP);
	safeEnableIRQ();

	// Stop transfer by enabling the bank busy flag in the EP's [UECONn]
	// (they call this "locking emission of new packet") - will AUTO-NAK everything on this EP
	*pUECONnSET = USBC_UECON0SET_BUSY0S;
	
	// Clear out any existing information in the banks for this EP,
	// info will be set later based on the type of EP and transaction size.
	EP_Table[numEP].Bank[0].EPn_PCKSIZE_BK.all = 0ul;
	
	// Determine if we have an IN or OUT interrupt and the job is either ongoing or completed
	// IN, D->H, Get, Tx - See section 17.6.2.15 for overall description of code flow
	if ((*pUECFGn & USBC_UECFG0_EPDIR) == USBC_UECFG0_EPDIR_IN)
	{
		*pUESTAnCLR = USBC_UESTA0CLR_TXINIC;	// Acknowledge the IN interrupt by clearing it
		*pUECONnCLR = USBC_UECON0CLR_TXINEC;	// Disable IN Tx interrupt
		
		// anything else needed?
	}
	// OUT, H->D, Set, Rx
	else
	{
		*pUESTAnCLR = USBC_UESTA0CLR_RXOUTIC;	// Acknowledge the OUT interrupt by clearing it
		*pUECONnCLR = USBC_UECON0CLR_RXSTPEC;	// Disable OUT Rx interrupt
		
		// anything else needed?
	}
	
	// ALEX ADDED - NOT USED RIGHT NOW, BUT WOULD BE GOOD TO ADD
	//if (pFunctionEnd != NULL) pFunctionEnd();	// If we have a pointer function, call it
		
	return 1;
}

void USB_EP_abort(uint8_t numEP)
{
	// Disable the endpoint interrupt
	safeDisableIRQ();
	USBC->bf.UDINTECLR.reg = USBC_UDINT_EP_MASK(numEP);
	safeEnableIRQ();
	
	// Stop transfer by enabling the bank busy flag in the endpoints respective register
	uint32_t* pUECONnSET = (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0SET, numEP));
	*pUECONnSET = USBC_UECON0SET_BUSY0S;	// stuff a busy bit in there...
	
	// If there is a job enabled on an endpoint, we need to kill it. (Atmel did it slightly differently)
	jobEP[numEP].bJobEnabled = 0;
	jobEP[numEP].pBuffer = NULL;
}

USBC_ERROR_TYPE USB_EP_allocate(USB_DESCRIPTOR_ENDPOINT_Type* pDescEP)
{
	uint8_t	 numEP		= pDescEP->bEndpointAddress.addr;
	uint32_t UECFGn		= 0;
	
	// Check to see if the endpoint is already enabled
	if ((USBC->bf.UERST.reg & USBC_UERST_EPENn(numEP)) == USBC_UERST_EPENn(numEP)) return USBC_ERROR_UNKNOWN;
	
	// Create endpoint settings, translate pDescEP settings into UECFGn register settings
	UECFGn	= (uint32_t)(pDescEP->bmAttributes & 0x03) << USBC_UECFG0_EPTYPE_Pos // Type
			| (uint32_t)(pDescEP->bEndpointAddress.dir) << USBC_UECFG0_EPDIR_Pos // Direction
			| (uint32_t)(32u - __CLZ(pDescEP->wMaxPacketSize >> 4)) << USBC_UECFG0_EPSIZE_Pos; // Size
	
	// Set up the endpoint, first figure the correct address to write to
	uint32_t* pUECFGn = (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
	*pUECFGn = UECFGn;

	// point at the correct address and push to enable the busy bank
	uint32_t* pUECONnSET = (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECON0SET, numEP));
	*pUECONnSET = USBC_UECON0SET_BUSY0S;	/// no work? doesn't appear to do anything and doesn't seem to in their code either...

	// Enable the endpoint
	USBC->bf.UERST.reg |= USBC_UERST_EPENn(numEP);
	
	return USBC_SUCCESS;
}

static void USBC_UDD_CTRL_EP_reset(void)
{
	// Reset USB address to 0 and enable address
	USBC->bf.UDCON.reg &= ~USBC_UDCON_UADD_Msk;
	USBC->bf.UDCON.bit.ADDEN = 1;
	
	// Allocate and configure control endpoint
	USBC->bf.UECFG0.reg = USBC_UECFG0_EPTYPE_CONTROL | USBC_UECFG0_EPDIR_OUT | USBC_UECFG0_EPSIZE_64 | USBC_UECFG0_EPBK_SINGLE;
	
	// Use internal buffer for endpoint control
	EP_Table[0].Bank[0].EPn_ADDRESS = bufferCTRL_TxRx;			// Use internal buffer for endpoint control
	EP_Table[0].Bank[0].EPn_PCKSIZE_BK.MULTI_PACKET_SIZE = 0;	// don't use multi packet on endpoint control
	USBC->bf.UERST.reg |= USBC_UERST_EPEN0;						// enable End Point 0
	USBC->bf.UECON0CLR.bit.BUSY0C = 1;							// clear/disable busy bank 0
	
	safeDisableIRQ();
	// enable SETUP and OUT received interrupts
	USBC->bf.UECON0SET.reg = USBC_UECON0SET_RXSTPES | USBC_UECON0SET_RXOUTES;
	// Set EP0 Interrupt
	USBC->bf.UDINTESET.bit.EP0INTES = 1;
	safeEnableIRQ();
}
static void USBC_UDD_CTRL_init(void) // Function used to reset/initialize CTRL EP
{
	// In case of abort of IN Data Phase:
	// No need to abort IN transfer (TXINI = 1), because it is automatically done by hardware when
	// a SETUP packet is received.
	// But the interrupt must be disabled to don't generate interrupt TXINI after SETUP reception.
	USBC->bf.UECON0CLR.bit.TXINEC = 1; // Clear/Disable transmitted data in [TXINE] interrupt
	// In case of OUT ZLP event is not processed before Setup event occurs
	USBC->bf.UESTA0CLR.bit.RXOUTIC = 1;
	
	// I ADDED THIS STUFF: Since this is designed to set the EP back to receive a SETUP packet,
	// need to wipe unnecessary UESTA flags.
	USBC->bf.UESTA0CLR.reg = USBC_UESTA0CLR_NAKOUTIC | USBC_UESTA0CLR_NAKINIC;
	
	// Reset the structure that holds information about the control request.
	udd_response.bMoreData = 0;
	pFunction = NULL;	// Point the function at nothing...
	mUSBC.state.EP[0] = UDD_EP_SETUP;
	mUSBC.state.trans = USB_TRANSCATION_STATE_WAITING;
}

static USBC_ERROR_TYPE USBC_UDD_CTRL_SETUP_Rx(void)
{
	// Error Check, if not awaiting SETUP packet set to said state and continue
	if (mUSBC.state.EP[0] != UDD_EP_SETUP) USBC_UDD_CTRL_init();
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	// SETUP STAGE (SETUP Packet received)
	///////////////////////////////////////////////////////////////////////////////////////////////
	mUSBC.state.trans = USB_TRANSACTION_STAGE_SETUP;
	
	// Check to make sure packet is of correct data length
	if (EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT != sizeof(USB_SETUP_REQUEST_Type))
	{
		USBC_UDD_CTRL_STALL_data();			// Stall all packets on EP0 IN/OUT Interrupts
		USBC->bf.UESTA0CLR.bit.RXSTPIC = 1; // Clear Setup request
		return USBC_ERROR_UNKNOWN;			// Error, data != expeceted size for this type of request
	}
	
	// DECODE/PROCESS SETUP REQUEST ///////////////////////////////////////////////////////////////

	// Stuff data that was received in the buffer into a request and process it...
	USB_SETUP_REQUEST_Type* pReq = (USB_SETUP_REQUEST_Type*)(bufferCTRL_TxRx);
	
	// Process the SETUP packet received
	// Test to see if the direction of the packet was incorrect (device --> host)
	
	// Process the request from SETUP packet get ready for next Stage, if anything but SUCCESS STALL
	if (USBC_UDC_SETUP_process(pReq) != USBC_SUCCESS)
	{
		USBC_UDD_CTRL_STALL_data();			// Stall all packets on IN & OUT CTRL endpoint
		USBC->bf.UESTA0CLR.bit.RXSTPIC = 1; // Clear SETUP received Flag, opens for next setup packet
		return USBC_ERROR_UNKNOWN;			// Error, jump out
	}
	// We've processed the setup request packet and need to move to the next step, first:
	USBC->bf.UESTA0CLR.bit.RXSTPIC = 1;	// Clear the Setup received packet.
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	// GET READY FOR DATA STAGE (Optional)
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	// We first have to determine if the data packet is an IN or an OUT type at this point.  This
	// is done by looking at the request, specifically, the [bmRequestType.Direction] bit value.
	// <1> means IN/GET <0> means OUT/SET.
	
	// A few potential routes at this point:
	// - We have one normal data packet
	// - We have a couple data packets as the data length exceeds what the protocol is capable of
	// - The data phase in skipped (occurs when [wLength] = 0) and the status stage begins (ZLP)
	
	// Note, data/status stages are always of opposite types.  If the transaction was of
	// IN/GET/D2H type the Data Stage will be of that type, and the Status Stage will be of the
	// OUT/SET/H2D type.  Conversely, if the Data Stage is of the OUT/SET/H2D type, the Status
	// Stage will be of the IN/GET/D2H type.  Additionally, if the request doesn't expect any data
	// back (as in the SET_ADDRESS request) the Data OUT Stage will be skipped, but the Status
	// Stage is still expected to obey this flip flop nature and respond accordingly.	
	
	// Requested an IN/GET/D2H Data Stage, get ready for it
	if (pReq->bmRequestType.dir == USB_REQ_DIR_IN_D2H)
	{
		mUSBC.state.EP[0] = UDD_EP_DATA_IN;				// Set to expect an IN packet
		mUSBC.state.trans = USB_TRANSACTION_STAGE_DATA;	// We are headed into the Data stage
		USB_CTRL_bufferWrite();							// Write the response to the CTRL TxRx buffer 
	}
	else // Requested an OUT/SET/H2D Data Stage
	{
		// If request had NO data length requested, OUT Data Phase is being skipped: jump straight
		// to STATUS stage by sending an IN ZLP to acknowledge the request
		if (pReq->wLength == 0) 
		{
			USBC_UDD_CTRL_IN_ZLP_Tx();
			return USBC_SUCCESS;
		}
		
		// They did some countdown priming stuff, which I don't know if we want to replicate.......
		
		mUSBC.state.EP[0] = UDD_EP_DATA_OUT;
		mUSBC.state.trans = USB_TRANSACTION_STAGE_DATA;

		
		// For detecting protocol errors
		safeDisableIRQ();
		USBC->bf.UESTA0CLR.bit.NAKINIC = 1;	// Clear NAK IN Flag
		USBC->bf.UECON0SET.bit.NAKINES = 1;	// Enable NAK IN Interrupt
		safeEnableIRQ();
	}	
	return USBC_SUCCESS;
}

USBC_ERROR_TYPE USB_CTRL_bufferWrite(void)
{
	// Disable DATA IN Tx interrupt so that we can't be interrupted in the middle of getting
	// the data into the buffer
	safeDisableIRQ();
	USBC->bf.UECON0CLR.bit.TXINEC = 1;
	
	// We are trying to write data of X size into the buffer, there are 3 cases:
	// 1) Data to Tx <= Max packet size, only one transfer necessary
	// 2) Data to Tx = 0 also falls in here, ZLP case, no data is actually stuffed
	if (udd_response.sizePayload <= USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE)
	{
		// Move the data from the response into the buffer
		memcpy(bufferCTRL_TxRx, udd_response.pPayload, udd_response.sizePayload);
		EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = udd_response.sizePayload;
		
		// Payload has been loaded, indicate there is no payload left to load..
		udd_response.sizePayload = 0;
		udd_response.bMoreData = 0;
		
		// NOTE (TO BE DELETED): this will prime the next DATA IN interrupt to send data back to the host
		// at that point we need to push the device to transition to the next state by sending a handshake WAIT ZLP
	} 
	// 3) Data to Tx > the max packet size, must do multiple transfers:
	else
	{
		memcpy(bufferCTRL_TxRx, udd_response.pPayload, USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE);
		EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE;
		
		udd_response.pPayload += USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE;		// Tick to the next good data location
		udd_response.sizePayload -= USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE;		// Countdown the payload
		
		udd_response.bMoreData = 1;
	
		// NOTE (TO BE DELETED): this will prime the next DATA IN interrupt to send data back to the host,
		// but we need to have something that causes it to come back to this function until the payload is
		// empty
	}
	
	// Enable transmission of IN (Normal Data) - doing this allows the device to put out data...
	USBC->bf.UESTA0CLR.bit.TXINIC = 1;	// Clear IN Data Tx flag, for CTRL EPs this will send the packet.
	USBC->bf.UECON0SET.bit.TXINES = 1;	// Enable IN Data Tx Interrupt, for the next time we hit an IN interrupt
	
	safeEnableIRQ();
	
	return USBC_SUCCESS;
}
USBC_ERROR_TYPE USB_CTRL_bufferRead(void)
{
	safeDisableIRQ();
	USBC->bf.UECON0CLR.bit.RXOUTEC = 1;	// Disable OUT Data Rx Interrupt while meddling
	safeEnableIRQ();
	
	// Count the number of bytes received in the last packet
	uint16_t numBytesRx = EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT;
	
	// We are trying to read data of X size from the buffer, there are 3 cases:
	// 1) Data Rx'd < Max packet size, only one transfer necessary
	// 2) Data Rx'd = 0 also falls in here, ZLP case, no data is actually pulled
	if (numBytesRx < USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE)
	{
		// Error check to make sure we got the expected amount of data
		if (numBytesRx == udd_response.sizePayload) // if Rx'd expected amount of data
		{
			// Move the data received from the EP_buffer into desired location (determined by request)
			memcpy(udd_response.pPayload, bufferCTRL_TxRx, udd_response.sizePayload);
			
			// Reset, no more data expected
			EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = 0;
			udd_response.sizePayload = 0;
			udd_response.bMoreData = 0;
		}
		else // error, data length mismatch - THIS IS AN UNDERFLOW CONDITION
		{
			return USBC_ERROR_UNKNOWN;	// There was an error, something went wrong
			// PROBABLY NEED TO DO SOMETHING HERE?
		}
	}
	// 3) Data Rx'd = max packet size, must do multiple transfers:
	else if (numBytesRx == USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE)
	{
		memcpy(udd_response.pPayload, bufferCTRL_TxRx, USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE);
		
		EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = 0;					// reset for next OUT data packet
		udd_response.bMoreData = 1;											// More Data expected		
		udd_response.pPayload += USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE;	// Tick to the next good data destinations - kind of dangerous doing this way, could overflow...
		udd_response.sizePayload -= USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE;	// Countdown the payload
	}
	else // THIS IS AN OVERFLOW CONDITION, DATA RECIEVED WAS BIGGER THAN THE BUFFER
	{
		return USBC_ERROR_UNKNOWN;	
	}
	
	// We need to figure out if this is even necessary at all. or this will all be handled above in a if statement............. ------------------
	safeDisableIRQ();
	// Enable transmission of IN (Normal Data) - doing this allows the device to put out data...
	USBC->bf.UESTA0CLR.bit.RXOUTIC = 1;	// Clear OUT Data Rx flag, for CTRL EPs this will send the packet.
	USBC->bf.UECON0SET.bit.RXOUTES = 1;	// Enable OUT Data Rx Interrupt, for the next time we hit an OUT interrupt
	safeEnableIRQ();

	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USBC_UDD_CTRL_IN_Tx(void)
{
	safeDisableIRQ();
	USBC->bf.UECON0CLR.bit.TXINEC = 1;	// Disable IN Data Tx Interrupt
	safeEnableIRQ();
	
	if (mUSBC.state.EP[0] == UDD_EP_DATA_IN) // If hit the awaited DATA IN interrupt
	{
		// Two Choices:
		// 1) If done data to buffer writing, get ready for ZLP The host will be sending.
		// NOTE: The first OUT packet received in status stage of a CTRL read will always result
		// in a NAK handshake packet the first time around. See pg. 354 of data sheet
		// 2) Not done writing, get the next data loaded, 
		if (udd_response.bMoreData == 0) USBC_UDD_CTRL_OUT_ZLP_Rx(); // If no more data, break out.
		else USB_CTRL_bufferWrite();
		
		return USBC_SUCCESS;
	} 
	// If a ZLP IN, indicates end of transaction has occurred: reset EP to default state
	else if (mUSBC.state.EP[0] == UDD_EP_STATUS_IN_ZLP) 	
	{
		if (pFunction != NULL) pFunction();	// If we have a pointer function, call it
		
		USBC_UDD_CTRL_init();				// Transaction complete.
		return USBC_SUCCESS;
	}
	// Test to see if we OUT Packet Rx, which means the DATA IN Phase is being aborted by an OUT ZLP
	else if (USBC->bf.UESTA0.bit.RXOUTI == 1)
	{
		
	}
	else {}	// do nothing..
	
	// We have to determine where in the buffer we actually are, so we can load in the right data

	
	
// 	if ()
// 	{
// 		__enable_irq();
// 		mUSBC.state.EP[0] = UDD_CTRL_EP_HANDSHAKE_WAIT_OUT_ZLP;
// 		return USBC_SUCCESS;
// 	}
	
	// At this point the DATA STAGE WILL BE COMPLETED. What 
	// In Packet is received from the host, which triggered this interrupt
	// The data was stuffed from the temporary payload buffer into the endpoint udd_buffer
	// an IN packet automatic Status packet will
	// be sent as well, ending the SETUP STAGE.


	// We need to figure out if this is even necessary at all. or this will all be handled above in a if statement............. ------------------
	safeDisableIRQ();
	// Enable transmission of IN (Normal Data) - doing this allows the device to put out data...
	USBC->bf.UESTA0CLR.bit.TXINIC = 1;	// Clear IN Data Tx flag, for CTRL EPs this will send the packet.
	USBC->bf.UECON0SET.bit.TXINES = 1;	// Enable IN Data Tx Interrupt, for the next time we hit an IN interrupt
	safeEnableIRQ();
	
	return USBC_SUCCESS;
}
static USBC_ERROR_TYPE USBC_UDD_CTRL_OUT_Rx(void)
{	
	if (mUSBC.state.EP[0] != UDD_EP_DATA_OUT) // If not OUT data stage, we need to determine what to do
	{
		// If at the DATA IN expected state, or waiting for the handshake ZLP OUT expected state,
		// we are at the end of CTRL request
		if ((mUSBC.state.EP[0] == UDD_EP_STATUS_OUT_ZLP) || (mUSBC.state.EP[0] == UDD_EP_DATA_IN))
		{
			// End of SETUP request as a result of:
			// - Data IN Phase aborted,
			// - last Data IN Phase hidden by ZLP OUT sending quickly,
			// - ZLP OUT received normally.
			
			// THEY HAD THIS HIT A CALLBACK... WHICH WASN'T ACTUALLY POPULATED...
			
			// The way that ATMEL designed this controller to work, is that if this is the STATUS
			// stage, OUT here is a response to a previous IN request. it is a ZLP.  The controller
			// will auto NAK the first OUT packet and actually fire the second.  This interrupt occurs
			// after the second.  The NAK OUT will have been triggered by the first hardware NAK.
			// need to clear it.
			USBC->bf.UESTA0CLR.reg = USBC_UESTA0CLR_NAKOUTIC;
		}
		else // We hit a protocol error in during a SETUP request?
		{
			USBC_UDD_CTRL_STALL_data();
		}
		USBC_UDD_CTRL_init();
		return USBC_SUCCESS;
	}
	else // normal DATA Stage, OUT was Rx'd
	{
		// Pull the data from the EP buffer
		USB_CTRL_bufferRead();
		
		// If no more data expected, enable transmission of handshake packet
		if (udd_response.bMoreData == 0) USBC_UDD_CTRL_IN_ZLP_Tx();
		
		return USBC_SUCCESS;
	}
	
	// if we got here, something went very wrong
	return USBC_ERROR_UNKNOWN;
}

/////////////////////////
// 
////////////////////
static void USBC_UDD_CTRL_IN_ZLP_Tx(void)
{
	mUSBC.state.EP[0] = UDD_EP_STATUS_IN_ZLP;			// State Waiting for a ZLP IN
	mUSBC.state.trans = USB_TRANSACTION_STAGE_STATUS;	// 
	EP_Table[0].Bank[0].EPn_PCKSIZE_BK.BYTE_COUNT = 0;	// Set ZLP (length = 0)
	
	safeDisableIRQ();
	
	// Enable transmission of IN (ZLP)
	USBC->bf.UESTA0CLR.bit.TXINIC = 1;		// Clear IN Data Tx flag
	USBC->bf.UECON0SET.bit.TXINES = 1;		// Enable IN Data Tx Interrupt
	
	// For Detecting Protocol Error enable NAK Interrupt on the OUT Data Phase?
	USBC->bf.UESTA0CLR.bit.NAKOUTIC = 1;	// Clear NAK OUT flag
	USBC->bf.UECON0SET.bit.NAKOUTES = 1;	// Enable NAK OUT interrupt
	
	safeEnableIRQ();
}
static void USBC_UDD_CTRL_OUT_ZLP_Rx(void)
{
	mUSBC.state.EP[0] = UDD_EP_STATUS_OUT_ZLP;	// Waiting for a ZLP OUT
	
	safeDisableIRQ();
	// For Detecting Protocol Error, enable NAK Interrupt on the Data IN Phase?
	USBC->bf.UESTA0CLR.bit.NAKINIC = 1;	// Clear IN Data NAK Tx flag
	USBC->bf.UECON0SET.bit.NAKINES = 1;	// Enable IN Data NAK Tx Interrupt
	safeEnableIRQ();
}

void USBC_UDD_CTRL_STALL_data(void)
{
	// Stall all packets on IN & OUT CTRL endpoint
	mUSBC.state.EP[0] = UDD_EP_STALL_REQ;
	USBC->bf.UECON0SET.bit.STALLRQS = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Type Requests
///////////////////////////////////////////////////////////////////////////////////////////////////
static USBC_ERROR_TYPE USBC_UDC_SETUP_process(USB_SETUP_REQUEST_Type* pReq)
{
	// This function sort's each request received based on the type of request.  Failure will return an error
	// and cause a return of stall.
	
	// If the direction device to host on a setup packet, and we have a wLength of 0, Error return fail, cause stall
	if (pReq->bmRequestType.dir == USB_REQ_DIR_IN_D2H) if (pReq->wLength == 0) return USBC_ERROR_INCORRECT_DIR;
	
	// Standard Request type, ---  The way they do this seems odd to me... why use type and not recipient?--------------
	// At this point, it seems like this would catch every thing coming through? Very odd? maybe a bug on their part?
	if (pReq->bmRequestType.type == USB_REQ_TYPE_STANDARD) return USBC_UDC_processStdReq(pReq);
	else if (pReq->bmRequestType.type == USB_REQ_TYPE_VENDOR) return USBC_UDC_processVendorReq(pReq);
	else if (pReq->bmRequestType.type == USB_REQ_TYPE_CLASS) return USBC_ERROR_UNKNOWN; // placeholder
	else { return USBC_ERROR_UNKNOWN; } // the value of bmRequestType.type was reserved or unknown, we jump out with an error
}
static USBC_ERROR_TYPE USBC_UDC_processStdReq(USB_SETUP_REQUEST_Type* pReq)	// This function processes all standard requests
{
	// I really don't like this setup, seems like you are subjecting each branch to more if statements than necessary.
	// I think that this should be a switch/case for all the standard values of bRequest.  Sorting between Device,
	// Interface, and Endpoint should occur after sorting it out into the correct Request type.
	
	if (pReq->bmRequestType.dir == USB_REQ_DIR_IN_D2H)	// GET Request
	{
		if (pReq->wLength == 0) return USBC_ERROR_UNKNOWN;

		if (pReq->bmRequestType.recipient == USB_REQ_RECIP_DEVICE)
		{
			if (pReq->bRequest == USB_bRequest_GET_STATUS) return USBC_UDC_stdReq_getStatus(pReq);
			else if (pReq->bRequest == USB_bRequest_GET_DESCRIPTOR) return USBC_UDC_stdReq_getDesc(pReq);
			else if (pReq->bRequest == USB_bRequest_GET_CONFIGURATION) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else {}	// Something wrong
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_INTERFACE)
		{
			if (pReq->bRequest == USB_bRequest_GET_STATUS) {return USBC_ERROR_CMD_NOT_SUPPORTED;}
			//else if (pReq->bRequest == USB_bRequest_GET_INTERFACE) {return USBC_ERROR_CMD_NOT_SUPPORTED;}	/// THIS CONDITION DIDN'T EXIST IN THEIR CODE
			//else {}	// something wrong
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_ENDPOINT)
		{
			if (pReq->bRequest == USB_bRequest_GET_STATUS) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else if (pReq->bRequest == USB_bRequest_SYNCH_FRAME) return USBC_ERROR_CMD_NOT_SUPPORTED;	/// THIS CONDITION DIDN'T EXIST IN THEIR CODE... seems different
			else {}	// something wrong			
 		}
		else {} // we got some issues.....
	}
	else // SET Request
	{
		if (pReq->bmRequestType.recipient == USB_REQ_RECIP_DEVICE)
		{
			if (pReq->bRequest == USB_bRequest_SET_ADDRESS) return USBC_UDC_stdReq_setAddr(pReq);
			else if (pReq->bRequest == USB_bRequest_CLEAR_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else if (pReq->bRequest == USB_bRequest_SET_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else if (pReq->bRequest == USB_bRequest_SET_CONFIGURATION) return USBC_UDC_stdReq_setConfig(pReq);
			else if (pReq->bRequest == USB_bRequest_SET_DESCRIPTOR) return USBC_ERROR_CMD_NOT_SUPPORTED;	// No action take in code, listed as optional in USB 2.0 Spec
			else {}	// Something wrong
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_INTERFACE)
		{
			if (pReq->bRequest == USB_bRequest_SET_INTERFACE) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else if (pReq->bRequest == USB_bRequest_CLEAR_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;	// THIS CONDITION DIDN'T EXIST IN THEIR CODE - There is a not that current USB 2.0 spec doesn't specify such interface features
			else if (pReq->bRequest == USB_bRequest_SET_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;		// THIS CONDITION DIDN'T EXIST IN THEIR CODE
			else {}	// something wrong
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_ENDPOINT)
		{
			if (pReq->bRequest == USB_bRequest_CLEAR_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else if (pReq->bRequest == USB_bRequest_SET_FEATURE) return USBC_ERROR_CMD_NOT_SUPPORTED;
			else {}	// something wrong
		}
		else {} // we got some issues.....
	}
	
	return USBC_ERROR_REQUEST_UKNOWN;
}
static USBC_ERROR_TYPE USBC_UDC_stdReq_getStatus(USB_SETUP_REQUEST_Type* pReq)
{
	udd_response.pPayload = (uint8_t*)(&mUSBC.status);
	udd_response.sizePayload = 2;
	
	udd_response.sizePayload = computePayloadLength(pReq, udd_response.sizePayload);

	return USBC_SUCCESS;
}
static USBC_ERROR_TYPE USBC_UDC_stdReq_getDesc(USB_SETUP_REQUEST_Type* pReq)
{
	// wValue for a Get Descriptor Request is composed of a Type in the MSB and a Index in the LSB
	uint8_t descIndex				= (uint8_t)(pReq->wValue & 0x00FF);						// Kill the MSB
	USB_DESCRIPTOR_TYPE descType	= (USB_DESCRIPTOR_TYPE)((uint8_t)(pReq->wValue >> 8));	// Kill the LSB
	
	// Determine the requested Descriptor type and load the appropriate response into the the device.
	if (descType == USB_DESCRIPTOR_DEVICE)
	{
		udd_response.pPayload = (uint8_t*)(&devDescriptor);
		udd_response.sizePayload = sizeof(devDescriptor);
	}
	else if (descType == USB_DESCRIPTOR_CONFIGURATION)
	{
		// Push the payload
		udd_response.pPayload = (uint8_t*)(&configResp);
		udd_response.sizePayload = configResp.config.wTotalLength;
	}
	else if (descType == USB_DESCRIPTOR_STRING)
	{
		if (descIndex == 0x00)
		{
			udd_response.pPayload = (uint8_t*)(stringTestArray); // This is the English language response
			udd_response.sizePayload = stringTestArray[0];
		}
		else if (descIndex == 0x01)
		{
			udd_response.pPayload = (uint8_t*)(manfName);
			udd_response.sizePayload = manfName[0];
		}
		else if (descIndex == 0x02)
		{
			udd_response.pPayload = (uint8_t*)(devName);
			udd_response.sizePayload = devName[0];
		}
		else if (descIndex == 0x03)
		{
			udd_response.pPayload = (uint8_t*)(int0Name);
			udd_response.sizePayload = int0Name[0];
		}
		else if (descIndex == MS_STRING_DESCRIPTOR_INDEX)
		{
			udd_response.pPayload = (uint8_t*)(MS_OS_10_stringDescriptor);
			udd_response.sizePayload = MS_OS_10_stringDescriptor[0];
		}
		else // TO BE REMOVED? OR DO SOMETHING ELSE FOR IF WE DON'T HAVE THE descIndex
		{ udd_response.sizePayload = 0;}
		
	}
	else if (descType == USB_DESCRIPTOR_DEVICE_QUALIFIER) return USBC_ERROR_UNKNOWN;	
	else {}
	// Going to have to add more here for the other types listed in USB_DESCRIPTOR_TYPE / Table 9-5
	

	// Compute the length of the payload that will be sent, 	
	udd_response.sizePayload = computePayloadLength(pReq, udd_response.sizePayload);

	return USBC_SUCCESS;
}
static USBC_ERROR_TYPE USBC_UDC_stdReq_setAddr(USB_SETUP_REQUEST_Type* pReq)	// §9.4.6
{
	// The actual changing of the address needs to occur AFTER the handshake of this transaction
	// is completed.  They install a callback and run it after the completion of the transaction
	// to change the address
	
	// wIndex and wLength must be 0, and wValue (address) can't be greater than 127 IAW USB2.0 spec
	if ((pReq->wIndex != 0) || (pReq->wLength != 0) || ((pReq->wValue & 0xFF80) != 0)) return USBC_ERROR_UNKNOWN;
	
	mUSBC.addr = (uint8_t)(pReq->wValue & 0x007F); // Filter address for pushing after transaction completion
	pFunction = USB_setAddress;
	return USBC_SUCCESS;
}
void USB_setAddress(void)
{
	//USBC->bf.UDCON.bit.ADDEN = 0;	// DOES NOTHING, [USBC.UDCON.ADDEN] specifically states only set to 0 by RESET
	USBC->bf.UDCON.bit.UADD = mUSBC.addr;		// Set address
	USBC->bf.UDCON.bit.ADDEN = 1;				// Enable address
	mUSBC.state.dev = USB_DEVICE_STATE_ADDRESS;	// Address assigned
}
static USBC_ERROR_TYPE USBC_UDC_stdReq_getConfig(USB_SETUP_REQUEST_Type* pReq)
{

	return USBC_SUCCESS;
}
static USBC_ERROR_TYPE USBC_UDC_stdReq_setConfig(USB_SETUP_REQUEST_Type* pReq)
{
	// This function usually occurs after the device has told the host of all of it's potential
	// configurations (generally just one).  The host determines which configuration to set the
	// device and instructs to do so.  This is usually the last step in the enumeration process.
	
	// All the device has to do in response to this command is to do some basic error checking to
	// ensure that we are in the right place in the enumeration process and that the configuration
	// sent by the host is correct.
	
	// If the length of the packet != 0, or the address hasn't been set yet have an error;
	if ((pReq->wLength != 0) || (USBC->bf.UDCON.bit.UADD == 0)) return USBC_ERROR_UNKNOWN;
	
	uint8_t requestedConfig = pReq->wValue & 0xFF;
	
	// Check to ensure the configuration number sent from the host is not greater than the number
	// of configurations actually available on the device (which is 1)
	if (requestedConfig > devDescriptor.bNumConfigurations) return USBC_ERROR_UNKNOWN;
	
	// Reset the current Configuration (doing so clears all Interfaces and Endpoints associated
	// as well)
	USB_resetConfiguration();
	
	// Now that everything is set back to default (no Configuration selected and thus no associated
	// Interfaces or Endpoints) we can enable the Configuration requested by the Host (plus the 
	// associated Interface(s) and Endpoint(s))
	
	// If the requested configuration was to the default empty one, then we're done, as the global
	// was already set to that state by resetConfig()
	if (requestedConfig == 0) return USBC_SUCCESS;

	// Based on the config number, we need to determine the number of interfaces that it requires
	// to operate. This functionality will be updated in the future, for now just do it directly -------------------
	// I think the way to fix this for endpoints (and interfaces) is to nest them?
	
// 	for (uint8_t numInterface = 0; numInterface < configResp.config.bNumInterfaces; numInterface++)
// 	{
// 		// enable interface
// 	}

	USB_EP_allocate((USB_DESCRIPTOR_ENDPOINT_Type*)&(configResp.EP1));
	

	// Should be noted, that the interrupt for this EP has not been enabled at this point, nor has
	// the descriptor table for the EP been filled out.
	
	// I think the best course of action is to actually do all the setting up necessary without
	// dealing with the interrupt functionality, it just convolute what is actually going on.
	
	// The issue is that you almost want to do this in a couple steps:
	// 1) allocate - which basically sets the registers to enable the EP
	// 2) setup the EP table default descriptors (and main TxRx buffer?) with default values
	// 3) install the "interface", or the code that will basically will handle what happens when we
	//    get and need to send data. In the case of CDC example, this is where the code to hand data
	//	  to/from the UART is installed (basically bridging the gap)
	// 4) enable the interrupt, waiting for next "job" to show up.
	
	// Set the requested configuration as the active one.
	mUSBC.numActiveConfiguration = requestedConfig;	
	
	return USBC_SUCCESS;
}


void USB_resetConfiguration(void)
{
	// This function is designed to clear out any existing configuration information (everything
	// below and including, the configuration descriptor level).  It can be used either upon device
	// RESET signal received, USB being unplugged (Vbus going low) or upon a setConfiguration
	// request (where configuration is changed).
	
	// If the current configuration is !0, device is in initialized state.  All interfaces must
	// be cleared. Under interfaces there are endpoints, which are cleared as well...
	
	// The way ATMEL wrote this code was that this routine would determine if we are in an active
	// configuration, then go through to determine the current active interface (i think?), and
	// from it's info from the interface descriptor, pull the endpoint descriptors and abort them
	// one by one (until it hit the end? via funding a NULL?).
	
	// This method is dumb as shit, the problem is while you go through determining all this stuff
	// endpoint could be getting hit and doing crap.  The best way is first thing to go through all
	// the endpoints available and killing them all.  Then play any games necessary to get the
	// interfaces and configurations correct.
	
// 	if (mUSBC.numActiveConfiguration > 0)
// 	{
// 		for (uint8_t numInterface = 0; numInterface < configResp.config.bNumInterfaces; numInterface++)
// 		{
// 			// do a thing, disable an interface ---- needs to be investigated and sussed out 
// 			
// 		}
// 	}

	// Kill all jobs on EPs (not EP0) and disable them as well.  There is no need to clear the
	// Respective UECFGn register, as it automatically cleared upon receiving a USB reset...
	// but a call from setting a new configuration will not do such a thing... best to wipe them anyway...
	
	uint32_t* pUECFGn;
	for (uint8_t numEP = 1; numEP < numEPused; numEP++)
	//for (uint8_t numEP = 1; numEP <= numEPmax; numEP++) //THIS SHOULD BE numEPMAX at somepoint, need to make sure I don't overflow.
	{
		USB_EP_abort(numEP);
		USBC->bf.UERST.reg &= ~USBC_UERST_EPENn(numEP);	// UDD disable endpoint - this should wipe out UECFGn, UESTAn, UECONn of the EP
		pUECFGn = (uint32_t*)(USBC_ADDR_EP_OFFSET(USBC_ADDR_UECFG0, numEP));
		*pUECFGn = USBC_UECFG0_RESETVALUE;
	}

	// set to not having a configuration
	mUSBC.numActiveConfiguration = 0;

	// reset the status of the device to default state (MAY NEED TO CHANGE BASED ON DESIRED DEVICE DEFUALT STATUS)...........................
	mUSBC.status.all = 0;
}

uint16_t computePayloadLength(USB_SETUP_REQUEST_Type* pReq, uint16_t desiredLength)
{
	// Sometimes request will ask for a descriptor, but not the full length of it.
	// this function determines the length that was actually requested.  If
	
	if (pReq->wLength < desiredLength) return pReq->wLength;
	else return desiredLength;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Vendor Type Requests
///////////////////////////////////////////////////////////////////////////////////////////////////

// Microsoft WinUSB Specific Requests//////////////////////////////////////////////////////////////
static USBC_ERROR_TYPE USB_MSReq_getDevDesc(USB_SETUP_REQUEST_Type* pReq)
{
	if (pReq->wIndex == 0x0004) // Extended Compat ID
	{
		udd_response.pPayload = (uint8_t*)(MS_extendedCompatID_featureDescriptor);
		udd_response.sizePayload = MS_extendedCompatID_featureDescriptor[0];
	}
	else if (pReq->wIndex == 0x0005) // Extended Properties
	{
		udd_response.pPayload = (uint8_t*)(MS_extendedPropertiesOS_featureDescriptor);
		udd_response.sizePayload = MS_extendedPropertiesOS_featureDescriptor[0];
	}
	else {}
	
	udd_response.sizePayload = computePayloadLength(pReq, udd_response.sizePayload);
	
	return USBC_SUCCESS;
}
static USBC_ERROR_TYPE USB_MSReq_getIntDesc(USB_SETUP_REQUEST_Type* pReq)
{
	if (pReq->wIndex == 0x0004) // Extended Compat ID
	{
		udd_response.pPayload = (uint8_t*)(MS_extendedCompatID_featureDescriptor);
		udd_response.sizePayload = MS_extendedCompatID_featureDescriptor[0];
	}
	else if (pReq->wIndex == 0x0005) // Extended Properties
	{
		udd_response.pPayload = (uint8_t*)(MS_extendedPropertiesOS_featureDescriptor);
		udd_response.sizePayload = MS_extendedPropertiesOS_featureDescriptor[0];
	}
	else {}
	
	udd_response.sizePayload = computePayloadLength(pReq, udd_response.sizePayload);
	
	return USBC_SUCCESS;
}

// HeartPulse Specific commands ///////////////////////////////////////////////////////////////////

// This function determines how much data is stored, other information of note
static USBC_ERROR_TYPE USB_HeartPulseReq_getStatus(USB_SETUP_REQUEST_Type* pReq)
{	
	if (pReq->wValue == 1) // Get normal device status
	{
		// get the last time sync value?
		// push all errors?
		uint8_t test[4]	= {1,2,3,4};
		udd_response.pPayload = (uint8_t*)(test);
		udd_response.sizePayload = sizeof(test);	
	}
	else if (pReq->wValue == 2)
	{
		// Not Used right now
	}	
	else return USBC_ERROR_UNKNOWN; // we had a bad request
	
	// Compute the length of the payload that will be sent (only necessary for transmission not controlled by us
	//udd_response.sizePayload = computePayloadLength(pReq, udd_response.sizePayload);

	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USB_HeartPulseReq_SetDataTransfer(USB_SETUP_REQUEST_Type* pReq)
{	
	// The data from this command should specify how much data it wants to transfer (blocks)
	// this will do the math (based on the stored last address pulled value) to start
	// unloading the data off the SD card and into a buffer.
	// The function should also get the IN EP interrupts set up to start pushing the data a
	// as soon as it's done.
	
	// this function should call USB_EP_createJob(), which will load up the data requested and ready
	// the IN interrupt to shuttle the data out.
	
	uint8_t numEP = 1;	// needs to be automated eventually
	
	if (pReq->wValue == 0) // this is the default test case
	{
		// load the buffer with junk data and create a job around it
		for (uint8_t i = 0; i < sizeof(bufferEP1Large)-1; i++)	bufferEP1Large[i] = i;
		USB_EP_createJob(numEP, bufferEP1Large, 200, 100);
	}
	else if (pReq->wValue == 1)
	{
		SD_2_USB_jobStarter(&dataHeader);
		
		// thought is that we create a function that calls something in main
		// this function would loads the buffer (1st half) and then create a job
		// make this function something like SD_2_USB_jobStarter?
		// it should handle loading the first buffer, and then creating the EP Job
		// all future transactions should be automatically handled by the USB EP interrupt - which should also probably just point to a
		// function living in the main, which would call manageJob and handle the reloading of the buffer by SD.
	}
	else if (pReq->wValue == 2) // NOTHING FOR NOW - USED TO HOLD INTIAL STUFF FOR USING [8192] dataBuffer also as sampling buffer
	{
		// USB_EP_createJob(numEP, dataBuffer, dataHeader.firstFile.numBlocks, sizeof(dataBuffer)); THIS SHOUDL 
	}
	else return USBC_ERROR_REQUEST_UKNOWN;
	
	// We SHOULD load the first buffer here (or in a callback pFunction), so that when we get pinged by the EP, the first data will be ready
	
	
	
	
	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USB_HeartPulseReq_setTime(USB_SETUP_REQUEST_Type* pReq)
{
	AST_stopReset();	// This is restarted by USB command
	
	// Set up to receive a payload from the device
	udd_response.sizePayload	= computePayloadLength(pReq, 8); 
	udd_response.bMoreData		= 1;	// We are expecting data incoming on this request
	udd_response.pPayload		= lastSyncHostTime.byte; // write time to global held in USBC.h
	
	// Set the timer to go right now! It was already reset by the interrupt form plugging in USB
	// It should be started now, because this will be the most consistent time since the setup
	// packet was received.
	AST_start();
	
	// In the future, may want to figure how to deal with ping? and delay from when the host
	// time is transmitted in the upcoming data phase?
	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USB_HeartPulse_setFile(USB_SETUP_REQUEST_Type* pReq)
{
	if (pReq->wValue == 1)		createNewFile();		// Create a New File
	else if (pReq->wValue == 2) updateNewFileTime();	// Update the time of the new file
	else if (pReq->wValue == 3) deleteFirstFile();		// Delete the First File in the buffer (called after successfully copying one file)
	else if (pReq->wValue == 4) resetAddress();			// Reset the SD address back to the base address
	else return USBC_ERROR_REQUEST_UKNOWN;				// we had an unknown command
	
	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USB_HeartPulse_getFile(USB_SETUP_REQUEST_Type* pReq)
{
	if (pReq->wValue == 1) // get Header
	{
		getDataHeader(&dataHeader); // pulls the first file information form the device and stores it in RAM
		udd_response.pPayload = (uint8_t*)(&dataHeader);
		udd_response.sizePayload = sizeof(dataHeader_Type);
	}
	else if (pReq->wValue == 2) // Gets Device information
	{
		getDeviceInfo(&devInfo);	// pulls info from the SD card and stores it in RAM
		udd_response.pPayload = (uint8_t*)&devInfo;
		udd_response.sizePayload = sizeof(deviceInfo_Type);
	}
	else if (pReq->wValue == 3) // getAllFileInfo
	{
		// pulls info from the SD card and stores it in RAM
		udd_response.pPayload = getAllFileInfo();
		udd_response.sizePayload = 512;
		pFunction = cleanDataBuffer;
	}
	else return USBC_ERROR_REQUEST_UKNOWN; // we had a bad request
	
	return USBC_SUCCESS;
}

static USBC_ERROR_TYPE USBC_UDC_processVendorReq(USB_SETUP_REQUEST_Type* pReq) // Function processes all vendor Requests
{
	if (pReq->bmRequestType.dir == USB_REQ_DIR_IN_D2H)	// GET Request
	{
		if (pReq->wLength == 0) return USBC_ERROR_UNKNOWN;

		if (pReq->bmRequestType.recipient == USB_REQ_RECIP_DEVICE)
		{
			if (pReq->bRequest == USB_bRequest_MS_GET_DESCRIPTOR) return USB_MSReq_getDevDesc(pReq);
			else if (pReq->bRequest == USB_bRequest_HEARTPULSE_GET_STATUS) return USB_HeartPulseReq_getStatus(pReq);
			else if (pReq->bRequest == USB_bRequest_HEARTPULSE_GET_FILE) return USB_HeartPulse_getFile(pReq);
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_INTERFACE)
		{
			if (pReq->bRequest == USB_bRequest_MS_GET_DESCRIPTOR) return USB_MSReq_getIntDesc(pReq);

		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_ENDPOINT) return USBC_ERROR_CMD_NOT_SUPPORTED;
	}
	else // SET Request
	{
		if (pReq->bmRequestType.recipient == USB_REQ_RECIP_DEVICE)
		{
			if (pReq->bRequest == USB_bRequest_HEARTPULSE_SET_TIME) return USB_HeartPulseReq_setTime(pReq);
			if (pReq->bRequest == USB_bRequest_HEARTPULSE_SET_DATATRANSFER) return USB_HeartPulseReq_SetDataTransfer(pReq);
			if (pReq->bRequest == USB_bRequest_HEARTPULSE_SET_FILE) return USB_HeartPulse_setFile(pReq);
		}
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_INTERFACE) return USBC_ERROR_CMD_NOT_SUPPORTED;
		else if (pReq->bmRequestType.recipient == USB_REQ_RECIP_ENDPOINT) return USBC_ERROR_CMD_NOT_SUPPORTED;
	}
	
	return USBC_ERROR_REQUEST_UKNOWN;
}







///////////////////////////////////////////////////////////////////////////////////////////////////
// UNUSED BUT TESTED CODE
///////////////////////////////////////////////////////////////////////////////////////////////////

// This is used if a GPIO interrupt is desired
/*void GPIO_0_Handler(void)	// Vusb - USB GPIO enable/disable handler
{
	// The Interrupt Flag Register (IFR) can be read by software to determine which pin(s) caused
	// the interrupt. The interrupt flag must be manually cleared by writing a zero to the
	// corresponding bit in IFR. GPIO interrupts will only be generated when CLK_GPIO is enabled.
	
	// Clear the interrupt on the pin, prime for next trigger
	GPIO->bf.Port[mUSBC.pin.VbusSense.port].bf.IFRC.reg = mUSBC.pin.VbusSense.gpioMask;
	
	uint32_t tempmask = GPIO->bf.Port[mUSBC.pin.VbusSense.port].bf.PVR.reg & mUSBC.pin.VbusSense.gpioMask;
	
	// See whether the device was plugged in or unplugged, based on [GPIO.PVR] state
	if (tempmask == mUSBC.pin.VbusSense.gpioMask)
	{
		USBC_UDD_enable();
		// Enable the PLL and stuff
		USBC_UDD_attach();
	}
	else USBC_UDD_disable();  // disable the PLL & stuff and enable the normal clock
}*/