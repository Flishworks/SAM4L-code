#ifndef	_USBC_SAM4L_H_	// Guards against double calling of headers
#define _USBC_SAM4L_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "USB_Protocol_SAM4L.h"
#include "datashuffle_Alex.h"
#include "EIC_SAM4L.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Global Defines/Enums
///////////////////////////////////////////////////////////////////////////////////////////////////
#define numEPmax	7	// This is the number of endpoints (not including EP0) that are available on the SAM4L series
#define numEPused	2	// this is the number of endpoints used by our particular device.
						// WE SHOULD FIGURE HOW TO USE THIS? something to look into in the future
#define maxEPsize	(uint16_t)(UINT16_MAX >> 1)	// 15 bit max value


typedef enum
{
	USBC_STATE_UNKNOWN						= 0,
	USBC_STATE_IDLE,
	USBC_STATE_INITIALIZING_CLOCK,
	USBC_STATE_INITIALIZING,
	USBC_STATE_ENABLED,
	USBC_STATE_DISABLING,
} USBC_STATE_TYPE;

typedef enum
{
	USB_DEVICE_STATE_UNKNOWN	= 0,
	USB_DEVICE_STATE_ATTACHED	= 1, // Device is attached to the USB, but not powered? - State of pins = attached... could be done without power
	USB_DEVICE_STATE_POWERED	= 2, // Device is attached to the USB, powered, but not reset yet
	USB_DEVICE_STATE_DEFAULT	= 3, // Device has been reset, not addressed yet though
	USB_DEVICE_STATE_SUSPENDED	= 4, // Device in suspended state, may/may not be configured yet
	USB_DEVICE_STATE_ADDRESS	= 5, // Device has had address assigned
	USB_DEVICE_STATE_CONFIGURED = 6, // Device has been configured
} USB_DEVICE_STATE_TYPE;

typedef enum
{
	USBC_SUCCESS					= 0,
	USBC_ERROR_UNKNOWN				= 1,		/// SHOULD BE AN OFFSET....---------------------------------------------------
	USBC_ERROR_INCORRECT_DIR,
	USBC_ERROR_REQUEST_UKNOWN,
	USBC_ERROR_CMD_NOT_SUPPORTED,
} USBC_ERROR_TYPE;

typedef enum
{
	USB_VBUS_INTERRUPT_NONE = 0,
	USB_VBUS_INTERRUPT_GPIO = 1,
	USB_VBUS_INTERRUPT_EIC = 2
} USB_VBUS_INTERRUPT_TYPE;

typedef enum
{
	USB_VBUS_INTERRUPT_LOW = 0,
	USB_VBUS_INTERRUPT_HIGH = 1
} USB_VBUS_INTERRUPT_LEVEL;

typedef enum
{
	USBC_INT_SUSP	= USBC_UDINT_SUSP,		// Suspend Interrupt
	//USBC_INT_MSOF	= USBC_UDINT_MSOF,		// Micro Start of Frame Interrupt -- NOT IN DATA SHEET
	USBC_INT_SOF	= USBC_UDINT_SOF,		// Start of Frame interrupt
	USBC_INT_EORST	= USBC_UDINT_EORST,		// End of Reset Interrupt
	USBC_INT_WAKEUP	= USBC_UDINT_WAKEUP,	// Wakeup Interrupt
	USBC_INT_EORSM	= USBC_UDINT_EORSM,		// End of Resume Interrupt
	USBC_INT_UPRSM	= USBC_UDINT_UPRSM,		// Upstream Resume Interrupt
} USBC_INT_MASKS_TYPE;

// EP Descriptor Table (Device) - See Section 17.6.2.11
typedef struct {
	uint8_t* EPn_ADDRESS;					// EPn_ADDR_BK		- Address of EP
	union {									// EPn_PCKSIZE_BK	- Packet size information for EP
		uint32_t all;
		struct
		{
			uint32_t BYTE_COUNT:15;			// [14:0]	- 
			uint32_t :1;					// [15]
			uint32_t MULTI_PACKET_SIZE:15;	// [30:16]	-
			uint32_t AUTO_ZLP:1;			// [31]		- 
		};
	} EPn_PCKSIZE_BK;
	union {									// EPn_CTR_STA_BK	- EP control and status
		uint32_t all;
		struct
		{
			uint32_t STALLREQ_NEXT:1;		// [0]		-
			uint32_t :15;					// [15:1]	-
			uint32_t CRCERR:1;				// [16]		-
			uint32_t OVERF:1;				// [17]		-
			uint32_t UNDERF:1;				// [18]		-
			uint32_t :13;					// [31:19]	-
		};
	} EPn_CTR_STA_BK;
	uint32_t reserved;
} USB_DEVICE_EP_DESCRIPTOR_BANK_Type;

typedef struct {
	USB_DEVICE_EP_DESCRIPTOR_BANK_Type	Bank[2];
} USB_DEVICE_EP_DESCRIPTOR_TABLE_Type;

typedef struct {
	uint8_t*	pPayload;		// Pointer to the address of the payload
	uint16_t	sizePayload;	// Contains the size elements to be transfered
	uint8_t		bMoreData;
} udd_response_type;

typedef struct
{
	uint64_t	sizeTotalPayload;	// contains the total number of bytes to be transfered
	uint64_t	sizeTotalTransfered;// count to keep track of where we are in the transfer
	uint16_t	bufferLengthLimit;	// The limit of the buffer size
	uint16_t	sizeNextTransfer;
	uint8_t*	pBuffer;		// pointer to the word aligned Tx/Rx buffer for an endpoint
	
	uint8_t		bJobEnabled:1;	// boolean indicator for whether job is enabled (and thus busy)
	uint8_t		bLastTransfer:1;	// boolean indicator for the last transfer on a job.
	
	// Boolean flags that may or may not be used
} udd_job_type;

// These are used to determine state, basically what kind of packet is being awaited.
typedef enum {
	UDD_EP_SETUP			= 0, // Awaiting a SETUP packet
	UDD_EP_DATA_OUT			= 1, // Awaiting a OUT data packet
	UDD_EP_DATA_IN			= 2, // Awaiting a IN data packet
	UDD_EP_STATUS_IN_ZLP	= 3, // Awaiting a IN ZLP packet
	UDD_EP_STATUS_OUT_ZLP	= 4, // Awaiting a OUT ZLP packet
	UDD_EP_STALL_REQ		= 5, // STALL enabled on IN & OUT packet
} UDD_EP_STATE_TYPE;


typedef enum {
	USB_TRANSCATION_STATE_WAITING	= 0,	// No Transaction running
	USB_TRANSACTION_STAGE_SETUP		= 1,	// In Setup Stage
	USB_TRANSACTION_STAGE_DATA		= 2,	// In Data Stage
	USB_TRANSACTION_STAGE_STATUS	= 3,	// In Status Stage
	USB_TRANSACTION_STAGE_CLEANUP	= 4,	// Post Transaction cleanup
} USB_TRANSACTION_STAGE_TYPE;

// I think that THE EP table setup is only designed to handle 4 EPs enabled simultaneously as
// it is written right now. I think the compiler alignment and EP_Table number should be 
// feed by the same define, which will indicate how many EPs (including EP0) are actually
// used. Note that each EP is 8 bytes (2 tables of 4 bytes).
// EP_TABLE base address is pointed to by [USBC.UDESCA], and must be word aligned due to the 
// 3 least significant bit always 0. I AM UNSURE IF THE THE BUFFER ITSELF NEEDS TO BE ALLIGNED, NEED TO TEST AT SOMEPOINT
COMPILER_ALIGNED(32) volatile USB_DEVICE_EP_DESCRIPTOR_TABLE_Type EP_Table[4];
COMPILER_ALIGNED(4) uint8_t bufferCTRL_TxRx[USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE];		// will need to figure out how to

// I DON'T THINK THAT THIS NEEDS TO BE ALLIGNED
COMPILER_ALIGNED(4) uint8_t bufferEP1[USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE];				// organize these better in the future,
COMPILER_ALIGNED(4)	uint8_t bufferEP1Large[USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE*4];
//COMPILER_ALIGNED(4) uint8_t bufferEP2[USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE];			// way to have the actual descriptors generate them?
//COMPILER_ALIGNED(4)	uint8_t udd_ctrl_request;
			
udd_response_type			udd_response;
udd_job_type				jobEP[numEPused];

///////////////////////////////////////////////////////////////////////////////////////////////////
// Macros ADDRESS DEFINES 
///////////////////////////////////////////////////////////////////////////////////////////////////
// offsets defines...?
// to be filled

// UDINT, UDINTCLR, UNINTSET, UDINTE, UDINECLR, UDINTSET macro for bit shifting endpoint locations.
#define USBC_UDINT_EP_MASK(numEP)	(uint32_t)(_U(0x1) << (USBC_UDINT_EP0INT_Pos + (numEP)))

// UERST macro for creating a enable/disable mask
#define USBC_UERST_EPENn(numEP)		(uint32_t)(_U(0x1) << (numEP))

// Base Address defines for UECFGn, UESTAn, and UECONn
#define USBC_ADDR_UECFG0	(uint32_t)(USBC_ADDR + USBC_UECFG0_OFFSET)		// EP Configuration (R/W)
#define USBC_ADDR_UESTA0	(uint32_t)(USBC_ADDR + USBC_UESTA0_OFFSET)		// EP Status Read	(R)
#define USBC_ADDR_UESTA0CLR	(uint32_t)(USBC_ADDR + USBC_UESTA0CLR_OFFSET)	// EP Status Clr	(W)
#define USBC_ADDR_UESTA0SET	(uint32_t)(USBC_ADDR + USBC_UESTA0SET_OFFSET)	// EP Status Set	(W)
#define USBC_ADDR_UECON0	(uint32_t)(USBC_ADDR + USBC_UECON0_OFFSET)		// EP Control Read	(R)
#define USBC_ADDR_UECON0SET	(uint32_t)(USBC_ADDR + USBC_UECON0SET_OFFSET)	// EP Control Set	(W)
#define USBC_ADDR_UECON0CLR	(uint32_t)(USBC_ADDR + USBC_UECON0CLR_OFFSET)	// EP Control Clr	(W)

// use this to calculate the appropriate register offsets
#define USBC_ADDR_EP_OFFSET(baseAddr, numEP)	(uint32_t)((baseAddr) + 4 * (numEP)) // calculates the 

///////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN USB STRUCT, will need work...
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct // this structure holds the information for the actual controller
{
	union
	{
		struct
		{
			uint8_t initUSBC:1;			// Controller initialized				-- need to figure if this is just the USB stuff or the actual device (CDC, HID, etc...)
					
			// These are a indicators as to what initialization state we're at.
			uint8_t devAttached:1;		// Device attached to USB bus?
			uint8_t devDefault:1;		// Device had been reset as part of initialization
			uint8_t devAddress:1;		// Device Address has been assigned
			uint8_t devConfigured:1;	// Device has been configured
			uint8_t postIntFunction:1;	// Function needs to be called after an interrupt
			uint8_t :2;					// nothing, to be filled eventually
		};
		uint8_t all;
	} bFlag;
	struct
	{
		USBC_STATE_TYPE						usbc;
		USB_DEVICE_STATE_TYPE				dev;
		volatile UDD_EP_STATE_TYPE			EP[numEPmax + 1]; // There are actually 8 EPs (including deafult EP0 CTRL)
		volatile USB_TRANSACTION_STAGE_TYPE trans;
	} state;
	
	
	uint8_t									addr;
	USBC_ERROR_TYPE							err;
	
	// USB device characteristics/globals
	USB_STATUS_Type							status;					// Holds the status of the device
	uint8_t									numActiveConfiguration;	// Holds the configuration that is currently active, 0 = not configured yet.
	
	// Information specific the the VUSB interrupt
	struct	 
	{
		void (*pHandler)(void);					// pointer to the interrupt handler
		IRQn_Type numIRQ;						// for holding IRQ numbers
		
		uint8_t									intChanNum;		// Could be EIC_INT_CHAN_NUM or GPIO Channel number
		USB_VBUS_INTERRUPT_TYPE					intType;
		USB_VBUS_INTERRUPT_LEVEL				intLvl;
	} VUSB;
	//PINS No need to store DM and DP in the manager, they are fixed on every device - this should be handled based on the device we're using...

	struct
	{
		Pin VUSB;
		Pin ID;			// don't think this will be used...
	} pin;
	
	uint8_t bUseDFLL;
		
} Manager_USB_Type;

Manager_USB_Type mUSBC;

u_64 lastSyncHostTime;			// a global held in ram that contains the last known sync date (will be updated by the USB)


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// TEMPORARY - NEED TO FIGURE OUT WHERE TO PUT THIS - This should probably go into main, as this a device specific structure
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

COMPILER_TIGHT_PACK();
typedef struct
{
	uint32_t				devSerialNum[4];
	uint16_t				projectNumber;
	uint16_t				HardwareRevNumMajor;
	uint16_t				HardwareRevNumMinor;
	uint16_t				HeartPulseSerialNum;
	uint32_t				firmwareVersion;// this should be changed to 2x uint_16... major and minor (the case where we have major software deployed on two different hardware platforms)
} deviceInfo_Type;
COMPILER_RESET_PACK();

COMPILER_TIGHT_PACK();
typedef struct
{
	uint32_t	firstAddress;	// The first address of the file
	uint32_t	numBlocks;		// the size of the file in 512 bytes blocks
	u_64		lastSyncTime;	// the date the file was created
} dataFile_Type;
COMPILER_RESET_PACK();

COMPILER_TIGHT_PACK();
typedef struct
{
	uint16_t		numFiles;	// The first address of the file
	dataFile_Type	firstFile;		// the size of the file in 512 bytes blocks
} dataHeader_Type;
COMPILER_RESET_PACK();


COMPILER_ALIGNED(4) volatile deviceInfo_Type	devInfo;
COMPILER_ALIGNED(4) volatile dataHeader_Type	dataHeader;
uint8_t USB_fileInit;											// variable used to determine USB state
uint8_t	bTestMode;												// This is a flag that forces the device into test mode
void getDeviceInfo(deviceInfo_Type* pDevInfo);
void getDataHeader(dataHeader_Type* pDataHeader);
uint8_t* getAllFileInfo(void);
void createNewFile(void);
void updateNewFileTime(void);
void deleteFirstFile(void);
void resetAddress(void);
//COMPILER_ALIGNED(4) volatile uint8_t SD_2_USB_Buffer[2][512]; // buffer which holds the data being shuffled out from SD and into the USB.  SHould eventually be replaced by the sampling buffer
volatile uint8_t sd_2_usb_bufferPart;
volatile uint32_t sd_2_usb_address;
void SD_2_USB_jobStarter(dataHeader_Type* pDataHeader);
uint8_t SD_2_USB_jobManager(void);
void SD_2_USB_jobAbort(void);
void cleanDataBuffer(void);	

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t USBC_initClocks(void);
// Old init functions prototypes -- these are to be deleted, these are for reference.
//void USBC_init			(Pin* DM, Pin* DP, Pin* VbusSense, USB_VBUS_INTERRUPT_TYPE type,
						 //USB_VBUS_INTERRUPT_LEVEL lvl, uint8_t bUseDFLL);
//void USBC_init(Pin* DM, Pin* DP, Pin* VbusSense, USB_VBUS_INTERRUPT_TYPE type,
//USB_VBUS_INTERRUPT_LEVEL lvl, void (*pIntFunc)(void), uint8_t bUseDFLL);

void USBC_init(			Pin* VbusSense,	// Pin information
						USB_VBUS_INTERRUPT_TYPE type,		// EIC/GPIO interrupt
						USB_VBUS_INTERRUPT_LEVEL trigLvl,	// High/Low level trigger
						EIC_INT_CHAN_NUM intNum,			// ONLY FOR EIC interrupts, GPIO calculated internally
						uint8_t bUseDFLL);					// use DFLL for USBC clock

void USBC_UDD_attach(void);
void USBC_UDD_enable(void);
void USBC_UDD_detach(void);
void USBC_UDD_disable(void);

uint8_t USB_EP_createJob(uint8_t numEP, uint8_t* pBuffer, uint64_t lengthJob, uint16_t bufferLengthLimit);
uint8_t USB_EP_manageJob(uint8_t numEP, uint8_t* pBufferNew);

uint8_t USB_EP_cleanupTransfer(uint8_t numEP);		// Temporary, should figure how to move this internally into the USBC functionality

#endif

///// UNUSED CODE, FOR THE FUTURE:

// Pipe Descriptor Table (Host) - See Section 17.6.3.8
// THIS NEEDS TO BE CLEANED UP BEFORE BEING USED, SEE EP TABLE as Example*****************************************
//typedef struct {
	//uint8_t *PIPE_ADDRESS;					// EPn_ADDR_BK		- Address of EP
	//union {									// EPn_PCKSIZE_BK	- Packet size information for EP
		//uint32_t all;
		//struct
		//{
			//uint32_t BYTE_COUNT:15;			// [14:0]	-
			//uint32_t :1;					// [15]
			//uint32_t MULTI_PACKET_SIZE:15;	// [30:16]	-
			//uint32_t AUTO_ZLP:1;			// [31]		-
		//} b;
	//} packet_size;
	//union {									// EPn_CTR_STA_BK	- EP control and status
		//uint32_t all;
		//struct
		//{
			//uint32_t STALLREQ_NEXT:1;		// [0]		-
			//uint32_t :15;					// [15:1]	-
			//uint32_t CRCERR:1;				// [16]		-
			//uint32_t OVERF:1;				// [17]		-
			//uint32_t UNDERF:1;				// [18]		-
			//uint32_t :13;					// [31:19]	-
		//}b ;
	//} EP_ctrl_stat;
	//uint32_t reserved;
	//union {						// Pn_CTR_STA1 - NOTE: only used in host mode
	 	//uint32_t all;
	 	//struct
	 	//{
	 		//uint32_t pipe_dev_addr:7;		//
	 		//uint32_t :1;
	 		//uint32_t pipe_num:4;			//
	 		//uint32_t pipe_error_cnt_max:4;	//
	 		//uint32_t pipe_error_status:8;	//
	 		//uint32_t :8;
	 	//} ep_ctrl_stat;
//} USB_HOST_DESCRIPTOR_TABLE_Type;