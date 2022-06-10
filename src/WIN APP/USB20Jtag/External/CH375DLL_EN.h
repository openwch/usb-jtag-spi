// 2003.09.08, 2003.12.28, 2004.10.15, 2004.12.05, 2004.12.10, 2005.01.20, 2005.02.23, 2005.07.15, 2005.08.17,2004.10.12,2,2.1522.
//************************************************ ********************
//** Application layer interface library of USB bus interface chip CH375 **
//** Copyright(C)WCH 2022                                                **
//** http://wch.cn **
//*************************************************************************
//** CH375DLL V3.1 **
//** DLL for USB interface chip CH37x **
//** Development environment: C, VC5.0 **
//** Support chip: CH372/CH375/CH376/CH378, and WCH USB external interface device interface **
//** Support USB 2.0/3.0 12Mbps/480Mbps/3Gbps speed
//** Operating environment: Windows 2000/XP/7/8/8.1/10/11 **
//************************************************ ********************
//

#ifndef _CH375_DLL_H
#define _CH375_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#define mOFFSET( s, m ) ( (ULONG) (ULONG_PTR)& ( ( ( s * ) 0 ) -> m ) ) // Define a macro that gets the relative offset address of a structure member

#ifndef max
#define max( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )               // larger value
#endif

#ifndef min
#define min( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )               // smaller value
#endif

#ifdef ExAllocatePool
#undef ExAllocatePool                                                   // delete memory allocation with TAG
#endif

#ifndef NTSTATUS
	typedef LONG NTSTATUS;                                              // return status
#endif


	typedef struct _USB_SETUP_PKT { // Data request packet structure in the setup phase of USB control transfer
		UCHAR mUspReqType;          // 00H request type
		UCHAR mUspRequest;          // 01H request code
		union {
			struct {
				UCHAR mUspValueLow; // 02H Value parameter low byte
				UCHAR mUspValueHigh;// 03H Value parameter high byte
			};
			USHORT mUspValue;       // 02H-03H value parameters
		};
		union {
			struct {
				UCHAR mUspIndexLow; // 04H index parameter low byte
				UCHAR mUspIndexHigh;// 05H index parameter high byte
			};
			USHORT mUspIndex;       // 04H-05H index parameter
		};
		USHORT mLength;             // 06H-07H data length of data stage
	} mUSB_SETUP_PKT, *mPUSB_SETUP_PKT;


#define mCH375_PACKET_LENGTH 64     // The length of the data packet supported by CH375
#define mCH375_PKT_LEN_SHORT 8      // The length of short packets supported by CH375


	typedef struct _WIN32_COMMAND { // Define the WIN32 command interface structure
		union {
			ULONG mFunction;        // Specify function code or pipe number when inputting
			NTSTATUS mStatus;       // return operation status when output
		};
		ULONG mLength;              // access length, return the length of subsequent data
		union {
			mUSB_SETUP_PKT mSetupPkt;            // Data request in the setup phase of USB control transfer
			UCHAR mBuffer[mCH375_PACKET_LENGTH]; // data buffer, the length is 0 to 255B
		};
	} mWIN32_COMMAND, *mPWIN32_COMMAND;


// WIN32 application layer interface command
#define IOCTL_CH375_COMMAND ( FILE_DEVICE_UNKNOWN << 16 | FILE_ANY_ACCESS << 14 | 0x0f37 << 2 | METHOD_BUFFERED ) // private interface

#define mWIN32_COMMAND_HEAD mOFFSET( mWIN32_COMMAND, mBuffer ) // Header length of WIN32 command interface

#define mCH375_MAX_NUMBER 16        // Maximum number of CH375 connected at the same time

#define mMAX_BUFFER_LENGTH 0x400000 // The maximum length of the data buffer is 4MB

#define mMAX_COMMAND_LENGTH ( mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH ) // maximum data length plus the length of the command structure header

#define mDEFAULT_BUFFER_LEN 0x0400  // The default length of the data buffer is 1024

#define mDEFAULT_COMMAND_LEN ( mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN ) // default data length plus the length of the command structure header


// CH375 endpoint address
#define mCH375_ENDP_INTER_UP 0x81   // The address of the interrupt data upload endpoint of CH375
#define mCH375_ENDP_AUX_DOWN 0x01   // The address of the auxiliary data download endpoint of CH375
#define mCH375_ENDP_DATA_UP 0x82    // The address of the data block upload endpoint of CH375
#define mCH375_ENDP_DATA_DOWN 0x02  // The address of the data block download endpoint of CH375


// Pipeline operation commands provided by the device layer interface
#define mPipeDeviceCtrl 0x00000004  // CH375 integrated control pipeline
#define mPipeInterUp 0x00000005     // CH375 interrupt data upload pipeline
#define mPipeDataUp 0x00000006      // CH375 data block upload pipeline
#define mPipeDataDown 0x00000007    // CH375 data block download pipeline
#define mPipeAuxDown 0x00000008     // Auxiliary data download pipeline of CH375

// function code of application layer interface
#define mFuncNoOperation 0x00000000 // no operation
#define mFuncGetVersion 0x00000001  // Get the driver version number
#define mFuncGetConfig 0x00000002   // Get the USB device configuration descriptor
#define mFuncSetExclusive 0x0000000b// Set exclusive use
#define mFuncResetDevice 0x0000000c // Reset the USB device
#define mFuncResetPipe 0x0000000d   // reset the USB pipe
#define mFuncAbortPipe 0x0000000e   // Cancel the data request of the USB pipe
#define mFuncSetTimeout 0x0000000f  // Set USB communication timeout
#define mFuncBufferMode 0x00000010  // Set the buffer upload mode and query the data length in the buffer
#define mFuncBufferModeDn 0x00000011// Set the buffer download mode and query the data length in the buffer

// USB device standard request code
#define mUSB_CLR_FEATURE 0x01
#define mUSB_SET_FEATURE 0x03
#define mUSB_GET_STATUS 0x00
#define mUSB_SET_ADDRESS 0x05
#define mUSB_GET_DESCR 0x06
#define mUSB_SET_DESCR 0x07
#define mUSB_GET_CONFIG 0x08
#define mUSB_SET_CONFIG 0x09
#define mUSB_GET_INTERF 0x0a
#define mUSB_SET_INTERF 0x0b
#define mUSB_SYNC_FRAME 0x0c

// The manufacturer-specific request type of CH375 control transmission
#define mCH375_VENDOR_READ 0xc0     // CH375 vendor-specific read operation through control transfer
#define mCH375_VENDOR_WRITE 0x40    // CH375 vendor-specific write operation through control transfer

// Manufacturer-specific request code for CH375 control transmission
#define mCH375_SET_CONTROL 0x51     // output control signal
#define mCH375_GET_STATUS 0x52      // Input status signal

// register bit definition
#define mBitInputRxd 0x02           // read only, RXD# pin input status, 1: high level, 0: low level
#define mBitInputReq 0x04           // read only, REQ# pin input status, 1: high level, 0: low level


// Bit definition of directly input status signal
#define mStateRXD 0x00000200        // RXD# pin input state, 1: high level, 0: low level
#define mStateREQ 0x00000400        // REQ# pin input state, 1: high level, 0: low level

#define MAX_DEVICE_PATH_SIZE 128    // Maximum number of characters for device name
#define MAX_DEVICE_ID_SIZE 64       // Maximum number of characters for device ID


typedef VOID ( CALLBACK * mPCH375_INT_ROUTINE ) ( // interrupt service callback routine
	PUCHAR	iBuffer );                            // Point to a buffer that provides the current interrupt characteristic data


HANDLE WINAPI CH375OpenDevice( // Open CH375 device, return the handle, invalid if error
	ULONG	iIndex );          // Specify the serial number of the CH375 device, 0 corresponds to the first device, -1 automatically searches for a device that can be opened and returns the serial number


VOID WINAPI CH375CloseDevice( // Close the CH375 device
	ULONG	iIndex );         // Specify the serial number of the CH375 device


ULONG WINAPI CH375GetVersion( ); // Get the DLL version number and return the version number


ULONG WINAPI CH375DriverCommand( // Directly pass the command to the driver, if there is an error, return 0, otherwise return the data length
	ULONG	iIndex,              // Specify the serial number of the CH375 device, the DLL above V1.6 can also be the handle after the device is opened
	mPWIN32_COMMAND	 ioCommand );// pointer to command structure
                                 // The program returns the data length after the call, and still returns the command structure, if it is a read operation, the data is returned in the command structure,
                                 // The returned data length is 0 when the operation fails, and the length of the entire command structure when the operation is successful. For example, if a byte is read, mWIN32_COMMAND_HEAD+1 is returned,
                                 // Before calling the command structure, provide: pipe number or command function code, length of access data (optional), data (optional)
                                 // After the command structure is called, it returns: the operation status code, the length of the subsequent data (optional),
                                 // The operation status code is the code defined by WINDOWS, you can refer to NTSTATUS.H,
                                 // The length of the subsequent data refers to the length of the data returned by the read operation, the data is stored in the subsequent buffer, and is generally 0 for the write operation


ULONG WINAPI CH375GetDrvVersion( ); // Get the driver version number, return the version number, or return 0 if there is an error


BOOL WINAPI CH375ResetDevice( // reset USB device
	ULONG	iIndex );         // Specify the serial number of the CH375 device


BOOL WINAPI CH375GetDeviceDescr( // read device descriptor
	ULONG	 iIndex,             // Specify the serial number of the CH375 device
	PVOID	 oBuffer,            // points to a buffer large enough to hold the descriptor
	PULONG	 ioLength );         // Point to the length unit, the length to be read when input, and the actual read length after return


BOOL WINAPI CH375GetConfigDescr( // Read configuration descriptor
	ULONG	 iIndex,             // Specify the serial number of the CH375 device
	PVOID	 oBuffer,            // points to a buffer large enough to hold the descriptor
	PULONG	 ioLength );         // Point to the length unit, the length to be read when input, and the actual read length after return


BOOL WINAPI CH375SetIntRoutine(        // Set interrupt service routine
	ULONG	iIndex,                    // Specify the serial number of the CH375 device
	mPCH375_INT_ROUTINE iIntRoutine ); // Specify the interrupt service callback program, if it is NULL, the interrupt service will be canceled, otherwise the program will be called when interrupted


BOOL WINAPI CH375ReadInter( // Read interrupt data
	ULONG	iIndex,         // Specify the serial number of the CH375 device
	PVOID	oBuffer,        // Point to a buffer large enough to hold the read interrupt data
	PULONG	ioLength );     // Point to the length unit, the length to be read when input, and the actual read length after return


BOOL WINAPI CH375AbortInter( // Abort interrupt data read operation
	ULONG	iIndex );        // Specify the serial number of the CH375 device


BOOL WINAPI CH375ReadData( // read data block
	ULONG	iIndex,        // Specify the serial number of the CH375 device
	PVOID	oBuffer,       // Point to a buffer large enough to hold the read data
	PULONG	ioLength );    // Point to the length unit, the length to be read when input, and the actual read length after return


BOOL WINAPI CH375AbortRead( // Abort data block read operation
	ULONG	 iIndex );      // Specify the serial number of the CH375 device


BOOL WINAPI CH375WriteData( // write out data block
	ULONG	iIndex,         // Specify the serial number of the CH375 device
	PVOID	iBuffer,        // Point to a buffer where the data to be written is placed
	PULONG	ioLength );     // Point to the length unit, the length to be written out when input, and the length actually written out after return


BOOL WINAPI CH375AbortWrite( // Abort data block write operation
	ULONG	iIndex );        // Specify the serial number of the CH375 device


BOOL WINAPI CH375WriteRead( // First write the standard data block (command), then read the standard data block (response)
	ULONG	iIndex,         // Specify the serial number of the CH375 device
	PVOID	iBuffer,        // Point to a buffer to place the data to be written, the length is not greater than mCH375_PACKET_LENGTH
	PVOID	oBuffer,        // Point to a large enough buffer, the length is not less than mCH375_PACKET_LENGTH, used to save the read data
	PULONG	ioLength );     // Point to the length unit, not greater than mCH375_PACKET_LENGTH, the length to be written out when input, and the length actually read after return


BOOL WINAPI CH375GetStatus( // Directly input data and status through CH375
	ULONG	iIndex,         // Specify the serial number of the CH375 device
	PULONG	iStatus );      // Point to a double word unit for saving status data
                            // Bit 7-bit 0 corresponds to D7-D0 pins of CH375, bit 9 corresponds to RXD# pin of CH375, bit 10 corresponds to REQ# pin of CH375


BOOL WINAPI CH375SetTimeout( // Set the timeout for reading and writing USB data
	ULONG	iIndex,          // Specify the serial number of the CH375 device
	ULONG	iWriteTimeout,   // Specify the timeout time for USB to write out data blocks, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
	ULONG	iReadTimeout );  // Specify the timeout time for USB read data block, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)


BOOL WINAPI CH375WriteAuxData( // write out auxiliary data
	ULONG	iIndex,            // Specify the serial number of the CH375 device
	PVOID	iBuffer,           // Point to a buffer where the data to be written is placed
	PULONG	ioLength );        // Point to the length unit, the length to be written out when input, and the length actually written out after return


BOOL WINAPI CH375SetExclusive( // Set exclusive use of the current CH375 device
	ULONG	iIndex,            // Specify the serial number of the CH375 device
	ULONG	iExclusive );      // If it is 0, the device can be shared, and if it is not 0, it can be used exclusively


ULONG WINAPI CH375GetUsbID( // Get the USB device ID, in the returned data, the low 16 bits are the manufacturer ID, the high 16 bits are the product ID, and all 0s (invalid ID) are returned in case of error.
	ULONG	iIndex );       // Specify the serial number of the CH375 device


PVOID WINAPI CH375GetDeviceName( // Returns the buffer pointing to the CH375 device name, or returns NULL if there is an error
	ULONG	iIndex );            // Specify the serial number of the CH375 device, 0 corresponds to the first device


BOOL WINAPI CH375SetBufUpload( // Set internal buffer upload mode
	ULONG	iIndex,            // Specify the serial number of the CH375 device, 0 corresponds to the first device
	ULONG	iEnableOrClear );  // If it is 0, the internal buffer upload mode is disabled, and direct upload is used. If it is not 0, the internal buffer upload mode is enabled and the existing data in the buffer is cleared.
                               // If the internal buffer upload mode is enabled, the CH375 driver creates a thread to automatically receive the USB upload data to the internal buffer, and at the same time clears the existing data in the buffer. When the application calls CH375ReadData, it will return the existing data in the buffer immediately data


LONG WINAPI CH375QueryBufUpload( // Query the number of existing data packets in the internal upload buffer, return the number of data packets successfully, return -1 if error occurs
	ULONG	iIndex );            // Specify the serial number of the CH375 device, 0 corresponds to the first device


BOOL WINAPI CH375SetBufDownload( // Set the internal buffer download mode
	ULONG	iIndex,              // Specify the serial number of the CH375 device, 0 corresponds to the first device
	ULONG	iEnableOrClear );    // If it is 0, the internal buffer download mode is disabled, and direct download is used. If it is not 0, the internal buffer download mode is enabled and the existing data in the buffer is cleared.
                                 // If the internal buffer download mode is enabled, then when the application calls CH375WriteData, it will just put the USB download data into the internal buffer and return immediately, and the thread created by the CH375 driver will automatically send it until the end


LONG WINAPI CH375QueryBufDownload( // Query the number of remaining data packets in the internal download buffer (not yet sent), return the number of data packets successfully, return -1 in error
	ULONG	 iIndex );             // Specify the serial number of the CH375 device, 0 corresponds to the first device


BOOL WINAPI CH375ResetInter( // Reset interrupt data read operation
	ULONG	iIndex );        // Specify the serial number of the CH375 device


BOOL WINAPI CH375ResetAux( // reset auxiliary data write operation
	ULONG	iIndex );      // Specify the serial number of the CH375 device


BOOL WINAPI CH375ResetRead( // Reset data block read operation
	ULONG	iIndex );       // Specify the serial number of the CH375 device


BOOL WINAPI CH375ResetWrite( // Reset data block write operation
	ULONG	iIndex );        // Specify the serial number of the CH375 device


typedef VOID ( CALLBACK * mPCH375_NOTIFY_ROUTINE ) ( // device event notification callback routine
	ULONG	iEventStatus );                          // device event and current status (defined in the next line): 0=device unplug event, 3=device plug event

#define CH375_DEVICE_ARRIVAL 3     // Device insertion event, already inserted
#define CH375_DEVICE_REMOVE_PEND 1 // The device will be unplugged
#define CH375_DEVICE_REMOVE 0      // The device is pulled out, it has been pulled out


BOOL WINAPI CH375SetDeviceNotify(           // Set device event notification program
	ULONG	iIndex,                         // Specify the serial number of the CH375 device, 0 corresponds to the first device
	PCHAR	iDeviceID,                      // optional parameter, pointing to a string, specifying the ID of the monitored device, the string is terminated with \0
	mPCH375_NOTIFY_ROUTINE iNotifyRoutine); // Specify the device event callback program, if it is NULL, the event notification will be canceled, otherwise the program will be called when an event is detected


BOOL WINAPI CH375SetTimeoutEx( // Set the timeout for reading and writing USB data
	ULONG	iIndex,            // Specify the serial number of the CH375 device
	ULONG	iWriteTimeout,     // Specify the timeout time for USB to write out data blocks, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
	ULONG	iReadTimeout,      // Specify the timeout time for USB read data block, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
	ULONG	iAuxTimeout,       // Specify the timeout time for USB-assisted download data, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)
	ULONG	iInterTimeout);    // Specify the timeout time for USB interrupt upload data, in milliseconds mS, 0xFFFFFFFF specifies no timeout (default value)

BOOL WINAPI CH375ReadEndP( // read data block
	ULONG	iIndex,        // Specify the serial number of the CH375 device
	ULONG	iPipeNum,      // Endpoint number, valid values are 1 to 8.
	PVOID	oBuffer,       // Point to a buffer large enough to hold the read data
	PULONG	ioLength);     // Point to the length unit, the length to be read when input, and the actual read length after return

BOOL WINAPI CH375WriteEndP( // write out data block
	ULONG	iIndex,         // Specify the serial number of the CH375 device
	ULONG	iPipeNum,       // Endpoint number, valid values are 1 to 8.
	PVOID	iBuffer,        // Point to a buffer where the data to be written is placed
	PULONG	ioLength);      // Point to the length unit, the length to be written out when input, and the length actually written out after return

BOOL WINAPI CH375AbortEndPRead( // Abort data block write operation
	ULONG	iIndex,             // Specify the serial number of the CH375 device
	ULONG	iPipeNum);          // endpoint number, valid values are 1 to 8

BOOL WINAPI CH375AbortEndPWrite( // Abort data block write operation
	ULONG	iIndex,              // Specify the serial number of the CH375 device
	ULONG	iPipeNum);           // endpoint number, valid values are 1 to 8
                                 // Use CH375ReadEndP to read the data buffered in the driver.
                                 // If using CH375SetBufUpload to enable buffer upload, the driver buffers data for endpoint 2;
                                 // If using CH375SetBufUploadEx to enable buffer upload, the driver will buffer data for the endpoint iPipeNum.
                                 // CH375SetBufUploadEx is used for USB3.0 transmission.

BOOL WINAPI CH375SetBufUploadEx( // Set internal buffer upload mode
	ULONG	iIndex,              // Specify the serial number of the CH375 device, 0 corresponds to the first device
	ULONG	iEnableOrClear,      // If it is 0, the internal buffer upload mode is disabled, and direct upload is used. If it is not 0, the internal buffer upload mode is enabled and the existing data in the buffer is cleared.
	ULONG	iPipeNum,            // endpoint number, valid values are 1 to 8
	ULONG	BufSize);            // Buffer size per packet, maximum 4MB

BOOL WINAPI CH375QueryBufUploadEx( // Query the number of existing data packets and the total number of bytes in the internal upload buffer, return TRUE if successful, FALSE if failed
	ULONG	iIndex,                // Specify the serial number of the CH375 device, 0 corresponds to the first device
	ULONG	iPipeNum,              // endpoint number, valid values are 1 to 8
	PULONG	oPacketNum,            // returns the number of existing packets in the internal buffer
	PULONG	oTotalLen);            // Returns the total bytes of existing packets in the internal buffer

BOOL WINAPI CH375ClearBufUpload(   // Clear the internal buffer, no need to suspend the internal buffer upload mode
	ULONG	iIndex,                // Specify the serial number of the CH375 device, 0 corresponds to the first device
	ULONG	iPipeNum);             // endpoint number, valid values are 1 to 8


#ifdef __cplusplus
}
#endif

#endif // _CH375_DLL_H