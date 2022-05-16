/******************** (C) COPYRIGHT ********************************************
* File Name          : PRINTF.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : 串口打印输出控制头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef	__PRINTF_H__
#define __PRINTF_H__

/*******************************************************************************/
/* 定义串口打印输出开关 */
#define MY_DEBUG_PRINTF            1				 				 			/* 常归打印监视开关 */

/* 如果定义上面的宏则通过串口打印输出,否则不处理 */
#if( MY_DEBUG_PRINTF == 1 )
#define DUG_PRINTF( format, arg... )    printf( format, ##arg )		 			/* 串口打印输出 */
#else
#define DUG_PRINTF( format, arg... )    do{ if( 0 )printf( format, ##arg ); }while( 0 );
#endif

#endif

