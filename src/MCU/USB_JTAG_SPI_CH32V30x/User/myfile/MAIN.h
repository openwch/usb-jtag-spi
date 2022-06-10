/********************************** (C) COPYRIGHT *******************************
* File Name          : MAIN.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/18
* Description        : ���ļ����ͷ�ļ�
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* ͷ�ļ����� */
#include <stdio.h>
#include <string.h>													
#include "debug.h"
#include "string.h"
#include <PRINTF.h>									                			/* ���ڵ��Դ�ӡ�������ͷ�ļ� */ 
#include <TYPE.h>										                		/* �������Ͷ������ͷ�ļ� */ 	
#include <HARDWARE.h>		 								                    /* ����Ӳ�����Ŷ������ͷ�ļ� */
#include "ch32vf30x_usb_device.h"
#include "ch32v30x_conf.h"
#include "ch32v30x_usb.h"
#include "JTAG.h"
#include "JTAG_Port.h"

/*******************************************************************************/
/* ģ��Ӳ��������汾�� */
#define DEF_IC_PRG_VER             ( 0x01 )                                     /* ����̼��汾 */

/*******************************************************************************/
/* ����������غ궨�� */
#define DEF_WWDG_FUN_EN            0x01                                         /* WWDG����ʹ����غ궨�� */
#define DEF_USBSLEEP_FUN_EN        0x00                                         /* USB���߹���ʹ����غ궨�� */
#define DEF_DEBUG_FUN_EN           0x00                                         /* ���Թ���ʹ����غ궨�� */

/* USB�豸VID-PID���� */
#define DEF_USB_VID                0x1A86                                       /* оƬUSB��Ĭ��VID */
#define DEF_USB_PID                0x55DE                                       /* оƬUSB��Ĭ��PID */

/******************************************************************************/
/* �������� */

/********************************************************************************/
/* �������� */
#if( DEF_WWDG_FUN_EN == 0x01 )
extern UINT8  WWDG_Tr, WWDG_Wr;
#endif

#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
