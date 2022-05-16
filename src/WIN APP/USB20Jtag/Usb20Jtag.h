/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    USB TO JTAG Protocol Funciton
    The USB2.0 to JTAG scheme based on CH32V305 MCU can be used to build 
	customized USB high-speed JTAG debugger and other products.  
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) 
	device universal driver (CH372DRV) and upper computer routines.  
    The current routine is a custom communication protocol,TDI transmission speed up to 4M bytes 
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (C) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

#include "Main.H"

//bit-bang out
//#define JTAGIO_STA_OUT_RESET     (0x20)
#define JTAGIO_STA_OUT_TDI         (0x10)
//#define JTAGIO_STA_OUT_NCS       (0x08)
//#define JTAGIO_STA_OUT_NCE       (0x04)
#define JTAGIO_STA_OUT_TMS         (0x02)
#define JTAGIO_STA_OUT_TCK         (0x01)

//bit-bang in
//#define JTAGIO_STA_IN_TDO          (0x01)
//#define JTAGIO_STA_IN_DATAOUT      (0x02)
//#define JTAGIO_STA_IN_TDO_BIT      (0)

// bit-bang in
//#define JTAGIO_STA_IN_TDO          (0x01)
//#define JTAGIO_STA_IN_DATAOUT      (0x02)

//#define JTAGIO_STA_IN_TDO_BIT      (0)
//#define JTAGIO_STA_IN_DATAOUT_BIT  (1)

#define TDI_H     JTAGIO_STA_OUT_TDI
#define TDI_L     0
#define TMS_H     JTAGIO_STA_OUT_TMS
#define TMS_L     0 
#define TCK_H     JTAGIO_STA_OUT_TCK
#define TCK_L     0 

#define HW_TDO_BUF_SIZE             4096
#define CMDPKT_DATA_MAX_BYTES_USBFS 59     //USB全速时每个命令包内包含数据长度
#define CMDPKT_DATA_MAX_BYTES_USBHS 507    //USB高速时每个命令包内包含数据长度

#define USB20_CMD_HEADER 3

#define USB20_CMD_INFO_RD               0xCA	//参数获取,用于获取固件版本、SPI、IIC、JTAG接口相关参数等
#define USB20_CMD_JTAG_INIT             0xD0	//JTAG接口初始化命令
#define USB20_CMD_JTAG_BIT_OP           0xD1	//JTAG接口引脚位控制命令
#define USB20_CMD_JTAG_BIT_OP_RD        0xD2	//JTAG接口引脚位控制并读取命令
#define USB20_CMD_JTAG_DATA_SHIFT       0xD3	//JTAG接口数据移位命令
#define USB20_CMD_JTAG_DATA_SHIFT_RD    0xD4	//JTAG接口数据移位并读取命令


#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

// Values for bmAttributes field of an
// endpoint descriptor
#define USB_ENDPOINT_TYPE_MASK                    0x03
#define USB_ENDPOINT_TYPE_CONTROL                 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01
#define USB_ENDPOINT_TYPE_BULK                    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03
#define USB_ENDPOINT_DIRECTION_MASK               0x80

#pragma pack(1) 
typedef struct _USB_COMMON_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;

typedef struct _USB_DEVICE_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT bcdUSB;
	UCHAR bDeviceClass;
	UCHAR bDeviceSubClass;
	UCHAR bDeviceProtocol;
	UCHAR bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR iManufacturer;
	UCHAR iProduct;
	UCHAR iSerialNumber;
	UCHAR bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bEndpointAddress;
	UCHAR bmAttributes;
	USHORT wMaxPacketSize;
	UCHAR bInterval;
} USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;

typedef struct _USB_CONFIGURATION_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT wTotalLength;
	UCHAR bNumInterfaces;
	UCHAR bConfigurationValue;
	UCHAR iConfiguration;
	UCHAR bmAttributes;
	UCHAR MaxPower;
} USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_INTERFACE_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bInterfaceNumber;
	UCHAR bAlternateSetting;
	UCHAR bNumEndpoints;
	UCHAR bInterfaceClass;
	UCHAR bInterfaceSubClass;
	UCHAR bInterfaceProtocol;
	UCHAR iInterface;
} USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

typedef	struct	_USBCMD_PKT{				// 定义与硬件通讯的命令接口结构	
	UCHAR 	mFunction;					    // 输入时指定功能代码
	USHORT  mLength;					    // 后续数据长度
	union	{
		UCHAR           mStatus;					        // 输出时返回操作状态
		UCHAR			mBuffer[ 512 ];	// 数据缓冲区,默认长度为0至512B
	};
} mUSB_HW_COMMAND, *mPUSB_HW_COMMAND;
#pragma pack() 


//构建状态机切换位带数据:Test-Idle至Shift-DR
ULONG BuildPkt_EnterShiftDR(PUCHAR BitBangPkt);

//构建状态机切换位带数据:Test-Idle至Shift-IR
ULONG BuildPkt_EnterShiftIR(PUCHAR BitBangPkt);

//构建状态机切换的位带数据命令包:Shift-IR/DR至Test-Idle
ULONG BuildPkt_ExitShiftToRunIdle(PUCHAR BitBangPkt);

//打开USB设备
BOOL USB20Jtag_OpenDevice(ULONG DevI);

//关闭USB设备
BOOL USB20Jtag_CloseDevice(ULONG DevI);

//清除驱动USB上传端点缓冲区数据.USB批量端点读，使用缓冲上传模式。
BOOL USB20Jtag_ClearUpBuf(ULONG iIndex);

//JTAG USB数据读函数，缓冲上传模式
UCHAR USB20Jtag_ReadData(ULONG iIndex,PULONG iLength,PUCHAR DataBuf,ULONG ReadTimeout);

//USB下传端点写，直接操作模式
UCHAR USB20Jtag_WriteData(ULONG iIndex,PULONG oLength,PUCHAR DataBuf);

//切换JTAG状态机
BOOL USB20Jtag_SwitchTapState(UCHAR TapState);

//位带方式JTAG IR/DR数据读写.适用于少量数据的读写。如指令操作、状态机切换等控制类传输。如批量数据传输，建议使用USB20Jtag_WriteRead_Fast
//命令包以4096字节为单位批量读写
//状态机:Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	USB20Jtag_WriteRead(ULONG			iIndex,           // 指定CH341设备序号
									BOOL            IsDR,             // =TRUE: DR data read/write,=FALSE:IR data read/write
									ULONG			iWriteBitLength,  // 写长度,准备写出的长度
									PVOID			iWriteBitBuffer,  // Points to a buffer to place data ready to be written out
									PULONG			oReadBitLength,   // 指向长度单元,返回后为实际读取的长度
									PVOID			oReadBitBuffer ); // 指向一个足够大的缓冲区,用于保存读取的数据

//JTAG IR/DR数据批量读写,用于多字节连续读写。如JTAG固件下载操作。因硬件有4K缓冲区，如先写后读，长度不超过4096字节。缓冲区大小可自行调整
//状态机:Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	USB20Jtag_WriteRead_Fast(ULONG		iIndex,            // 指定CH341设备序号
										 BOOL       IsDR,              // =TRUE: DR data read/write,=FALSE:IR data read/write
										 ULONG		iWriteBitLength,   // 写长度,准备写出的长度
										 PVOID		iWriteBitBuffer,   // Points to a buffer to place data ready to be written out
										 PULONG		oReadBitLength,    // 指向长度单元,返回后为实际读取的长度
										 PVOID		oReadBitBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

//JTAG接口初始化，设置模式及速度
BOOL	WINAPI	USB20Jtag_INIT(ULONG iIndex,
							   UCHAR iClockRate);   //通信速度；有效值为0-4，值越大通信速度越快

//JTAG DR写,以字节为单位,用于多字节连续读写。如JTAG固件下载操作。
//状态机:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteWriteDR(ULONG			iIndex,        // 指定CH375设备序号									
									  ULONG			iWriteLength,  // Write length, the length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out

//JTAG DR读,以字节为单位,多字节连续读。
//状态机:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteReadDR(ULONG			iIndex,        // 指定设备序号									
									 PULONG			oReadLength,   // 指向长度单元,返回后为实际读取的字节长度
									 PVOID			oReadBuffer ); // 指向一个足够大的缓冲区,用于保存读取的数据

//JTAG IR写,以字节为单位,多字节连续写。
//状态机:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteWriteIR(ULONG			iIndex,        // 指定CH375设备序号									
									  ULONG			iWriteLength,  // Write length, the length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out									

//JTAG IR读,以字节为单位,多字节连续读写。
//状态机:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteReadIR(ULONG			iIndex,        // 指定设备序号									
									 PULONG			oReadLength,   // 指向长度单元,返回后为实际读取的字节长度
									 PVOID			oReadBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

//位带方式JTAG DR数据写.适用于少量数据的读写。如指令操作、状态机切换等控制类传输。如批量数据传输，建议使用USB20Jtag_ByeWriteDR
//状态机:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteDR(ULONG			iIndex,           // 指定设备序号									
									 ULONG    	    iWriteBitLength,   // 指向长度单元,返回后为实际读取的字节长度
									 PVOID			iWriteBitBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

//位带方式JTAG DR数据写.适用于少量数据的读写。如指令操作、状态机切换等控制类传输。如批量数据传输，建议使用USB20Jtag_ByteWriteIR
//状态机:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteIR(ULONG			iIndex,           // 指定设备序号									
									 ULONG    	    iWriteBitLength,   // 指向长度单元,返回后为实际读取的字节长度
									 PVOID			iWriteBitBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

//位带方式JTAG IR数据读.适用于少量数据的读写。如指令操作、状态机切换等。如批量数据传输，建议使用USB20Jtag_ByteReadIR
//状态机:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitReadIR(ULONG			iIndex,           // 指定设备序号									
									PULONG    	oReadBitLength,   // 指向长度单元,返回后为实际读取的字节长度
									PVOID			oReadBitBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

//位带方式JTAG DR数据读.适用于少量数据的读写。如批量和高速数据传输，建议使用USB20Jtag_ByteReadDR
//状态机:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitReadDR(ULONG			iIndex,           // 指定设备序号									
									PULONG    	oReadBitLength,   // 指向长度单元,返回后为实际读取的字节长度
									PVOID			oReadBitBuffer );  // 指向一个足够大的缓冲区,用于保存读取的数据

