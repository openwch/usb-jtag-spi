/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    USB SPI high-speed interface function, which provides commands and functions for USB SPI controller initialization, stream read/write, and batch read/write.
    Usb2.0 (480M high speed) to SPI based on CH32V305 MCU, can be used to construct
    Build custom USB high speed FASH programmer and other products.
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) device universal driver (CH372DRV) and upper computer routines.
    The current routine is a custom communication protocol, and the SPI transmission speed can reach 2MB/S
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
*/

#define uint8_t UCHAR
#define uint16_t USHORT
#define uint32_t ULONG

#define mCH341_PACKET_LENGTH 512
#define CMDPKT_DATA_MAX_BYTES_USBFS 59     //USBȫ��ʱÿ��������ڰ������ݳ���
#define CMDPKT_DATA_MAX_BYTES_USBHS 507    //USB����ʱÿ��������ڰ������ݳ���

#define USB20_CMD_HEADER                mOFFSET( USB20SPI_CMD, mBuffer )	// WIN32����ӿڵ�ͷ����

#define USB20_CMD_SPI_INIT              0xC0   //���ڳ�ʼ��SPI�ӿڣ�����SPI�ӿڵ�����λ��ʱ�ӷ�Ƶ���ߵ��ֽ�˳��ȵȲ�����
#define USB20_CMD_SPI_CONTROL           0xC1	//SPI�ӿڿ�������,���ڿ���SPI�ӿ�Ƭѡ��������ߵ͵�ƽ�Լ���ƽ��ʱʱ�䡣
#define USB20_CMD_SPI_RD_WR             0xC2	//SPI�ӿڳ����ȡд����������,����SPI�ӿ�ͨ�ö�ȡд�������һ�����ڼ�̳������������������дN���ֽ����ݵ�ͬʱ��ض�N���ֽ����ݡ�
#define USB20_CMD_SPI_BLCK_RD           0xC3	//SPI�ӿ�������ȡ��������,����SPI�ӿ�������ȡ���ݣ�һ�������������ݵĶ�ȡ���������ø������ȡ���ݺ󣬶�ȡ�����ݻᰴ������С���д���ϴ���ֱ���������ݶ�ȡ������ϡ�
#define USB20_CMD_SPI_BLCK_WR           0xC4	//SPI�ӿ�����д����������,����SPI�ӿ�����д�����ݣ�һ�������������ݵ�д�������
#define USB20_CMD_INFO_RD               0xCA	//������ȡ,���ڻ�ȡSPI�ӿ���ز�����

#define SPI_CS_ACTIVE    0x00
#define SPI_CS_DEACTIVE  0x01

/* SPI_data_direction */  
#define SPI_Direction_2Lines_FullDuplex ((uint16_t)0x0000)
#define SPI_Direction_2Lines_RxOnly     ((uint16_t)0x0400)
#define SPI_Direction_1Line_Rx          ((uint16_t)0x8000)
#define SPI_Direction_1Line_Tx          ((uint16_t)0xC000)

/* SPI_mode */
#define SPI_Mode_Master                 ((uint16_t)0x0104)
#define SPI_Mode_Slave                  ((uint16_t)0x0000)

/* SPI_data_size */
#define SPI_DataSize_16b                ((uint16_t)0x0800)
#define SPI_DataSize_8b                 ((uint16_t)0x0000)

/* SPI_Clock_Polarity */
#define SPI_CPOL_Low                    ((uint16_t)0x0000)
#define SPI_CPOL_High                   ((uint16_t)0x0002)

/* SPI_Clock_Phase */
#define SPI_CPHA_1Edge                  ((uint16_t)0x0000)
#define SPI_CPHA_2Edge                  ((uint16_t)0x0001)

/* SPI_Slave_Select_management */
#define SPI_NSS_Soft                    ((uint16_t)0x0200)
#define SPI_NSS_Hard                    ((uint16_t)0x0000)

/* SPI_BaudRate_Prescaler */
#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000)
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008)
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010)
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018)
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020)
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028)
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030)
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038)

/* SPI_MSB_LSB_transmission */
#define SPI_FirstBit_MSB                ((uint16_t)0x0000)
#define SPI_FirstBit_LSB                ((uint16_t)0x0080)

/* SPI Init structure definition */
typedef struct _SPI_InitTypeDef
{
  uint16_t SPI_Direction;           /* Specifies the SPI unidirectional or bidirectional data mode.
                                       This parameter can be a value of @ref SPI_data_direction */

  uint16_t SPI_Mode;                /* Specifies the SPI operating mode.
                                       This parameter can be a value of @ref SPI_mode */

  uint16_t SPI_DataSize;            /* Specifies the SPI data size.
                                       This parameter can be a value of @ref SPI_data_size */

  uint16_t SPI_CPOL;                /* Specifies the serial clock steady state.
                                       This parameter can be a value of @ref SPI_Clock_Polarity */

  uint16_t SPI_CPHA;                /* Specifies the clock active edge for the bit capture.
                                       This parameter can be a value of @ref SPI_Clock_Phase */

  uint16_t SPI_NSS;                 /* Specifies whether the NSS signal is managed by
                                       hardware (NSS pin) or by software using the SSI bit.
                                       This parameter can be a value of @ref SPI_Slave_Select_management */
 
  uint16_t SPI_BaudRatePrescaler;   /* Specifies the Baud Rate prescaler value which will be
                                       used to configure the transmit and receive SCK clock.
                                       This parameter can be a value of @ref SPI_BaudRate_Prescaler.
                                       @note The communication clock is derived from the master
                                             clock. The slave clock does not need to be set. */

  uint16_t SPI_FirstBit;            /* Specifies whether data transfers start from MSB or LSB bit.
                                       This parameter can be a value of @ref SPI_MSB_LSB_transmission */

  uint16_t SPI_CRCPolynomial;       /* Specifies the polynomial used for the CRC calculation. */
}SPI_InitTypeDef;


typedef struct _SpiUSBCFG{
	SPI_InitTypeDef SPIInitCfg;
	USHORT          SpiWriteReadInterval; //SPI�ӿڳ����ȡд����������(DEF_CMD_SPI_RD_WR))����λΪuS
	UCHAR           SpiOutDefaultData;    //SPI������ʱĬ���������
	UCHAR           OtherCfg;             //1���ֽ�������ƣ�
                                          //λ7��ƬѡCS1���Կ��ƣ�0���͵�ƽ��Ч��1���е�ƽ��Ч��
                                          //λ6��ƬѡCS2���Կ��ƣ�0���͵�ƽ��Ч��1���е�ƽ��Ч��
                                          //λ5��IICʱ����չ���ܿ��ƣ�0����ֹ��1��ʹ�ܣ�
                                          //λ4��IIC��ȡ���1���ֽ����ʱ���ɻ�����NACK��
                                          //λ3-0��������
	UCHAR           Reserved[4];          //����
}SpiHwCfgS,*PSpiHwCfgS;

typedef struct _CH347_USB_CMD_S{
	UCHAR       mFunction;
	USHORT      mLength;
	UCHAR		mBuffer[512];
}USB20SPI_CMD,*mPUSB20SPI_CMD;

#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

// Values for bmAttributes field of an
// endpoint descriptor
//
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
#pragma pack() 


//��USB�豸
BOOL USB20SPI_OpenDevice(ULONG DevI);

//�ر�USB�豸
BOOL USB20SPI_CloseDevice(ULONG DevI);

// SPI��������ʼ��
BOOL	WINAPI	USB20SPI_Init(ULONG iIndex,
							  UCHAR iMode,                 // 0-3:SPI Mode0/1/2/3
							  UCHAR iClock,                // 1=36MHz, 2=18MHz, 3=9MHz, 4=4.5MHz, 5=2.25MHz, 6=1.125MHz, 7=562.5KHz)
							  UCHAR iCsIndex);

//����Ƭѡ״̬,ʹ��ǰ���ȵ���USB20SPI_Init��CS��������
BOOL	WINAPI	USB20SPI_ChangeCS(ULONG			iIndex,         // Specify device number	
								  UCHAR         iStatus);        // 0=����Ƭѡ,1=����Ƭѡ

//SPI4д����
BOOL	WINAPI	USB20SPI_Write(ULONG			iIndex,          // Specify device number	
							   ULONG			iChipSelect,     // Ƭѡ����, λ7Ϊ0�����Ƭѡ����, λ7Ϊ1����Ƭѡ����
							   ULONG			iLength,         // ׼������������ֽ���	
							   ULONG			iWriteStep,      // ׼����ȡ�ĵ�����ĳ���
							   PVOID			ioBuffer);        // ָ��һ��������,����׼����MOSIд��������

//SPI4������.������д���ݣ�Ч�ʽ�USB20SPI_WriteRead�ߺܶ�
BOOL	WINAPI	USB20SPI_Read(ULONG			iIndex,           // Specify device number	
							  ULONG			iChipSelect,      // Ƭѡ����, λ7Ϊ0�����Ƭѡ����, λ7Ϊ1����Ƭѡ����
							  ULONG           oLength,          // ׼���������ֽ���
							  PULONG			iLength,          // ׼������������ֽ���	
							  PVOID			ioBuffer);         // ָ��һ��������,����׼����MOSIд��������,���غ��Ǵ�MISO���������

// ����SPI������,4�߽ӿ�
BOOL	WINAPI	USB20SPI_WriteRead(ULONG			iIndex,       // Specify device number
								   ULONG			iChipSelect,  // Ƭѡ����, λ7Ϊ0�����Ƭѡ����, λ7Ϊ1�����Ƭѡ
								   ULONG			iLength,      // ׼������������ֽ���
								   PVOID			ioBuffer );    // ָ��һ��������,����׼����MOSIд��������,���غ��Ǵ�MISO���������