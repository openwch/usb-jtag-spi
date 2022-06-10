/***************************************************************************                                                                     *
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
#define SF_PACKET_BUF_SIZE 						  51200			//命令包长度
#define CMDPKT_DATA_MAX_BYTES_USBFS 			  59     		//USB全速时每个命令包内包含数据长度
#define USB_PACKET_USBFS						  64			//USB全速时单包最大数据长度
#define CMDPKT_DATA_MAX_BYTES_USBHS 			  507    		//USB高速时每个命令包内包含数据长度
#define USB_PACKET_USBHS						  512			//USB高速时单包最大数据长度

#define USB20Jtag_CMD_HEADER                	  3				//协议包头长度

#define USB20Jtag_CMD_INFO_RD               	  0xCA			//参数获取,用于获取固件版本、JTAG接口相关参数等
#define USB20Jtag_CMD_JTAG_INIT             	  0xD0			//JTAG接口初始化命令
#define USB20Jtag_CMD_JTAG_BIT_OP           	  0xD1			//JTAG接口引脚位控制命令
#define USB20Jtag_CMD_JTAG_BIT_OP_RD        	  0xD2			//JTAG接口引脚位控制并读取命令
#define USB20Jtag_CMD_JTAG_DATA_SHIFT       	  0xD3			//JTAG接口数据移位命令
#define USB20Jtag_CMD_JTAG_DATA_SHIFT_RD    	  0xD4			//JTAG接口数据移位并读取命令

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

typedef	struct	_USBCMD_PKT{				// 定义与硬件通讯的命令接口结构	
	unsigned char 	mFunction;				// 输入时指定功能代码
	unsigned short  mLength;				// 后续数据长度
	union	{
		 unsigned char mStatus;				// 输出时返回操作状态
		 unsigned char mBuffer[ 512 ];	    // 数据缓冲区,默认长度为0至512B
	};
} mUSB_HW_COMMAND, *mPUSB_HW_COMMAND;

typedef struct _PIN_STATUS{					// 记录USB20JTAG引脚状态
	int TMS;
	int TDI;
	int TCK;
}JtagPinStatus;
#pragma pack() 

#ifdef _WIN32
#include <windows.h>
typedef int (__stdcall *pCH375OpenDevice)(	unsigned long iIndex);
typedef void (__stdcall *pCH375CloseDevice)(	unsigned long iIndex);          
typedef unsigned long (__stdcall *pCH375SetTimeoutEx)(	unsigned long iIndex,               // 指定设备序号
                                                        unsigned long iWriteTimeout,        // 指定USB写出数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
														unsigned long iReadTimeout,         // 指定USB读取数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
                                                        unsigned long iAuxTimeout,          // 指定USB辅助下传数据的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
                                                        unsigned long iInterTimeout);       // 指定USB中断上传数据的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
typedef unsigned long (__stdcall *pCH375SetBufUploadEx)(  									// 设定内部缓冲上传模式
														unsigned long iIndex,  				// 指定设备序号,0对应第一个设备
														unsigned long iEnableOrClear,		// 为0则禁止内部缓冲上传模式,使用直接上传,非0则启用内部缓冲上传模式并清除缓冲区中的已有数据
														unsigned long iPipeNum,				// 端点号，有效值为1到8
														unsigned long BufSize );   			// 每包缓冲区大小，最大4MB
typedef unsigned long (__stdcall *pCH375ClearBufUpload)(  									// 清空内部缓冲区，无需暂停内部缓冲上传模式
														unsigned long iIndex,  				// 指定设备序号,0对应第一个设备
														unsigned long iPipeNum );  			// 端点号，有效值为1到8
typedef unsigned long (__stdcall *pCH375QueryBufUploadEx)(  								// 查询内部上传缓冲区中的已有数据包个数和总字节数,成功返回TRUE,失败FALSE
														unsigned long iIndex,				// 指定设备序号,0对应第一个设备	
														unsigned long iPipeNum,				// 端点号，有效值为1到8
														unsigned long* oPacketNum,			// 返回内部缓冲中的已有数据包个数	
														unsigned long* oTotalLen );  		// 返回内部缓冲中的已有数据包总字节数	
typedef unsigned long (__stdcall *pCH375GetConfigDescr)(unsigned long iIndex,   			// 指定CH375设备序号
	                                                    void* oBuffer,          			// 指向一个足够大的缓冲区,用于保存描述符
	                                                    unsigned long* ioLength );  		// 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
typedef unsigned long (__stdcall *pCH375WriteData)(		unsigned long iIndex,   			// 指定设备序号
	                                                    void* oBuffer,         				// 指向一个足够大的缓冲区,用于保存描述符
	                                                    unsigned long* ioLength );  		// 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
typedef unsigned long (__stdcall *pCH375ReadData)(		unsigned long iIndex,   			// 指定设备序号
	                                                    void* oBuffer,          			// 指向一个足够大的缓冲区,用于保存描述符
	                                                    unsigned long* ioLength );  		// 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
typedef unsigned long (__stdcall *pCH375ReadEndP)(		unsigned long iIndex,       		// 指定设备序号
                                                    	unsigned long iPipeNum,     		// 端点号，有效值为1到8。
														void* iBuffer,              		// 指向一个足够大的缓冲区,用于保存读取的数据
														unsigned long*	ioLength);  		// 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
typedef unsigned long (__stdcall *pCH375WriteEndP)(		unsigned long iIndex,       		// 指定设备序号
														unsigned long iPipeNum,     		// 端点号，有效值为1到8。
														void* iBuffer,              		// 指向一个缓冲区,放置准备写出的数据
	 													unsigned long*	ioLength);  		// 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度
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

bool AfxDevIsOpened;  													  		// 设备是否打开
bool AfxUsbHighDev = true;
unsigned long   USB_PACKET;
unsigned long   MaxBitsPerBulk = 0;
unsigned long   MaxBytesPerBulk = 0;
unsigned long 	CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; 	  		// USB PKT由命令码(1B)+长度(2B)+数据
unsigned long 	CMDPKT_DATA_MAX_BITS   =  (CMDPKT_DATA_MAX_BYTES_USBHS/2);      // USB PKT每包最大数据位数，每个数据位需两个TCK由低到高的位带组成。
unsigned char 	DataUpEndp = 0, DataDnEndp = 0;
unsigned short 	BulkInEndpMaxSize = 512, BulkOutEndpMaxSize = 512;

JtagPinStatus usb20Jtag = {0, 0, 0};									  		// 初始化设备引脚状态

static char *HexToString(uint8_t *buf, unsigned int size)
{
	unsigned int i;
	char *str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2*i, "%02x ", buf[i]);
	return str;
}

/**
 *  USB20Jtag_Write - USB20Jtag 写方法
 *  @param oBuffer    指向一个缓冲区,放置准备写出的数据
 *  @param ioLength   指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度
 * 
 *  @return 		  写成功返回1，失败返回0
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
 * USB20Jtag_Read - USB20Jtag 读方法
 * @param oBuffer  	指向一个足够大的缓冲区,用于保存读取的数据
 * @param ioLength 	指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
 * 
 * @return 			读成功返回1，失败返回0
 */
static int USB20Jtag_Read(void* oBuffer,unsigned long* ioLength)					   
{
    unsigned long rlength = *ioLength, packetNum, bufferNum, RI, RLen, WaitT = 0, timeout = 20;
	int ret = false;

	// 单次读取最大允许读取4096B数据，超过则按4096B进行计算
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
 * USB20Jtag_ClockTms - 功能函数，用于在TCK的上升沿改变TMS值，使其Tap状态切换
 * @param BitBangPkt 	协议包
 * @param tms 		 	需要改变的TMS值
 * @param BI		 	协议包长度
 * 	
 * @return 			 	返回协议包长度
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
 * USB20Jtag_IdleClock - 功能函数，确保时钟处于拉低状态
 * @param BitBangPkt 	 协议包
 * @param BI  		 	 协议包长度
 * 
 * @return 			 	 返回协议包长度
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
 * USB20Jtag_TmsChange - 功能函数，通过改变TMS的值来进行状态切换
 * @param tmsValue 		 需要进行切换的TMS值按切换顺序组成一字节数据
 * @param step 	   		 需要读取tmsValue值的位值数
 * @param skip 	   		 从tmsValue的skip位处开始计数到step
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
 * USB20Jtag_TMS - 由usb20jtag_execute_queue调用
 * @param cmd 	   上层传递命令参数
 * 
 */
static void USB20Jtag_TMS(struct tms_command *cmd)
{
	LOG_DEBUG_IO("(step: %d)", cmd->num_bits);
	USB20Jtag_TmsChange(cmd->bits, cmd->num_bits, 0);
}

/**
 * USB20Jtag_Reset - USB20Jtag 复位Tap状态函数
 * @brief 连续六个以上TCK且TMS为高将可将状态机置为Test-Logic Reset状态
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
 * USB20Jtag_MovePath - 获取当前Tap状态并切换至cmd传递下去的状态TMS值
 * @param cmd 上层传递命令参数
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
 * USB20Jtag_MoveState - 切换Tap状态至目标状态stat
 * @param stat 预切换目标路径
 * @param skip 需跳过的位数
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
 * USB20Jtag_WriteRead - USB20Jtag 批量读写函数
 * @param bits 
 * @param nb_bits 		 传入数据长度
 * @param scan			 传入数据的传输方式
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

	// 最后一个TDI位将会按照位带模式输出，其nb1确保不为0，使其能在TMS变化时输出最后1bit数据
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
		// 构建数据包
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

		// 若需回读数据则判断当前BI值进行命令下发
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
		
		// 在传输过程中，若不回读则根据命令包长度将要达到饱和时将命令下发
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

	// 构建输出最后1位TDI数据的命令包
	if (bits)
	{
		BitBangPkt[BI++] = IsRead?USB20Jtag_CMD_JTAG_BIT_OP_RD:USB20Jtag_CMD_JTAG_BIT_OP;	
		DLen = nb1*2;
		BitBangPkt[BI++] = (uint8_t)(DLen>>0)&0xFF;
		BitBangPkt[BI++] = (uint8_t)(DLen>>8)&0xFF; 		//长度高8位
		TMS_Bit = TMS_L;
		for(i = 0; i < nb1; i++)
		{
			if((bits[nb8] >> i) & 0x01)
				TDI_Bit = TDI_H;
			else
				TDI_Bit = TDI_L;	
			if((i + 1) == nb1)									//最后一位在Exit1-DR状态输出
				TMS_Bit = TMS_H;
			BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_L;
			BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_H;
		}
		BitBangPkt[BI++] = TMS_Bit | TDI_Bit | TCK_L;
	}

	// 读取Bit-Bang模式下的最后一字节数据
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

	// 清空此次批量读写函数中未处理命令
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

	// 将TCK、TDI拉低为低电平，应TDI采样在TCK上升沿，若状态未改变，则TDI采样将可能发生在TCK下降沿
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
 * USB20JTAG_Scan - 切换至SHIFT-DR或者SHIFT-IR状态
 * @param cmd 	    上层传递命令参数
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
 * USB20Jtag_ClockRateInit - 初始化USB20Jtag时钟速率
 * @param Index 			 USB20Jtag 设备操作句柄
 * @param iClockRate		 设置的USB20Jtag时钟参数(0-5)
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

	// 获取的USB速度,默认为480MHz USB2.0高速，如果连接至全速HUB下则为12MHz USB全速。
	DescBufSize = sizeof(DescBuf);
	if( !pGetConfigDescr(gIndex,DescBuf,&DescBufSize))
		goto Exit;

	// 根据USB BULK端点大小来判断。如端点大小为512B，则为480MHz USB2.0高速
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
					BulkInEndpMaxSize = EndpDesc->wMaxPacketSize;			// 端点大小
					AfxUsbHighDev = (EndpDesc->wMaxPacketSize == 512);		// USB速度类型
					
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
	// 根据USB速度,设置每个命令包最大数据长度		
	if(AfxUsbHighDev)
	{
		USB_PACKET 			   =  USB_PACKET_USBHS;
		CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; 				//507B
	}
	else				
	{
		USB_PACKET			   =  USB_PACKET_USBFS;
		CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBFS; 				//59B
	}
	CMDPKT_DATA_MAX_BITS = CMDPKT_DATA_MAX_BYTES/16*16/2;  					//每个命令所传输的最大位数，每位需由两个字节表示，取2字节的整数倍

	// 根据硬件缓冲区大小计算每批量传输传输的位数,多命令拼包
	MaxBitsPerBulk = HW_TDO_BUF_SIZE/CMDPKT_DATA_MAX_BYTES*CMDPKT_DATA_MAX_BITS;  
	// 根据硬件缓冲区大小计算每批量传输传输的字数,多命令拼包
	MaxBytesPerBulk = HW_TDO_BUF_SIZE - (HW_TDO_BUF_SIZE+CMDPKT_DATA_MAX_BYTES-1)/CMDPKT_DATA_MAX_BYTES*3;;

	// USB BULKIN上传数据采用驱动缓冲上传方式,较直接上传效率更高
	pSetBufUpload(gIndex, true, DataUpEndp, 4096);							//指上传端点为缓冲上传模式，缓冲区大小为8192
	pClearBufUpload(gIndex, DataUpEndp);									//清空驱动内缓冲区数据

	if (!USB20Jtag_Read(clearBuffer, &TxLen))								//读取硬件缓冲区数据
	{	
		LOG_ERROR("USB20Jtag_WriteRead read usb data failure.");
		return 0;
	}

	//构建USB JTAG初始化命令包，并执行
	i = 0;
	mBuffer[i++] = USB20Jtag_CMD_JTAG_INIT;
	mBuffer[i++] = 6;
	mBuffer[i++] = 0;
	mBuffer[i++] = 0;														//保留字节
	mBuffer[i++] = iClockRate;												//JTAG时钟速度
	i += 4; 																//保留字节
	mLength = i;
	if( !USB20Jtag_Write(mBuffer,&mLength) || (mLength!=i) )
		goto Exit;

	// 读取返回值并判断初始化是否成功
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

// 退出函数
static int usb20jtag_quit(void)
{ 
	// 退出前将信号线全部设置为低电平
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
