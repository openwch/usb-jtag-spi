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

//jtag�ڹ̼����أ�����ʾ��DR�ڿ��ٴ����ļ���δ��Ծ����ͺš���JTAG�ٶ�����Ϊ4ʱ������Jtag_WriteRead_Fast�����������ٶȿɴ�4MB/S����
DWORD WINAPI DownloadFwFile(LPVOID lpParameter);

//jtag �ӿ�����
BOOL Jtag_InterfaceConfig();

//JTAT IR/DR���ݶ�д����λ���ж�д��״̬����Run-Test-Idle ->IR/DR->Exit IR/DR ->Run-Test-Idle
BOOL Jtag_DataRW_Byte(BOOL IsRead);

//JTAT IR/DR���ݶ�д����λ���ж�д��״̬����Run-Test-Idle ->IR/DR->Exit IR/DR ->Run-Test-Idle
BOOL Jtag_DataRW_Bit(BOOL IsRead);
