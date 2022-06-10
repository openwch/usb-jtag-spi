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

HWND ParentHwnd; //控件所在窗体句柄

static BOOL ShiftStatus=FALSE,DelKeyStatus;
CHAR DelHexCharIndex,DelHexCharCount;
WNDPROC OldEditWndProc; //控件原有窗体程序
ULONG EditCnt=0;

//设置插入符形状为黑体
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
    SelectObject(hdc5, iFnt);  //将字体加入显示场景中
    
	aChar = 0;
	SendDlgItemMessage(DialogHWND,CtrlID,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
	txr.chrg.cpMin=SelStart-1;
	txr.chrg.cpMax=SelStart;
	txr.lpstrText = &aChar;
	SendDlgItemMessage(DialogHWND,CtrlID,WM_GETTEXT,100,(ULONG)tem);	
	aChar = tem[SelStart-1];
    
    if(aChar == 0)//可能是在text最後面，所以Caret所在位置没有字元
	{
        if(SelStart > 1)
		{
			txr.chrg.cpMin=SelStart-1;
			txr.chrg.cpMax=SelStart;
			txr.lpstrText = &aChar;
			SendDlgItemMessage(DialogHWND,CtrlID,EM_EXGETSEL,0,(ULONG)&txr);
		}
        else
            aChar = 'x'; //内定字元x,选取默认的字体宽高        
    }
    GetTextExtentPoint32(hdc5, &aChar, 1, &sz); //取得该字元的长、宽
	
    width5 = sz.cx;
    height5 = sz.cy;
    ReleaseDC(DialogHWND, hdc5);
    CreateCaret(CtlHwnd,NULL,width5, height5);//重新Create Caret
    ShowCaret(CtlHwnd);     //显示Caret
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
			if( (Message == WM_LBUTTONDOWN)||(Message == WM_MBUTTONDOWN) )//移动mouse时,如果最后一个十六进制只有1个字符时,自动添加一个0
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
				if( (x>0)&&( (x-1)%3 == 0 )&&(SelStart==x) )
				{
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,x,x);
					SendMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_CHAR,(ULONG)'0',1);
				}
			}
			{//如果光标在空格上,自动向后移一个字符
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

			if( !(((chCharCode>='0')&&(chCharCode<='9')) ||//不是有效十六进制字符输入,忽略输入
				((chCharCode>='A')&&(chCharCode<='F')) ||
				((chCharCode>='a')&&(chCharCode<='f'))) )
			{
				if(chCharCode==VK_BACK)
					CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam);
				return 0;
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
			if( (x!=SelStart)  ) //此是是修改数据,需要提前监视插入符的位置
			{
				if( (SelStart+1)%3 == 0 )
				{
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
				}
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);

			//如果用VK_BACK,VK_DELETE键删除字符时,光标停在空格处时自动前移一个,保证不停留在空格前.
			if( ((SelStart>1)&&( !((SelStart+1)%3)&&(chCharCode!=VK_BACK)&&(chCharCode!=VK_DELETE) ) ) )
			{				
				CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,' ',3735553);
			}
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			if( (SelStart+1)%3==0 )
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));
			//如果光标停在空格处,自动移到下一个位置
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

			if( (nVirtKey== VK_RETURN)||(nVirtKey== VK_CONTROL) )//禁用回车和ctrl键
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
			//如果选择VK_BACK键进行数据删除,删除单元是两个十六进制字符加一空格
			if( (nVirtKey== VK_BACK) )
			{
				x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));					
				if(!DelHexCharIndex)//删除十六进制字符加空格中的空格
				{						
					if(SelStart==0)//在行首,不删除
						DelHexCharCount = 0;
					else if( (x-SelStart)<2 )//在最后一个十六进制字符,保证最后一个字符不是空格
					{
						DelHexCharCount = 2;
						SelStart+=3;						
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
					}
					else if( ((SelStart-1)%3==0) )//插入符正好在两个十六进制字符中间,先移到两个字符后
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
					if( (SelStart%3)==0 )//插入符正好在十六进制字符前,删除前一个十六进制二个字符加一空格
					{						
						PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_BACK,LParam);
						DelHexCharIndex++;
						DelHexCharIndex++;
						return 0;
					}
				}
				if(DelHexCharIndex<DelHexCharCount){//删除十六进制第二个字符												
					PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_BACK,LParam);
					DelHexCharIndex++;
					return 0;
				}
				else if(DelHexCharIndex >=DelHexCharCount )//删除十六进制第一个字符
				{
					DelHexCharIndex = 0;
					return 0;
				}					
			}				
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			//如果选择VK_DELETE键进行数据删除,删除单元是两个十六进制字符加一空格
			if( (nVirtKey== VK_DELETE) )
			{
				if( !DelHexCharIndex )//删除十六进制字符中的第一个字符
				{
					x = GetWindowTextLength(GetDlgItem(ParentHwnd,EditCtlId));

					if( (x-SelStart)==1 )//光标停在最后一个字节
					{
						SelStart-=2;								
					}
					if( (x-SelStart)==2 )//光标停在最后一个字节
					{
						SelStart-=1;						
					}
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,(ULONG)SelStart,(ULONG)SelStart);
					SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
					if( !((SelStart-1)%3) )//光标停在十进制数的两个字符的中间
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
				if(DelHexCharIndex<DelHexCharCount){//删除十六进制字符中的第二个字符
					DelHexCharIndex++;
					PostMessage(GetDlgItem(ParentHwnd,EditCtlId),WM_KEYDOWN,VK_DELETE,LParam);
					CallWindowProc((WNDPROC)OldEditWndProc,WindowHandle,Message,WParam,LParam); 
					return 0;
				}
				else if(DelHexCharIndex >=DelHexCharCount )//删除十六进制字符中的空格
				{
					DelHexCharCount = DelHexCharIndex = 0;
					return 0;
				}					
			}				
			DelHexCharCount = DelHexCharIndex = 0;
			SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
			if( (nVirtKey== VK_LEFT) )//禁止移到空格前
			{
				if( (SelStart%3)==0 )
				{
					if(!ShiftStatus)//禁止VK_SHIFT+VK_LEFT
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart-1,SelEnd-1);
				}
			}
			if(nVirtKey== VK_RIGHT)//禁止移到空格前
			{
				if( (((SelStart-1)%3)==0) && (SelStart>0) )
				{
					if(!ShiftStatus)//禁止VK_SHIFT+VK_RIGHT
						SendDlgItemMessage(ParentHwnd,EditCtlId,EM_SETSEL,SelStart+1,SelEnd+1);
				}
			}								
			{//禁用SHIFT加VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN
				if(nVirtKey == VK_SHIFT) //已按下VK_SHIFT
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


			if(nVirtKey == VK_SHIFT)//已释放VK_SHIFT
			{
				ShiftStatus = FALSE;
			}	
			if( (((nVirtKey>='0')&&(nVirtKey<='9')) ||
				((nVirtKey>='A')&&(nVirtKey<='F')) ||
				((nVirtKey>='a')&&(nVirtKey<='f'))) )
			{
				SendDlgItemMessage(ParentHwnd,EditCtlId,EM_GETSEL,(ULONG)&SelStart,(ULONG)&SelEnd);
				//如果光标停在空格处,自动移到下一个位置
				if(!((SelStart+1)%3))//覆盖数据时,插入符移到空格前时,跳到后一字符前
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

//设置EDIT控件文本输入和十进制输入
VOID SetEditlInputMode(
					   HWND hWnd,       // handle of window
					   ULONG CtrlID,    //控件ID号
					   UCHAR InputMode //输入模式:0:文本模式,1:十六进制模式
					   )
{
	ParentHwnd = hWnd;	
	if ( ( InputMode == 0 ) && ( OldEditWndProc != 0 ) )//文本模式
	{
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_USERDATA,0);
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_WNDPROC,(LONG)OldEditWndProc);
		SetDlgItemText(hWnd,CtrlID,"");	
	}
	else if( InputMode == 1 )//十六进制模式
	{	
		OldEditWndProc = (WNDPROC)SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),GWL_WNDPROC,(LONG)NewEditWndProc);
		SetWindowLong(GetDlgItem(ParentHwnd,CtrlID),DWL_USER,(LONG)CtrlID);
		SetDlgItemText(hWnd,CtrlID,"");
	}
}