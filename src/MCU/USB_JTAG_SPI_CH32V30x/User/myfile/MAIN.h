/********************************** (C) COPYRIGHT *******************************
* File Name          : MAIN.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/18
* Description        : 主文件相关头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* 头文件包含 */
#include <stdio.h>
#include <string.h>													
#include "debug.h"
#include "string.h"
#include <PRINTF.h>									                			/* 串口调试打印开关相关头文件 */ 
#include <TYPE.h>										                		/* 数据类型定义相关头文件 */ 	
#include <HARDWARE.h>		 								                    /* 所有硬件引脚定义相关头文件 */
#include "ch32vf30x_usb_device.h"
#include "ch32v30x_conf.h"
#include "ch32v30x_usb.h"
#include "JTAG.h"
#include "JTAG_Port.h"

/*******************************************************************************/
/* 模块硬件、软件版本号 */
#define DEF_IC_PRG_VER             ( 0x01 )                                     /* 程序固件版本 */

/*******************************************************************************/
/* 功能配置相关宏定义 */
#define DEF_WWDG_FUN_EN            0x01                                         /* WWDG功能使能相关宏定义 */
#define DEF_USBSLEEP_FUN_EN        0x00                                         /* USB休眠功能使能相关宏定义 */
#define DEF_DEBUG_FUN_EN           0x00                                         /* 调试功能使能相关宏定义 */

/* USB设备VID-PID定义 */
#define DEF_USB_VID                0x1A86                                       /* 芯片USB的默认VID */
#define DEF_USB_PID                0x55DE                                       /* 芯片USB的默认PID */

/******************************************************************************/
/* 变量外扩 */

/********************************************************************************/
/* 函数外扩 */
#if( DEF_WWDG_FUN_EN == 0x01 )
extern UINT8  WWDG_Tr, WWDG_Wr;
#endif

#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
