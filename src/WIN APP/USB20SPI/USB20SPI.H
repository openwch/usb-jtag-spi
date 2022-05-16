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
#define CMDPKT_DATA_MAX_BYTES_USBFS 59     //USB全速时每个命令包内包含数据长度
#define CMDPKT_DATA_MAX_BYTES_USBHS 507    //USB高速时每个命令包内包含数据长度

#define USB20_CMD_HEADER                mOFFSET( USB20SPI_CMD, mBuffer )	// WIN32命令接口的头长度

#define USB20_CMD_SPI_INIT              0xC0   //用于初始化SPI接口，设置SPI接口的数据位、时钟分频、高低字节顺序等等参数。
#define USB20_CMD_SPI_CONTROL           0xC1	//SPI接口控制命令,用于控制SPI接口片选引脚输出高低电平以及电平延时时间。
#define USB20_CMD_SPI_RD_WR             0xC2	//SPI接口常规读取写入数据命令,用于SPI接口通用读取写入操作，一般用于简短常规命令操作。该命令写N个字节数据的同时会回读N个字节数据。
#define USB20_CMD_SPI_BLCK_RD           0xC3	//SPI接口批量读取数据命令,用于SPI接口批量读取数据，一般用于批量数据的读取操作。启用该命令读取数据后，读取的数据会按最大包大小进行打包上传，直到所有数据读取返回完毕。
#define USB20_CMD_SPI_BLCK_WR           0xC4	//SPI接口批量写入数据命令,用于SPI接口批量写入数据，一般用于批量数据的写入操作。
#define USB20_CMD_INFO_RD               0xCA	//参数获取,用于获取SPI接口相关参数等

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
	USHORT          SpiWriteReadInterval; //SPI接口常规读取写入数据命令(DEF_CMD_SPI_RD_WR))，单位为uS
	UCHAR           SpiOutDefaultData;    //SPI读数据时默认输出数据
	UCHAR           OtherCfg;             //1个字节杂项控制；
                                          //位7：片选CS1极性控制：0：低电平有效；1：有电平有效；
                                          //位6：片选CS2极性控制：0：低电平有效；1：有电平有效；
                                          //位5：IIC时钟延展功能控制：0：禁止；1：使能；
                                          //位4：IIC读取最后1个字节完成时生成或不生成NACK；
                                          //位3-0：保留；
	UCHAR           Reserved[4];          //保留
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


//打开USB设备
BOOL USB20SPI_OpenDevice(ULONG DevI);

//关闭USB设备
BOOL USB20SPI_CloseDevice(ULONG DevI);

// SPI控制器初始化
BOOL	WINAPI	USB20SPI_Init(ULONG iIndex,
							  UCHAR iMode,                 // 0-3:SPI Mode0/1/2/3
							  UCHAR iClock,                // 1=36MHz, 2=18MHz, 3=9MHz, 4=4.5MHz, 5=2.25MHz, 6=1.125MHz, 7=562.5KHz)
							  UCHAR iCsIndex);

//设置片选状态,使用前需先调用USB20SPI_Init对CS进行设置
BOOL	WINAPI	USB20SPI_ChangeCS(ULONG			iIndex,         // Specify device number	
								  UCHAR         iStatus);        // 0=撤消片选,1=设置片选

//SPI4写数据
BOOL	WINAPI	USB20SPI_Write(ULONG			iIndex,          // Specify device number	
							   ULONG			iChipSelect,     // 片选控制, 位7为0则忽略片选控制, 位7为1进行片选操作
							   ULONG			iLength,         // 准备传输的数据字节数	
							   ULONG			iWriteStep,      // 准备读取的单个块的长度
							   PVOID			ioBuffer);        // 指向一个缓冲区,放置准备从MOSI写出的数据

//SPI4读数据.无需先写数据，效率较USB20SPI_WriteRead高很多
BOOL	WINAPI	USB20SPI_Read(ULONG			iIndex,           // Specify device number	
							  ULONG			iChipSelect,      // 片选控制, 位7为0则忽略片选控制, 位7为1进行片选操作
							  ULONG           oLength,          // 准备发出的字节数
							  PULONG			iLength,          // 准备读入的数据字节数	
							  PVOID			ioBuffer);         // 指向一个缓冲区,放置准备从MOSI写出的数据,返回后是从MISO读入的数据

// 处理SPI数据流,4线接口
BOOL	WINAPI	USB20SPI_WriteRead(ULONG			iIndex,       // Specify device number
								   ULONG			iChipSelect,  // 片选控制, 位7为0则忽略片选控制, 位7为1则操作片选
								   ULONG			iLength,      // 准备传输的数据字节数
								   PVOID			ioBuffer );    // 指向一个缓冲区,放置准备从MOSI写出的数据,返回后是从MISO读入的数据
