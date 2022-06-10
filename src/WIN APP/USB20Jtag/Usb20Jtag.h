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
#define CMDPKT_DATA_MAX_BYTES_USBFS 59     //USBȫ��ʱÿ��������ڰ������ݳ���
#define CMDPKT_DATA_MAX_BYTES_USBHS 507    //USB����ʱÿ��������ڰ������ݳ���

#define USB20_CMD_HEADER 3

#define USB20_CMD_INFO_RD               0xCA	//������ȡ,���ڻ�ȡ�̼��汾��SPI��IIC��JTAG�ӿ���ز�����
#define USB20_CMD_JTAG_INIT             0xD0	//JTAG�ӿڳ�ʼ������
#define USB20_CMD_JTAG_BIT_OP           0xD1	//JTAG�ӿ�����λ��������
#define USB20_CMD_JTAG_BIT_OP_RD        0xD2	//JTAG�ӿ�����λ���Ʋ���ȡ����
#define USB20_CMD_JTAG_DATA_SHIFT       0xD3	//JTAG�ӿ�������λ����
#define USB20_CMD_JTAG_DATA_SHIFT_RD    0xD4	//JTAG�ӿ�������λ����ȡ����


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

typedef	struct	_USBCMD_PKT{				// ������Ӳ��ͨѶ������ӿڽṹ	
	UCHAR 	mFunction;					    // ����ʱָ�����ܴ���
	USHORT  mLength;					    // �������ݳ���
	union	{
		UCHAR           mStatus;					        // ���ʱ���ز���״̬
		UCHAR			mBuffer[ 512 ];	// ���ݻ�����,Ĭ�ϳ���Ϊ0��512B
	};
} mUSB_HW_COMMAND, *mPUSB_HW_COMMAND;
#pragma pack() 


//����״̬���л�λ������:Test-Idle��Shift-DR
ULONG BuildPkt_EnterShiftDR(PUCHAR BitBangPkt);

//����״̬���л�λ������:Test-Idle��Shift-IR
ULONG BuildPkt_EnterShiftIR(PUCHAR BitBangPkt);

//����״̬���л���λ�����������:Shift-IR/DR��Test-Idle
ULONG BuildPkt_ExitShiftToRunIdle(PUCHAR BitBangPkt);

//��USB�豸
BOOL USB20Jtag_OpenDevice(ULONG DevI);

//�ر�USB�豸
BOOL USB20Jtag_CloseDevice(ULONG DevI);

//�������USB�ϴ��˵㻺��������.USB�����˵����ʹ�û����ϴ�ģʽ��
BOOL USB20Jtag_ClearUpBuf(ULONG iIndex);

//JTAG USB���ݶ������������ϴ�ģʽ
UCHAR USB20Jtag_ReadData(ULONG iIndex,PULONG iLength,PUCHAR DataBuf,ULONG ReadTimeout);

//USB�´��˵�д��ֱ�Ӳ���ģʽ
UCHAR USB20Jtag_WriteData(ULONG iIndex,PULONG oLength,PUCHAR DataBuf);

//�л�JTAG״̬��
BOOL USB20Jtag_SwitchTapState(UCHAR TapState);

//λ����ʽJTAG IR/DR���ݶ�д.�������������ݵĶ�д����ָ�������״̬���л��ȿ����ഫ�䡣���������ݴ��䣬����ʹ��USB20Jtag_WriteRead_Fast
//�������4096�ֽ�Ϊ��λ������д
//״̬��:Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	USB20Jtag_WriteRead(ULONG			iIndex,           // ָ��CH341�豸���
									BOOL            IsDR,             // =TRUE: DR data read/write,=FALSE:IR data read/write
									ULONG			iWriteBitLength,  // д����,׼��д���ĳ���
									PVOID			iWriteBitBuffer,  // Points to a buffer to place data ready to be written out
									PULONG			oReadBitLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ�ĳ���
									PVOID			oReadBitBuffer ); // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//JTAG IR/DR����������д,���ڶ��ֽ�������д����JTAG�̼����ز�������Ӳ����4K������������д��������Ȳ�����4096�ֽڡ���������С�����е���
//״̬��:Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	USB20Jtag_WriteRead_Fast(ULONG		iIndex,            // ָ��CH341�豸���
										 BOOL       IsDR,              // =TRUE: DR data read/write,=FALSE:IR data read/write
										 ULONG		iWriteBitLength,   // д����,׼��д���ĳ���
										 PVOID		iWriteBitBuffer,   // Points to a buffer to place data ready to be written out
										 PULONG		oReadBitLength,    // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ�ĳ���
										 PVOID		oReadBitBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//JTAG�ӿڳ�ʼ��������ģʽ���ٶ�
BOOL	WINAPI	USB20Jtag_INIT(ULONG iIndex,
							   UCHAR iClockRate);   //ͨ���ٶȣ���ЧֵΪ0-4��ֵԽ��ͨ���ٶ�Խ��

//JTAG DRд,���ֽ�Ϊ��λ,���ڶ��ֽ�������д����JTAG�̼����ز�����
//״̬��:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteWriteDR(ULONG			iIndex,        // ָ��CH375�豸���									
									  ULONG			iWriteLength,  // Write length, the length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out

//JTAG DR��,���ֽ�Ϊ��λ,���ֽ���������
//״̬��:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteReadDR(ULONG			iIndex,        // ָ���豸���									
									 PULONG			oReadLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									 PVOID			oReadBuffer ); // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//JTAG IRд,���ֽ�Ϊ��λ,���ֽ�����д��
//״̬��:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteWriteIR(ULONG			iIndex,        // ָ��CH375�豸���									
									  ULONG			iWriteLength,  // Write length, the length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out									

//JTAG IR��,���ֽ�Ϊ��λ,���ֽ�������д��
//״̬��:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_ByteReadIR(ULONG			iIndex,        // ָ���豸���									
									 PULONG			oReadLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									 PVOID			oReadBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//λ����ʽJTAG DR����д.�������������ݵĶ�д����ָ�������״̬���л��ȿ����ഫ�䡣���������ݴ��䣬����ʹ��USB20Jtag_ByeWriteDR
//״̬��:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteDR(ULONG			iIndex,           // ָ���豸���									
									 ULONG    	    iWriteBitLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									 PVOID			iWriteBitBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//λ����ʽJTAG DR����д.�������������ݵĶ�д����ָ�������״̬���л��ȿ����ഫ�䡣���������ݴ��䣬����ʹ��USB20Jtag_ByteWriteIR
//״̬��:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteIR(ULONG			iIndex,           // ָ���豸���									
									 ULONG    	    iWriteBitLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									 PVOID			iWriteBitBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//λ����ʽJTAG IR���ݶ�.�������������ݵĶ�д����ָ�������״̬���л��ȡ����������ݴ��䣬����ʹ��USB20Jtag_ByteReadIR
//״̬��:Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitReadIR(ULONG			iIndex,           // ָ���豸���									
									PULONG    	oReadBitLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									PVOID			oReadBitBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

//λ����ʽJTAG DR���ݶ�.�������������ݵĶ�д���������͸������ݴ��䣬����ʹ��USB20Jtag_ByteReadDR
//״̬��:Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitReadDR(ULONG			iIndex,           // ָ���豸���									
									PULONG    	oReadBitLength,   // ָ�򳤶ȵ�Ԫ,���غ�Ϊʵ�ʶ�ȡ���ֽڳ���
									PVOID			oReadBitBuffer );  // ָ��һ���㹻��Ļ�����,���ڱ����ȡ������

