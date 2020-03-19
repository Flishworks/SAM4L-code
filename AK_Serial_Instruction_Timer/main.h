#define HEARTPULSE_SERIAL_NUMBER			45u
#define DEVICE_FIRMWARE_VERSION				1u
#define DEVICE_HARDWARE_MAJOR_REV_NUMBER	3u
#define DEVICE_HARDWARE_MINOR_REV_NUMBER	3u

uint8_t BLOCKS_IN_BUFFER = 16;
uint16_t MAIN_BUFFER_BYTE_LENGTH_FULL = 512 * BLOCKS_IN_BUFFER;
uint16_t MAIN_BUFFER_BYTE_LENGTH_HALF = MAIN_BUFFER_BYTE_LENGTH_FULL/2;
volatile uint8_t	dataBuffer[MAIN_BUFFER_BYTE_LENGTH_FULL];
volatile uint8_t	bufferPart = 0;		// indicates which part of the buffer is ready to write 0 = bottom, 1 = top

void USB_setup(void);

