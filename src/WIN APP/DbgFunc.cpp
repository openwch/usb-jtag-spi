/**********************************************************
**  Copyright  (C)  WCH  2001-2022                       **
**  Web:  http://wch.cn                                  **
***********************************************************
Abstract:
    Auxiliary function
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

//To disable deprecation, use _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h.>
#pragma comment(lib,"ws2_32")
#include "Main.h"


CHAR AfxShareBuf[1024]; //�����ڴ�
extern HWND AfxMainHwnd;
ULONG AfxDbgI = 0;

// Outputs the formatted string, which is received by dbgView software
VOID  DbgPrint (LPCTSTR lpFormat,...)
{   
   CHAR TextBufferTmp[10240]="";
   {
	   SYSTEMTIME lpSystemTime;
	   GetLocalTime(&lpSystemTime);
	   sprintf(TextBufferTmp,"%04d#%02d:%02d:%02d:%03d:: \0",AfxDbgI++,		   
		   lpSystemTime.wHour ,lpSystemTime.wMinute ,lpSystemTime.wSecond,lpSystemTime.wMilliseconds );
   }

   va_list arglist;
   va_start(arglist, lpFormat);
   vsprintf(&TextBufferTmp[strlen(TextBufferTmp)],lpFormat,arglist);
   va_end(arglist);
   strcat(TextBufferTmp,"\r\n");
   OutputDebugString(TextBufferTmp);

   SendDlgItemMessage(AfxMainHwnd,IDC_InforShow,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   SendDlgItemMessage(AfxMainHwnd,IDC_InforShow,EM_REPLACESEL,0,(LPARAM)TextBufferTmp);
   SendDlgItemMessage(AfxMainHwnd,IDC_InforShow,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
  
   return ;
}

/*��ʾ�ϴ����д���*/
void ShowLastError(LPCTSTR lpFormat,...) 
{
	DWORD LastResult=0; // pointer to variable to receive error codes	
	CHAR szSysMsg[4096] = "";
	CHAR PreBuffer[4096] = "";  	
	LastResult=GetLastError();
    {
		va_list arglist;
		va_start(arglist, lpFormat);
		vsprintf(PreBuffer,lpFormat,arglist);
		va_end(arglist);   
		
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,LastResult,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),szSysMsg,sizeof(szSysMsg),0);	
		DbgPrint("%s����:(0x%X)%s",PreBuffer,LastResult,szSysMsg);
	}	
}

//��ȡӲ��������������ʱ��,msΪ��λ,��GetTickCount��׼ȷ
double GetCurrentTimerVal()
{
	LARGE_INTEGER litmp; 
	double dfFreq,QPart1; 
	QueryPerformanceFrequency(&litmp);  //Ƶ����HZΪ��λ
	dfFreq = (double)litmp.QuadPart;    //��ü�������ʱ��Ƶ��
	QueryPerformanceCounter(&litmp);
	QPart1 = (double)litmp.QuadPart;        //��ó�ʼֵ
	return(QPart1 *1000/dfFreq  );  //��ö�Ӧ��ʱ��ֵ=�񵴴���/��Ƶ�ʣ���λΪ��
}

/*��ʮ�������ַ�ת����ʮ������,����ת�����ַ���ltoa()����*/
ULONG mStrToHEX(PCHAR str) 
{  
	char mlen,i=0;
	UCHAR iChar=0,Char[9]="";
	UINT mBCD=0,de=1;
	mlen=strlen(str);
	memcpy(Char,str,mlen);
	for(i=mlen-1;i>=0;i--)
	{	iChar=Char[i];
	if ( iChar >= '0' && iChar <= '9' )
		mBCD = mBCD+(iChar -'0')*de;
	else if ( iChar >= 'A' && iChar <= 'F' ) 
		mBCD =mBCD+ (iChar - 'A' + 0x0a)*de;
	else if ( iChar >= 'a' && iChar <= 'f' )
		mBCD =mBCD+ (iChar - 'a' + 0x0a)*de;
	else return(0);
	de*=16;
	}
	return(mBCD);
}

//����ʽ���ַ���Ϣ������ı���ĩβ
VOID  AddStrToEdit (HWND hDlg,ULONG EditID,const char * Format,...)
{
   va_list arglist;   
   int cb;
   CHAR buffer[10240]="";

   va_start(arglist, Format);
   cb = _vsnprintf(&buffer[strlen(buffer)], sizeof(buffer), Format, arglist);
   if (cb == -1) 
   {
      buffer[sizeof(buffer) - 2] = '\n';
   }
   if(strlen(buffer) && buffer[strlen(buffer)-1]!='\n' )
	   strcat(buffer,"\0");   
   va_end(arglist);
   
   SendDlgItemMessage(hDlg,EditID,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   SendDlgItemMessage(hDlg,EditID,EM_REPLACESEL,0,(LPARAM)buffer);
   SendDlgItemMessage(hDlg,EditID,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   return ;
}


//��С���л�
ULONG EndSwitch(ULONG dwVal)
{
	ULONG SV;

	((PUCHAR)&SV)[0] = ((PUCHAR)&dwVal)[3];
	((PUCHAR)&SV)[1] = ((PUCHAR)&dwVal)[2];
	((PUCHAR)&SV)[2] = ((PUCHAR)&dwVal)[1];
	((PUCHAR)&SV)[3] = ((PUCHAR)&dwVal)[0];
	return SV;
}
