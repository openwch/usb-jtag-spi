/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    Form program, providing SPI FLASH model identification, block read, block write, block erase, FLASH content read to file, file
    Write FLASH, speed test and other operations.
    Usb2.0 (480M high speed) to SPI based on CH32V305 MCU, can be used to construct
    Build custom USB high speed FASH programmer and other products.
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) device universal driver (CH372DRV) and upper computer routines.
    The current routine is a custom communication protocol, and the SPI transmission speed can reach 2MB/S
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/
#include "Main.h"

//全局变量
HWND AfxMainHwnd; //主窗体句柄
HINSTANCE AfxMainIns; //进程实例
BOOL AfxDevIsOpened;  //设备是否打开
ULONG Flash_Sector_Count;                                              /* FLASHNumber of sectors on the chip */
USHORT Flash_Sector_Size;                                              /* FLASHSector size of the chip */
BOOL AfxSpiIsCfg;

#define WM_USB20SpiDevArrive WM_USER+10         //The device inserts the notification event and the window process receives it
#define WM_USB20SpiDevRemove WM_USER+11         //Device unplug notification event received by window process

#define SpiUSBID "VID_1A86&PID_5537\0"

//函数声明
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//初始化窗体
VOID InitWindows()
{	
	//查找并显示设备列表 
	SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_OpenDevice),!AfxDevIsOpened);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CloseDevice),AfxDevIsOpened);
	//Flash地址斌初值
	SetDlgItemText(AfxMainHwnd,IDC_FlashStartAddr,"0");
	//Flash操作数斌初值
	SetDlgItemText(AfxMainHwnd,IDC_FlashDataSize,"100");
	//清空Flash数据模式
	SetDlgItemText(AfxMainHwnd,IDC_FlashData,"");
	//输出框设置显示的最大字符数
	SendDlgItemMessage(AfxMainHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	return;
}

// USB device plug detection notification program. Because the callback function has restrictions on function operations, the window message is moved to the message handler function for processing.
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH375_DEVICE_ARRIVAL)// Device insertion event, already inserted
		PostMessage(AfxMainHwnd,WM_USB20SpiDevArrive,0,0);
	else if(iEventStatus==CH375_DEVICE_REMOVE)// The device is removed
		PostMessage(AfxMainHwnd,WM_USB20SpiDevRemove,0,0);
	return;
}

// Enable button. JTAG must be enabled and configured first; otherwise, the button cannot be operated
VOID EnableButtonEnable()
{
	if(!AfxDevIsOpened)
		AfxSpiIsCfg = FALSE;

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CMD_InitSPI),AfxDevIsOpened);

	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_OpenDevice),!AfxDevIsOpened);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CloseDevice),AfxDevIsOpened);				

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashIdentify),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashRead),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashWrite),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashErase),AfxSpiIsCfg);

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_ReadToFile),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_WriteFormFile),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashVerify),AfxSpiIsCfg);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_FlashRWSpeedTest),AfxSpiIsCfg);	
}

//Enumeration device
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
	return 	DialogBox(hInstance, (LPCTSTR)IDD_MainWnd, 0, (DLGPROC)WndProc);
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
		//InitializeCriticalSection(&AfxDbgPrintCr);
			
		// Seed the random-number generator with current time so that
		// the numbers will be different every time we run.		
		srand( (unsigned)time( NULL ) );
		{//添加alt+tab切换时显示的图标
			HICON hicon;
			hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
		}
		{//初始化Listview控件，否则界面无法显示
			INITCOMMONCONTROLSEX InitComCtrl = {0};			

			InitComCtrl.dwSize = sizeof(InitComCtrl);
			InitComCtrl.dwICC = ICC_LISTVIEW_CLASSES;
			InitCommonControlsEx(&InitComCtrl);
		}
		AfxDevIsOpened = FALSE;		
		InitWindows(); //初始化窗体
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
		}
		{//1=36MHz, 2=18MHz, 3=9MHz, 4=4.5MHz, 5=2.25MHz, 6=1.125MHz, 7=562.5KHz	
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"   36MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"   18MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"    9MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  4.5MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)" 2.25MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1.125MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"562.5KHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_SETCURSEL,1,0);
		}	
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_SETCURSEL,0,0);
		}		
		SetDlgItemInt(AfxMainHwnd,IDC_TestLen,100,FALSE);
		SetEditlInputMode(hWnd,IDC_FlashData,1);
		EnableButtonEnable();

		//Set insert and unplug notifications for USB2.0JTAG devices. The device is automatically turned on after insertion and turned off after removal
		if(CH375SetDeviceNotify(0,SpiUSBID, UsbDevPnpNotify) )       // Device plug notification callback function
			DbgPrint("USB device plug - out monitoring has been enabled");
		break;	
	case WM_USB20SpiDevArrive:
		DbgPrint("Usb Device is arrived,open device");
		//先枚举USB设备
		SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
		//打开设备
		SendDlgItemMessage(AfxMainHwnd,IDC_OpenDevice,BM_CLICK,0,0);
		break;
	case WM_USB20SpiDevRemove:
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
			EnumDevice();
			break;
		case IDC_OpenDevice:
			OpenDevice();
			EnableButtonEnable();	
			break;
		case IDC_CloseDevice:			
			CloseDevice();				
			EnableButtonEnable();	
			break;	
		case IDC_FlashIdentify:
			FlashIdentify();
			break;
		case IDC_FlashRead:
			FlashBlockRead();
			break;
		case IDC_FlashWrite://
			FlashBlockWrite();
			break;
		case IDC_FlashErase:
			FlashBlockErase();
			break;	
		case IDC_CMD_InitSPI:			
			InitFlashSPI();
			EnableButtonEnable();
			break;
		case IDC_FlashVerify:
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID));
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID)); 
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID)); 
			break;
		case IDC_FlashRWSpeedTest://
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID)); 
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			CH375SetDeviceNotify(0,SpiUSBID,NULL);
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

