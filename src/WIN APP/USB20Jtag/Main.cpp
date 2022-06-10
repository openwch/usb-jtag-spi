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

//ȫ�ֱ���
HWND AfxMainHwnd;     //��������
HINSTANCE AfxMainIns; //����ʵ��
BOOL AfxDevIsOpened;  //�豸�Ƿ��
HINSTANCE MainInstan;
UCHAR AfxDataTransType=0;
BOOL AfxJtagIsCfg = FALSE;

#define WM_JtagDevArrive WM_USER+10         //�豸����֪ͨ�¼�,������̽���
#define WM_JtagDevRemove WM_USER+11         //�豸�γ�֪ͨ�¼�,������̽���

#define JtagUSBID "VID_1A86&PID_5537\0"

//��������
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//��ʼ������
VOID InitWindows()
{
	//���Ҳ���ʾ�豸�б� 
	SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_OpenDevice),!AfxDevIsOpened);
	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_CloseDevice),AfxDevIsOpened);	
	//�����������ʾ������ַ���
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
	SetFocus(GetDlgItem(AfxMainHwnd,IDC_JtaShiftgChannel)); //���²�����ť״̬

	{
		//JTAG�ٶ�����ֵ����ЧֵΪ0-5��ֵԽ��ͨ���ٶ�Խ�죻
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"2.25MHz");    //speed 0��2.25MHZ
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)" 4.5MHz");    //speed 1��4.5MHZ��
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"   9MHz");    //speed2��9MHZ��
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  18MHz");    //speed3��18MHZ��
		SendDlgItemMessage(AfxMainHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  36MHz");    //speed4��36MHZ��
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

//ʹ�ܲ�����ť�����ȴ򿪺�����JTAG�������޷�����
VOID EnableButtonEnable()
{
	if(!AfxDevIsOpened)
		AfxJtagIsCfg = FALSE;

	EnableWindow(GetDlgItem(AfxMainHwnd,IDC_JtagIfConfig),AfxDevIsOpened);

	//���´�/�ر��豸��ť״̬
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

// USB�豸��μ��֪ͨ����.��ص������Ժ������������ƣ�ͨ��������Ϣת�Ƶ���Ϣ�������ڽ��д���
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH375_DEVICE_ARRIVAL)// �豸�����¼�,�Ѿ�����
		PostMessage(AfxMainHwnd,WM_JtagDevArrive,0,0);
	else if(iEventStatus==CH375_DEVICE_REMOVE)// �豸�γ��¼�,�Ѿ��γ�
		PostMessage(AfxMainHwnd,WM_JtagDevRemove,0,0);
	return;
}

//ö���豸
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

//Ӧ�ó������
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	AfxMainIns = hInstance;
	return 	DialogBox(hInstance, (LPCTSTR)IDD_MainWnd_EN, 0, (DLGPROC)WndProc);
}
//���������
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
		{//���alt+tab�л�ʱ��ʾ��ͼ��
			HICON hicon;
			hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
			PostMessage(AfxMainHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
		}
		AfxDevIsOpened = FALSE;		
		InitWindows(); //��ʼ������			
		EnableButtonEnable(); //ʹ�ܲ�����ť�����ȴ򿪺�����JTAG�������޷�����
		//ΪUSB2.0JTAG�豸���ò���Ͱγ���֪ͨ.������Զ����豸,�γ���ر��豸
		CH375SetDeviceNotify(0,
			JtagUSBID,  //�豸USB ID
			UsbDevPnpNotify);       //�豸���֪ͨ�ص�����

		break;	
	case WM_JtagDevArrive:
			DbgPrint("Usb Device is arrived,open device");
			//��ö��USB�豸
			SendDlgItemMessage(AfxMainHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
			//���豸
			SendDlgItemMessage(AfxMainHwnd,IDC_OpenDevice,BM_CLICK,0,0);
			break;
	case WM_JtagDevRemove:
		DbgPrint("Usb Device is removed,close device");
		//�ر��豸
		SendDlgItemMessage(AfxMainHwnd,IDC_CloseDevice,BM_CLICK,0,0);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{		
		case IDC_RefreshObjList:
			EnumDevice(); //ö�ٲ���ʾ�豸
			break;
		case IDC_OpenDevice://���豸
			{
				ULONG CI;
				//��ȡ�豸���
				CI = SendDlgItemMessage(hWnd,IDC_ObjList,CB_GETCURSEL,0,0);
				if(CI==CB_ERR)
				{
					DbgPrint("Open device failure.");
					break; //�˳�
				}
				DbgPrint(">>Device Open...");
				AfxDevIsOpened = USB20Jtag_OpenDevice(CI);
				if( AfxDevIsOpened ) //���豸
					DbgPrint("          Succ");
				else
					DbgPrint("          Failure");
				EnableButtonEnable();//���°�ť״̬				
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
				//���°�ť״̬
				EnableButtonEnable();				
			}
			break;		
		case IDC_Jtag_BitWrite:
			Jtag_DataRW_Bit(FALSE); //JTAGд
			break;
		case IDC_Jtag_BitRead:
			Jtag_DataRW_Bit(TRUE); //JTAG��
			break;
		case IDC_Jtag_ByteWrite:
			Jtag_DataRW_Byte(FALSE); //JTAGд
			break;
		case IDC_Jtag_ByteRead:
			Jtag_DataRW_Byte(TRUE); //JTAG��
			break;
		case IDC_Jtag_DnFile_Exam:
			CloseHandle(CreateThread(NULL,0,DownloadFwFile,NULL,0,&ThreadID)); //JTAGд�ļ�
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
			EnableButtonEnable(); //���óɹ��󣬿�����ذ�ť����
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

