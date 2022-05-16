/******************** (C) COPYRIGHT 2011 WCH ***********************************
* File Name          : JTAG_PORT.C
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : JTAG硬件底层操作
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include <MAIN.h>                                                               /* 头文件包含 */
#include "stdio.h"
#include "stdlib.h"
#include "JTAG_Port.h"
#include "debug.h"

/*******************************************************************************/
/* 常、变量宏定义 */

/*******************************************************************************
* Function Name  : JTAG_Port_Init
* Description    : JTAG接口硬件初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_Init( void )
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );

    /* GPIO Out Configuration: TCK(PB13), TDI(PB15), TMS(PB12) */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIO In Configuration: TDO(PB14) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIO Out Configuration: TRST(PC6) */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init( GPIOC, &GPIO_InitStructure );
}

/*******************************************************************************
* Function Name  : JTAG_Port_BitShift
* Description    : JTAG接口位输出
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_BitShift( UINT8 dat )
{
    PIN_TDI_OUT( 0 != ( dat & DEF_TDI_BIT_OUT ) );
    PIN_TMS_OUT( 0 != ( dat & DEF_TMS_BIT_OUT ) );
    PIN_TCK_OUT( 0 != ( dat & DEF_TCK_BIT_OUT ) );
    PIN_TRST_OUT( 0 != ( dat & DEF_TRST_BIT_OUT ) );
}

/*******************************************************************************
* Function Name  : JTAG_Port_BitRead
* Description    : JTAG接口位读取
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 JTAG_Port_BitRead( void )
{
    UINT32  dat = 0;

    dat |= PIN_TDO_IN( ) << DEF_TDO_BIT_IN;
    return(UINT8)dat;
}

/*******************************************************************************
* Function Name  : JTAG_Port_DataShift
* Description    : JTAG接口数据移位输出
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_DataShift( UINT8 dat )
{
    PIN_TDI_OUT( dat & 0x01 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x02 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x04 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x08 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x10 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x20 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x40 );
    PIN_TCK_1( );
    PIN_TCK_0( );

    PIN_TDI_OUT( dat & 0x80 );
    PIN_TCK_1( );
    PIN_TCK_0( );
}

/*******************************************************************************
* Function Name  : JTAG_Port_DataShift
* Description    : JTAG接口数据移位输出并读取
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 JTAG_Port_DataShift_RD( UINT8 dat )
{
    UINT32 dout = dat;
    UINT32 din;

#define JTGA_SHIFT_BIT( )          PIN_TDI_OUT( dout & 0x01 ); din = PIN_TDO_IN( ); PIN_TCK_1( ); dout = ( dout >> 1 ) | ( din << 7 ); PIN_TCK_0( )

    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );
    JTGA_SHIFT_BIT( );

    return( dout & 0xff );
}

