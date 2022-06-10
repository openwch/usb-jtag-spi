/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  USB TO JTAG debug Funciton
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

#include "Main.H"
#include "Usb20Jtag.h"
extern UCHAR AfxDataTransType;
extern HWND AfxMainHwnd; //主窗体句柄
extern HINSTANCE AfxMainIns; //进程实例
extern BOOL AfxDevIsOpened;  //设备是否打开
extern BOOL AfxJtagIsCfg;

/* Brings target into the Debug mode and get device id */
VOID Jtag_InitTarget (VOID)
{
	ULONG DevID = 0xFFFFFFFF,Len;
	BOOL  RetVal;
	
	USB20Jtag_SwitchTapState(0); //复位 
	//After the reset, the target DEVICE is switched to DEVICE ID DR.
	Len = 4; //4字节	
	RetVal = USB20Jtag_ByteReadDR(0,&Len,(PUCHAR)&DevID);
	DbgPrint("USB20Jtag_ByteWriteDR %s,read jtag ID: %08X\r\n",RetVal?"succ":"failure",DevID);

	Len = 4*8; //32位
	RetVal = USB20Jtag_BitReadDR(0,&Len,(PUCHAR)&DevID);
	DbgPrint("USB20Jtag_BitWriteDR  %s,read jtag ID: %08X\r\n",RetVal?"succ":"failure",DevID);
}

//JTAT IR/DR data read and write. Read and write by byte. State machine from run-test-idle->; IR/DR-&gt; Exit IR/DR -&gt; Run-Test-Idle
BOOL Jtag_DataRW_Byte(BOOL IsRead)
{
	BOOL RetVal = FALSE;
	BOOL IsDr;
	ULONG StrLen,i,OutLen=0,DLen=0,InLen=0;
	CHAR ValStr[64]="";
	PCHAR FmtStr=NULL;
	PUCHAR OutBuf=NULL,InBuf=NULL;
	double UsedT = 0,BT = 0;

	IsDr = (SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);

	DbgPrint("**>>**Jtag_DataRW_Byte");
	if(!AfxDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		goto Exit;
	}
	if(IsRead) //JTAG read, apply for read cache
	{
		InLen = GetDlgItemInt(AfxMainHwnd,IDC_JtagInLen,NULL,FALSE);	
		if(InLen<1)
		{
			DbgPrint("No read length is specified");
			goto Exit;
		}		
		InBuf = (PUCHAR)malloc(InLen+16);
		if(InBuf==NULL)
		{
			DbgPrint("Failed to apply for the read cache. Procedure");
			goto Exit;
		}
	}
	else //JTAG write, get write data length
	{	
		StrLen = GetWindowTextLength(GetDlgItem(AfxMainHwnd,IDC_JtagOut))+1;
		if(StrLen<2)
		{
			DbgPrint("No write data is entered");
			goto Exit;
		}
		OutLen = (StrLen+2)/3; //Three characters make a byte

		OutBuf = (PUCHAR)malloc(OutLen+16);
		if(OutBuf==NULL)
		{
			DbgPrint("Failed to apply for write cache. Procedure");
			goto Exit;
		}
	}		
	{//申请字符转数据缓存
		if(!IsRead)
		{
			FmtStr = (PCHAR)malloc(OutLen*3+16);
			memset(FmtStr,0,OutLen*3+16);
		}
		else
		{
			FmtStr = (PCHAR)malloc(InLen*3+16);
			memset(FmtStr,0,InLen*3+16);
		}
		if(FmtStr==NULL)
		{
			DbgPrint("Failed to apply for data conversion cache");
			goto Exit;
		}
	}
	if(!IsRead)//Obtain data to be written from the interface. Converts hexadecimal characters to hexadecimal data
	{
		StrLen = GetDlgItemText(AfxMainHwnd,IDC_JtagOut,FmtStr,OutLen*3+1);
		if((StrLen%3)==1)
		{
			FmtStr[StrLen] = '0';
			FmtStr[StrLen+1] = ' ';
			StrLen += 2;
		}
		else if((StrLen%3)==2)
		{
			FmtStr[StrLen] = ' ';
			StrLen += 1;
		}
		SetDlgItemText(AfxMainHwnd,IDC_JtagOut,FmtStr);		
		StrLen = GetDlgItemText(AfxMainHwnd,IDC_JtagOut,FmtStr,StrLen+2);
		OutLen = 0;
		for(i=0;i<StrLen;i+=3)
		{		
			memcpy(&ValStr[0],&FmtStr[i],2);
			OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
			OutLen++;
		}
		SetDlgItemInt(AfxMainHwnd,IDC_JtagOutLen,OutLen,FALSE);
	}
	
	BT = GetCurrentTimerVal();
	if(IsRead)
	{		
		if(IsDr)
			RetVal = USB20Jtag_ByteReadDR(0,&InLen,InBuf);
		else
			RetVal = USB20Jtag_ByteReadIR(0,&InLen,InBuf);
	}
	else 
	{
		if(IsDr)
			RetVal = USB20Jtag_ByteWriteDR(0,OutLen,OutBuf);
		else
			RetVal = USB20Jtag_ByteWriteIR(0,OutLen,OutBuf);
	}
	UsedT = GetCurrentTimerVal()-BT;	

	if(InLen)
	{//打印
		//Hexadecimal display
		for(i=0;i<InLen;i++)
		{
			sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);	
		}
		SetDlgItemText(AfxMainHwnd,IDC_JtagIn,FmtStr);		
		SetDlgItemInt(AfxMainHwnd,IDC_JtagInLen,InLen,FALSE);		
	}
	if(IsRead)
	{		
		DbgPrint("**<<**Jtag_Data %s %s read %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",InLen,InLen,UsedT,InLen/UsedT);
	}
	else
	{
		DbgPrint("**<<**Jtag_Data %s %s,write %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",OutLen,OutLen,UsedT,OutLen/UsedT);
	}

Exit:
	if(InBuf)
		free(InBuf);
	if(OutBuf)
		free(OutBuf);
	if(FmtStr)
		free(FmtStr);
	return RetVal;;
}

//JTAT IR/DR data read and write. Read and write by bit. State machine from run-test-idle->; IR/DR-&gt; Exit IR/DR -&gt; Run-Test-Idle
BOOL Jtag_DataRW_Bit(BOOL IsRead)
{
	BOOL RetVal = FALSE;
	BOOL IsDr;
	ULONG i,OutBitLen=0,DLen=0,InBitLen=0,BitCount,BI;
	CHAR ValStr[64]="";
	CHAR FmtStr[4096]="";
	UCHAR OutBuf[4096]="",InBuf[4096]="";	
	double UsedT = 0,BT = 0;

	IsDr = (SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);	

	DbgPrint("**>>**Jtag_DataRW_Bit");
	if(!AfxDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		goto Exit;
	}
	if(IsRead) //JTAG read
	{
		InBitLen = GetDlgItemInt(AfxMainHwnd,IDC_JtagInBitLen,NULL,FALSE);	
		if(InBitLen<1)
		{
			DbgPrint("No read length is specified");
			goto Exit;
		}
	}
	else //JTAG write
	{		
		BitCount = GetDlgItemText(AfxMainHwnd,IDC_JtagOutBit,FmtStr,sizeof(FmtStr));
		OutBitLen = GetDlgItemInt(AfxMainHwnd,IDC_JtagOutBitLen,NULL,FALSE);	

		if( (BitCount<1) && (OutBitLen<1) )
		{
			DbgPrint("No write data is entered");
			goto Exit;
		}
		if(BitCount<OutBitLen) //If the actual input digit is insufficient, 0 will be added
		{
			memmove(&FmtStr[OutBitLen-BitCount],&FmtStr[0],BitCount);
			memset(&FmtStr[0],'0',OutBitLen-BitCount);
			SetDlgItemText(AfxMainHwnd,IDC_JtagOutBit,FmtStr);			
		}
		BitCount = GetDlgItemText(AfxMainHwnd,IDC_JtagOutBit,FmtStr,sizeof(FmtStr));
		if(BitCount>OutBitLen)
		{
			memmove(&FmtStr[0],&FmtStr[BitCount-OutBitLen],OutBitLen);
			BitCount = OutBitLen;
		}
		BI = 0;
		//LSB方式:
		for(i=0;i<BitCount;i++)
		{
			if( (i>0)&& ((i%8)==0) )
			{
				DbgPrint("OutBit[%d]:%02X",BI,OutBuf[BI]);
				BI++;				
			}
			if(FmtStr[BitCount-i-1]=='1')
				OutBuf[BI] |= (1<<(i%8))&0xFF;	
			else if(FmtStr[BitCount-i-1]=='0')
				OutBuf[BI] &= ~(1<<(i%8))&0xFF;	
			else
			{
				DbgPrint("invalid bit input.");
				return FALSE;
			}			
		}		
		{//debug
			DbgPrint("OutBit[%d]:%02X",BI,OutBuf[BI]);
		}
	}	
	BT = GetCurrentTimerVal();
	if(IsRead)
	{
		BitCount = GetDlgItemInt(AfxMainHwnd,IDC_JtagInBitLen,NULL,FALSE);	
		if(IsDr)
			RetVal = USB20Jtag_BitReadDR(0,&InBitLen,InBuf);
		else
			RetVal = USB20Jtag_BitReadIR(0,&InBitLen,InBuf);
		//RetVal = JTAG_WriteRead(0,IsDr,OutBitLen,OutBuf,&InBitLen,InBuf);
		//if(BitCount<InBitLen)
		{
		//	DbgPrint("USB20Jtag_BitReadIR return too many data bit.(%d-%d)",BitCount,InBitLen);
		//	BitCount = InBitLen;
		}
	}
	else 
	{
		//位带方式
		//RetVal = JTAG_WriteRead(0,IsDr,BitCount,OutBuf,NULL,NULL);
		if(IsDr)
			RetVal = USB20Jtag_BitWriteDR(0,OutBitLen,OutBuf);
		else
			RetVal = USB20Jtag_BitWriteIR(0,OutBitLen,OutBuf);
	}
	UsedT = GetCurrentTimerVal()-BT;
		
	if(InBitLen)
	{//打印
		for(i=0;i<InBitLen;i++)
		{			
			if( InBuf[i/8] & (1<<(i%8)) )
				FmtStr[InBitLen-i-1] = '1';
			else
				FmtStr[InBitLen-i-1] = '0';			
		}
		SetDlgItemText(AfxMainHwnd,IDC_JtagInBit,FmtStr);
		SetDlgItemInt(AfxMainHwnd,IDC_JtagInBitLen,InBitLen,FALSE);
	}
	SetDlgItemInt(AfxMainHwnd,IDC_JtagInBitLen,InBitLen,FALSE);		
	
	if(IsRead)
	{		
		DbgPrint("**<<**Jtag_Data %s %s read %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",InBitLen,InBitLen,UsedT,InBitLen/UsedT);
	}
	else
	{
		DbgPrint("**<<**Jtag_Data %s %s,write %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",OutBitLen,OutBitLen,UsedT,OutBitLen/UsedT);
	}

Exit:
	return RetVal;
}

// JTAG port firmware download, only demonstrates the fast file transfer from the DR port, not specific model. 
// When the JTAG speed is set to 4, the Jtag_WriteRead_Fast function is called, and the download speed can reach about 4MB/S
DWORD WINAPI DownloadFwFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[512]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen;
	PUCHAR FileBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	UCHAR RBuf[4096] = "";
	BOOL IsDr;
	
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
	mOpenFile.lpstrTitle        = "Select the data file to be written";
	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Write data via jtag from:%s",FileName);
	}
	else goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("DownloadFwFile.OpenFile");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		DbgPrint("DownloadFwFile.File is empty");
		goto Exit;
	}
	DbgPrint("*>>*DownloadFwFile.Flash Write %dB",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		DbgPrint("DownloadFwFile.malloc failure");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("DownloadFwFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("DownloadFwFile.ReadFlashFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}	

	IsDr = (SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);
	DbgPrint("*>>*WriteFlashFromFile.Flash block write");	
	BT = GetCurrentTimerVal();	

	AfxDataTransType = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_DataTransFunc,CB_GETCURSEL,0,0);
	if(AfxDataTransType)  //DEBUG
	{
		//RetVal = JTAG_WriteRead(0,IsDr,TestLen*8,FileBuf,NULL,NULL);			
		RetVal = USB20Jtag_BitWriteDR(0,TestLen*8,FileBuf);
	}
	else
	{
		//RetVal = JTAG_WriteRead_Fast(0,IsDr,TestLen,FileBuf,NULL,NULL);			
		RetVal = USB20Jtag_ByteWriteDR(0,TestLen,FileBuf);
	}

	if(!RetVal)
		DbgPrint("DownloadFwFile.Write 0x%X] failure",TestLen);	
	UsedT = (GetCurrentTimerVal()-BT)/1000; //Download time, in seconds

	sprintf(FmtStr,"*<<*DownloadFwFile. Write %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure ", TestLen/UsedT / 1000, UsedT);
	OutputDebugString(FmtStr);
	DbgPrint("*<<*DownloadFwFile. Write %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure ", TestLen/UsedT / 1000, UsedT);

Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

//jtag interface config
BOOL Jtag_InterfaceConfig()
{
    UCHAR iClockRate;    //Communication speed; The value ranges from 0 to 5. A larger value indicates a faster communication speed
	
	if(!AfxDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		return FALSE;
	}	
	iClockRate = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_GETCURSEL,0,0);

	AfxJtagIsCfg = USB20Jtag_INIT(0,iClockRate);
	DbgPrint("CH347JTAG_INIT %s.iClockRate=%X",AfxJtagIsCfg?"succ":"failure",iClockRate);
	return AfxJtagIsCfg;
}