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
#include "Usb20Jtag.h"

ULONG CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; //USB PKT command code(1B)+length(2B)+data
ULONG CMDPKT_DATA_MAX_BITS   =  (CMDPKT_DATA_MAX_BYTES/2);   //USB PKT maximum number of data bits per packet, each data bit requires two TCK composed of low to high bit bands.  
BOOL AfxUsbHighDev = TRUE;
UCHAR  DataUpEndp=0,DataDnEndp=0;
USHORT BulkInEndpMaxSize=512,BulkOutEndpMaxSize=512;
ULONG  MaxBitsPerBulk;
ULONG  MaxBytesPerBulk;
HANDLE AfxDevH[8] = {INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};

//构建JTAG复位命令包，位带数据
ULONG BuildPkt_Reset(PUCHAR BitBangPkt)
{
	UCHAR BI = 0,i;

	//Reserved 3-byte packet header :1 byte command code +2 bytes subsequent length (small end)
	BI = USB20_CMD_HEADER; 
	for(i=0;i<7;i++)
	{
		BitBangPkt[BI++] = TMS_H | TDI_H | TCK_L;
		BitBangPkt[BI++] = TMS_H | TDI_H | TCK_H;
	}
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;

	BitBangPkt[0] = USB20_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = BI - USB20_CMD_HEADER;
	BitBangPkt[2] = 0;
	
	return BI;
}

//Build the bitband data command package for state machine switching:Shift-IR/DR->Test-Idle
ULONG BuildPkt_ExitShiftToRunIdle(PUCHAR BitBangPkt)
{
	UCHAR BI = 0;

	//usb to jtag communication protocol packet header
	BitBangPkt[BI++] = USB20_CMD_JTAG_BIT_OP;  //Cmd code
	BitBangPkt[BI++] = 4;                       //data length lower 8 bit
	BitBangPkt[BI++] = 0;                       //data length higher 8 bit

	//Exit1-DR -> Update-DR
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_H;

	//Update-DR -> Run-Test-Idle
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;
	return BI;
}

//Build the bitband data command package for state machine switching:Test-Idle至Shift-IR
ULONG BuildPkt_EnterShiftIR(PUCHAR BitBangPkt)
{
	UCHAR BI = 0;	
	
	BI = USB20_CMD_HEADER; //Reserved 3-byte packet header :1 byte command code +2 bytes subsequent length (small end)
	//Go to Run-Test-Idle
	//BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	//BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;
	//Go to Select-DR-Scan
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_H;
	//Go to Select-IR-Scan
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_H;
	//Go to Capture-IR
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;
	// Go to Shift-IR
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;

	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;

	//usb to jtag usb packet header
	BitBangPkt[0] = USB20_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = BI - USB20_CMD_HEADER;
	BitBangPkt[2] = 0;

	return BI;
}

//Build the bitband data command package for state machine switching:Test-Idle至Shift-DR
ULONG BuildPkt_EnterShiftDR(PUCHAR BitBangPkt)
{
	UCHAR BI = 0;	
	
	BI = USB20_CMD_HEADER; //Reserved 3-byte packet header :1 byte command code +2 bytes subsequent length (small end)
	//Go to Run-Test/Idle
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	//Go to Select-DR-Scan
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_H | TDI_H | TCK_H;
	//Go to Capture-DR 
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;
	//Go to Shift-DR
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_H;

	BitBangPkt[BI++] = TMS_L | TDI_H | TCK_L;
	//usb to jtag usb packet head
	BitBangPkt[0] = USB20_CMD_JTAG_BIT_OP;
	BitBangPkt[1] = BI - USB20_CMD_HEADER;
	BitBangPkt[2] = 0;

	return BI;
}

// Build the TDO/TDI bitband command package to send data via USB jTAG-tDI or read data via USB Jtag-tDO.  You are advised to use BuildPkt_DataShift_FAST for batch data  
// The function returns the size of the command package  
// State machine changes: shift-dr /IR -> data Shift- > Exit DR/IR  
ULONG BuildPkt_DataShift(PUCHAR  DataBuf,           //Bytes of data removed from TDI
						 ULONG   BitCount,          //The total number of bits moved out, returns the number of bits to read
						 PUCHAR  CmdPktBuf,         //Bitband buffer
						 BOOL    IsRead,            //Whether to read TDO data after TDI output
						 PULONG  CmdPktReturnSize,  //After USB JTAG reads TDO data, it returns the size of the command package containing TDO data, including the header
						 BOOL    IsLastPkt)         //Whether the last batch of shifted data
{
	ULONG BI,i,DI,DLen,RetPktBytes;
	UCHAR TMS_Bit,TDI_Bit;

	//下传JTAG TDI位带数据
	TMS_Bit = TMS_L;
	BI = 0;
	RetPktBytes = 0;

	for(i=0;i<BitCount;i++)
	{
		if( (i%CMDPKT_DATA_MAX_BITS)==0 ) //The command header is filled.
		{
			//计算命令包内放入的字节数
			if( (BitCount-i) > CMDPKT_DATA_MAX_BITS )  //full packet
				DLen = CMDPKT_DATA_MAX_BITS*2;         //Bitband data for each bit needs to be represented by two bytes (shift on edge, two clocks)
			else                                       //lasted packet
				DLen = (BitCount-i)*2;                 //Bitband data for each bit needs to be represented by two bytes (shift on edge, two clocks)
			
			//填充命令码
			if(IsRead)  //位带方式读操作，使用USB20_CMD_JTAG_BIT_OP_RD
				CmdPktBuf[BI++] = USB20_CMD_JTAG_BIT_OP_RD;
			else        //位带方式写操作
				CmdPktBuf[BI++] = USB20_CMD_JTAG_BIT_OP;			
			
			//Fill the length of data in the command package. The lower eight bits are the first
			CmdPktBuf[BI++] = (UCHAR)(DLen&0xFF);
			CmdPktBuf[BI++] = (UCHAR)((DLen>>8)&0xFF);

			//USB20_CMD_JTAG_BIT_OP_RD Command package size when the command is returned. Unlike sending, each bit returned is 
			//a byte representing the size of the command package returned when the USB20_CMD_JTAG_BIT_OP_RD command is returned.
			//Unlike sending, each bit returned is represented by a byte
			RetPktBytes += (DLen/2 + USB20_CMD_HEADER);
		}

		//TDI assignment: Bit data is stored in bytes. Bit0 corresponds to byte0 bit0, and bit1 corresponds to byte bit1 from the lowest to the highest. LSB
		DI = i/8;  //8位为一字节
		if( (DataBuf[DI]>>(i%8))&0x01 ) 
			TDI_Bit = TDI_H;
		else
			TDI_Bit = TDI_L;

		//TMS assignment: The value must be switched only when the last bit is output in exIT1-dr state, that is, TMS=H
		if( ((i+1)==BitCount) && IsLastPkt) 
			TMS_Bit = TMS_H;
		
		//每数据位需要两个位带字节,时钟高和低
		CmdPktBuf[BI++] = TMS_Bit | TDI_Bit | TCK_L;
		CmdPktBuf[BI++] = TMS_Bit | TDI_Bit | TCK_H;
	}
	if(CmdPktReturnSize)
		*CmdPktReturnSize = RetPktBytes;

	return BI;  //命令包大小
}

// Build the TDO/TDI data exchange command package in fast mode, used to send data via USB JTAG-tDI or read data via USB Jtag-TDO.  
// USB transmission on the software and hardware side, only TDI/TDO data bits are transmitted in bytes. At the same time, the hardware side is enabled to send TDI bits in DMA mode, which is more efficient than bitband mode  
ULONG BuildPkt_DataShift_Fast(PUCHAR DataBuf,    //Bytes of data removed from TDI
							  ULONG  DataBytes,  //Data bytes
							  PUCHAR CmdPktBuf,  //Command packet buffer
							  BOOL   IsRead,     //Whether to read TDO data after TDI output
							  BOOL   IsLastPkt)  //Whether the last packet of shifted data
{
	ULONG BI, i, DI, RemainBits, TdiBytes, PktDataLen;	

	//JTAG TDX data is transmitted through a custom fast protocol. TCK signals are generated by hardware and transmitted to USB20 JTAG in bytes
	TdiBytes = DataBytes;
	if ((TdiBytes) && (IsLastPkt))//The last bit of the last packet of data needs to be sent under exit Dr/IR using the D1/D2 bit belt command.
		TdiBytes--;  //Reserve the last byte and send the command using D1/D2 bits

	DI = BI = 0;	
	while ( DI < TdiBytes)
	{	
		if ((TdiBytes - DI) > CMDPKT_DATA_MAX_BYTES) //满包传递
			PktDataLen = CMDPKT_DATA_MAX_BYTES;
		else
			PktDataLen = TdiBytes - DI;              
		if(IsRead)
			CmdPktBuf[BI++] = USB20_CMD_JTAG_DATA_SHIFT_RD;
		else
			CmdPktBuf[BI++] = USB20_CMD_JTAG_DATA_SHIFT;
		CmdPktBuf[BI++] = (UCHAR)(PktDataLen >> 0) & 0xFF;
		CmdPktBuf[BI++] = (UCHAR)(PktDataLen >> 8) & 0xFF; //长度高8位
		memcpy(&CmdPktBuf[BI],&DataBuf[DI],PktDataLen);
		BI += PktDataLen;
		DI += PktDataLen; //TDX数据偏移
	}

	//D1/D2 command package, which is grouped in bitband mode
	RemainBits = (DataBytes-DI)*8; //最后一字节	
	if(RemainBits)
	{
		UCHAR TMS_Bit,TDI_Bit;

		CmdPktBuf[BI++] = IsRead?USB20_CMD_JTAG_BIT_OP_RD:USB20_CMD_JTAG_BIT_OP;
		PktDataLen = RemainBits*2;  //Each data bit consists of a two-byte bit band

		CmdPktBuf[BI++] = (UCHAR)(PktDataLen>>0)&0xFF;
		CmdPktBuf[BI++] = (UCHAR)(PktDataLen>>8)&0xFF; //The length is 8 bits high

		TMS_Bit = TMS_L;
		for(i=0;i<RemainBits;i++)
		{
			if( ((DataBuf[DI]>>i)&0x01) )
				TDI_Bit = TDI_H;
			else
				TDI_Bit = TDI_L;

			if( ((i+1)==RemainBits ) && (IsLastPkt) )//The last bit is output in the exit1-dr state
				TMS_Bit = TMS_H;
			CmdPktBuf[BI++] = TMS_Bit | TDI_Bit | TCK_L;
			CmdPktBuf[BI++] = TMS_Bit | TDI_Bit | TCK_H;
		}
	}
	/*
	BitBuf[BI++] = USB20_CMD_JTAG_BIT_OP;
	BitBuf[BI++] = 4;
	BitBuf[BI++] = 0; //长度高8位
	//Exit1-DR -> Update-DR
	BitBuf[BI++] = TMS_H | TDI_H | TCK_L;
	BitBuf[BI++] = TMS_H | TDI_H | TCK_H;
	//Update-DR -> Run-Test-Idle
	BitBuf[BI++] = TMS_L | TDI_H | TCK_L;
	BitBuf[BI++] = TMS_L | TDI_H | TCK_H;
	*/
	return BI;
}

// Parse the command package returned by D2/D4 to extract TDI/TDO data.  When the D4 command returns, the last digit will be returned using D2  
ULONG ParseCmdRetPkt(PUCHAR CmdPktBuf,ULONG CmdPktBufSize,PUCHAR DataBuf,PULONG DataLen,PULONG DataBitCnt)
{
	ULONG BI,RI,P,j,PktLen,BitCount;

	BI = RI = 0;
	BitCount = 0;

	if( (CmdPktBuf[0] != USB20_CMD_JTAG_BIT_OP_RD) && (CmdPktBuf[0] != USB20_CMD_JTAG_DATA_SHIFT_RD) )
		DbgPrint("!!!Invalid First data.%02X %02X",CmdPktBuf[0],CmdPktBuf[1]);

	while(BI<CmdPktBufSize)
	{
		if( (BI+USB20_CMD_HEADER) >= CmdPktBufSize )//不完整数据
		{
			DbgPrint("ParseCmdRetPkt.imperfect return data");
			break;
		}
		//if( (BitBang[BI]!=USB20_CMD_JTAG_BIT_OP) && (BitBang[BI]!=USB20_CMD_JTAG_BIT_OP_RD) ) //Invalid packet
		if( CmdPktBuf[BI]==USB20_CMD_JTAG_BIT_OP_RD ) //Data is returned in bitband format
		{
			PktLen = CmdPktBuf[BI+1] + ((CmdPktBuf[BI+2]<<8)&0xFF00);
			BI += USB20_CMD_HEADER;

			//将位按字节存放
			for(P=0;P<PktLen;)
			{
				((PUCHAR)DataBuf)[RI] = 0;
				for(j=0;j<8;j++)
				{				
					if(CmdPktBuf[BI]&0x01)
						((PUCHAR)DataBuf)[RI] |= (1<<j);
					BI++;					
					P++;
					if( BI >= CmdPktBufSize )
						break;
				}
				RI++;
			}
			BitCount += P; //实际上传位数
		}
		else if( CmdPktBuf[BI]==USB20_CMD_JTAG_DATA_SHIFT_RD ) //Data is returned in bytes without conversion
		{
			PktLen = CmdPktBuf[BI+1] + ((CmdPktBuf[BI+2]<<8)&0xFF00);			
			BitCount += PktLen*8;
			BI += USB20_CMD_HEADER;
			if( BI+PktLen > CmdPktBufSize ) //Incomplete transfer data
			{
				DbgPrint("!!SPIRead incomplete(%X-%X),%02X %02X",PktLen,CmdPktBufSize - BI,CmdPktBuf[BI],CmdPktBuf[BI+1]);
				PktLen = CmdPktBufSize - BI;
			}			
			memcpy(DataBuf+RI,CmdPktBuf+BI,PktLen);
			RI += PktLen;
			BI += PktLen;
		}
		else
			BI++;
	}
	*DataLen = RI;
	if(DataBitCnt)
		*DataBitCnt = BitCount;
	return RI;
}

// Bit band mode JTAG IR/DR data read and write.  Suitable for reading and writing small amounts of data.  Such as command operation, state machine switching and other control transmission.  For batch data transfer, USB20Jtag_WriteRead_Fast is recommended  
// The command package is read and written in batches in 4096 bytes  
// State machine: run-test -> shift-ir /DR..  ->Exit IR/DR -> Run-Test  
BOOL	WINAPI	USB20Jtag_WriteRead(ULONG			iIndex,           // USB Device Serial number
							   BOOL             IsDR,             // =TRUE: DR data read/write,=FALSE:IR data read/write
							   ULONG			iWriteBitLength,  // 位长度,准备写出的二进制位长度,
							   PVOID			iWriteBitBuffer,  // 指向一个缓冲区,放置准备写出的数据,,字节存储
							   PULONG			oReadBitLength,   // 指向长度单元,返回后为实际读取的二进制位长度
							   PVOID			oReadBitBuffer )  // Points to a buffer large enough to hold the read data,字节存储
{
	ULONG BI,TxLen,BitCount,WBI,RI,BufSize,BulkOutBits,RLen,BitLen,RetPktBytes,TotalRx=0;
	UCHAR CmdPktBuf[4096+16]="";
	BOOL IsLastData = FALSE,IsRead,RetVal=FALSE;
		
	USB20Jtag_ClearUpBuf(0); //清空驱动内上传数据缓冲	

	IsRead = (oReadBitLength!=NULL) && (oReadBitBuffer!=NULL);
	BufSize = sizeof(CmdPktBuf);
	if(IsRead) BitCount = *oReadBitLength;
	else       BitCount = iWriteBitLength;
	
	//先构建命令包，再将多个命令包组成4K为单位进行USB传输。4K批数据内需都是完整命令包
	memset(CmdPktBuf,0,BufSize);
	BI = 0;
	WBI = 0;
	RI = 0;
	if(IsRead)
		*oReadBitLength = 0;
	IsLastData = FALSE;

	//构建JTAG状态机命令包,Run-Test-Idle -> Shift-DR	
	if(IsDR)
		BI += BuildPkt_EnterShiftDR(&CmdPktBuf[0]);
	else
		BI += BuildPkt_EnterShiftIR(&CmdPktBuf[0]);	
	
	while(WBI<BitCount)
	{	
		//单批次写入位数,4KB USB传输块为单位计算。
		if( (WBI+MaxBitsPerBulk)>BitCount )
		{
			BulkOutBits = BitCount-WBI;
			IsLastData = TRUE;
		}
		else
			BulkOutBits = MaxBitsPerBulk; 

		//构建DR/IR命令包
		BI += BuildPkt_DataShift((PUCHAR)iWriteBitBuffer+WBI/8,BulkOutBits,&CmdPktBuf[BI],IsRead,&RetPktBytes,IsLastData); //返回是位带组包后字节数
		WBI += BulkOutBits;
		//DumpBitBangData(BitBang,BI);

		//发送命令包
		TxLen = BI;
		if( !CH375WriteEndP(0,DataDnEndp,CmdPktBuf,&TxLen) && (TxLen!=BI) )
		{
			DbgPrint("JTAG_WriteRead send usb data failure.");
			goto Exit;
		}

		if(IsRead)//接收命令包
		{
			RLen = RetPktBytes;
			if( !USB20Jtag_ReadData(iIndex,&RLen,CmdPktBuf,20) )
			{
				DbgPrint("JTAG_WriteRead read usb data failure.");
				goto Exit;
			}
			TotalRx += RLen;
			//从返回的多个命令包内提取数据
			ParseCmdRetPkt(CmdPktBuf,RLen,(PUCHAR)oReadBitBuffer+RI,&RLen,&BitLen);
			if(BitLen<1)
			{
				//goto Exit;
			}
			else
			{
				RI += RLen;
				*oReadBitLength += BitLen;
			}
		}
		BI = 0;
	}
	RetVal = TRUE;

Exit:
	USB20Jtag_SwitchTapState(6);
	return RetVal;
}

//JTAG IR/DR data batch read and write, used for multi-byte continuous read and write.  Such as JTAG firmware download operation.  The hardware has a 4K buffer. If the buffer is written before the buffer is read, the length cannot exceed 4096 bytes.  Buffer size can be adjusted  
// State machine: run-test -> shift-ir /DR..  ->Exit IR/DR -> Run-Test  
BOOL	WINAPI	USB20Jtag_WriteRead_Fast(ULONG			iIndex,        // 指定CH375设备序号
									BOOL            IsDR,          // =TRUE: DR data read/write,=FALSE:IR data read/write
									ULONG			iWriteLength,  // Write length, the length of bytes to be written
									PVOID			iWriteBuffer,  // Points to a buffer to place data ready to be written out
									PULONG			oReadLength,   // Points to the length unit and returns the length of the bytes actually read
									PVOID			oReadBuffer )  // Points to a buffer large enough to hold the read data
{
	ULONG BI,TxLen,RxLen,WI,RI,BufSize,TotalWriteLen,BulkOutBytes,CmdPktBufSize;
	UCHAR CmdPktBuf[8192] = ""; 
	BOOL RetVal = FALSE,IsLastData = FALSE,IsRead;
	
	//清空驱动内上传数据缓冲
	USB20Jtag_ClearUpBuf(0);
	IsRead = (oReadLength!=NULL) && (oReadBuffer!=NULL);
	BufSize = sizeof(CmdPktBuf);
	if(IsRead)
		TotalWriteLen = *oReadLength;
	else 
		TotalWriteLen = iWriteLength;	
	
	memset(CmdPktBuf,0,BufSize);
	BI = WI = RI = 0;
	IsLastData = FALSE;

	//构建JTAG状态机命令包,Run-Test-Idle -> Shift-DR	
	if(IsDR)
		BI += BuildPkt_EnterShiftDR(&CmdPktBuf[0]);
	else
		BI += BuildPkt_EnterShiftIR(&CmdPktBuf[0]);
	while(WI<TotalWriteLen)
	{		
		if( (WI + MaxBytesPerBulk)>TotalWriteLen )//BI只有在传递第一包数据时会与DR/IR拼包
		{
			BulkOutBytes = TotalWriteLen - WI;
			IsLastData = TRUE;
		}
		else
			BulkOutBytes = MaxBytesPerBulk;
		if( (BulkOutBytes+BI) > MaxBytesPerBulk)
			BulkOutBytes = BulkOutBytes -BI;

		//构建快速模式下TDO/TDI多字节输入输出的数据包，转换以位为单片
		CmdPktBufSize = BuildPkt_DataShift_Fast((PUCHAR)iWriteBuffer+WI,BulkOutBytes,&CmdPktBuf[BI],IsRead,IsLastData);;
		BI += CmdPktBufSize;
		WI += BulkOutBytes;

		TxLen = BI;
		//DumpBitBangData(BitBang,TxLen); //打印位带波形,仅调试
		if( !CH375WriteEndP(0,DataDnEndp,CmdPktBuf,&TxLen) && (TxLen!=BI) )
		{
			DbgPrint("JTAG_WriteRead_B send usb data failure.");
			goto Exit;
		}
		if(IsRead)
		{
			if(IsLastData) //最后一字节与发送相差8个字节。发送时是一位用两个字节位带表示。接收是一个位用一个字节带表示
				RxLen = CmdPktBufSize-8;
			else
				RxLen = CmdPktBufSize;

			if( !USB20Jtag_ReadData(iIndex,&RxLen,CmdPktBuf,30) )
			{
				DbgPrint("JTAG_WriteRead_B read usb data failure.");
				goto Exit;
			}
			//从返回的多个命令包内提取数据
			ParseCmdRetPkt(CmdPktBuf,RxLen,(PUCHAR)oReadBuffer+RI,&RxLen,NULL);
			if(RxLen<1)
				goto Exit;
			RI += RxLen;
		}
		BI = 0;	
	}
	RetVal = TRUE;
Exit:
	USB20Jtag_SwitchTapState(6);
	if(IsRead) 
		*oReadLength = RI;
	
	return RetVal;
}


//Open Device
BOOL USB20Jtag_OpenDevice(ULONG DevI)
{
	if(DevI > 7)
		return FALSE;
	AfxDevH[DevI] = CH375OpenDevice(DevI);	
	if(AfxDevH[DevI] != INVALID_HANDLE_VALUE)
	{
		CH375SetTimeoutEx(DevI,1000,1000,1000,1000);
	}
	return(AfxDevH[DevI] != INVALID_HANDLE_VALUE);
}

//CloseDevice
BOOL USB20Jtag_CloseDevice(ULONG DevI)
{
	if( AfxDevH[DevI] != INVALID_HANDLE_VALUE )
	{
		CH375CloseDevice(DevI);
		AfxDevH[DevI] = INVALID_HANDLE_VALUE;
	}
	return TRUE;
}

// Clear drive USB upload endpoint buffer data.  USB bulk endpoint reads, using buffered upload mode.  
BOOL USB20Jtag_ClearUpBuf(ULONG iIndex)
{
	UCHAR DB[HW_TDO_BUF_SIZE] = "";
	ULONG PacketCnt = 0,TotalLen = 0,RLen = 0;	
	
	CH375ClearBufUpload(0,DataUpEndp);
	return TRUE;
}

//JTAG USB data read function, buffer upload mode
UCHAR USB20Jtag_ReadData(ULONG iIndex,PULONG iLength,PUCHAR DataBuf,ULONG ReadTimeout)
{
	ULONG RLen,RI,WaitT = 0,PacketCnt=0,TotalLen=0;
	BOOL  RetVal = FALSE;
	
	RI = 0;	
	while(1)
	{
		RLen = *iLength-RI;
		if( !CH375QueryBufUploadEx(iIndex,DataUpEndp,&PacketCnt,&TotalLen) )
			break;
		if(TotalLen>=*iLength)
		{
			if( !CH375ReadEndP(iIndex,DataUpEndp,DataBuf+RI,&RLen) )
			{
				DbgPrint("USB20Jtag_ReadData failure.");
				goto Exit;
			}		
			RI += RLen;
			if(RI>=*iLength)
				break;
		}
		if(WaitT++ >= ReadTimeout)
			break;
		Sleep(1); //此处延时可能比1MS大
	}	
	RetVal = TRUE;
Exit:
	*iLength = RI; //返回从USB批量上传端点读的数据长度
	return RetVal;
}

//USB downpass endpoint write, direct operation mode
UCHAR USB20Jtag_WriteData(ULONG iIndex,PULONG oLength,PUCHAR DataBuf)
{	
	BOOL  RetVal = FALSE;

	RetVal = CH375WriteEndP(iIndex,DataDnEndp,DataBuf,oLength);
	return RetVal;
}

//JTAG IR read, in bytes, multi-byte continuous read and write.  
// State machine: run-test -> shift-ir..  ->Exit IR -> Run-Test  
BOOL	WINAPI	USB20Jtag_ByteReadIR(ULONG			iIndex,        // 指定设备序号									
								   PULONG			oReadLength,   // Points to the length unit and returns the length of the bytes actually read
								   PVOID			oReadBuffer )  // Points to a buffer large enough to hold the read data
{
	PUCHAR iWriteBuffer;
	ULONG iWriteLength;
	BOOL   RetVal;

	iWriteLength = *oReadLength;
	iWriteBuffer = (PUCHAR)malloc(iWriteLength);
	if(iWriteBuffer==NULL)
		return FALSE;
	memset(iWriteBuffer,0xFF,iWriteLength);
	
	RetVal = USB20Jtag_WriteRead_Fast(iIndex,FALSE,iWriteLength,iWriteBuffer,oReadLength,oReadBuffer);

	free(iWriteBuffer);
	return RetVal;
}

//JTAG IR write, multi-byte continuous write in bytes.  
// State machine: run-test -> shift-ir..  ->Exit IR -> Run-Test  
BOOL	WINAPI	USB20Jtag_ByteWriteIR(ULONG			iIndex,        // 指定CH375设备序号									
									ULONG			iWriteLength,  // Write length, the length of bytes to be written
									PVOID			iWriteBuffer)  // Points to a buffer to place data ready to be written out									
{
	return USB20Jtag_WriteRead_Fast(iIndex,FALSE,iWriteLength,iWriteBuffer,NULL,NULL);
}

//JTAG DR read, multi-byte continuous read, in bytes.  
// State machine: run-test -> shift-dr..  ->Exit DR -> Run-Test  
BOOL	WINAPI	USB20Jtag_ByteReadDR(ULONG			iIndex,        // 指定设备序号									
								   PULONG			oReadLength,   // Points to the length unit and returns the length of the bytes actually read
								   PVOID			oReadBuffer )  // Points to a buffer large enough to hold the read data
{
	PUCHAR iWriteBuffer;
	ULONG iWriteLength;
	BOOL   RetVal;

	iWriteLength = *oReadLength;
	iWriteBuffer = (PUCHAR)malloc(*oReadLength);
	if(iWriteBuffer==NULL)
		return FALSE;
	memset(iWriteBuffer,0xFF,iWriteLength);
	
	RetVal = USB20Jtag_WriteRead_Fast(iIndex,TRUE,iWriteLength,iWriteBuffer,oReadLength,oReadBuffer);

	free(iWriteBuffer);
	return RetVal;
}

//JTAG DR write, in bytes, used for multi-byte continuous read and write. Such as JTAG firmware download operation.
// State machine: run-test - > Shift-DR.. -&gt; Exit DR -&gt; Run-Test
BOOL	WINAPI	USB20Jtag_ByteWriteDR(ULONG			iIndex,        // 指定CH375设备序号									
									ULONG			iWriteLength,  // Write length, the length of bytes to be written
									PVOID			iWriteBuffer)  // Points to a buffer to place data ready to be written out									
{
	return USB20Jtag_WriteRead_Fast(iIndex,TRUE,iWriteLength,iWriteBuffer,NULL,NULL);
}

// Bit band mode JTAG DR data read. Suitable for reading and writing small amounts of data. For batch and high-speed data transmission, USB20Jtag_ByteReadDR is recommended
// State machine: run-test - > Shift-DR.. -&gt; Exit DR -&gt; Run-Test
BOOL	WINAPI	USB20Jtag_BitReadDR(ULONG			iIndex,           // 指定设备序号									
						          PULONG    	oReadBitLength,   // Points to the length unit and returns the length of the bytes actually read
								  PVOID			oReadBitBuffer )  // Points to a buffer large enough to hold the read data
{
	PUCHAR iWriteBitBuffer;
	ULONG iWriteBitLength;
	BOOL   RetVal;

	iWriteBitLength = *oReadBitLength;
	iWriteBitBuffer = (PUCHAR)malloc(iWriteBitLength);
	if(iWriteBitBuffer==NULL)
		return FALSE;
	memset(iWriteBitBuffer,0xFF,iWriteBitLength);
	RetVal = USB20Jtag_WriteRead(iIndex,TRUE,iWriteBitLength,iWriteBitBuffer,oReadBitLength,oReadBitBuffer);

	free(iWriteBitBuffer);
	return RetVal;
}

// Bit band mode JTAG IR data read. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching, etc. For batch data transmission, USB20Jtag_ByteReadIR is recommended
// State machine: run-test - > Shift-IR.. -&gt; Exit IR -&gt; Run-Test
BOOL	WINAPI	USB20Jtag_BitReadIR(ULONG			iIndex,           // Specify device number									
						          PULONG    	oReadBitLength,   // Points to the length unit and returns the length of the bytes actually read
								  PVOID			oReadBitBuffer )  // Points to a buffer large enough to hold the read data
{
	PUCHAR iWriteBitBuffer;
	ULONG  iWriteBitLength;
	BOOL   RetVal;

	iWriteBitLength = *oReadBitLength;
	iWriteBitBuffer = (PUCHAR)malloc(iWriteBitLength);
	if(iWriteBitBuffer==NULL)
		return FALSE;
	memset(iWriteBitBuffer,0xFF,iWriteBitLength);
	RetVal = USB20Jtag_WriteRead(iIndex,FALSE,iWriteBitLength,iWriteBitBuffer,oReadBitLength,oReadBitBuffer);

	free(iWriteBitBuffer);
	return RetVal;
}


// Bit band mode JTAG DR data write. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching and other control transmission. For batch data transmission, USB20Jtag_ByteWriteIR is recommended
// State machine: run-test - > Shift-IR.. -&gt; Exit IR -&gt; Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteIR(ULONG			iIndex,           // Specify device number									
						           ULONG    	    iWriteBitLength,   // Points to the length unit and returns the length of the bytes actually read
								   PVOID			iWriteBitBuffer )  // Points to a buffer large enough to hold the read data
{
	return USB20Jtag_WriteRead(iIndex,FALSE,iWriteBitLength,iWriteBitBuffer,NULL,NULL);
}

// Bit band mode JTAG DR data write. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching and other control transmission. For batch data transfer, USB20Jtag_ByeWriteDR is recommended
// State machine::Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	USB20Jtag_BitWriteDR(ULONG			iIndex,           // Specify device number									
						           ULONG    	    iWriteBitLength,   // Points to the length unit and returns the length of the bytes actually read
								   PVOID			iWriteBitBuffer )  // Points to a buffer large enough to hold the read data
{
	return USB20Jtag_WriteRead(iIndex,TRUE,iWriteBitLength,iWriteBitBuffer,NULL,NULL);
}

// Switch the JTAG state machine
BOOL USB20Jtag_SwitchTapState(UCHAR TapState)
{	
	BOOL RetVal = FALSE;
	ULONG TxLen,BI;
	UCHAR BitBang[4096]="";

	//Build the bitband command package using the D1 command
	switch(TapState)
	{
	case 0: //Test-Logic Reset
		BI = (UCHAR)BuildPkt_Reset(BitBang);
		break;
	case 1: //Run-Test/Idle
		{
			BI = USB20_CMD_HEADER; //3 bytes header :1 byte command code +2 bytes subsequent length (small endian)	
			BitBang[BI++] = TMS_L | TDI_H | TCK_L;
			BitBang[BI++] = TMS_L | TDI_H | TCK_H;

			BitBang[BI++] = TMS_L | TDI_H | TCK_L;

			BitBang[0] = USB20_CMD_JTAG_BIT_OP;
			BitBang[1] = (UCHAR)BI-USB20_CMD_HEADER;
			BitBang[2] = 0; 
		}
		break;
	case 2: //Run-Test/Idle -> Shift-DR
		{	
			BI = (UCHAR)BuildPkt_EnterShiftDR(BitBang);
		}
		break;
	case 3: //Shift-DR -> Run-Test/Idle
		{
			BI = USB20_CMD_HEADER; //3 bytes header :1 byte command code +2 bytes subsequent length (small endian)
			//Shift-DR -> Exit1-DR
			BitBang[BI++] = TMS_H | TDI_H | TCK_L;
			BitBang[BI++] = TMS_H | TDI_H | TCK_H;
			//Exit1-DR -> Update-DR
			BitBang[BI++] = TMS_H | TDI_H | TCK_L;
			BitBang[BI++] = TMS_H | TDI_H | TCK_H;
			//Update-DR -> Run-Test-Idle
			BitBang[BI++] = TMS_L | TDI_H | TCK_L;
			BitBang[BI++] = TMS_L | TDI_H | TCK_H;

			BitBang[0] = USB20_CMD_JTAG_BIT_OP;
			BitBang[1] = (UCHAR)BI - USB20_CMD_HEADER;
			BitBang[2] = 0; //长度高8位
		}
		break;
	case 4: //Run-Test/Idle -> Shift-IR
		BI = BuildPkt_EnterShiftIR(BitBang);
		break;	
	case 5: //Shift-IR -> Run-Test/Idle
		{
			BI = USB20_CMD_HEADER; //3 bytes header :1 byte command code +2 bytes subsequent length (small endian)
			//Shift-IR -> Exit1-IR
			BitBang[BI++] = TMS_H | TDI_H | TCK_L;
			BitBang[BI++] = TMS_H | TDI_H | TCK_H;
			//Exit1-IR -> Update-IR
			BitBang[BI++] = TMS_H | TDI_H | TCK_L;
			BitBang[BI++] = TMS_H | TDI_H | TCK_H;
			//Update-IR -> Run-Test-Idle
			BitBang[BI++] = TMS_L | TDI_H | TCK_L;
			BitBang[BI++] = TMS_L | TDI_H | TCK_H;

			BitBang[0] = USB20_CMD_JTAG_BIT_OP;
			BitBang[1] = (UCHAR)BI - USB20_CMD_HEADER;
			BitBang[2] = 0; //长度高8位
		}
		break;
	case 6:
		BI = BuildPkt_ExitShiftToRunIdle(BitBang);
		break;
	}
	TxLen = BI;
	//DumpBitBangData(BitBang,TxLen);
	RetVal = ( CH375WriteEndP(0,DataDnEndp,BitBang,&TxLen) && (TxLen==BI) );
	return RetVal;
}

//USB20JTAG initializes and sets the JTAG speed
BOOL	WINAPI	USB20Jtag_INIT(ULONG iIndex,
							   UCHAR iClockRate)    //Communication speed; The value ranges from 0 to 4. A larger value indicates a faster communication speed
{
	UCHAR mBuffer[256] = "";
	ULONG mLength ,i,DescBufSize;
	BOOL  RetVal = FALSE;
	UCHAR DescBuf[64] = "";
	PUSB_ENDPOINT_DESCRIPTOR       EndpDesc;
	PUSB_COMMON_DESCRIPTOR         UsbCommDesc;

	if( (iClockRate > 5) )
		goto Exit;

	//The default USB speed is 480MHz USB2.0 speed, or 12MHz USB full speed if connected to a full speed HUB.
	DescBufSize = sizeof(DescBuf);
	if( !CH375GetConfigDescr(iIndex,DescBuf,&DescBufSize))
		goto Exit;

	//Determine by the SIZE of the USB BULK endpoint. If the endpoint size is 512B, the speed is 480MHz USB2.0
	AfxUsbHighDev = FALSE;
	i = 0;
	while(i<DescBufSize)
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
					BulkInEndpMaxSize = EndpDesc->wMaxPacketSize;      //endpoint size
					AfxUsbHighDev = (EndpDesc->wMaxPacketSize == 512); //USB speed	
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
	{//Set the maximum data length of each command package based on the USB speed	
		if(AfxUsbHighDev)
			CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBHS; //507B
		else
			CMDPKT_DATA_MAX_BYTES  =  CMDPKT_DATA_MAX_BYTES_USBFS; //59B

		CMDPKT_DATA_MAX_BITS = CMDPKT_DATA_MAX_BYTES/16*16/2;  //The maximum number of bits to be transmitted by each command. Each byte must be a multiple of 2 bytes

		//According to the size of the hardware buffer to calculate the number of bits per batch transmission, multi-command packet
		MaxBitsPerBulk = HW_TDO_BUF_SIZE/CMDPKT_DATA_MAX_BYTES*CMDPKT_DATA_MAX_BITS;  
		//According to the size of the hardware buffer to calculate the number of words per batch transmission, multi-command packet
		MaxBytesPerBulk = HW_TDO_BUF_SIZE - (HW_TDO_BUF_SIZE+CMDPKT_DATA_MAX_BYTES-1)/CMDPKT_DATA_MAX_BYTES*3;;
	}

	//USB BULKIN upload data using the drive buffer upload mode, more efficient than direct upload
	CH375SetBufUploadEx(iIndex,TRUE,DataUpEndp,8192); //Indicates that the upload endpoint is in buffered upload mode and the buffer size is 8192

	USB20Jtag_ClearUpBuf(iIndex);//Clear the buffer data in the driver

	{//Build the USB JTAG initialization command package and execute it
		i = 0;
		mBuffer[i++] = USB20_CMD_JTAG_INIT;
		mBuffer[i++] = 6;  //The data length is 6
		mBuffer[i++] = 0;

		mBuffer[i++] = 0;  //保留字节
		mBuffer[i++] = iClockRate;	 //JTAG Clock
		i += 4; //保留字节
		mLength = i;
		if( !CH375WriteEndP(iIndex,DataDnEndp,mBuffer,&mLength) || (mLength!=i) )
			goto Exit;

		mLength = 4;
		//if( !CH375ReadEndP(iIndex,DataUpEndp,mBuffer,&mLength) || (mLength!=4) )
		if( !USB20Jtag_ReadData(iIndex,&mLength,mBuffer,10) || (mLength!=4) )
			goto Exit;
		RetVal = ( (mBuffer[0] == USB20_CMD_JTAG_INIT) && (mBuffer[USB20_CMD_HEADER]==0) );
	}
	USB20Jtag_SwitchTapState(0); //Reset Target
	USB20Jtag_SwitchTapState(1); //Run-Test/Idle
	DbgPrint("Tap state: Rest -> Run-Test/Idle");
Exit:
	return (RetVal);
}
