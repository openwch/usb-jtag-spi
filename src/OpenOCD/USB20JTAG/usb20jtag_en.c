/***************************************************************************                                             *                    
 *                                                                         *
 *   Driver for USB2.0Jtag                                                 * 
 *   The USB2.0 to JTAG scheme based on CH32V305 MCU can be used to build  * 
 *   customized USB high-speed JTAG debugger and other products.           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if IS_CYGWIN == 1
#include "windows.h"
#undef LOG_ERROR
#endif

/* project specific includes */
#include <jtag/interface.h>
#include <jtag/commands.h>
#include <helper/time_support.h>
#include <helper/replacements.h>


/* system includes */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define JTAGIO_STA_OUT_TDI         (0x10)
#define JTAGIO_STA_OUT_TMS         (0x02)
#define JTAGIO_STA_OUT_TCK         (0x01)

#define TDI_H JTAGIO_STA_OUT_TDI
#define TDI_L 0
#define TMS_H JTAGIO_STA_OUT_TMS
#define TMS_L 0 
#define TCK_H JTAGIO_STA_OUT_TCK
#define TCK_L 0 

#define HW_TDO_BUF_SIZE          				  4096
#define SF_PACKET_BUF_SIZE 						  51200			// More Protocol package length
#define CMDPKT_DATA_MAX_BYTES_USBFS 			  59     		// Full speed USB single package with data length
#define USB_PACKET_USBFS						  64			// Full speed USB single packet length
#define CMDPKT_DATA_MAX_BYTES_USBHS 			  507    		// High speed USB single package with data length
#define USB_PACKET_USBHS						  512			// High speed USB single packet length

#define USB20Jtag_CMD_HEADER                	  3				// Protocol packet length

#define USB20Jtag_CMD_INFO_RD               	  0xCA			// Obtain firmware version, JTAG interface related parameters, etc
#define USB20Jtag_CMD_JTAG_INIT             	  0xD0			// JTAG interface initialization command
#define USB20Jtag_CMD_JTAG_BIT_OP           	  0xD1			// JTAG interface pin bit control command
#define USB20Jtag_CMD_JTAG_BIT_OP_RD        	  0xD2			// JTAG interface pin bit control and read command
#define USB20Jtag_CMD_JTAG_DATA_SHIFT       	  0xD3			// JTAG interface data shift command
#define USB20Jtag_CMD_JTAG_DATA_SHIFT_RD    	  0xD4			// JTAG interface data shift and read command

#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

#define USB_ENDPOINT_TYPE_MASK                    0x03
#define USB_ENDPOINT_TYPE_CONTROL                 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01
#define USB_ENDPOINT_TYPE_BULK                    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03
#define USB_ENDPOINT_DIRECTION_MASK               0x80

#pragma pack(1) 
typedef struct _USB_COMMON_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;

typedef struct _USB_DEVICE_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bEndpointAddress;
    unsigned char bmAttributes;
    unsigned short wMaxPacketSize;
    unsigned char bInterval;
} USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;

typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short wTotalLength;
    unsigned char bNumInterfaces;
    unsigned char bConfigurationValue;
    unsigned char iConfiguration;
    unsigned char bmAttributes;
    unsigned char MaxPower;
} USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_INTERFACE_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bInterfaceNumber;
    unsigned char bAlternateSetting;
    unsigned char bNumEndpoints;
    unsigned char bInterfaceClass;
    unsigned char bInterfaceSubClass;
    unsigned char bInterfaceProtocol;
    unsigned char iInterface;
} USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

typedef	struct	_USBCMD_PKT{				// Define the command interface structure for communication with hardware	
	unsigned char 	mFunction;				// Specify function code when entering
	unsigned short  mLength;				// Subsequent data length
	union	{
		 unsigned char mStatus;				// Return to operation state on output
		 unsigned char mBuffer[ 512 ];	    // Data buffer, default length 0 to 512 bytes
	};
} mUSB_HW_COMMAND, *mPUSB_HW_COMMAND;

typedef struct _PIN_STATUS{					// Record USB pin status
	int TMS;
	int TDI;
	int TCK;
}JtagPinStatus;
#pragma pack() 

#ifdef _WIN32
#include <windows.h>
typedef int (__stdcall *pCH375OpenDevice)(                                                  // Open CH375 device, return the handle, invalid if error
				                                        unsigned long iIndex);              // Specify the serial number of the CH375 device, 0 corresponds to the first device, -1 automatically searches for a device that can be opened and returns the serial number
typedef void (__stdcall *pCH375CloseDevice)(	                                            // Close the CH375 device
	                                                    unsigned long iIndex);              // Specify the serial number of the CH375 device
typedef unsigned long (__stdcall *pCH375SetTimeoutEx)(	                                    // Set the timeout for reading and writing USB data
	                                                    unsigned long iIndex,               // Specify the serial number of the CH375 device
                                                        unsigned long iWriteTimeout,        // Specify the timeout time for USB to write out data blocks, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
														unsigned long iReadTimeout,         // Specify the timeout time for USB read data block, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
                                                        unsigned long iAuxTimeout,          // Specify the timeout time for USB-assisted download data, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
                                                        unsigned long iInterTimeout);       // Specify the timeout time for USB interrupt upload data, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
typedef unsigned long (__stdcall *pCH375SetBufUploadEx)(  									// Set internal buffer upload mode
														unsigned long iIndex,  				// Specify the serial number of the CH375 device, 0 corresponds to the first device
														unsigned long iEnableOrClear,		// If it is 0, the internal buffer upload mode is disabled, and direct upload is used. If it is not 0, the internal buffer upload mode is enabled and the existing data in the buffer is cleared.
														unsigned long iPipeNum,				// endpoint number, valid values are 1 to 8
														unsigned long BufSize );   			// Buffer size per packet, maximum 4MB
typedef unsigned long (__stdcall *pCH375ClearBufUpload)(  									// Clear the internal buffer, no need to suspend the internal buffer upload mode
														unsigned long iIndex,  				// Specify the serial number of the CH375 device, 0 corresponds to the first device
														unsigned long iPipeNum );  			// endpoint number, valid values are 1 to 8
typedef unsigned long (__stdcall *pCH375QueryBufUploadEx)(  								// Query the number of existing data packets and the total number of bytes in the internal upload buffer, return TRUE if successful, FALSE if failed
														unsigned long iIndex,				// Specify the serial number of the CH375 device, 0 corresponds to the first device
														unsigned long iPipeNum,				// endpoint number, valid values are 1 to 8
														unsigned long* oPacketNum,			// returns the number of existing packets in the internal buffer
														unsigned long* oTotalLen );  		// Returns the total bytes of existing packets in the internal buffer
typedef unsigned long (__stdcall *pCH375GetConfigDescr)(                                    // Read configuration descriptor
	                                                    unsigned long iIndex,   			// Specify the serial number of the CH375 device
	                                                    void* oBuffer,          			// points to a buffer large enough to hold the descriptor
	                                                    unsigned long* ioLength );  		// Point to the length unit, the length to be read when input, and the actual read length after return
typedef unsigned long (__stdcall *pCH375WriteData)(		                                    // write out data block
	                                                    unsigned long iIndex,   			// Specify the serial number of the CH375 device
	                                                    void* oBuffer,         				// Point to a buffer where the data to be written is placed
	                                                    unsigned long* ioLength );  		// Point to the length unit, the length to be written out when input, and the length actually written out after return
typedef unsigned long (__stdcall *pCH375ReadData)(		                                    // read data block
	                                                    unsigned long iIndex,   			// Specify the serial number of the CH375 device
	                                                    void* oBuffer,          			// Point to a buffer large enough to hold the read data
	                                                    unsigned long* ioLength );  		// Point to the length unit, the length to be read when input, and the actual read length after return
typedef unsigned long (__stdcall *pCH375ReadEndP)(		                                    // read data block
	                                                    unsigned long iIndex,       		// Specify the serial number of the CH375 device
                                                    	unsigned long iPipeNum,     		// Endpoint number, valid values are 1 to 8.
														void* iBuffer,              		// Point to a buffer large enough to hold the read data
														unsigned long*	ioLength);  		// Point to the length unit, the length to be read when input, and the actual read length after return
typedef unsigned long (__stdcall *pCH375WriteEndP)(		                                    // write out data block
	                                                    unsigned long iIndex,       		// Specify the serial number of the CH375 device
														unsigned long iPipeNum,     		// Endpoint number, valid values are 1 to 8.
														void* iBuffer,              		// Point to a buffer where the data to be written is placed
	 													unsigned long*	ioLength);  		// Point to the length unit, the length to be written out when input, and the length actually written out after return
HMODULE 				hModule ;
BOOL    				gOpen ;
unsigned long   		gIndex ;										
pCH375OpenDevice  		pOpenDev;
pCH375CloseDevice 		pCloseDev;
pCH375SetTimeoutEx  	pSetTimeout;
pCH375SetBufUploadEx 	pSetBufUpload;
pCH375ClearBufUpload 	pClearBufUpload;
pCH375QueryBufUploadEx 	pQueryBufUploadEx;
pCH375ReadData    		pReadData;
pCH375WriteData   		pWriteData;
pCH375ReadEndP    		pReadDataEndP;
pCH375WriteEndP   		pWriteDataEndP;
pCH375GetConfigDescr 	pGetConfigDescr;
#endif

bool AfxDevIsOpened;  													  		// Current status of device : ON or OFF
bool AfxUsbHighDev = true;
unsigned long   USB_PACKET;
unsigned long   MaxBitsPerBulk = 0;
unsigned long   MaxBytesPerBulk = 0;
unsigned long 	CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; 	  		// USB PKT command code(1B)+length(2B)+data
unsigned long 	CMDPKT_DATA_MAX_BITS   =  (CMDPKT_DATA_MAX_BYTES_USBHS/2);      // USB PKT maximum number of data bits per packet, each data bit requires two TCK composed of low to high bit bands
unsigned char 	DataUpEndp = 0, DataDnEndp = 0;
unsigned short 	BulkInEndpMaxSize = 512, BulkOutEndpMaxSize = 512;

JtagPinStatus usb20Jtag = {0, 0, 0};									  		// Initialize device pin status

static char *HexToString(uint8_t *buf, unsigned int size)
{
	unsigned int i;
	char *str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2*i, "%02x ", buf[i]);
	return str;
}

/**
 *  USB20Jtag_Write - USB20Jtag witre method
 *  @param oBuffer    Point to a buffer to place the data to be written out
 *  @param ioLength   Point to the length unit. When entering, it is the length to be written out. When returning, it is the actual length to be written out
 * 
 *  @return 		  success returns 1 and failure returns 0
 */
static int USB20Jtag_Write(void* oBuffer,unsigned long* ioLength)					   
{
    unsigned long wlength = *ioLength;
    int ret = pWriteData(gIndex, oBuffer, ioLength);
    LOG_DEBUG_IO("(size=%d, DataDnEndp=%d, buf=[%s]) -> %" PRIu32, wlength, DataDnEndp, HexToString((uint8_t*)oBuffer, *ioLength),
		       *ioLength);
    return ret;
}

/**
 * USB20Jtag_Read - USB20Jtag read method
 * @param oBuffer  	Point to a buffer large enough to hold the read data
 * @param ioLength 	Point to the length unit. When input, it is the length to be read, and when returned, it is the length actually read
 * 
 * @return 			success returns 1 and failure returns 0
 */
static int USB20Jtag_Read(void* oBuffer,unsigned long* ioLength)					   
{
    unsigned long rlength = *ioLength, packetNum, bufferNum, RI, RLen, WaitT = 0, timeout = 20;
	int ret = false;

	// The maximum allowable data for a single reading is 4096 bytes. If it exceeds 4096b, it shall be calculated as 4096 bytes
	if (rlength > HW_TDO_BUF_SIZE)
		rlength = HW_TDO_BUF_SIZE;

	RI = 0;
	while (1)
	{
		RLen = 8192;
		if ( !pQueryBufUploadEx(gIndex, DataUpEndp, &packetNum, &bufferNum))
			break;

		if (!pReadDataEndP(gIndex, DataUpEndp, oBuffer+RI, &RLen))
		{
			LOG_ERROR("USB20Jtag_Read read data failure.");
			goto Exit;
		}
		RI += RLen;
		if (RI >= *ioLength)
			break;
		if (WaitT++ >= timeout)
			break;
		Sleep(1);
	}
    LOG_DEBUG_IO("(size=%d, DataDnEndp=%d, buf=[%s]) -> %" PRIu32, rlength, DataUpEndp,HexToString((uint8_t*)oBuffer, *ioLength),*ioLength);
	ret = true;
Exit:
    *ioLength = RI;
	return ret;
}

/**
 * USB20Jtag_ClockTms - The function function is used to change the TMS value at the rising edge of TCK to switch its tap state
 * @param BitBangPkt 	Protocol package
 * @param tms 		 	TMS value to be changed
 * @param BI		 	Protocol packet length
 * 	
 * @return 			 	Return the length of the protocol package
 */
static unsigned long USB20Jtag_ClockTms(unsigned char* BitBangPkt, int tms, unsigned long BI)
{
	unsigned char cmd = 0;

	if (tms == 1) 
		cmd = TMS_H;
	else 
		cmd = TMS_L; 
	BitBangPkt[BI++] = cmd | TDI_H | TCK_L;
	BitBangPkt[BI++] = cmd | TDI_H | TCK_H;

	usb20Jtag.TMS = cmd;
	usb20Jtag.TDI = TDI_H;
	usb20Jtag.TCK = TCK_H;

	return BI;
}

/**
 * USB20Jtag_IdleClock - Function function to ensure that the clock is pulled down
 * @param BitBangPkt 	 Protocol package
 * @param BI  		 	 Protocol packet length
 * 
 * @return 			 	 Return the length of the protocol package
 */
static unsigned long USB20Jtag_IdleClock(unsigned char* BitBangPkt, unsigned long BI)
{
	unsigned char byte = 0;
	byte |= usb20Jtag.TMS ? TMS_H : TMS_L;
	byte |= usb20Jtag.TDI ? TDI_H : TDI_L;
	BitBangPkt[BI++] = byte;

	return BI;
}

/**
 * USB20Jtag_TmsChange - Function function to switch the state by changing the value of TMS
 * @param tmsValue 		 The TMS values to be switched form one byte of data according to the switching order
 * @param step 	   		 The number of bits that need to read the tmsvalue value
 * @param skip 	   		 Count from the skip bit of tmsvalue to step
 * 
 */
static void USB20Jtag_TmsChange(const unsigned char* tmsValue, int step, int skip)
{
	int i;
	unsigned long BI,retlen,TxLen; 
	unsigned char BitBangPkt[4096] = "";

	BI = USB20Jtag_CMD_HEADER;
	retlen = USB20Jtag_CMD_HEADER;
	LOG_DEBUG_IO("(TMS Value: %02x..., step = %d, skip = %d)", tmsValue[0], step, skip);

	for (i = skip; i < step; i++)
	{
		retlen = USB20Jtag_ClockTms(BitBangPkt,(tmsValue[i/8] >> (i % 8)) & 0x01, BI);
		BI = retlen;
	}
	retlen = USB20Jtag_IdleClock(BitBangPkt, BI);
	BI = retlen;

	BitBangPkt[0] = USB20Jtag_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = (unsigned char)BI - USB20Jtag_CMD_HEADER;
	BitBangPkt[2] = 0;

	TxLen = BI;

	if (!USB20Jtag_Write(BitBangPkt, &TxLen) && (TxLen != BI))
	{
		LOG_ERROR("JTAG Write send usb data failure.");
		return NULL;
	}
}

/**
 * USB20Jtag_TMS - By usbtag_ execute_ Queue call
 * @param cmd 	   Upper layer transfer command parameters
 * 
 */
static void USB20Jtag_TMS(struct tms_command *cmd)
{
	LOG_DEBUG_IO("(step: %d)", cmd->num_bits);
	USB20Jtag_TmsChange(cmd->bits, cmd->num_bits, 0);
}

/**
 * USB20Jtag_Reset - Usb20jtag reset tap status function
 * @brief If there are more than six consecutive TCKs and TMS is high, the state machine can be set to test logic reset state
 * 
 */
static int USB20Jtag_Reset()
{
    unsigned char BitBang[512]="",BI,i;
	unsigned long TxLen;

	BI = USB20Jtag_CMD_HEADER; 
	for(i=0;i<7;i++)
	{
		BitBang[BI++] = TMS_H | TDI_H | TCK_L;
		BitBang[BI++] = TMS_H | TDI_H | TCK_H;
	}
	BitBang[BI++] = TMS_H | TDI_H | TCK_L;

	BitBang[0] = USB20Jtag_CMD_JTAG_BIT_OP;
	BitBang[1] = BI-USB20Jtag_CMD_HEADER;
	BitBang[2] = 0; 

	TxLen = BI;
	if( !USB20Jtag_Write(BitBang,&TxLen) && (TxLen!=BI) )
	{
		LOG_ERROR("JTAG_Init send usb data failure.");
		return false;
	}
	return true;
}

/**
 * USB20Jtag_MovePath - Get the current tap status and switch to the status TMS value passed down by CMD
 * @param cmd Upper layer transfer command parameters
 * 
 */
static void USB20Jtag_MovePath(struct pathmove_command *cmd)
{
	int i;
	unsigned long BI, retlen, TxLen;
	unsigned char BitBangPkt[4096] = "";

	BI = USB20Jtag_CMD_HEADER;

	LOG_DEBUG_IO("(num_states=%d, last_state=%d)",
		  cmd->num_states, cmd->path[cmd->num_states - 1]);
	for (i = 0; i < cmd->num_states; i++) {
		if (tap_state_transition(tap_get_state(), false) == cmd->path[i])
			retlen = USB20Jtag_ClockTms(BitBangPkt, 0, BI);
			BI = retlen;
		if (tap_state_transition(tap_get_state(), true) == cmd->path[i])
			retlen = USB20Jtag_ClockTms(BitBangPkt, 1, BI);
			BI = retlen;
		tap_set_state(cmd->path[i]);
	}
	retlen = USB20Jtag_IdleClock(BitBangPkt, BI);
	BI = retlen;

	BitBangPkt[0] = USB20Jtag_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = (unsigned char)BI - USB20Jtag_CMD_HEADER;
	BitBangPkt[2] = 0;

	TxLen = BI;

	if (!USB20Jtag_Write(BitBangPkt, &TxLen) && (TxLen != BI))
	{
		LOG_ERROR("JTAG Write send usb data failure.");
		return NULL;
	}
}

/**
 * USB20Jtag_MoveState - Switch tap state to target state stat
 * @param stat Pre switching target path
 * @param skip Number of bits to skip
 * 
 */
static void USB20Jtag_MoveState(tap_state_t state, int skip)
{
	uint8_t tms_scan;		
	int tms_len;

	LOG_DEBUG_IO("(from %s to %s)", tap_state_name(tap_get_state()),
		  tap_state_name(state));
	if (tap_get_state() == state)
		return;
	tms_scan = tap_get_tms_path(tap_get_state(), state);
	tms_len = tap_get_tms_path_len(tap_get_state(), state);
	USB20Jtag_TmsChange(&tms_scan, tms_len, skip);
	tap_set_state(state);
}

/**
 * USB20Jtag_WriteRead - Usb20jtag batch read / write function
 * @param bits 
 * @param nb_bits 		 Incoming data length
 * @param scan			 Transmission mode of incoming data
 * 
 */
static void USB20Jtag_WriteRead(uint8_t *bits, int nb_bits, enum scan_type scan)
{
	int nb8 = nb_bits / 8;
	int nb1 = nb_bits % 8;
	int nbfree_in_packet, i, trans = 0;
	bool IsRead = false;
	uint8_t TMS_Bit, TDI_Bit;
	uint8_t *tdos = calloc(1, nb_bits / 8 + 1);
	static uint8_t BitBangPkt[SF_PACKET_BUF_SIZE];
	static uint8_t byte0[SF_PACKET_BUF_SIZE];
	unsigned char temp[512] = "";
	uint8_t clearHW[4096] = "";
	unsigned long BI = 0,DLen,TxLen,RxLen,DI,PktDataLen,templong;
	uint32_t retlen;
	int ret = ERROR_OK;

	// The last TDI bit will be output according to the BitBang mode, and its NB1 shall not be 0, so that it can output the last 1 bit of data when TMS changes
	if (nb8 > 0 && nb1 == 0) 
	{
		nb8--;
		nb1 = 8;
	}

	DLen = USB_PACKET;
	IsRead = (scan == SCAN_IN || scan == SCAN_IO);

	DI = BI = 0;
	while (DI < nb8)
	{
		// Build package
		if ((nb8 - DI) > CMDPKT_DATA_MAX_BYTES)
			PktDataLen = CMDPKT_DATA_MAX_BYTES;
		else 
			PktDataLen = nb8 - DI;
		if (IsRead)
			BitBangPkt[BI++] = USB20Jtag_CMD_JTAG_DATA_SHIFT_RD;
		else
			BitBangPkt[BI++] = USB20Jtag_CMD_JTAG_DATA_SHIFT;
		BitBangPkt[BI++] = (uint8_t)(PktDataLen >> 0)&0xFF;
		BitBangPkt[BI++] = (uint8_t)(PktDataLen >> 8)&0xFF;
		if (bits)
			memcpy(&BitBangPkt[BI], &bits[DI], PktDataLen);
		else
			memcpy(&BitBangPkt[BI], byte0, PktDataLen);
		BI += PktDataLen;

		// If the data needs to be read back, judge the current Bi value and issue the command
		if (IsRead) 
		{
			TxLen = BI;
			if( !USB20Jtag_Write(BitBangPkt,&TxLen) && (TxLen!=BI) )
			{
				LOG_ERROR("USB20Jtag_WriteRead write usb data failure.");
				return NULL;
			}
			BI = 0;

			while (ret == ERROR_OK && PktDataLen > 0)
			{
				RxLen = PktDataLen + USB20Jtag_CMD_HEADER;
				if (!(ret = USB20Jtag_Read(temp, &RxLen))) 
				{
					LOG_ERROR("USB20Jtag_WriteRead read usb data failure.\n");
					return NULL;
				}
				if (RxLen != 0)
					memcpy(&tdos[DI], &temp[USB20Jtag_CMD_HEADER], (RxLen - USB20Jtag_CMD_HEADER));
				PktDataLen -= RxLen;
			}
		}

		DI += PktDataLen;
		
		// In the transmission process, if the read back is not performed, the command will be issued when the command packet length is about to reach saturation
		if ((SF_PACKET_BUF_SIZE-BI) < USB_PACKET || (SF_PACKET_BUF_SIZE-BI) == USB_PACKET)
		{
			TxLen = BI;
			if( !USB20Jtag_Write(BitBangPkt,&TxLen) && (TxLen!=BI) )
			{
				LOG_ERROR("USB20Jtag_WriteRead send usb data failure.");
				return NULL;
			}
			BI = 0;
		}
	}

	// Build a command package that outputs the last bit of TDI data
	if (bits)
	{
		BitBangPkt[BI++] = IsRead?USB20Jtag_CMD_JTAG_BIT_OP_RD:USB20Jtag_CMD_JTAG_BIT_OP;	
		DLen = nb1*2;
		BitBangPkt[BI++] = (uint8_t)(DLen>>0)&0xFF;
		BitBangPkt[BI++] = (uint8_t)(DLen>>8)&0xFF; 		    // Upper 8 bits of command length
		TMS_Bit = TMS_L;
		for(i = 0; i < nb1; i++)
		{
			if((bits[nb8] >> i) & 0x01)
				TDI_Bit = TDI_H;
			else
				TDI_Bit = TDI_L;	
			if((i + 1) == nb1)									// The last bit is output in exit1 Dr status
				TMS_Bit = TMS_H;
			BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_L;
			BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_H;
		}
		BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_L;
	}

	// Read the last byte of data in BitBang mode
	if (nb1 && IsRead) 
	{
		TxLen = BI;
		if( !USB20Jtag_Write(BitBangPkt,&TxLen) && (TxLen!=BI) )
		{
			LOG_ERROR("USB20Jtag_WriteRead send usb data failure.");
			return NULL;
		}
		BI = 0;

		RxLen = TxLen + USB20Jtag_CMD_HEADER;
		if (!(ret = USB20Jtag_Read(temp, &RxLen))) 
		{
			LOG_ERROR("USB20Jtag_WriteRead read usb data failure.");
		}

		ret = ERROR_OK;
		for ( i = 0; ret == ERROR_OK && i < nb1; i++)
		{
			if (temp[USB20Jtag_CMD_HEADER + i] & 1)
				tdos[nb8] |= (1 << i);
			else
				tdos[nb8] &= ~(1 << i);
		}
	}

	// Clear the unprocessed commands in this batch read-write function
	if (BI > 0) 
	{
		TxLen = BI;
		if( !USB20Jtag_Write(BitBangPkt,&TxLen) && (TxLen!=BI) )
		{
			LOG_ERROR("USB20Jtag_WriteRead send usb data failure.");
			return NULL;
		}
		BI = 0;
	}

	if (bits)
		memcpy(bits, tdos, DIV_ROUND_UP(nb_bits, 8));
	free(tdos);
	LOG_DEBUG_IO("bits %d str value: [%s].\n", DIV_ROUND_UP(nb_bits, 8), HexToString(bits, DIV_ROUND_UP(nb_bits, 8)));

	// When TCK and TDI are pulled down to low level, TDI sampling shall be at the rising edge of TCK. If the state does not change, TDI sampling may occur at the falling edge of TCK
	BI = USB20Jtag_CMD_HEADER;
	BI = USB20Jtag_IdleClock(BitBangPkt, BI);
	
	BitBangPkt[0] = USB20Jtag_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = (unsigned char)BI - USB20Jtag_CMD_HEADER;
	BitBangPkt[2] = 0;

	TxLen = BI;
	if (!USB20Jtag_Write(BitBangPkt, &TxLen) && (TxLen != BI))
	{
		LOG_ERROR("JTAG Write send usb data failure.");
		return NULL;
	}
}

static void USB20JTAG_RunTest(int cycles, tap_state_t state)
{
	LOG_DEBUG_IO("%s(cycles=%i, end_state=%d)", __func__, cycles, state);
	USB20Jtag_MoveState(TAP_IDLE, 0);

	USB20Jtag_WriteRead(NULL, cycles, SCAN_OUT);
	USB20Jtag_MoveState(state, 0);
}

static void USB20JTAG_TableClocks(int cycles)
{
	LOG_DEBUG_IO("%s(cycles=%i)", __func__, cycles);
	USB20Jtag_WriteRead(NULL, cycles, SCAN_OUT);
}

/**
 * USB20JTAG_Scan - Switch to shift-dr or shift-ir status
 * @param cmd 	    Upper layer transfer command parameters
 * 
 */
static int USB20JTAG_Scan(struct scan_command *cmd)
{
	int scan_bits;
	uint8_t *buf = NULL;
	enum scan_type type;
	int ret = ERROR_OK;
	static const char * const type2str[] = { "", "SCAN_IN", "SCAN_OUT", "SCAN_IO" };
	char *log_buf = NULL;

	type = jtag_scan_type(cmd);
	scan_bits = jtag_build_buffer(cmd, &buf);

	if (cmd->ir_scan)
		USB20Jtag_MoveState(TAP_IRSHIFT, 0);
	else
		USB20Jtag_MoveState(TAP_DRSHIFT, 0);

	log_buf = HexToString(buf, DIV_ROUND_UP(scan_bits, 8));
	LOG_DEBUG_IO("%s(scan=%s, type=%s, bits=%d, buf=[%s], end_state=%d)", __func__,
		  cmd->ir_scan ? "IRSCAN" : "DRSCAN",
		  type2str[type],
		  scan_bits, log_buf, cmd->end_state);
	free(log_buf);

	USB20Jtag_WriteRead(buf, scan_bits, type);

	ret = jtag_read_buffer(buf, cmd);
	free(buf);

	USB20Jtag_MoveState(cmd->end_state, 1);

	return ret;
}

static void USB20Jtag_Sleep(int us)
{
	LOG_DEBUG_IO("%s(us=%d)",  __func__, us);
	jtag_sleep(us);
}

/**
 * USB20Jtag_ClockRateInit - Initialize usb20jtag clock rate
 * @param Index 			 USB20justag device handle
 * @param iClockRate		 Set usb20jtag clock parameters (0-5)
 * 
 * iClockRate Value:
 * args:				0 - - - - 1 - - - - 2 - - - - 3 - - - - 4
 *      	    		|		  |		    |		  |			|
 * clockeRate:	 	 2.25MHz	4.5MHz	   9MHz		18MHz	  36MHz
 * 
 */
static int USB20Jtag_ClockRateInit(unsigned long Index,unsigned char iClockRate)
{
    unsigned char mBuffer[256] = "";
	unsigned long mLength ,i,DescBufSize;
	bool  RetVal = false;
	unsigned char DescBuf[256] = "";
	unsigned char clearBuffer[8192] = "";
	unsigned long TxLen = 8192;
	PUSB_ENDPOINT_DESCRIPTOR       EndpDesc;
	PUSB_COMMON_DESCRIPTOR         UsbCommDesc;

    if( (iClockRate > 4) )
		goto Exit;

	// The obtained USB speed is 480mhz USB2 by default 0 high speed, 12Mhz USB full speed if connected to the full speed hub
	DescBufSize = sizeof(DescBuf);
	if( !pGetConfigDescr(gIndex,DescBuf,&DescBufSize))
		goto Exit;

	// Judge according to the size of USB bulk endpoint. If the endpoint size is 512b, it is 480mhz USB2 0 high speed
	AfxUsbHighDev = false;
	i = 0;
	while(i < DescBufSize)
	{
		UsbCommDesc = (PUSB_COMMON_DESCRIPTOR)&DescBuf[i];
		if( UsbCommDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE )
		{
			EndpDesc = (PUSB_ENDPOINT_DESCRIPTOR)&DescBuf[i];
			if( (EndpDesc->bmAttributes&0x03)==USB_ENDPOINT_TYPE_BULK )
			{
				if((EndpDesc->bEndpointAddress&USB_ENDPOINT_DIRECTION_MASK))
				{
					DataUpEndp = EndpDesc->bEndpointAddress&(~USB_ENDPOINT_DIRECTION_MASK);
					BulkInEndpMaxSize = EndpDesc->wMaxPacketSize;			// Endpoint size
					AfxUsbHighDev = (EndpDesc->wMaxPacketSize == 512);		// USB speed type
					
				}
				else
				{
					BulkOutEndpMaxSize = EndpDesc->wMaxPacketSize;
					DataDnEndp = EndpDesc->bEndpointAddress;				
				}
			}
		}
		i += UsbCommDesc->bLength;
	}
	// Set the maximum data length of each command packet according to the USB speed	
	if(AfxUsbHighDev)
	{
		USB_PACKET 			   =  USB_PACKET_USBHS;
		CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; 				// 507 bytes
	}
	else				
	{
		USB_PACKET			   =  USB_PACKET_USBFS;
		CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBFS; 				// 59 bytes
	}
	CMDPKT_DATA_MAX_BITS = CMDPKT_DATA_MAX_BYTES/16*16/2;  					// The maximum number of bits transmitted by each command shall be represented by two bytes, taking an integer multiple of 2 bytes

	// The number of bits per batch transmission is calculated according to the size of the hardware buffer, and the multi command package is assembled
	MaxBitsPerBulk = HW_TDO_BUF_SIZE/CMDPKT_DATA_MAX_BYTES*CMDPKT_DATA_MAX_BITS;  
	// Calculate the number of words transmitted in each batch according to the size of the hardware buffer, and spell the package with multiple commands
	MaxBytesPerBulk = HW_TDO_BUF_SIZE - (HW_TDO_BUF_SIZE+CMDPKT_DATA_MAX_BYTES-1)/CMDPKT_DATA_MAX_BYTES*3;;

	// USB bulk upload data adopts driver buffer upload mode, which is more efficient than direct upload
	pSetBufUpload(gIndex, true, DataUpEndp, 4096);							// It means that the upload endpoint is in the buffer upload mode, and the buffer size is 8192
	pClearBufUpload(gIndex, DataUpEndp);									// Clear buffer data in driver

	if (!USB20Jtag_Read(clearBuffer, &TxLen))								// Read hardware buffer data
	{	
		LOG_ERROR("USB20Jtag_WriteRead read usb data failure.");
		return 0;
	}

	// Build USB JTAG initialization command package and execute it
	i = 0;
	mBuffer[i++] = USB20Jtag_CMD_JTAG_INIT;
	mBuffer[i++] = 6;
	mBuffer[i++] = 0;
	mBuffer[i++] = 0;														// Reserved byte
	mBuffer[i++] = iClockRate;												// JTAG clock speed
	i += 4; 																// Reserved byte
	mLength = i;
	if( !USB20Jtag_Write(mBuffer,&mLength) || (mLength!=i) )
		goto Exit;

	// Read the return value and judge whether the initialization is successful
	mLength = 4;
	if( !USB20Jtag_Read(mBuffer,&mLength) || (mLength!=4) ) 
	{
		LOG_ERROR("USB20Jtag clock initialization failed.\n");
		goto Exit;
	}
	RetVal = ( (mBuffer[0] == USB20Jtag_CMD_JTAG_INIT) && (mBuffer[USB20Jtag_CMD_HEADER]==0) );
Exit:
	return (RetVal);
}

static int usb20jtag_execute_queue(void)
{ 
	struct jtag_command *cmd;
	static int first_call = 1;
	int ret = ERROR_OK;
	unsigned long TxLen = 8192;
	unsigned char clearBuffer[8192] = "";

	if (first_call) {
		first_call--;
		USB20Jtag_Reset();
	}

	for (cmd = jtag_command_queue; ret == ERROR_OK && cmd;
	     cmd = cmd->next) {
		switch (cmd->type) {
		case JTAG_RESET:
			USB20Jtag_Reset();
			break;
		case JTAG_RUNTEST:
			USB20JTAG_RunTest(cmd->cmd.runtest->num_cycles,
				       cmd->cmd.runtest->end_state);
			break;
		case JTAG_STABLECLOCKS:
			USB20JTAG_TableClocks(cmd->cmd.stableclocks->num_cycles);
			break;
		case JTAG_TLR_RESET:
			USB20Jtag_MoveState(cmd->cmd.statemove->end_state, 0);
			break;
		case JTAG_PATHMOVE:
			USB20Jtag_MovePath(cmd->cmd.pathmove);
			break;
		case JTAG_TMS:
			USB20Jtag_TMS(cmd->cmd.tms);
			break;
		case JTAG_SLEEP:
			USB20Jtag_Sleep(cmd->cmd.sleep->us);
			break;
		case JTAG_SCAN:
			ret = USB20JTAG_Scan(cmd->cmd.scan);
			break;
		default:
			LOG_ERROR("BUG: unknown JTAG command type 0x%X",
				  cmd->type);
			ret = ERROR_FAIL;
			break;
		}
	}
	return ret;
}

static int usb20jtag_init(void)
{ 
    if(hModule == 0)
	{
		hModule = LoadLibrary("CH375DLL.dll");
        if (hModule)
        {
            pOpenDev    		= (pCH375OpenDevice)  GetProcAddress(hModule, "CH375OpenDevice"); 
			pCloseDev   		= (pCH375CloseDevice) GetProcAddress(hModule, "CH375CloseDevice"); 
			pReadData   		= (pCH375ReadData)  GetProcAddress(hModule, "CH375ReadData"); 
			pWriteData  		= (pCH375WriteData) GetProcAddress(hModule, "CH375WriteData"); 
			pReadDataEndP   	= (pCH375ReadEndP)  GetProcAddress(hModule, "CH375ReadEndP"); 
			pWriteDataEndP  	= (pCH375WriteEndP) GetProcAddress(hModule, "CH375WriteEndP"); 
			pSetTimeout 		= (pCH375SetTimeoutEx) GetProcAddress(hModule, "CH375SetTimeout"); 
			pSetBufUpload 		= (pCH375SetBufUploadEx) GetProcAddress(hModule, "CH375SetBufUploadEx"); 
			pClearBufUpload 	= (pCH375ClearBufUpload) GetProcAddress(hModule, "CH375ClearBufUpload"); 
			pQueryBufUploadEx 	= (pCH375QueryBufUploadEx) GetProcAddress(hModule, "CH375QueryBufUploadEx");
            pGetConfigDescr  	= (pCH375GetConfigDescr) GetProcAddress(hModule, "CH375GetConfigDescr"); 
			if(pOpenDev == NULL || pCloseDev == NULL || pSetTimeout == NULL || pSetBufUpload == NULL || pClearBufUpload == NULL || pQueryBufUploadEx == NULL || pReadData == NULL || pWriteData == NULL || pReadDataEndP == NULL || pWriteDataEndP == NULL || pGetConfigDescr == NULL)
			{
				LOG_ERROR("GetProcAddress error ");
				return ERROR_FAIL;
			}
        }
        AfxDevIsOpened = pOpenDev(gIndex);
        if (AfxDevIsOpened == false)
        {
            gOpen = false;
            LOG_ERROR("USB20JTAG Open Error.");
            return ERROR_FAIL;
        }
		pSetTimeout(gIndex,1000,1000,1000,1000);
        USB20Jtag_ClockRateInit(0, 4);

        tap_set_state(TAP_RESET);
    }
	return 0;
}

// Exit function
static int usb20jtag_quit(void)
{ 
	// Set all signal lines to low level before exiting
    uint32_t retlen = 4;
    unsigned char byte[4] = {USB20Jtag_CMD_JTAG_BIT_OP, 0x01, 0x00, 0x00};
    USB20Jtag_Write(byte, &retlen);

    if (AfxDevIsOpened) 
    {
        pCloseDev(gIndex);
        LOG_INFO("Close the USB20JTAG.");
		AfxDevIsOpened = false;
    }
	return 0;
}

COMMAND_HANDLER(usb20jtag_handle_vid_pid_command)
{
	// TODO
	return ERROR_OK;
}

COMMAND_HANDLER(usb20jtag_handle_pin_command)
{	
	// TODO
	return ERROR_OK;
}
 
static const struct command_registration usb20jtag_subcommand_handlers[] = {
	{
		.name = "vid_pid",
		.handler = usb20jtag_handle_vid_pid_command,
		.mode = COMMAND_CONFIG,
		.help = "",			
		.usage = "",
	},
	{
		.name = "pin",
		.handler = usb20jtag_handle_pin_command,
		.mode = COMMAND_ANY,
		.help = "",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration usb20jtag_command_handlers[] = {
	{
		.name = "usb20jtag",
		.mode = COMMAND_ANY,
		.help = "perform usb20jtag management",
		.chain = usb20jtag_subcommand_handlers,
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

static struct jtag_interface usb20jtag_interface = {
	.supported = DEBUG_CAP_TMS_SEQ,
	.execute_queue = usb20jtag_execute_queue,
};

struct adapter_driver usb20jtag_adapter_driver = {
	.name = "usb20jtag",
	.transports = jtag_only,
	.commands = usb20jtag_command_handlers,

	.init = usb20jtag_init,
	.quit = usb20jtag_quit,

	.jtag_ops = &usb20jtag_interface,
};
