// 2003.09.08, 2003.12.28, 2004.10.15, 2004.12.05, 2004.12.10, 2005.01.20, 2005.02.23, 2005.07.15, 2005.08.17,2021.10.12,,2022.03.15,2022.4.20
//*********************************************************************
//** USB总线接口芯片CH375的应用层接口库                              ** 
//** Copyright(C)WCH 2022                                            **
//** http://wch.cn                                                   ** 
//*********************************************************************
//** CH375DLL   V3.1                                                 ** 
//** DLL for USB interface chip CH37x                                **
//** 开发环境：C, VC5.0                                              **
//** 支持芯片: CH372/CH375/CH376/CH378,以及WCH USB外部接口类设备接口 ** 
//** 运行环境：Windows 2000/XP/7/8/8.1/10/11                         **
//*********************************************************************
//

#ifndef		_CH375_DLL_H
#define		_CH375_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#define		mOFFSET( s, m )			( (ULONG) (ULONG_PTR)& ( ( ( s * ) 0 ) -> m ) )	// 定义获取结构成员相对偏移地址的宏

#ifndef		max
#define		max( a, b )				( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )	// 较大值
#endif

#ifndef		min
#define		min( a, b )				( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )	// 较小值
#endif

#ifdef		ExAllocatePool
#undef		ExAllocatePool						// 删除带TAG的内存分配
#endif

#ifndef		NTSTATUS
typedef		LONG	NTSTATUS;					// 返回状态
#endif


typedef	struct	_USB_SETUP_PKT {				// USB控制传输的建立阶段的数据请求包结构
	UCHAR			mUspReqType;				// 00H 请求类型
	UCHAR			mUspRequest;				// 01H 请求代码
	union	{
		struct	{
			UCHAR	mUspValueLow;				// 02H 值参数低字节
			UCHAR	mUspValueHigh;				// 03H 值参数高字节
		};
		USHORT		mUspValue;					// 02H-03H 值参数
	};
	union	{
		struct	{
			UCHAR	mUspIndexLow;				// 04H 索引参数低字节
			UCHAR	mUspIndexHigh;				// 05H 索引参数高字节
		};
		USHORT		mUspIndex;					// 04H-05H 索引参数
	};
	USHORT			mLength;					// 06H-07H 数据阶段的数据长度
} mUSB_SETUP_PKT, *mPUSB_SETUP_PKT;


#define		mCH375_PACKET_LENGTH	64			// CH375支持的数据包的长度
#define		mCH375_PKT_LEN_SHORT	8			// CH375支持的短数据包的长度


typedef	struct	_WIN32_COMMAND {				// 定义WIN32命令接口结构
	union	{
		ULONG		mFunction;					// 输入时指定功能代码或者管道号
		NTSTATUS	mStatus;					// 输出时返回操作状态
	};
	ULONG			mLength;					// 存取长度,返回后续数据的长度
	union	{
		mUSB_SETUP_PKT	mSetupPkt;				// USB控制传输的建立阶段的数据请求
		UCHAR			mBuffer[ mCH375_PACKET_LENGTH ];	// 数据缓冲区,长度为0至255B
	};
} mWIN32_COMMAND, *mPWIN32_COMMAND;


// WIN32应用层接口命令
#define		IOCTL_CH375_COMMAND		( FILE_DEVICE_UNKNOWN << 16 | FILE_ANY_ACCESS << 14 | 0x0f37 << 2 | METHOD_BUFFERED )	// 专用接口

#define		mWIN32_COMMAND_HEAD		mOFFSET( mWIN32_COMMAND, mBuffer )	// WIN32命令接口的头长度

#define		mCH375_MAX_NUMBER		16			// 最多同时连接的CH375数

#define		mMAX_BUFFER_LENGTH		0x400000		// 数据缓冲区最大长度4MB

#define		mMAX_COMMAND_LENGTH		( mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH )	// 最大数据长度加上命令结构头的长度

#define		mDEFAULT_BUFFER_LEN		0x0400		// 数据缓冲区默认长度1024

#define		mDEFAULT_COMMAND_LEN	( mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN )	// 默认数据长度加上命令结构头的长度


// CH375端点地址
#define		mCH375_ENDP_INTER_UP	0x81		// CH375的中断数据上传端点的地址
#define		mCH375_ENDP_AUX_DOWN	0x01		// CH375的辅助数据下传端点的地址
#define		mCH375_ENDP_DATA_UP		0x82		// CH375的数据块上传端点的地址
#define		mCH375_ENDP_DATA_DOWN	0x02		// CH375的数据块下传端点的地址


// 设备层接口提供的管道操作命令
#define		mPipeDeviceCtrl			0x00000004	// CH375的综合控制管道
#define		mPipeInterUp			0x00000005	// CH375的中断数据上传管道
#define		mPipeDataUp				0x00000006	// CH375的数据块上传管道
#define		mPipeDataDown			0x00000007	// CH375的数据块下传管道
#define		mPipeAuxDown			0x00000008	// CH375的辅助数据下传管道

// 应用层接口的功能代码
#define		mFuncNoOperation		0x00000000	// 无操作
#define		mFuncGetVersion			0x00000001	// 获取驱动程序版本号
#define		mFuncGetConfig			0x00000002	// 获取USB设备配置描述符
#define		mFuncSetExclusive		0x0000000b	// 设置独占使用
#define		mFuncResetDevice		0x0000000c	// 复位USB设备
#define		mFuncResetPipe			0x0000000d	// 复位USB管道
#define		mFuncAbortPipe			0x0000000e	// 取消USB管道的数据请求
#define		mFuncSetTimeout			0x0000000f	// 设置USB通讯超时
#define		mFuncBufferMode			0x00000010	// 设定缓冲上传模式及查询缓冲区中的数据长度
#define		mFuncBufferModeDn		0x00000011	// 设定缓冲下传模式及查询缓冲区中的数据长度

// USB设备标准请求代码
#define		mUSB_CLR_FEATURE		0x01
#define		mUSB_SET_FEATURE		0x03
#define		mUSB_GET_STATUS			0x00
#define		mUSB_SET_ADDRESS		0x05
#define		mUSB_GET_DESCR			0x06
#define		mUSB_SET_DESCR			0x07
#define		mUSB_GET_CONFIG			0x08
#define		mUSB_SET_CONFIG			0x09
#define		mUSB_GET_INTERF			0x0a
#define		mUSB_SET_INTERF			0x0b
#define		mUSB_SYNC_FRAME			0x0c

// CH375控制传输的厂商专用请求类型
#define		mCH375_VENDOR_READ		0xc0		// 通过控制传输实现的CH375厂商专用读操作
#define		mCH375_VENDOR_WRITE		0x40		// 通过控制传输实现的CH375厂商专用写操作

// CH375控制传输的厂商专用请求代码
#define		mCH375_SET_CONTROL		0x51		// 输出控制信号
#define		mCH375_GET_STATUS		0x52		// 输入状态信号

// 寄存器的位定义
#define		mBitInputRxd			0x02		// 只读,RXD#引脚输入状态,1:高电平,0:低电平
#define		mBitInputReq			0x04		// 只读,REQ#引脚输入状态,1:高电平,0:低电平


// 直接输入的状态信号的位定义
#define		mStateRXD				0x00000200	// RXD#引脚输入状态,1:高电平,0:低电平
#define		mStateREQ				0x00000400	// REQ#引脚输入状态,1:高电平,0:低电平

#define		MAX_DEVICE_PATH_SIZE	128			// 设备名称的最大字符数
#define		MAX_DEVICE_ID_SIZE		64			// 设备ID的最大字符数


typedef		VOID	( CALLBACK	* mPCH375_INT_ROUTINE ) (  // 中断服务回调程序
	PUCHAR			iBuffer );  // 指向一个缓冲区,提供当前的中断特征数据


HANDLE	WINAPI	CH375OpenDevice(  // 打开CH375设备,返回句柄,出错则无效
	ULONG			iIndex );  // 指定CH375设备序号,0对应第一个设备,-1则自动搜索一个可以被打开的设备并返回序号


VOID	WINAPI	CH375CloseDevice(  // 关闭CH375设备
	ULONG			iIndex );  // 指定CH375设备序号


ULONG	WINAPI	CH375GetVersion( );  // 获得DLL版本号,返回版本号


ULONG	WINAPI	CH375DriverCommand(  // 直接传递命令给驱动程序,出错则返回0,否则返回数据长度
	ULONG			iIndex,  // 指定CH375设备序号,V1.6以上DLL也可以是设备打开后的句柄
	mPWIN32_COMMAND	ioCommand );  // 命令结构的指针
// 该程序在调用后返回数据长度,并且仍然返回命令结构,如果是读操作,则数据返回在命令结构中,
// 返回的数据长度在操作失败时为0,操作成功时为整个命令结构的长度,例如读一个字节,则返回mWIN32_COMMAND_HEAD+1,
// 命令结构在调用前,分别提供:管道号或者命令功能代码,存取数据的长度(可选),数据(可选)
// 命令结构在调用后,分别返回:操作状态代码,后续数据的长度(可选),
//   操作状态代码是由WINDOWS定义的代码,可以参考NTSTATUS.H,
//   后续数据的长度是指读操作返回的数据长度,数据存放在随后的缓冲区中,对于写操作一般为0


ULONG	WINAPI	CH375GetDrvVersion( );  // 获得驱动程序版本号,返回版本号,出错则返回0


BOOL	WINAPI	CH375ResetDevice(  // 复位USB设备
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375GetDeviceDescr(  // 读取设备描述符
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,用于保存描述符
	PULONG			ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度


BOOL	WINAPI	CH375GetConfigDescr(  // 读取配置描述符
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,用于保存描述符
	PULONG			ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度


BOOL	WINAPI	CH375SetIntRoutine(  // 设定中断服务程序
	ULONG			iIndex,  // 指定CH375设备序号
	mPCH375_INT_ROUTINE	iIntRoutine );  // 指定中断服务回调程序,为NULL则取消中断服务,否则在中断时调用该程序


BOOL	WINAPI	CH375ReadInter(  // 读取中断数据
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,用于保存读取的中断数据
	PULONG			ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度


BOOL	WINAPI	CH375AbortInter(  // 放弃中断数据读操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375ReadData(  // 读取数据块
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,用于保存读取的数据
	PULONG			ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度


BOOL	WINAPI	CH375AbortRead(  // 放弃数据块读操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375WriteData(  // 写出数据块
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			iBuffer,  // 指向一个缓冲区,放置准备写出的数据
	PULONG			ioLength );  // 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度


BOOL	WINAPI	CH375AbortWrite(  // 放弃数据块写操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375WriteRead(  // 先写出标准的数据块(命令),再读取标准的数据块(应答)
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			iBuffer,  // 指向一个缓冲区,放置准备写出的数据,长度不大于mCH375_PACKET_LENGTH
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,长度不小于mCH375_PACKET_LENGTH,用于保存读取的数据
	PULONG			ioLength );  // 指向长度单元,不大于mCH375_PACKET_LENGTH,输入时为准备写出的长度,返回后为实际读取的长度


BOOL	WINAPI	CH375GetStatus(  // 通过CH375直接输入数据和状态
	ULONG			iIndex,  // 指定CH375设备序号
	PULONG			iStatus );  // 指向一个双字单元,用于保存状态数据
// 位7-位0对应CH375的D7-D0引脚,位9对应CH375的RXD#引脚,位10对应CH375的REQ#引脚


BOOL	WINAPI	CH375SetTimeout(  // 设置USB数据读写的超时
	ULONG			iIndex,  // 指定CH375设备序号
	ULONG			iWriteTimeout,  // 指定USB写出数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
	ULONG			iReadTimeout );  // 指定USB读取数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)


BOOL	WINAPI	CH375WriteAuxData(  // 写出辅助数据
	ULONG			iIndex,  // 指定CH375设备序号
	PVOID			iBuffer,  // 指向一个缓冲区,放置准备写出的数据
	PULONG			ioLength );  // 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度


BOOL	WINAPI	CH375SetExclusive(  // 设置独占使用当前CH375设备
	ULONG			iIndex,  // 指定CH375设备序号
	ULONG			iExclusive );  // 为0则设备可以共享使用,非0则独占使用


ULONG	WINAPI	CH375GetUsbID(  // 获取USB设备ID,返回数据中,低16位为厂商ID,高16位为产品ID,错误时返回全0(无效ID)
	ULONG			iIndex );  // 指定CH375设备序号


PVOID	WINAPI	CH375GetDeviceName(  // 返回指向CH375设备名称的缓冲区,出错则返回NULL
	ULONG			iIndex );  // 指定CH375设备序号,0对应第一个设备


BOOL	WINAPI	CH375SetBufUpload(  // 设定内部缓冲上传模式
	ULONG			iIndex,  // 指定CH375设备序号,0对应第一个设备
	ULONG			iEnableOrClear );  // 为0则禁止内部缓冲上传模式,使用直接上传,非0则启用内部缓冲上传模式并清除缓冲区中的已有数据
// 如果启用内部缓冲上传模式,那么CH375驱动程序创建线程自动接收USB上传数据到内部缓冲区,同时清除缓冲区中的已有数据,当应用程序调用CH375ReadData后将立即返回缓冲区中的已有数据


LONG	WINAPI	CH375QueryBufUpload(  // 查询内部上传缓冲区中的已有数据包个数,成功返回数据包个数,出错返回-1
	ULONG			iIndex );  // 指定CH375设备序号,0对应第一个设备


BOOL	WINAPI	CH375SetBufDownload(  // 设定内部缓冲下传模式
	ULONG			iIndex,  // 指定CH375设备序号,0对应第一个设备
	ULONG			iEnableOrClear );  // 为0则禁止内部缓冲下传模式,使用直接下传,非0则启用内部缓冲下传模式并清除缓冲区中的已有数据
// 如果启用内部缓冲下传模式,那么当应用程序调用CH375WriteData后将仅仅是将USB下传数据放到内部缓冲区并立即返回,而由CH375驱动程序创建的线程自动发送直到完毕


LONG	WINAPI	CH375QueryBufDownload(  // 查询内部下传缓冲区中的剩余数据包个数(尚未发送),成功返回数据包个数,出错返回-1
	ULONG			iIndex );  // 指定CH375设备序号,0对应第一个设备


BOOL	WINAPI	CH375ResetInter(  // 复位中断数据读操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375ResetAux(  // 复位辅助数据写操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375ResetRead(  // 复位数据块读操作
	ULONG			iIndex );  // 指定CH375设备序号


BOOL	WINAPI	CH375ResetWrite(  // 复位数据块写操作
	ULONG			iIndex );  // 指定CH375设备序号


typedef		VOID	( CALLBACK	* mPCH375_NOTIFY_ROUTINE ) (  // 设备事件通知回调程序
	ULONG			iEventStatus );  // 设备事件和当前状态(在下行定义): 0=设备拔出事件, 3=设备插入事件

#define		CH375_DEVICE_ARRIVAL		3		// 设备插入事件,已经插入
#define		CH375_DEVICE_REMOVE_PEND	1		// 设备将要拔出
#define		CH375_DEVICE_REMOVE			0		// 设备拔出事件,已经拔出


BOOL	WINAPI	CH375SetDeviceNotify(  // 设定设备事件通知程序
	ULONG					iIndex,  // 指定CH375设备序号,0对应第一个设备
	PCHAR					iDeviceID,  // 可选参数,指向字符串,指定被监控的设备的ID,字符串以\0终止
	mPCH375_NOTIFY_ROUTINE	iNotifyRoutine );  // 指定设备事件回调程序,为NULL则取消事件通知,否则在检测到事件时调用该程序


BOOL	WINAPI	CH375SetTimeoutEx(  // 设置USB数据读写的超时
	ULONG			iIndex,  // 指定CH375设备序号
	ULONG			iWriteTimeout,  // 指定USB写出数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
	ULONG			iReadTimeout,  // 指定USB读取数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
	ULONG			iAuxTimeout,  // 指定USB辅助下传数据的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
	ULONG			iInterTimeout );  // 指定USB中断上传数据的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)

BOOL	WINAPI	CH375ReadEndP(  // 读取数据块
	ULONG			iIndex,  // 指定CH375设备序号
	ULONG			iPipeNum,  //端点号，有效值为1到8。
	PVOID			oBuffer,  // 指向一个足够大的缓冲区,用于保存读取的数据
	PULONG		ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度
	
BOOL	WINAPI	CH375WriteEndP(  // 写出数据块
	ULONG			iIndex,  // 指定CH375设备序号
	ULONG			iPipeNum,  //端点号，有效值为1到8。
	PVOID			iBuffer,  // 指向一个缓冲区,放置准备写出的数据
	PULONG		ioLength );  // 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度

BOOL	WINAPI	CH375AbortEndPRead(	// 放弃数据块写操作
	ULONG			iIndex, 									// 指定CH375设备序号
	ULONG			iPipeNum  );  							// 端点号，有效值为1到8

BOOL	WINAPI	CH375AbortEndPWrite(	// 放弃数据块写操作
									ULONG			iIndex, 									// 指定CH375设备序号
									ULONG			iPipeNum  );  							// 端点号，有效值为1到8
//使用CH375ReadEndP读取驱动中缓冲的数据。
//如果使用CH375SetBufUpload启用缓冲上传时，是驱动对端点2进行数据缓冲；
//如果使用CH375SetBufUploadEx启用缓冲上传时，是驱动对端点iPipeNum进行数据缓冲。
//CH375SetBufUploadEx用于USB3.0的传输。	
BOOL	WINAPI	CH375SetBufUploadEx(  // 设定内部缓冲上传模式
									ULONG			iIndex,  // 指定CH375设备序号,0对应第一个设备
									ULONG			iEnableOrClear,// 为0则禁止内部缓冲上传模式,使用直接上传,非0则启用内部缓冲上传模式并清除缓冲区中的已有数据
									ULONG			iPipeNum,		// 端点号，有效值为1到8
									ULONG			BufSize );   //每包缓冲区大小，最大4MB

BOOL	WINAPI	CH375QueryBufUploadEx(  // 查询内部上传缓冲区中的已有数据包个数和总字节数,成功返回TRUE,失败FALSE
									  ULONG			iIndex,				// 指定CH375设备序号,0对应第一个设备	
									  ULONG			iPipeNum,			// 端点号，有效值为1到8
									  PULONG		oPacketNum,		// 返回内部缓冲中的已有数据包个数	
									  PULONG		oTotalLen );  // 返回内部缓冲中的已有数据包总字节数	

BOOL	WINAPI	CH375ClearBufUpload(// 清空内部缓冲区，无需暂停内部缓冲上传模式
									ULONG			iIndex,  			// 指定CH375设备序号,0对应第一个设备
									ULONG			iPipeNum );  // 端点号，有效值为1到8


#ifdef __cplusplus
}
#endif

#endif		// _CH375_DLL_H
