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

#ifndef		_DEBUGFUNC_H
#define		_DEBUGFUNC_H

VOID  DbgPrint (LPCTSTR lpFormat,...); //��ʾ��ʽ���ַ���
void ShowLastError(LPCTSTR lpFormat,...); //��ʾ�ϴ����д���
double GetCurrentTimerVal(); //��ȡӲ��������������ʱ��,msΪ��λ,��GetTickCount��׼ȷ
ULONG mStrToHEX(PCHAR str); //��ʮ�������ַ�ת����ʮ������,����ת�����ַ���ltoa()����
VOID  AddStrToEdit (HWND hDlg,ULONG EditID,const char * Format,...); //����ʽ���ַ���Ϣ������ı���ĩβ
ULONG EndSwitch(ULONG dwVal);//��С���л�
VOID SetEditlInputMode(//����EDIT�ؼ��ı������ʮ��������
					    HWND hWnd,        // handle of window
						ULONG CtrlID,     //�ؼ�ID��
						UCHAR InputMode); //����ģʽ:0:�ı�ģʽ,1:ʮ������ģʽ
					   
#endif