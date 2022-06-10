/********************************** (C) COPYRIGHT *******************************
* File Name          : USB_Desc.h
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : 主文件相关头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/


#ifndef __USB_DESC_H__
#define __USB_DESC_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* 头文件包含 */
#include <stdint.h>
#include "MAIN.h"                                                               /* 头文件包含 */


/******************************************************************************/
/* 以下为USB相关描述符 */

/******************************************************************************/
/* USB设备描述符 */
const UINT8  MyDevDescr[ 18 ] =
{
    0x12, 0x01, 0x00, 0x02, 0xFF, 0x80, 0x55, 0x40,
    (UINT8)( DEF_USB_VID ), (UINT8)( DEF_USB_VID >> 8 ), (UINT8)( DEF_USB_PID ), (UINT8)( DEF_USB_PID >> 8 ),
    DEF_IC_PRG_VER, 0x00, 0x01, 0x02, 0x03, 0x01
};

/* USB配置描述符(全速) */
const UINT8  MyCfgDescr_FS[ ] =
{
    /* 配置描述符 */
    0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,

    /* 接口描述符 */
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0x00,

    /* 端点描述符 */
    0x07, 0x05, 0x02, 0x02, (UINT8)DEF_USB_EP2_FS_SIZE, (UINT8)( DEF_USB_EP2_FS_SIZE >> 8 ), 0x00,

    /* 端点描述符 */
    0x07, 0x05, 0x81, 0x02, (UINT8)DEF_USB_EP2_FS_SIZE, (UINT8)( DEF_USB_EP2_FS_SIZE >> 8 ), 0x00
};

/* USB配置描述符-(高速) */
const UINT8  MyCfgDescr_HS[ ] =
{
    /* 配置描述符 */
    0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,

    /* 接口描述符 */
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0x00,

    /* 端点描述符 */
    0x07, 0x05, 0x02, 0x02, (UINT8)DEF_USB_EP2_HS_SIZE, (UINT8)( DEF_USB_EP2_HS_SIZE >> 8 ), 0x00,

    /* 端点描述符 */
    0x07, 0x05, 0x81, 0x02, (UINT8)DEF_USB_EP2_HS_SIZE, (UINT8)( DEF_USB_EP2_HS_SIZE >> 8 ), 0x00
};


/* USB语言字符串描述符 */
const UINT8  MyLangDescr[ ] =
{
    0x04, 0x03, 0x09, 0x04
};

/* USB产商字符串描述符 */
const UINT8  MyManuInfo[ ] =
{
    0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0
};

/* USB产品字符串描述符 */
const UINT8  MyProdInfo[ ] =
{
    0x2C, 0x03, 'U', 0x00, 'S', 0x00, 'B', 0x00, ' ', 0x00, 'T', 0x00, 'o', 0x00, ' ', 0x00,
    'H',  0x00, 'i', 0x00, 'g', 0x00, 'h', 0x00, 'S', 0x00, 'p', 0x00, 'e', 0x00, 'e', 0x00,
    'd',  0x00, ' ', 0x00, 'J', 0x00, 'T', 0x00, 'A', 0x00, 'G',  0x00
};

/* USB序列号字符串描述符 */
const UINT8  MySerNumInfo[ ] =
{
    0x16,  0x03, '0', 0, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0, '9', 0
};

/* USB设备限定描述符 */
const UINT8 MyUSBQUADesc[ ] =
{
    0x0A, 0x06, 0x00, 0x02, 0xFF, 0x00, 0xFF, 0x40, 0x01, 0x00,
};

/* USB全速模式,其他速度配置描述符 */
UINT8 TAB_USB_FS_OSC_DESC[ sizeof( MyCfgDescr_HS ) ] =
{
    0x09, 0x07,                                                                 /* 其他部分通过程序复制 */
};

/* USB高速模式,其他速度配置描述符 */
UINT8 TAB_USB_HS_OSC_DESC[ sizeof( MyCfgDescr_FS ) ] =
{
    0x09, 0x07,                                                                 /* 其他部分通过程序复制 */
};


#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
