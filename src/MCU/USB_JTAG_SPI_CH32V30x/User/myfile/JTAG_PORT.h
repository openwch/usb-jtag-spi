/********************************** (C) COPYRIGHT *******************************
* File Name          : JTAG_PORT.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USB转JTAG硬件底层操作相关头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef __JTAG_PORT_H__
#define __JTAG_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* 头文件包含 */
#include "debug.h"

/******************************************************************************/
/* 相关宏定义 */
#define DEF_SHIFT_MODE             ( 0x80 )
#define DEF_READ_MODE              ( 0x40 )
#define DEF_CNT_MASK               ( 0x3F )

#define DEF_TRST_BIT_OUT           ( 0x20 )
#define DEF_TDI_BIT_OUT            ( 0x10 )
#define DEF_NCS_BIT_OUT            ( 0x08 )
#define DEF_NCE_BIT_OUT            ( 0x04 )
#define DEF_TMS_BIT_OUT            ( 0x02 )
#define DEF_TCK_BIT_OUT            ( 0x01 )
#define DEF_TDO_BIT_IN             ( 0 )

/* JTAG引脚操作相关宏定义 */
/* TCK: PB13 */
#define PIN_TCK_OUT( d )           if( d ) GPIOB->BSHR = GPIO_Pin_13; else GPIOB->BCR = GPIO_Pin_13;
#define PIN_TCK_0( )               GPIOB->BCR = GPIO_Pin_13
#define PIN_TCK_1( )               GPIOB->BSHR = GPIO_Pin_13

/* TDO: PB14 */
#define PIN_TDO_IN( )              ( GPIOB->INDR & GPIO_Pin_14 ) ? 1 : 0

/* TDI: PB15 */
#define PIN_TDI_OUT( d )           if( d ) GPIOB->BSHR = GPIO_Pin_15; else GPIOB->BCR = GPIO_Pin_15;

/* TMS: PB12 */
#define PIN_TMS_OUT( d )           if( d ) GPIOB->BSHR = GPIO_Pin_12; else GPIOB->BCR = GPIO_Pin_12;

/* TRST: PC6 */
#define PIN_TRST_OUT( d )          if( d ) GPIOC->BSHR = GPIO_Pin_6; else GPIOC->BCR = GPIO_Pin_6;  

/******************************************************************************/
/* 变量外扩 */

/********************************************************************************/
/* 函数外扩 */
extern void JTAG_Port_Init( void );                                             /* JTAG接口硬件初始化 */
extern void JTAG_Port_BitShift( UINT8 dat );                                    /* JTAG接口位输出 */
extern UINT8 JTAG_Port_BitRead( void );                                         /* JTAG接口位读取 */
extern void JTAG_Port_DataShift( UINT8 dat );                                   /* JTAG接口数据移位输出 */
extern UINT8 JTAG_Port_DataShift_RD( UINT8 dat );                               /* JTAG接口数据移位输出并读取 */

#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
