/**********************************************************
**  Copyright  (C)  WCH  2001-2022                       **
**  Web:  http://wch.cn                                  **
***********************************************************
Abstract:
    Auxiliary function
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  3/1/2022: TECH30 create
--*/

#ifndef		_DEBUGFUNC_H
#define		_DEBUGFUNC_H

VOID   DbgPrint (LPCTSTR lpFormat,...); //显示格式化字符串
void   ShowLastError(LPCTSTR lpFormat,...); //显示上次运行错误
double GetCurrentTimerVal(); //获取硬件计数器已运行时间,ms为单位,比GetTickCount更准确
//延时函数,以ms为单位
ULONG mStrToHEX(PCHAR str); //将十六进制字符转换成十进制码,数字转换成字符用ltoa()函数

//设置EDIT控件文本输入和十进制输入
VOID SetEditlInputMode(
					    HWND hWnd,       // handle of window
						ULONG CtrlID,    //控件ID号
						UCHAR InputMode //输入模式:0:文本模式,1:十六进制模式
					   );
VOID DumpBitBangData(PUCHAR BitBang,ULONG DLen);
#endif