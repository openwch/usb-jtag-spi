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

#include "Main.h"
//#include "SPI_FLASH.H"

#define _CRT_SECURE_NO_WARNINGS

ULONG CMDPKT_DATA_MAX_BYTES = CMDPKT_DATA_MAX_BYTES_USBHS; //USB PKT consists of command code (1B)+ length (2B)+ data
UCHAR  DataUpEndp=0,DataDnEndp=0;
USHORT BulkInEndpMaxSize=512,BulkOutEndpMaxSize=512;
ULONG  AfxCsIndex = 0; //CS1/2
HANDLE AfxDevH[8] = {INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};
extern BOOL AfxSpiIsCfg;

//获取SPI硬件配置信息
BOOL    WINAPI  USB20SPI_GetHwCfg(ULONG iIndex,         // Specify device number
		                          SpiHwCfgS *SpiCfg)
{
	UCHAR	mBuffer[ mCH341_PACKET_LENGTH ]="";
	ULONG	mLength,i,RetLen;
	BOOL    RetVal = FALSE;

	i =0;
	mBuffer[i++] = USB20_CMD_INFO_RD;
	mBuffer[i++] = 1; //Subsequent length, small end mode
	mBuffer[i++] = 0; 
	mBuffer[i++] = 1;
	mLength = i;
	if( !CH375WriteEndP(iIndex,DataDnEndp, mBuffer, &mLength ) ) 
		goto Exit;
	mLength = RetLen = 26+3; //Agreed length
	if ( !CH375ReadEndP(iIndex,DataUpEndp, mBuffer, &mLength ) || ( mLength != RetLen ) ) 
		goto Exit;
	if(SpiCfg)
		memcpy(SpiCfg,&mBuffer[3],RetLen-3);

	RetVal = TRUE;
Exit:
	return RetVal;
}

//Get USB device information, whether it is 480M high speed and USB communication endpoint address and size
BOOL   WINAPI   USB20SPI_GetUsbDevInfor(ULONG iIndex)
{
	UCHAR	mBuffer[ mCH341_PACKET_LENGTH ]="",mRetBuf[16]="";
	ULONG	i,DescBufSize;
	SpiHwCfgS HwCfg = {0};
	UCHAR DescBuf[64] = "";
	PUSB_ENDPOINT_DESCRIPTOR       EndpDesc;
	PUSB_COMMON_DESCRIPTOR         UsbCommDesc;
	BOOL IsUsbHighDev;

	DescBufSize = sizeof(DescBuf);
	if( !CH375GetConfigDescr(iIndex,DescBuf,&DescBufSize))
		return FALSE;

	IsUsbHighDev = FALSE;
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
					BulkInEndpMaxSize = EndpDesc->wMaxPacketSize;
					IsUsbHighDev = (EndpDesc->wMaxPacketSize == 512);					
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

	if( iIndex >= 16 )
		return FALSE;
	if(IsUsbHighDev==1)
		CMDPKT_DATA_MAX_BYTES = CMDPKT_DATA_MAX_BYTES_USBHS;
	else
		CMDPKT_DATA_MAX_BYTES = CMDPKT_DATA_MAX_BYTES_USBFS;

	DbgPrint("USB Speed:%s,DataDnEndp:%0d(%d),DataUpEndp:%0d(%d),",IsUsbHighDev?"480M HS":"12M FS",DataDnEndp,BulkOutEndpMaxSize,DataUpEndp,BulkInEndpMaxSize);

	return TRUE;
}

// The SPI controller is initialized
BOOL	WINAPI	USB20SPI_Init(ULONG iIndex,
							  UCHAR iMode,                 // 0-3:SPI Mode0/1/2/3
							  UCHAR iClock,                // 1=36MHz, 2=18MHz, 3=9MHz, 4=4.5MHz, 5=2.25MHz, 6=1.125MHz, 7=562.5KHz)
							  UCHAR CsIndex)
{
	UCHAR	mBuffer[ 128 ]="",mRetBuf[16]="";
	ULONG	mLength,RetLen,i;
	SpiHwCfgS HwCfg = {0};

	CH375SetTimeout(iIndex,5000,2000);   //Setting read/write Timeout
	USB20SPI_GetUsbDevInfor(iIndex);     //Obtain USB device communication endpoint information
	USB20SPI_GetHwCfg(iIndex,&HwCfg);    //Obtain the existing configuration value of the device

	//设置SPIiMode
	switch(iMode){ //iMode0-3:SPI Mode0/1/2/3
	case 0: //mode0:CPOL=0, CPHA=0 
		HwCfg.SPIInitCfg.SPI_CPHA = SPI_CPHA_1Edge;  HwCfg.SPIInitCfg.SPI_CPOL = SPI_CPOL_Low;  break;
	case 1: //mode1:CPOL=0, CPHA=1
		HwCfg.SPIInitCfg.SPI_CPHA = SPI_CPHA_2Edge;  HwCfg.SPIInitCfg.SPI_CPOL = SPI_CPOL_Low;  break;
	case 2: //mode2:CPOL=1, CPHA=0 
		HwCfg.SPIInitCfg.SPI_CPHA = SPI_CPHA_1Edge;  HwCfg.SPIInitCfg.SPI_CPOL =SPI_CPOL_High;  break;		
	case 3: //mode3:CPOL=1, CPHA=1
		HwCfg.SPIInitCfg.SPI_CPHA = SPI_CPHA_2Edge;  HwCfg.SPIInitCfg.SPI_CPOL =SPI_CPOL_High;  break;		
	default:  //default=mode3
		HwCfg.SPIInitCfg.SPI_CPHA = SPI_CPHA_2Edge;  HwCfg.SPIInitCfg.SPI_CPOL =SPI_CPOL_High;  break;
	}
	//设置SPI iClock
	HwCfg.SPIInitCfg.SPI_BaudRatePrescaler = iClock * 8;
	if(CsIndex>1)//It's just CS1 and CS2
	{
		DbgPrint("Invalid CS pin selection");
		return FALSE;
	}
	AfxCsIndex = CsIndex;
	
	//C0 initializes the SPI controller command package
	i = 0;
	mBuffer[i++] = USB20_CMD_SPI_INIT;
	mLength = sizeof(SpiHwCfgS);
	mBuffer[i++] = mLength&0xFF;
	mBuffer[i++] = (mLength>>8)&0xFF;
	memcpy(&mBuffer[i],&HwCfg,mLength);	
	mLength += i;
	if( CH375WriteEndP(iIndex,DataDnEndp,mBuffer,&mLength) )
	{// Write out blocks of data
		mLength = RetLen = USB20_CMD_HEADER + 1;
		if( CH375ReadEndP(iIndex,DataUpEndp,mRetBuf,&mLength) )
		{
			if ( RetLen != RetLen )
				return( FALSE );
			if(mRetBuf[USB20_CMD_HEADER]==0) //Set up the success
			{
				AfxSpiIsCfg = TRUE;				
			}
		}
	}
	return( AfxSpiIsCfg );
}

//USB20SPI_Init is used to set the CS state
BOOL	WINAPI	USB20SPI_ChangeCS(ULONG			iIndex,         // Specify device number
								  UCHAR         iStatus)        // 0= undo selection,1= set selection
{
	UCHAR	mBuffer[ mCH341_PACKET_LENGTH ]="";
	ULONG	mLength,i,RetLen;

	memset(mBuffer,0,sizeof(mBuffer));
	i=0;
	mBuffer[i++] = USB20_CMD_SPI_CONTROL;
	i += 2; //Two bytes long last assignment

	//CS1设置
	if(AfxCsIndex==0)
		mBuffer[i] |= 0x80;  //Enable CS1	
	else
		mBuffer[i] |= 0x00;  //Ignore the CS1	
	if((iStatus&0x01)==SPI_CS_ACTIVE) 
		mBuffer[i] &= 0xBF;  //active CS1
	else
		mBuffer[i] |= 0x40;  //Deactive CS1
	//mBuffer[i] |= 0x20;  //Automatically undo slice selection when the operation is complete
	//mBuffer[i] &= 0xDF;  //Manually undo slice selection
	i++;
	mBuffer[i++] = 0;//(UCHAR)(iActiveDelay&0xFF);         //Set the latency for read/write operations after chip selection to 8 bits lower
	mBuffer[i++] = 0;//(UCHAR)((iActiveDelay>>8)&0xFF);    //Set the latency for read/write operations after chip selection. The value is 8 bits high
	mBuffer[i++] = 0;//(UCHAR)(iDelayDeactive&0xFF);       //Delay time for read and write operations after slice selection is unselected, 8 bits lower
	mBuffer[i++] = 0;//(UCHAR)((iDelayDeactive>>8)&0xFF);  //Delay time for read and write operations after slice selection is unselected, up to 8 bits
	
	//CS2设置
	if(AfxCsIndex==1)
		mBuffer[i] |= 0x80;  //Enable CS2
	else
		mBuffer[i] |= 0x00;  //Ignore CS2
	if((iStatus&0x01)==SPI_CS_ACTIVE) 
		mBuffer[i] &= 0xBF;  //Active CS2
	else
		mBuffer[i] |= 0x40;  //Deactive CS2	
	//mBuffer[i] |= 0x20;  //Automatically undo slice selection when the operation is complete
	//mBuffer[i] &= 0xDF;  //Manually undo slice selection
	i++;
	mBuffer[i++] = 0;//(UCHAR)(iActiveDelay&0xFF);         //Set the latency for read/write operations after chip selection to 8 bits lower
	mBuffer[i++] = 0;//(UCHAR)((iActiveDelay>>8)&0xFF);    //Set the latency for read/write operations after chip selection. The value is 8 bits high
	mBuffer[i++] = 0;//(UCHAR)(iDelayDeactive&0xFF);       //Delay time for read and write operations after slice selection is unselected, 8 bits lower
	mBuffer[i++] = 0;//(UCHAR)((iDelayDeactive>>8)&0xFF);  //Delay time for read and write operations after slice selection is unselected, up to 8 bits

	mBuffer[1] = (i-3)&0xFF;
	mBuffer[2] = ((i-3)>>8)&0xFF;
	
	RetLen = mLength = i;
	return ( CH375WriteEndP(iIndex,DataDnEndp, mBuffer, &mLength ) || (RetLen !=mLength) );
}

// Handle SPI data stream,4 wire interface
BOOL	WINAPI	USB20SPI_WriteRead(  
	ULONG			iIndex,       // Specify device number
	ULONG			iChipSelect,  // Chip selection control, when bit 7 is 0, slice selection control is ignored, and when bit 7 is 1, chip selection is operated
	ULONG			iLength,      // Number of bytes of data to be transferred
	PVOID			ioBuffer )    // Points to a buffer, places data to be written from MOSI, and returns data to be read from MISO
{
	UCHAR			mBuffer[ mDEFAULT_COMMAND_LEN + mDEFAULT_COMMAND_LEN / 8 ]="";
	ULONG			i, mLength;
	PUCHAR			mWrBuf,pBuf;
	ULONG           DLen,PI;
	BOOL            RetVal = FALSE;

	if ( iLength > mMAX_BUFFER_LENGTH ) return( FALSE );
	if ( iLength <= mDEFAULT_BUFFER_LEN/2 ) mWrBuf = (PUCHAR)mBuffer;  // 不超过默认缓冲区长度
	else {  // If more than that, additional memory needs to be allocated
		mWrBuf = (PUCHAR)LocalAlloc( LMEM_FIXED, ( mMAX_COMMAND_LENGTH + mMAX_COMMAND_LENGTH / 8 ) * 2 );  // 分配内存
		memset(mWrBuf,0,( mMAX_COMMAND_LENGTH + mMAX_COMMAND_LENGTH / 8 ) * 2);
		if ( mWrBuf == NULL ) return( FALSE );  // 分配内存失败
	}	
	if ( iLength < 1 ) goto Exit;

	PI = 0;
	while(PI<iLength)
	{
		if( (PI+CMDPKT_DATA_MAX_BYTES) > iLength)
			DLen = iLength - PI;
		else
			DLen = CMDPKT_DATA_MAX_BYTES;
		pBuf = (PUCHAR)ioBuffer+PI; 		
		if( !USB20SPI_ChangeCS(iIndex,SPI_CS_ACTIVE) )    goto Exit;
		i = 0;
		mWrBuf[i++] = USB20_CMD_SPI_RD_WR;
		mWrBuf[i++] = (UCHAR)(DLen&0xFF);
		mWrBuf[i++] = (UCHAR)((DLen>>8)&0xFF);
		memcpy(&mWrBuf[i],pBuf,DLen);
		i += DLen;

		mLength = i;
		if( !CH375WriteEndP(iIndex,DataDnEndp,mWrBuf,&mLength) || (mLength!=i) )
			goto Exit;

		memset(mWrBuf,0,mLength);
		if(!CH375ReadEndP(iIndex,DataUpEndp,mWrBuf,&mLength) )
			goto Exit;
		if( mWrBuf[0] != USB20_CMD_SPI_RD_WR )  
			goto Exit;
		if(mLength != i)
			goto Exit;		
		DLen = ((PUCHAR)mWrBuf)[1] + (((PUCHAR)mWrBuf)[2]<<8); //Subsequent data length		
		memcpy(pBuf,(PCHAR)mWrBuf+USB20_CMD_HEADER,DLen);
		PI += DLen;
	}
	RetVal = TRUE;
Exit:
	USB20SPI_ChangeCS(iIndex,SPI_CS_DEACTIVE); 
	if ( mWrBuf != mBuffer ) LocalFree( mWrBuf );

	return( RetVal );
}

//SPI4 reads data. No need to write data first, efficiency is much higher than USB20SPI_WriteRead
BOOL	WINAPI	USB20SPI_Read(
	ULONG			iIndex,           // Specify device number	
	ULONG			iChipSelect,      // Chip selection control, when bit 7 is 0, slice selection control is ignored, and when bit 7 is 1, chip selection is operated
	ULONG           oLength,          // Number of bytes to send
	PULONG			iLength,          // Number of bytes of data to be read in	
	PVOID			ioBuffer)         // Points to a buffer, places data to be written from MOSI, and returns data to be read from MISO
{
	UCHAR			mBuffer[8192] = "";
	ULONG			i, mLength,RI;	
	PUCHAR			mWrBuf,pBuf;
	ULONG			iReadStep;        // The length of a single block to be read. The total length to be read is (iReadStep*iReadTimes).
	ULONG			iReadTimes;       // Number of times you are ready to read
	BOOL            RetVal = FALSE;

	mWrBuf = (PUCHAR)mBuffer;  // Does not exceed the default buffer length

	if( !USB20SPI_ChangeCS(iIndex,SPI_CS_ACTIVE) )    goto Exit;

	if(oLength)//write,read
	{
		i=0;

		mWrBuf[i++] = USB20_CMD_SPI_RD_WR;
		mWrBuf[i++] = (UCHAR)(oLength&0xFF);
		mWrBuf[i++] = (UCHAR)((oLength>>8)&0xFF);
		mLength = oLength;	
		
		memcpy(&mWrBuf[i],ioBuffer,oLength);
		mLength = i + oLength;
		if(!CH375WriteEndP(iIndex,DataDnEndp,mWrBuf,&mLength)) 
			goto Exit;
		if(mLength!=(i + oLength)) 
			goto Exit;

		mLength = i + oLength;//512; //The read length is the write length
		if(!CH375ReadEndP(iIndex,DataUpEndp,mWrBuf,&mLength))
			goto Exit;
		if( mWrBuf[0] != USB20_CMD_SPI_RD_WR ) 
			goto Exit;
		if(mLength != (i + oLength) ) 
			goto Exit;		
		
	}
	i = 0;
	if(*iLength < 1) //No data to read
	{
		RetVal = TRUE;
		goto Exit;
	}	
	mWrBuf[i++] = USB20_CMD_SPI_BLCK_RD;
	mWrBuf[i++] = 0x04;
	mWrBuf[i++] = 0x00;
	mWrBuf[i++] = (UCHAR)( (*iLength>>0)&0xFF);
	mWrBuf[i++] = (UCHAR)( (*iLength>>8)&0xFF);
	mWrBuf[i++] = (UCHAR)( (*iLength>>16)&0xFF);
	mWrBuf[i++] = (UCHAR)( (*iLength>>24)&0xFF);;
	
	mLength = i;
	if( !CH375WriteEndP(iIndex,DataDnEndp,mWrBuf,&mLength) )
		goto Exit;
	if(mLength != i)
		goto Exit;
	iReadStep = CMDPKT_DATA_MAX_BYTES; //Full speed and high speed
	RI = 0;
	iReadTimes = (*iLength+CMDPKT_DATA_MAX_BYTES-1)/CMDPKT_DATA_MAX_BYTES;
	for(i=0;i<iReadTimes;i++)
	{
		mLength = CMDPKT_DATA_MAX_BYTES+5;//512;
		memset(mWrBuf,0xFF,mLength+5);
		pBuf = (PUCHAR)ioBuffer+RI; 
		if( !CH375ReadEndP(iIndex,DataUpEndp,mWrBuf,&mLength) ) 
			goto Exit;
		if(mLength < USB20_CMD_HEADER)
			goto Exit;
		if(mWrBuf[0]!=USB20_CMD_SPI_BLCK_RD) 
			goto Exit;
		mLength = (mWrBuf[1]&0xFF) + ((mWrBuf[2]<<8)&0xFF00);		
		memcpy(pBuf,mWrBuf+USB20_CMD_HEADER,mLength);
		RI += mLength;
	}
	RetVal = TRUE;

Exit:
	USB20SPI_ChangeCS(iIndex,SPI_CS_DEACTIVE); 
	if ( mWrBuf != mBuffer ) LocalFree( mWrBuf ); 
	*iLength = RI;
	return RetVal;
}

//SPI4写数据
BOOL	WINAPI	USB20SPI_Write(
	ULONG			iIndex,          // Specify device number	
	ULONG			iChipSelect,     // Chip selection control, when bit 7 is 0, chip selection control is ignored, and when bit 7 is 1, chip selection operation is performed
	ULONG			iLength,         // Number of bytes of data to be transferred	
	ULONG			iWriteStep,      // The length of a single block to be read
	PVOID			ioBuffer)        // Point to a buffer to place the data to be written out from MOSI
{
	UCHAR			mBuffer[8192]="";
	ULONG			i, mLength,DLen,WI;
	PUCHAR			mWrBuf,pBuf;
	BOOL            RetVal = FALSE;
	
	if ( iWriteStep > mMAX_BUFFER_LENGTH ) 
		goto Exit;

	mWrBuf = (PUCHAR)mBuffer;
	i=0;
	//启动片选L	
	if( !USB20SPI_ChangeCS(iIndex,SPI_CS_ACTIVE) )  
		goto Exit;

	WI = 0;
	while(WI < iLength)
	{
		if( (WI+iWriteStep) > iLength )
			DLen = iLength - WI;
		else
			DLen = iWriteStep;
		
		pBuf = (PUCHAR)ioBuffer+WI; //temp
		i = 0;

		mWrBuf[i++] = USB20_CMD_SPI_BLCK_WR;
		mWrBuf[i++] = (UCHAR)( (DLen>>0)&0xFF);
		mWrBuf[i++] = (UCHAR)( (DLen>>8)&0xFF);		
		memcpy(&mWrBuf[i],pBuf,DLen);
		i += DLen;
		mLength = i;

		if( !CH375WriteEndP(iIndex,DataDnEndp,mWrBuf,&mLength) ) 
			goto Exit;
		if(mLength != i) 
			goto Exit;

		mLength = 64;
		if( !CH375ReadEndP(iIndex,DataUpEndp,mWrBuf,&mLength ) ) 
			goto Exit;
		WI += DLen;
	}	
	RetVal = TRUE;
Exit:	
	USB20SPI_ChangeCS(iIndex,SPI_CS_DEACTIVE);
	if ( mWrBuf != mBuffer ) LocalFree( mWrBuf );
	return RetVal;
}


//Open device
BOOL USB20SPI_OpenDevice(ULONG DevI)
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

//Close device
BOOL USB20SPI_CloseDevice(ULONG DevI)
{
	if( AfxDevH[DevI] != INVALID_HANDLE_VALUE )
	{
		CH375CloseDevice(DevI);
		AfxDevH[DevI] = INVALID_HANDLE_VALUE;
	}
	return TRUE;
}