/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  Edit control rewrite to achieve hexadecimal editing function
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  4/3/2022: V.100 TECH30
--*/

//#include "stdafx.h"
#include "Main.h"
//#include "USBNicDbg.h"
#include <richedit.h>

HWND ParentHwnd; //�ؼ����ڴ�����

static BOOL ShiftStatus=FALSE,DelKeyStatus;
CHAR DelHexCharIndex,DelHexCharCount;
WNDPROC OldEditWndProc; //�ؼ�ԭ�д������
ULONG EditCnt=0;

//���ò������״Ϊ����
void SetCaretShap(HWND DialogHWND,ULONG CtrlID)
{
	
    HANDLE iFnt;
    HDC hdc5;
	ULONG SelStart,SelEnd;
    SIZE sz;
    ULONG  width5, height5;
	CHAR aChar,tem[200]="";
	TEXTRANGE txr={0};
	    
	HWND CtlHwnd;
	DestroyCaret();
	CtlHwnd = GetDlgItem(DialogHWND,CtrlID);
    iFnt = (HANDLE)SendDlgItemMessage(DialogHWND,CtrlID,WM_GETFONT,0,0);
    hdc5 = GetDC(CtlHwnd);
    SelectObject(hdc5, iFnt);  //�����������ʾ������
    
	aChar = 0;
	SendDlgItemMessage(DialogHWND,CtrlID,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
	txr.chrg.cpMin=SelStart-1;
	txr.chrg.cpMax=SelStart;
	txr.lpstrText = &aChar;
	SendDlgItemMessage(DialogHWND,CtrlID,WM_GETTEXT,100,(ULONG)tem);	
	aChar = tem[SelStart-1];
    
    if(aChar == 0)//��������text�����棬����Caret����λ��û����Ԫ
	{
        if(SelStart > 1)
		{
			txr.chrg.cpMin=SelStart-1;
			txr.chrg.cpMax=SelStart;
			txr.lpstrText = &aChar;
			SendDlgItemMessage(DialogHWND,CtrlID,EM_EXGETSEL,0,(ULONG)&txr);
		}
        else
            aChar = 'x'; //�ڶ���Ԫx,ѡȡĬ�ϵ�������        
    }
    GetTextExtentPoint32(hdc5, &aChar, 1, &sz); //ȡ�ø���Ԫ�ĳ�����
	
    width5 = sz.cx;
    height5 = sz.cy;
    ReleaseDC(DialogHWND, hdc5);
    CreateCaret(CtlHwnd,NULL,width5, height5);//����Create Caret
    ShowCaret(CtlHwnd);     //��ʾCaret
}

long CALLBACK NewEditWndProc(IN HWND   WindowHandle,
							 IN UINT   Message,
							 IN WPARAM WParam,
							 IN LPARAM LParam)  
{
	ULONG SelStart=0,SelEnd=0,EditCtlId;

	EditCtlId = GetWindowLong(WindowHandle,GWL_ID);	
	switch (Message) { 
	case WM_RBUTTONDOWN:
	case  WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN  :
	case WM_LBUTTONUP:
	case WM_MBUTTONDBLCLK  :
	case WM_MBUTTONDOWN  :
	case WM_MBUTTONUP  :
		{
			ULONG SelStart,SelEnd,x;
			SetCaretShap(ParentHwnd,EditCtlId);
			if( (Message == WM_LBUTTONDOWN)||(Message == WM_MBUTTONDOWN) )//�ƶ�mouseʱ,������һ��ʮ������ֻ��1���ַ�ʱ,�Զ����һ��0
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
				if( (x>0)&&( (x-1)%3 == 0 )&&(SelStart==x) )
				{
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,x,x);
					SendMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_CHAR,(ULONG)'0',1);
				}
			}
			{//�������ڿո���,�Զ������һ���ַ�
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				if( (x!=SelStart)&&( !((SelStart+1)%3) ) )
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart-1,SelEnd-1);
			}
		}
		return (CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam)); 
	case WM_CHAR: 
		{
			CHAR tem[100]="";
			CHAR chCharCode = (TCHAR) WParam;    // character code 
			ULONG lKeyData = LParam,x;              // key data 

			if( !(((chCharCode>='0')&&(chCharCode<='9')) ||//������Чʮ�������ַ�����,��������
				((chCharCode>='A')&&(chCharCode<='F')) ||
				((chCharCode>='a')&&(chCharCode<='f'))) )
			{
				if(chCharCode==VK_BACK)
					CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam);
				return 0;
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
			if( (x!=SelStart)  ) //�������޸�����,��Ҫ��ǰ���Ӳ������λ��
			{
				if( (SelStart+1)%3 == 0 )
				{
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
				}
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);

			//�����VK_BACK,VK_DELETE��ɾ���ַ�ʱ,���ͣ�ڿո�ʱ�Զ�ǰ��һ��,��֤��ͣ���ڿո�ǰ.
			if( ((SelStart>1)&&( !((SelStart+1)%3)&&(chCharCode!=VK_BACK)&&(chCharCode!=VK_DELETE) ) ) )
			{				
				CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,' ',3735553);
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			if( (SelStart+1)%3==0 )
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
			//������ͣ�ڿո�,�Զ��Ƶ���һ��λ��
			if(x!=SelEnd)
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)(SelStart+1),(ULONG)(SelEnd) );				
			}
			CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam);
		}	
		break;
	case WM_KEYDOWN:
		{
			ULONG x,chCharCode,nVirtKey = (int) WParam;
			CHAR tem[100]="";
			ULONG SelStart,SelEnd;

			SetCaretShap(ParentHwnd,EditCtlId);
			chCharCode = nVirtKey;					

			x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			if( (x==SelStart)&&( !((SelStart+1)%3) )&&((nVirtKey==VK_UP)||(nVirtKey==VK_DOWN)) )
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart-1,SelEnd-1);

			if( (nVirtKey== VK_RETURN)||(nVirtKey== VK_CONTROL) )//���ûس���ctrl��
				return 0;
			if( (nVirtKey==VK_LEFT) || (nVirtKey==VK_RIGHT) ||(nVirtKey==VK_UP) ||
				(nVirtKey==VK_DOWN) ||(nVirtKey==VK_HOME) ||(nVirtKey==VK_END) )
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
				if( ( (x-1)%3 == 0 )&&(SelStart==x) )
				{
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,x,x);
					SendMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_CHAR,(ULONG)'0',LParam);
					//return 0;
				}
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			//���ѡ��VK_BACK����������ɾ��,ɾ����Ԫ������ʮ�������ַ���һ�ո�
			if( (nVirtKey== VK_BACK) )
			{
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));					
				if(!DelHexCharIndex)//ɾ��ʮ�������ַ��ӿո��еĿո�
				{						
					if(SelStart==0)//������,��ɾ��
						DelHexCharCount = 0;
					else if( (x-SelStart)<2 )//�����һ��ʮ�������ַ�,��֤���һ���ַ����ǿո�
					{
						DelHexCharCount = 2;
						SelStart+=3;						
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
					}
					else if( ((SelStart-1)%3==0) )//���������������ʮ�������ַ��м�,���Ƶ������ַ���
					{						
						SelStart+=2;						
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
						DelHexCharCount = 3;
					}	
					else 
					{
						DelHexCharCount = 3;
					}

					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
					if( (SelStart%3)==0 )//�����������ʮ�������ַ�ǰ,ɾ��ǰһ��ʮ�����ƶ����ַ���һ�ո�
					{						
						PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_BACK,LParam);
						DelHexCharIndex++;
						DelHexCharIndex++;
						return 0;
					}
				}
				if(DelHexCharIndex<DelHexCharCount){//ɾ��ʮ�����Ƶڶ����ַ�												
					PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_BACK,LParam);
					DelHexCharIndex++;
					return 0;
				}
				else if(DelHexCharIndex >=DelHexCharCount )//ɾ��ʮ�����Ƶ�һ���ַ�
				{
					DelHexCharIndex = 0;
					return 0;
				}					
			}				
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			//���ѡ��VK_DELETE����������ɾ��,ɾ����Ԫ������ʮ�������ַ���һ�ո�
			if( (nVirtKey== VK_DELETE) )
			{
				if( !DelHexCharIndex )//ɾ��ʮ�������ַ��еĵ�һ���ַ�
				{
					x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));

					if( (x-SelStart)==1 )//���ͣ�����һ���ֽ�
					{
						SelStart-=2;								
					}
					if( (x-SelStart)==2 )//���ͣ�����һ���ֽ�
					{
						SelStart-=1;						
					}
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
					if( !((SelStart-1)%3) )//���ͣ��ʮ�������������ַ����м�
					{
						SelStart-=1;
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
					}
					DelHexCharCount = 3;

					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
					if( !(SelStart%3) )
					{
						DelHexCharIndex++;
						PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_DELETE,LParam);						
						CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam); 						
						return 0;
					}
				}					
				if(DelHexCharIndex<DelHexCharCount){//ɾ��ʮ�������ַ��еĵڶ����ַ�
					DelHexCharIndex++;
					PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_DELETE,LParam);
					CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam); 
					return 0;
				}
				else if(DelHexCharIndex >=DelHexCharCount )//ɾ��ʮ�������ַ��еĿո�
				{
					DelHexCharCount = DelHexCharIndex = 0;
					return 0;
				}					
			}				
			DelHexCharCount = DelHexCharIndex = 0;
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			if( (nVirtKey== VK_LEFT) )//��ֹ�Ƶ��ո�ǰ
			{
				if( (SelStart%3)==0 )
				{
					if(!ShiftStatus)//��ֹVK_SHIFT+VK_LEFT
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart-1,SelEnd-1);
				}
			}
			if(nVirtKey== VK_RIGHT)//��ֹ�Ƶ��ո�ǰ
			{
				if( (((SelStart-1)%3)==0) && (SelStart>0) )
				{
					if(!ShiftStatus)//��ֹVK_SHIFT+VK_RIGHT
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
				}
			}								
			{//����SHIFT��VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN
				if(nVirtKey == VK_SHIFT) //�Ѱ���VK_SHIFT
				{
					ShiftStatus = TRUE;					
				}
				if( ShiftStatus && (
					( (nVirtKey>0x24)&&(nVirtKey<0x29) ) ||
					( nVirtKey == VK_HOME ) ||
					( nVirtKey == VK_END ) )
					)
					return 0;					
			}
		}
		CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam); 
		break;
	case WM_KEYUP:
		{
			ULONG SelStart,SelEnd;
			int nVirtKey = (int) WParam;	


			if(nVirtKey == VK_SHIFT)//���ͷ�VK_SHIFT
			{
				ShiftStatus = FALSE;
			}	
			if( (((nVirtKey>='0')&&(nVirtKey<='9')) ||
				((nVirtKey>='A')&&(nVirtKey<='F')) ||
				((nVirtKey>='a')&&(nVirtKey<='f'))) )
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				//������ͣ�ڿո�,�Զ��Ƶ���һ��λ��
				if(!((SelStart+1)%3))//��������ʱ,������Ƶ��ո�ǰʱ,������һ�ַ�ǰ
				{						
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)(SelStart+1),(ULONG)(SelEnd+1) );
				}
			}
		}
		CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam); 

		break;
	default:
		return (CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam)); 
	}
	return 0;
}

//����EDIT�ؼ��ı������ʮ��������
VOID SetEditlInputMode(
					   HWND hWnd,       // handle of window
					   ULONG CtrlID,    //�ؼ�ID��
					   UCHAR InputMode //����ģʽ:0:�ı�ģʽ,1:ʮ������ģʽ
					   )
{
	ParentHwnd = hWnd;	
	if ( ( InputMode == 0 ) && ( OldEditWndProc != 0 ) )//�ı�ģʽ
	{
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_USERDATA,0);
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_WNDPROC,(LONG)OldEditWndProc);
		SetDlgItemText(hWnd,CtrlID,"");	
	}
	else if( InputMode == 1 )//ʮ������ģʽ
	{	
		OldEditWndProc = (WNDPROC)SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_WNDPROC,(LONG)NewEditWndProc);
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),DWL_USER,(LONG)CtrlID);
		SetDlgItemText(hWnd,CtrlID,"");
	}
}