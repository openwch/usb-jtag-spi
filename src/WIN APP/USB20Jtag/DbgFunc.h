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

VOID   DbgPrint (LPCTSTR lpFormat,...); //��ʾ��ʽ���ַ���
void   ShowLastError(LPCTSTR lpFormat,...); //��ʾ�ϴ����д���
double GetCurrentTimerVal(); //��ȡӲ��������������ʱ��,msΪ��λ,��GetTickCount��׼ȷ
//��ʱ����,��msΪ��λ
ULONG mStrToHEX(PCHAR str); //��ʮ�������ַ�ת����ʮ������,����ת�����ַ���ltoa()����

//����EDIT�ؼ��ı������ʮ��������
VOID SetEditlInputMode(
					    HWND hWnd,       // handle of window
						ULONG CtrlID,    //�ؼ�ID��
						UCHAR InputMode //����ģʽ:0:�ı�ģʽ,1:ʮ������ģʽ
					   );
VOID DumpBitBangData(PUCHAR BitBang,ULONG DLen);
#endif