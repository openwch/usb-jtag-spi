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


//VOID DumpBitBangData(PUCHAR BitBang,ULONG DLen);

/* Brings target into the Debug mode and get device id */
VOID Jtag_InitTarget (VOID);

//jtag口固件下载，仅演示从DR口快速传输文件，未针对具体型号。在JTAG速度配置为4时，调用Jtag_WriteRead_Fast函数，下载速度可达4MB/S左右
DWORD WINAPI DownloadFwFile(LPVOID lpParameter);

//jtag 接口配置
BOOL Jtag_InterfaceConfig();

//JTAT IR/DR数据读写。按位进行读写。状态机从Run-Test-Idle ->IR/DR->Exit IR/DR ->Run-Test-Idle
BOOL Jtag_DataRW_Byte(BOOL IsRead);

//JTAT IR/DR数据读写。按位进行读写。状态机从Run-Test-Idle ->IR/DR->Exit IR/DR ->Run-Test-Idle
BOOL Jtag_DataRW_Bit(BOOL IsRead);
