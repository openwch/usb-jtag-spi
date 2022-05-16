/********************************** (C) COPYRIGHT *******************************
* File Name          : MAIN.C
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/18
* Description        : 采用CH32V305\307芯片实现USB2.0(480M high speed)转高速JTAG/SPI功能
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "MAIN.h"                                                               /* 头文件包含 */
#include "debug.h"

/******************************************************************************/
/* 常、变量定义 */
#if( DEF_WWDG_FUN_EN == 0x01 )
UINT8  WWDG_Tr, WWDG_Wr;
#endif

/*******************************************************************************
* Function Name o : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main( void )
{
    RCC_ClocksTypeDef   RCC_Clocks;

    /*****************************系统相关初始化*******************************/
    Delay_Init( );
    USART_Printf_Init( 921600 );
    RCC_Configuration( );                                                       /* 系统时钟配置 */
    NVIC_Configuration( );                                                      /* 系统中断配置 */
    RCC_GetClocksFreq( &RCC_Clocks );                                           /* 获取系统时钟频率 */

    /*****************************模块所有引脚初始化**************************/
    DUG_PRINTF("USB_JTAG_SPI_V1.%d\n",(UINT16)DEF_IC_PRG_VER - 0x01 );
    DUG_PRINTF("Edit Date and Time is: "__DATE__"  " __TIME__"\n");

    /*****************************定时器初始化************************************/
    TIM2_Init( );                                                               /* 定时器2初始化 */

    /* JTAG初始化 */
    DUG_PRINTF("JTAG Init\n");
    JTAG_Init( );
    JTAG_SPI_Init( SPI_BaudRatePrescaler_4 );
    DUG_PRINTF("JTAG_Mode:%x\n",JTAG_Mode);
    DUG_PRINTF("JTAG_Speed:%x\n",JTAG_Speed);

    /* SPI接口初始化 */
    DUG_PRINTF("SPI Init\n");
    SPIx_Cfg_DefInit( );
    SPIx_Tx_DMA_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)&SPI_TxDMA_Buf[ 0 ], 0x00 );
    SPIx_Rx_DMA_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)&SPI_Com_Buf[ 0 ], 0x00 );

    /*****************************USB从机相关初始化******************************/
    DUG_PRINTF("USBHS Init\n");
    USBHS_Device_Init( ENABLE );
    NVIC_EnableIRQ( USBHS_IRQn );

    /*****************************主程序**************************************/
    DUG_PRINTF("Main\n");

#if( DEF_WWDG_FUN_EN == 0x01 )
    /* WWDG看门狗初始化 */
    WWDG_Config( 0x7F, 0x5F, WWDG_Prescaler_8 );
    WWDG_Wr = WWDG->CFGR & 0x7F;
#endif

    while( 1 )
    {
        /*************************************************************/
        /* USB转JTAG处理 */
        if( JTAG_Mode == 0 )
        {
            COMM_CMDPack_Deal( );
        }
        else
        {
            JTAG_Mode1_Deal( );
        }

        /*************************************************************/
        /* 看门狗喂狗 */
#if( DEF_WWDG_FUN_EN == 0x01 )
        WWDG_Tr = WWDG->CTLR & 0x7F;
        if( WWDG_Tr < WWDG_Wr )
        {
            WWDG->CTLR = WWDG_CNT;
        }
#endif
    }
}




