/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  FLASH application function, FLASH model identification, block read, block write, block erase, FLASH content read to file, file
Write FLASH, speed test and other operating functions.

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

#include "Main.h"

//全局变量
extern HWND AfxMainHwnd;     //主窗体句柄
extern HINSTANCE AfxMainIns; //进程实例
extern BOOL AfxDevIsOpened;  //设备是否打开
extern ULONG Flash_Sector_Count; /* FLASH芯片扇区数 */
extern USHORT Flash_Sector_Size; /* FLASH芯片扇区大小 */

//打开设备
BOOL OpenDevice()
{
	ULONG CI;

	//获取设备序号
	CI = SendDlgItemMessage(AfxMainHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(CI==CB_ERR)
	{
		DbgPrint("Failed to open the device. Please select the device first");
		goto Exit; //退出
	}	
	AfxDevIsOpened = USB20SPI_OpenDevice(CI);
	DbgPrint(">>Open...%s",AfxDevIsOpened?"Succ":"Failure");
Exit:
	return AfxDevIsOpened;
}

//关闭设备
BOOL CloseDevice()
{
	ULONG CI;				
	CI = SendDlgItemMessage(AfxMainHwnd,IDC_ObjList,CB_GETCURSEL,0,0);				

	DbgPrint(">>Close...");

	USB20SPI_CloseDevice(CI);
	AfxDevIsOpened = FALSE;
	DbgPrint(">>Succ");

	return TRUE;
}

//FLASH Model Identification
BOOL FlashIdentify()
{
	double BT,UsedT;
	BOOL RetVal;

	if(!AfxDevIsOpened)
	{
		DbgPrint("Please turn on the device first");
		return FALSE;
	}

	DbgPrint(">>FLASHChip type detection...");
	BT = GetCurrentTimerVal();
	RetVal = FLASH_IC_Check();
	UsedT = GetCurrentTimerVal()-BT;
	DbgPrint(">>FLASH Chip Model Testing... %s, available: %3fs", RetVal?"Success":"Failure",UsedT);

	return RetVal;
}

//FLASH Block Read
BOOL FlashBlockRead()
{
	double BT,UseT;
	ULONG DataLen,FlashAddr=0,i;
	UCHAR DBuf[8192] = {0};
	CHAR FmtStr[512] = "",FmtStr1[8*1024*3+16]="";

	if(!AfxDevIsOpened)
	{
		DbgPrint("First open device");
		return FALSE;
	}

	//Gets the start address that the FLASH reads
	GetDlgItemText(AfxMainHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//Gets the number of bytes read by FLASH, in hexadecimal
	GetDlgItemText(AfxMainHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);
	if(DataLen<1)
	{
		DbgPrint("Please enter the Flash data operation length");
		return FALSE;
	}
	else if(DataLen>(8*1024*3)) //显示限制
	{
		DbgPrint("Enter a Flash data operation length less than 0x%X",8*1024);
		return FALSE;
	}

	BT = GetCurrentTimerVal();
	DataLen = FLASH_RD_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;

	if(DataLen<1)
		DbgPrint(">>Flash read: read %d bytes from [%X] address... failure.",FlashAddr,DataLen);
	else
	{	
		DbgPrint(">>Flash read: read %d bytes from [%X] address... Succeeded. Time %.3fs",FlashAddr,DataLen,UseT/1000);
		{//Display FLASH data in hexadecimal format
			for(i=0;i<DataLen;i++)		
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",DBuf[i]);						
			SetDlgItemText(AfxMainHwnd,IDC_FlashData,FmtStr1);
		}
	}
	return TRUE;
}

//Flash Block Write
BOOL FlashBlockWrite()
{
	ULONG DataLen,FlashAddr=0,i,StrLen;
	UCHAR DBuf[8*1024+16] = {0};
	CHAR FmtStr[8*1024*3+16] = "",ValStr[16]="";
	double BT,UseT;

	//Erase
	SendDlgItemMessage(AfxMainHwnd,IDC_FlashErase,BM_CLICK,0,0);

	//Gets the start address for writing to the FLASH in hexadecimal format
	GetDlgItemText(AfxMainHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);				

	//Gets the number of bytes written to FLASH, in hexadecimal
	DataLen = 0;
	StrLen = GetDlgItemText(AfxMainHwnd,IDC_FlashData,FmtStr,sizeof(FmtStr));	
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);

		DBuf[DataLen] = (UCHAR)mStrToHEX(ValStr);
		DataLen++;
	}
	
	BT = GetCurrentTimerVal();
	DataLen = FLASH_WR_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;
	if(DataLen<1)
		DbgPrint(">>Flash write: write %d bytes from [%X] address... failure",FlashAddr,DataLen);
	else
		DbgPrint(">>Flash write: write %d bytes from [%X] address... Succeeded. Time %.3fs",FlashAddr,DataLen,UseT/1000);

	return TRUE;
}

BOOL FlashBlockErase()
{
	ULONG DataLen,FlashAddr=0;
	CHAR FmtStr[128] = "";
	double BT,UseT;
	BOOL RetVal;

	//Gets the starting address for erasing FLASH, in hexadecimal
	GetDlgItemText(AfxMainHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//Gets the number of bytes erased from FLASH, in hexadecimal
	GetDlgItemText(AfxMainHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);

	BT = GetCurrentTimerVal();
	RetVal = FLASH_Erase_Sector(FlashAddr);
	UseT = GetCurrentTimerVal()-BT;
	if( !RetVal )
		DbgPrint(">>FLASH erases: [X] %... failure",FlashAddr);
	else
		DbgPrint(">>FLASH erases: [X] %... Success, time %.3fs",FlashAddr,UseT/1000);

	return TRUE;
}

BOOL InitFlashSPI()
{	
	BOOL RetVal = FALSE;
	UCHAR iMode,iClock;

	iMode = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_SpiCfg_Mode,CB_GETCURSEL,0,0);
	iClock = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_SpiCfg_Clock,CB_GETCURSEL,0,0)+1;	

	RetVal = USB20SPI_Init(0,iMode,iClock,0);
	DbgPrint("USB20SPI_Init %s",RetVal?"succ":"failure");

	//FLASH型号识别
	FlashIdentify();

	return RetVal;
}			

//Display error content of test data
VOID DumpDataBuf(ULONG Addr,PUCHAR Buf1,PUCHAR SampBuf2,ULONG DataLen1,ULONG ErrLoca)
{
	CHAR FmtStr1[8192*3] = "",FmtStr2[8192*3] = "";
	ULONG i;

	memset(FmtStr1,0,sizeof(FmtStr1));	
	memset(FmtStr2,0,sizeof(FmtStr2));	
	//16进制显示
	for(i=0;i<DataLen1;i++)
	{
		if( ((i%16)==0) && (i>0) )
		{
			AddStrToEdit(AfxMainHwnd,IDC_InforShow,"Data[%08X]:%s\n",i-16+Addr,FmtStr1);
			AddStrToEdit(AfxMainHwnd,IDC_InforShow,"Samp[%08X]:%s\n",i-16+Addr,FmtStr2);
			memset(FmtStr1,0,16*4);
			memset(FmtStr2,0,16*4);
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}
		}
		else
		{
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}

		}
	}
	AddStrToEdit(AfxMainHwnd,IDC_InforShow,"Data[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr1);
	AddStrToEdit(AfxMainHwnd,IDC_InforShow,"Samp[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr2);
}

//FLASH test, write first then read
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG Timeout,PrintLen,ErrLoca;
	UCHAR temp;
	OPENFILENAME mOpenFile={0};
	
	EnableWindow(AfxMainHwnd,FALSE);

	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = AfxMainHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "Select the data file to be written to the Flash";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Write data to flash from:%s",FileName);
		//SetDlgItemText(AfxMainHwnd,IDC_RxLogFilePath,FileName);
		//SendDlgItemMessage(AfxMainHwnd,IDC_RxLogFilePath,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
	}
	else goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("The test file is invalid. Please re-select it");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//打开文件
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		DbgPrint("The test file %d bytes are too small, please select again",FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
		TestLen = Flash_Sector_Size * Flash_Sector_Count;
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		DbgPrint("Apply for test file memory%dB failure",FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		DbgPrint("Failed to request test read memory %dB",FileSize);
		goto Exit;
	}
	RLen = FileSize;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		DbgPrint("Failed to read test data from the file",FileSize);
		goto Exit;
	}
	DbgPrint("\r\n");
	DbgPrint("****Start speed test, test data length is%dB",FileSize);

	DbgPrint("*>>*1.FlashErasing speed test");
	TestLen = FileSize;
	Flash_Sector_Size = 4096; //BLOCK ERASE 32768
	TC = (TestLen+Flash_Sector_Size-1)/Flash_Sector_Size; //page	

	BT = GetCurrentTimerVal();
	/*
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(FlashAddr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	*/
	RetVal = FLASH_Erase_Full();
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*Erase all %s, average speed :%.2fKB/ s, total time%.3fS",
		RetVal?"Succ":"Failure",
		TestLen/UsedT/1000,UsedT);	
	
	DbgPrint("*>>*2.FlashBlock write speed test");
	//RetVal = W25XXX_WR_Page(FileBuf+Addr,Addr,RLen);
	Flash_Sector_Size = 0x100;
	TestLen = FileSize;	
	TC = (FileSize+Flash_Sector_Size-1)/Flash_Sector_Size;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*Flash_Sector_Size;
		if( (i+1)==TC )
			RLen = FileSize-i*Flash_Sector_Size;
		else
			RLen = Flash_Sector_Size;		
	
		RetVal = FLASH_WriteEnable();		
		if(!RetVal)		
		{
			DbgPrint("FLASH_WriteEnable failure.");
			break;
		}
		RBuf[0] = CMD_FLASH_BYTE_PROG;
		RBuf[1] = (UINT8)( Addr >> 16 );
		RBuf[2] = (UINT8)( Addr >> 8 );
		RBuf[3] = (UINT8)Addr&0xFF;
		RLen += 4;
		memcpy(&RBuf[4],FileBuf+Addr,RLen);
		RetVal = USB20SPI_Write(0,0x80,RLen,Flash_Sector_Size+4,RBuf);
		if(!RetVal)	
		{
			DbgPrint("Write FLASH[0x%X][0x%X] failure",Addr,RLen);
			break;
		}

		Timeout = 0;
		do//等待写结束
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT*20)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; 
				break; //退出写
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*Block write %d bytes %s. Average speed :%.3fKB/s, total time %.3fs ",TestLen,RetVal?" Success ":" failure",TestLen/UsedT/1000,UsedT);
	
	DbgPrint("*>>*3.Flash Block read speed test");
	TestLen = FileSize;

	//Send the read address first
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//A quick readSPI FLASH
	RLen = TestLen;//Once finished		

	BT = GetCurrentTimerVal();
	RetVal = USB20SPI_Read(0,0x80,WLen,&RLen,RBuf);
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if( !RetVal )
	{
		DbgPrint("Failed to read %dB data from FLASH start address",TestLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("The read data is incomplete(0x%X-0x%X)",RLen,TestLen);
	}	
	DbgPrint("*<<*Block read %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure",TestLen/UsedT/1000,UsedT);	

	if( !RetVal )
		goto Exit;

	DbgPrint("*>>*4.Flash Write and read content check");
	TestLen = FileSize;
	TC = (TestLen+8192-1)/8192;
	for(i=0;i<TC;i++)
	{
		Addr = i*8192;
		if( (i+1)==TC)
			UnitSize = FileSize-i*8192;
		else
			UnitSize = 8192;
		for(k=0;k<UnitSize;k++)
		{
			if(FileBuf[Addr+k]!=RBuf[Addr+k])
			{	
				if(((Addr+k)&0xFFF0+16)>FileSize)
					PrintLen = FileSize - ((Addr+k)&0xFFF0+16);
				else 
					PrintLen = 16;
				ErrLoca = (Addr+k)%16;
				DbgPrint("[%04X]:%02X-%02X:Data written and read does not match",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	DbgPrint("*<<*Check Flash write and read total %dB, all match",TestLen);
	DbgPrint("Test Result: P A S S");
Exit:
	//CH347SPI_ChangeCS(0,SPI_CS_DEACTIVE); //撤消CS片选
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	EnableWindow(AfxMainHwnd,TRUE);
	return RetVal;
}

DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr,TC,i,Timeout,temp;
	PUCHAR FileBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	UCHAR RBuf[4096] = "";
	
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = AfxMainHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "Select the data file to be written to the Flash";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Write data to flash from:%s",FileName);
	}
	else
		goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("WriteFlashFromFile.Open save file");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		DbgPrint("WriteFlashFromFile.Write flash data file is empty, please select again");
		goto Exit;
	}
	if( TestLen > (Flash_Sector_Size * Flash_Sector_Count) ) //Flash容量
		TestLen = Flash_Sector_Size * Flash_Sector_Count;

	DbgPrint("*>>*WriteFlashFromFile.Flash写%d字节",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		DbgPrint("WriteFlashFromFile.Memory allocation failed.");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("WriteFlashFromFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("WriteFlashFromFile.ReadFlashFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}
	DbgPrint("*>>*1.WriteFlashFromFile.erase");
	Flash_Sector_Size = 4096; //BLOCK ERASE 32768
	Addr=0;
	TC = (TestLen+Flash_Sector_Size-1)/Flash_Sector_Size; //page	
	BT = GetCurrentTimerVal();
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(Addr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*WriteFlashFromFile. Erase %d block (%dB) %s, average speed :%.2fKB/s, total time %.3fs",
		TC,TC*32768,RetVal?"Succ":"Failure",
		TestLen/UsedT/1000,UsedT);
	
	DbgPrint("*>>*2.WriteFlashFromFile.Flash Block Write");
	//RetVal = W25XXX_WR_Page(FileBuf+Addr,Addr,RLen);
	Flash_Sector_Size = 0x100;
	TC = (TestLen+Flash_Sector_Size-1)/Flash_Sector_Size;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*Flash_Sector_Size;
		if( (i+1)==TC )
			RLen = TestLen-i*Flash_Sector_Size;
		else
			RLen = Flash_Sector_Size;		
	
		RetVal = FLASH_WriteEnable();		
		if(RetVal)		
		{			
			RBuf[0] = CMD_FLASH_BYTE_PROG;
			RBuf[1] = (UINT8)( Addr >> 16 );
			RBuf[2] = (UINT8)( Addr >> 8 );
			RBuf[3] = (UINT8)Addr;
			RLen += 4;			
			memcpy(&RBuf[4],FileBuf+Addr,RLen);
			RetVal = USB20SPI_Write(0,0x80,RLen,Flash_Sector_Size+4,RBuf);
			if(!RetVal)	DbgPrint("WriteFlashFromFile.Write FLASH[0x%X][0x%X]failure",Addr,RLen);
		}
		else break;
		Timeout = 0;
		do
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; break; //退出写
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*WriteFlashFromFile.Block write %d bytes %s. Average speed :%.3fKB/s, total time %.3fs ",TestLen,RetVal?" Success ":" failure",TestLen/UsedT/1000,UsedT);

Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG PrintLen,ErrLoca;
	OPENFILENAME mOpenFile={0};

	DbgPrint("*>>*Start validating FLASH content ");
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = AfxMainHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "Select the data file to be written to the Flash";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Verify flash with file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("Invalid file, please select again");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//打开文件
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		DbgPrint("The file %d bytes used for comparison are too small, please select again",FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
	{
		TestLen = Flash_Sector_Size * Flash_Sector_Count;		
	}
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		DbgPrint("Failed to apply for file memory %dB.",FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		DbgPrint("Failed to apply for test memory %dB",FileSize);
		goto Exit;
	}
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		DbgPrint("Failed to read data from file",FileSize);
		goto Exit;
	}

	//先发送读地址
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//快速读取SPI FLASH
	RLen = TestLen;//一次读完		

	BT = GetCurrentTimerVal();
	RetVal = USB20SPI_Read(0,0x80,WLen,&RLen,RBuf);//读	
	if( !RetVal )
	{
		DbgPrint("Failed to read %dB data from FLASH start address",TestLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("Incomplete read data (0x%X-0x%X)",RLen,TestLen);
	}	

	if( !RetVal )
		goto Exit;

	TestLen = FileSize;
	TC = (TestLen+8192-1)/8192;
	for(i=0;i<TC;i++)
	{
		Addr = i*8192;
		if( (i+1)==TC)
			UnitSize = FileSize-i*8192;
		else
			UnitSize = 8192;
		for(k=0;k<UnitSize;k++)
		{
			if(FileBuf[Addr+k]!=RBuf[Addr+k])
			{	
				if(((Addr+k)&0xFFF0+16)>FileSize)
					PrintLen = FileSize - ((Addr+k)&0xFFF0+16);
				else 
					PrintLen = 16;
				ErrLoca = (Addr+k)%16;
				DbgPrint("[%04X]:%02X-%02X: the data read and write do not match",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	DbgPrint("*<<*Verify Flash content %dB, match file content, average speed :%.3fKB/S, total time %.3fs",TestLen,TestLen/UsedT/1000,UsedT);
Exit:
	//CH347SPI_ChangeCS(0,SPI_CS_DEACTIVE); //撤消CS片选
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	DbgPrint("*>>*FLASH verify %s ", RetVal?" Success ":" failure ");
	DbgPrint("\r\n");	

	return RetVal;
}

DWORD WINAPI ReadFlashToFile(LPVOID lpParameter)
{
	// 获取将要发送的文件名
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr;
	PUCHAR RBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;

	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = AfxMainHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "Select Flash data save file";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetSaveFileName(&mOpenFile))
	{ 	
		DbgPrint("FlashData will save to:%s",FileName);
	}
	else
		goto Exit;

	TestLen = Flash_Sector_Size * Flash_Sector_Count; //Flash容量
	DbgPrint("*>>*ReadFlashToFile.Flash Read %d bytes to file",TestLen);

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("ReadFlashToFile.Open the file");
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(TestLen+64);
	if(RBuf==NULL)
	{
		DbgPrint("ReadFlashToFile.Memory allocation failed. Procedure");
		goto Exit;
	}	
	//if(RetVal)
	{   //发送读地址
		Addr = 0;		
		RBuf[0] = CMD_FLASH_READ;
		RBuf[1] = (UINT8)( Addr >> 16 );
		RBuf[2] = (UINT8)( Addr >> 8 );
		RBuf[3] = (UINT8)( Addr );

		BT = GetCurrentTimerVal();
		//快速读取SPI FLASH
		RLen = TestLen;//待读长度
		RetVal = USB20SPI_Read(0,0x80,4,&RLen,RBuf);
		if( !RetVal )
		{
			DbgPrint("ReadFlashToFile.Failed to read %dB data from FLASH start address",RLen);
		}
		if(RLen != TestLen)
		{
			DbgPrint("ReadFlashToFile.The read data is incomplete(0x%X-0x%X)",RLen,TestLen);
		}
		UsedT = (GetCurrentTimerVal()-BT)/1000;
		DbgPrint("*<<*ReadFlashToFile.Block read %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure",TestLen/UsedT/1000,UsedT);			
		if( !WriteFile(hFile,RBuf,RLen,&RLen,NULL) )
		{
			ShowLastError("ReadFlashToFile.Data write file:");
			goto Exit;
		}
		if(RLen!=TestLen)
		{
			DbgPrint("ReadFlashToFile Incomplete write");
			goto Exit;
		}
	}	
	RetVal = TRUE;
Exit:
	if(RBuf!=NULL)
		free(RBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return RetVal;
}


