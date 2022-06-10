/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    Application form  
    The USB2.0 to JTAG scheme based on CH32V305 MCU can be used to build 
	customized USB high-speed JTAG debugger and other products.  
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) 
	device universal driver (CH372DRV) and upper computer routines.  
    The current routine is a custom communication protocol,TDI transmission speed up to 4M bytes 
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (C) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/
#include "Main.h"
#include "Usb20Jtag.h"

//全局变量
HWND AfxMainHwnd;     //主窗体句柄
HINSTANCE AfxMainIns; //进程实例
BOOL AfxDevIsOpened;  //设备是否打开
HINSTANCE MainInstan;
UCHAR AfxDataTransType=0;
BOOL AfxJtagIsCfg = FALSE;

#define WM_JtagDevArrive WM_USER+10         //设备插入通知事件,窗体进程接收
#define WM_JtagDevRemove WM_USER+11         //设备拔出通知事件,窗体进程接收

#define JtagUSBID "VID_1A86&PID_5537\0"

//函数声明
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//初始化窗体
VOID InitWindows()
{
	//查找并显示设备列表 
	SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_OpenDevice),!AfxDevIsOpened);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CloseDevice),AfxDevIsOpened);	
	//输出框设置显示的最大字符数
	SendDlgItemMessage(AfxMainHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	{
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Test-Logic Reset");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle -> Shift-DR");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Shift-DR -> Run-Test/Idle");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle -> Shift-IR");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Shift-IR -> Run-Test/Idle");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_SETCURSEL,0,0);
	}
	{
		SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  IR");
		SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  DR");			
		SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_SETCURSEL,1,0);
	}		
	{			
		SendDlgItemMessage(AfxMainHwnd,IDC_DataTransFunc,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Byte");
		SendDlgItemMessage(AfxMainHwnd,IDC_DataTransFunc,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"bit ");
		SendDlgItemMessage(AfxMainHwnd,IDC_DataTransFunc,CB_SETCURSEL,0,0);
	}
	SetFocus(GetDlgItem(AfxMainHwnd,IDC_JtaShiftgChannel)); //更新操作按钮状态

	{
		//JTAG速度配置值；有效值为0-5，值越大通信速度越快；
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"2.25MHz");    //speed 0：2.25MHZ
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)" 4.5MHz");    //speed 1：4.5MHZ；
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"   9MHz");    //speed2：9MHZ；
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  18MHz");    //speed3：18MHZ；
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  36MHz");    //speed4：36MHZ；
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_SETCURSEL,4,0);
	}	
	SetEditlInputMode(AfxMainHwnd,IDC_JtagOut,1);
	SetEditlInputMode(AfxMainHwnd,IDC_JtagIn,1);	

	SetDlgItemText(AfxMainHwnd,IDC_JtagOutBitLen,"0");
	SetDlgItemText(AfxMainHwnd,IDC_JtagInBitLen,"0");
	SetDlgItemText(AfxMainHwnd,IDC_JtagOutLen,"0");
	SetDlgItemText(AfxMainHwnd,IDC_JtagOutLen,"0");		

	return;
}

//使能操作按钮，需先打开和配置JTAG，否则无法操作
VOID EnableButtonEnable()
{
	if(!AfxDevIsOpened)
		AfxJtagIsCfg = FALSE;

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_JtagIfConfig),AfxDevIsOpened);

	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_OpenDevice),!AfxDevIsOpened);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CloseDevice),AfxDevIsOpened);				

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_ByteWrite),AfxJtagIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_ByteRead),AfxJtagIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_BitWrite),AfxJtagIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_BitRead),AfxJtagIsCfg);

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_DnFile_Exam),AfxJtagIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_Jtag_InitTarget),AfxJtagIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_JtagState_Switch),AfxJtagIsCfg);
}

// USB设备插拔检测通知程序.因回调函数对函数操作有限制，通过窗体消息转移到消息处理函数内进行处理。
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH375_DEVICE_ARRIVAL)// 设备插入事件,已经插入
		PostMessage(AfxMainHwnd,WM_JtagDevArrive,0,0);
	else if(iEventStatus==CH375_DEVICE_REMOVE)// 设备拔出事件,已经拔出
		PostMessage(AfxMainHwnd,WM_JtagDevRemove,0,0);
	return;
}

//枚举设备
ULONG EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";

	SendDlgItemMessage(AfxMainHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH375OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH375GetDeviceDescr(i,&DevDesc,&oLen);
			sprintf(tem,"%d.VID_%04X&&PID_%04X,mbcdDevice:%04X",i,DevDesc.idVendor,DevDesc.idProduct,DevDesc.bcdDevice);
			SendDlgItemMessage(AfxMainHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);
			DevCnt++;
		}
		CH375CloseDevice(i);
	}
	if(DevCnt)
		SendDlgItemMessage(AfxMainHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
	return DevCnt;
}

//应用程序入口
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	AfxMainIns = hInstance;
	return 	DialogBox(hInstance, (LPCTSTR)IDD_MainWnd_EN, 0, (DLGPROC)WndProc);
}
//主窗体进程
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;

	switch (message)
	{
	case WM_INITDIALOG:
		AfxMainHwnd = hWnd;
		// Seed the random-number generator with current time so that
		// the numbers will be different every time we run.		
		srand( (unsigned)time( NULL ) );
		{//添加alt+tab切换时显示的图标
			HICON hicon;
			hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
		}
		AfxDevIsOpened = FALSE;		
		InitWindows(); //初始化窗体			
		EnableButtonEnable(); //使能操作按钮，需先打开和配置JTAG，否则无法操作
		//为USB2.0JTAG设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
		CH375SetDeviceNotify(0,
			JtagUSBID,  //设备USB ID
			UsbDevPnpNotify);       //设备插拔通知回调函数

		break;	
	case WM_JtagDevArrive:
			DbgPrint("Usb Device is arrived,open device");
			//先枚举USB设备
			SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
			//打开设备
			SendDlgItemMessage(AfxMainHwnd,IDC_OpenDevice,BM_CLICK,0,0);
			break;
	case WM_JtagDevRemove:
		DbgPrint("Usb Device is removed,close device");
		//关闭设备
		SendDlgItemMessage(AfxMainHwnd,IDC_CloseDevice,BM_CLICK,0,0);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{		
		case IDC_RefreshObjList:
			EnumDevice(); //枚举并显示设备
			break;
		case IDC_OpenDevice://打开设备
			{
				ULONG CI;
				//获取设备序号
				CI = SendDlgItemMessage(hWnd,IDC_ObjList,CB_GETCURSEL,0,0);
				if(CI==CB_ERR)
				{
					DbgPrint("Open device failure.");
					break; //退出
				}
				DbgPrint(">>Device Open...");
				AfxDevIsOpened = USB20Jtag_OpenDevice(CI);
				if( AfxDevIsOpened ) //打开设备
					DbgPrint("          Succ");
				else
					DbgPrint("          Failure");
				EnableButtonEnable();//更新按钮状态				
			}
			break;
		case IDC_CloseDevice:
			{
				ULONG CI;				
				CI = SendDlgItemMessage(hWnd,IDC_ObjList,CB_GETCURSEL,0,0);				
				
				DbgPrint(">>Device close...");				
				if( !USB20Jtag_CloseDevice(CI) )
					DbgPrint("          Failure");
				else
				{
					DbgPrint("          Succ");
					AfxDevIsOpened = FALSE;
				}
				//更新按钮状态
				EnableButtonEnable();				
			}
			break;		
		case IDC_Jtag_BitWrite:
			Jtag_DataRW_Bit(FALSE); //JTAG写
			break;
		case IDC_Jtag_BitRead:
			Jtag_DataRW_Bit(TRUE); //JTAG读
			break;
		case IDC_Jtag_ByteWrite:
			Jtag_DataRW_Byte(FALSE); //JTAG写
			break;
		case IDC_Jtag_ByteRead:
			Jtag_DataRW_Byte(TRUE); //JTAG读
			break;
		case IDC_Jtag_DnFile_Exam:
			CloseHandle(CreateThread(NULL,0,DownloadFwFile,NULL,0,&ThreadID)); //JTAG写文件
			break;
		case IDC_JtagState_Switch:
			{
				UCHAR TapState;
				BOOL RetVal = FALSE;
				CHAR StatusStr[64]="";

				TapState = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_JtagTapState,CB_GETCURSEL,0,0);
				RetVal = USB20Jtag_SwitchTapState(TapState);
				GetDlgItemText(AfxMainHwnd,IDC_JtagTapState,StatusStr,sizeof(StatusStr));

				DbgPrint("Jtag_SwitchState %s.%s",RetVal?"succ":"failure",StatusStr);
			}
			break;
		case IDC_Jtag_InitTarget:
			Jtag_InitTarget();
			break;
		case IDC_JtaShiftgChannel:
			{
				CHAR BtName[64]="";
				ULONG DataReg;

				DataReg = SendDlgItemMessage(AfxMainHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0);

				sprintf(BtName,"JTAG BitRead%s(D2)",DataReg?"DR":"IR");
				SetDlgItemText(AfxMainHwnd,IDC_Jtag_BitRead,BtName);

				sprintf(BtName,"JTAG BitWrite%s(D1)",DataReg?"DR":"IR");
				SetDlgItemText(AfxMainHwnd,IDC_Jtag_BitWrite,BtName);

				sprintf(BtName,"JTAG ByteRead%s(D4)",DataReg?"DR":"IR");
				SetDlgItemText(AfxMainHwnd,IDC_Jtag_ByteRead,BtName);

				sprintf(BtName,"JTAG ByteWrite%s(D3)",DataReg?"DR":"IR");
				SetDlgItemText(AfxMainHwnd,IDC_Jtag_ByteWrite,BtName);
			}
			break;
		case IDC_JtagIfConfig:
			Jtag_InterfaceConfig();
			EnableButtonEnable(); //配置成功后，开启相关按钮操作
			break;
		case IDC_DataTransFunc:
			AfxDataTransType = (UCHAR)SendDlgItemMessage(AfxMainHwnd,IDC_DataTransFunc,CB_GETCURSEL,0,0);
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			CH375SetDeviceNotify(0,JtagUSBID,NULL);
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;		
	}
	return 0;
}

