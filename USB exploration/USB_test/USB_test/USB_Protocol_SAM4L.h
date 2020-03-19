// File: USB Protocol
//
// Description:
// This file holds all the structures and enumerations need for standard (non-class) specific USB
// Device communications.  All of these are provided for in the bases USB 2.0 specification


#ifndef _USB_PROTOCOL_SAM4L_H_
#define _USB_PROTOCOL_SAM4L_H_



///////////////////////////////////////////////////////////////////////////////////////////////////
// Protocol Standards
///////////////////////////////////////////////////////////////////////////////////////////////////
// Control Transfers - §5.5
#define USB_FULLSPEED_CTRL_EP_MAX_PAYLOAD_SIZE	64		// §5.5.3

// Isochronous Transfers - §5.6
#define USB_FULLSPEED_ISO_EP_MAX_PAYLOAD_SIZE	1023	// §5.6.3

// Interrupt Transfers - §5.7
#define USB_FULLSPEED_INT_EP_MAX_PAYLOAD_SIZE	64		// §5.7.3

// Bulk Transfers - §5.8
#define USB_FULLSPEED_BULK_EP_MAX_PAYLOAD_SIZE	64		// §5.8.3


///////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Requests
///////////////////////////////////////////////////////////////////////////////////////////////////
// Per §9.4 of the USB 2.0 specification, every USB device is required to be able to handle
// these standard request types

typedef enum  // Part of bmRequestType
{
	USB_REQ_DIR_OUT_H2D	= 0, // Host to Device
	USB_REQ_DIR_IN_D2H	= 1, // Device to Host
} USB_REQUEST_DIRECTION_TYPE;

typedef enum  // Part of bmRequestType
{
	USB_REQ_TYPE_STANDARD	= 0, // Standard Request
	USB_REQ_TYPE_CLASS		= 1, // Class-specific Request
	USB_REQ_TYPE_VENDOR		= 2, // Vendor-specific Request
} USB_REQUEST_TYPE_TYPE;

typedef enum // Part of bmRequestType
{
	USB_REQ_RECIP_DEVICE	= 0, // Recipient Device
	USB_REQ_RECIP_INTERFACE	= 1, // Recipient Interface
	USB_REQ_RECIP_ENDPOINT	= 2, // Recipient Endpoint
	USB_REQ_RECIP_OTHER		= 3, // Recipient Other
} USB_REQUEST_RECIPIENT_TYPE;


typedef enum
{
	USB_bRequest_GET_STATUS			= 0,	//
	USB_bRequest_CLEAR_FEATURE		= 1,	//
	USB_bRequest_SET_FEATURE		= 3,	//
	USB_bRequest_SET_ADDRESS		= 5,	//
	USB_bRequest_GET_DESCRIPTOR		= 6,	//
	USB_bRequest_SET_DESCRIPTOR		= 7,	//
	USB_bRequest_GET_CONFIGURATION	= 8,	//
	USB_bRequest_SET_CONFIGURATION	= 9,	//
	USB_bRequest_GET_INTERFACE		= 10,	//
	USB_bRequest_SET_INTERFACE		= 11,	//
	USB_bRequest_SYNCH_FRAME		= 12,	//
	
	// Custom Requests
	USB_bRequest_MS_GET_DESCRIPTOR		= 0x45,	// OS feature descriptor - See MS_OS_10_stringDescriptor (bDescriptorType)
	
	USB_bRequest_HEARTPULSE_GET_STATUS	= 0x80,
	USB_bRequest_HEARTPULSE_SET_TIME	= 0x81,
	USB_bRequest_HEARTPULSE_SET_DATATRANSFER	= 0x82,
	USB_bRequest_HEARTPULSE_SET_FILE			= 0x83,
	USB_bRequest_HEARTPULSE_GET_FILE			= 0x84,
} USB_bRequest_TYPE;

typedef union
{
	uint8_t all;
	struct
	{
		USB_REQUEST_RECIPIENT_TYPE	recipient:5;
		USB_REQUEST_TYPE_TYPE		type:2;
		USB_REQUEST_DIRECTION_TYPE	dir:1;
	};
	
} USB_bmRequestType_Type;

// STANDARD DEVICE REQUEST, SETUP Packet, DATA Phase payload. 
typedef struct {
	USB_bmRequestType_Type	bmRequestType;	// Characteristics of request
	USB_bRequest_TYPE		bRequest;		// Specific request
	uint16_t				wValue;			// Varies by request type
	uint16_t				wIndex;			// Varies by request type; typically used to pass an index or offset
	uint16_t				wLength;		// Number of bytes to transfer (if zero, no Data Stage)
} USB_SETUP_REQUEST_Type;

#define USB_SETUP_REQUEST_LENGTH	8


typedef union
{
	uint16_t all;
	struct
	{
		uint16_t bSelfPowered:1;	// Self Powered Device, or bus powered?
		uint16_t bRemoteWakeup:1;	// Remote Wakeup Enabled?
		uint16_t :14;				// Reserved
	};
} USB_STATUS_Type;



///////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Descriptors
///////////////////////////////////////////////////////////////////////////////////////////////////
// Per §9.5 of the USB 2.0 specification, every USB device is required to be able to handle
// these standard descriptor types
// wValue field contents
typedef enum
{
	// From Table 9-5 in the USB 2.0 Specification
	USB_DESCRIPTOR_DEVICE				= 0x01, // §9.6.1
	USB_DESCRIPTOR_CONFIGURATION		= 0x02,	// §9.6.3
	USB_DESCRIPTOR_STRING				= 0x03,	// §9.6.7
	USB_DESCRIPTOR_INTERFACE			= 0x04,	// §9.6.5
	USB_DESCRIPTOR_ENDPOINT				= 0x05,	// §9.6.6
	USB_DESCRIPTOR_DEVICE_QUALIFIER		= 0x06,	// §9.6.2
	USB_DESCRIPTOR_OTHER_SPEED_CONFIG	= 0x07,	// §9.6.4
	USB_DESCRIPTOR_INTERFACE_POWER		= 0x08,	// Detailed in USB Interface Power Management Specification
	USB_DESCRIPTOR_OTG					= 0x09,	// On the Go - from an ECN
	USB_DESCTIPTOR_DEBUG				= 0x0A, // Debug - from an ECN
	USB_DESCRIPTOR_IAD					= 0x0B,	// Interface association, from an ECN
	USB_DESCRIPTOR_BOS					= 0x0F,	// Binary Device Object Store, from LPM ECN? USB 3.0 related
	USB_DESCRIPTOR_DEVICE_CAPABILITY	= 0x10,	// USB 3.0 related?
} USB_DESCRIPTOR_TYPE;

#pragma pack(1) // Set's all following structures to be tightly packed (until #pragma pack() call)


// Descriptor - Device (§9.6.1)
// ------------------------------------------------------------------------------------------------
typedef enum // field bcdUSB
{
	USB_V1_0 = 0x0100,
	USB_v1_1 = 0x0110,
	USB_V2_0 = 0x0200,
	USB_V2_1 = 0x0201,
} USB_SPEC_VERSION_TYPE;
typedef enum // field bDeviceClass
{
	USB_DEVICE_CLASS_UNSPECIFIED		= 0x00,	// Device class unspecified, interface descriptors
												// used to determine needed drivers
	USB_DEVICE_CLASS_CDC				= 0x02, //
	USB_DEVICE_CLASS_HUB				= 0x09,
	USB_DEVICE_CLASS_BILLBOARD			= 0x11,
	USB_DEVICE_CLASS_DIAGNOSTIC			= 0xDC,
	USB_DEVICE_CLASS_MISC				= 0xEF,
	USB_DEVICE_CLASS_VENDOR_SPECIFIC	= 0xFF
} USB_DEV_CLASS_TYPE;

typedef struct
{
	uint8_t					bLength;			// Size of this descriptor in bytes
	USB_DESCRIPTOR_TYPE		bDescriptorType;	// DEVICE Descriptor Type (CONSTANT)
	USB_SPEC_VERSION_TYPE	bcdUSB;				// USB Spec. Release Number in Binary-Coded Decimal
	USB_DEV_CLASS_TYPE		bDeviceClass;		// Class code (assigned by the USB-IF)
	uint8_t					bDeviceSubClass;	// Subclass code (assigned by the USB-IF)
	uint8_t					bDeviceProtocol;	// Protocol code (assigned by the USB-IF)
	uint8_t					bMaxPacketSize0;	// Maximum packet size for EP0 (only 8, 16, 32,
												// or 64 are valid)
	uint16_t				idVendor;			// Vendor ID (assigned by the USB-IF)
	uint16_t				idProduct;			// Product ID (assigned by the manufacturer)
	uint16_t				bcdDevice;			// Device release number in binary-coded decimal
	uint8_t					iManufacturer;		// Index of string descriptor describing manufacturer
	uint8_t					iProduct;			// Index of string descriptor describing product
	uint8_t					iSerialNumber;		// Index of string descriptor describing device's serial num.
	uint8_t					bNumConfigurations;	// Number of possible configurations
} USB_DESCRIPTOR_DEVICE_Type;

// Descriptor - Configuration (§9.6.3)
// ------------------------------------------------------------------------------------------------
typedef struct 
{
	uint8_t					bLength;			// Size of this descriptor in bytes
	USB_DESCRIPTOR_TYPE		bDescriptorType;	// CONFIGURATION Descriptor Type (CONSTANT)
	uint16_t				wTotalLength;		// Total length of data returned descriptors
												// for this config (combined length of config and
												// all , interface, endpoint, and class/vendor
												// specific descriptors)
	uint8_t					bNumInterfaces;		// Number of interfaces supported by this config.
	uint8_t					bConfigurationValue;// Value used as argument in by setConfig() cmd
	uint8_t					iConfiguration;		// Index of string descriptor describing config.
	uint8_t					bmAttributes;		// NOTE - THIS NEEDS  TO BE BROKEN OUT................................ maybe?
	uint8_t					bMaxPower;			// Max power consumption of USB (2mA increments)
} USB_DESCRIPTOR_CONIGURATION_Type;

// Descriptor - Interface (§9.6.5)
// ------------------------------------------------------------------------------------------------
typedef enum // field bInterfaceClass
{
	USB_IFACE_CLASS_AUDIO				= 0x01,
	USB_IFACE_CLASS_CDC_COMMCONTROL		= 0x02,
	USB_IFACE_CLASS_HID					= 0x03,	// Human Interface Device
	USB_IFACE_CLASS_PID					= 0x05,	// Physical Interface Device
	USB_IFACE_CLASS_IMAGE				= 0x06,
	USB_IFACE_CLASS_PRINTER				= 0x07,
	USB_IFACE_CLASS_MASS_STORAGE		= 0x08,
	USB_IFACE_CLASS_CDC_DATA			= 0x0A,
	USB_IFACE_CLASS_SMART_CARD			= 0x0B,
	USB_IFACE_CLASS_CONTENT_SECURITY	= 0x0D,
	USB_IFACE_CLASS_VIDEO				= 0x0E,
	USB_IFACE_CLASS_PHDC				= 0x0F,	// Personal Health Care Device
	USB_IFACE_CLASS_AUDIOVIDEO			= 0x10,
	USB_IFACE_CLASS_DIAGNOSTIC			= 0xDC,
	USB_IFACE_CLASS_WIRELESS			= 0xE0,
	USB_IFACE_CLASS_MISC				= 0xEF,
	USB_IFACE_CLASS_APP_SPECIFIC		= 0xFE,
	USB_IFACE_CLASS_VENDOR_SPECIFIC		= 0xFF
} USB_IFACE_CLASS_TYPE;

typedef struct
{
	uint8_t					bLength;			// Size of this descriptor in bytes
	USB_DESCRIPTOR_TYPE		bDescriptorType;	// INTERFACE Descriptor Type (CONSTANT)
	uint8_t					bInterfaceNumber;	// Number of this interface, zero-based
	uint8_t					bAlternatingSetting;// Used to select and alternate of Interface
	uint8_t					bNumEndpoints;		// Number endpoints used by Interface, EP0 not included 
	USB_IFACE_CLASS_TYPE	bInterfaceClass;	// Class code (assigned by the USB-IF)
	uint8_t					bInterfaceSubClass;	// Subclass code (assigned by the USB-IF)
	uint8_t					bInterfaceProtocol;	// Protocol code (assigned by the USB-IF)
	uint8_t					iInterface;			// Index of string descriptor describing interface
} USB_DESCRIPTOR_INTERFACE_Type;

// Descriptor - Endpoint (§9.6.6)
// ------------------------------------------------------------------------------------------------
typedef union
{
	struct
	{
		uint8_t		addr:4;
		uint8_t		:3;
		uint8_t		dir:1;	// direction 1 = in
	};
	uint8_t all;
} USB_EP_ADDRESS_Type;

typedef struct
{
	uint8_t					bLength;			// Size of this descriptor in bytes
	USB_DESCRIPTOR_TYPE		bDescriptorType;	// ENDPOINT Descriptor Type (CONSTANT)
	USB_EP_ADDRESS_Type		bEndpointAddress;	// 
	uint8_t					bmAttributes;		// -- NEEDS TO BE BROKEN OUT........................................
	uint16_t				wMaxPacketSize;		// --- break out? Max packet size endpoint capable of receiving
	uint8_t					bInterval;			// interval for polling data transfers.
} USB_DESCRIPTOR_ENDPOINT_Type;

// Descriptor - Qualifier (§9.6.2)
// ------------------------------------------------------------------------------------------------
typedef struct
{
	uint8_t					bLength;			// Size of this descriptor in bytes
	USB_DESCRIPTOR_TYPE		bDescriptorType;	// QUALIFIER Descriptor Type (CONSTANT)
	USB_SPEC_VERSION_TYPE	bcdUSB;				// USB Spec. Release Number in Binary-Coded Decimal
	USB_DEV_CLASS_TYPE		bDeviceClass;		// Class code (assigned by the USB-IF).	
	uint8_t					bDeviceSubClass;	// Subclass code (assigned by the USB-IF)
	uint8_t					bDeviceProtocol;	// Protocol code (assigned by the USB-IF)
	uint8_t					bMaxPacketSize0;	// Maximum packet size for other speed
	uint8_t					bNumConfigurations;	// Number of possible configurations
	uint8_t					bReserved;			// Reserved for future use, set to zero
} USB_DESCRIPTOR_QUALIFIER_Type;


#pragma pack()


















#endif