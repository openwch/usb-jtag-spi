/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_usb_device.c
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USB高速操作相关文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "main.h"
#include "USB_Desc.h"
#include "JTAG.h"

/******************************************************************************/
/* 常、变量定义 */

/* 函数声明 */
void USBHS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/* USB缓冲区定义 */
__attribute__ ((aligned(16))) UINT8 EP0_Databuf[ USBHS_UEP0_SIZE ]          __attribute__((section(".DMADATA"))); /*端点0数据收发缓冲区*/
__attribute__ ((aligned(16))) UINT8 EP1_Rx_Databuf[ DEF_USB_EP1_FS_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点1数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP1_Tx_Databuf[ DEF_USB_EP1_FS_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点1数据发送缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP2_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点2数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP2_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点2数据发送缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP3_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点3数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP3_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点3数据发送缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP4_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点4数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP4_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点4数据发送缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP6_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点6数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP6_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点6数据发送缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP8_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点8数据接收缓冲区 */
__attribute__ ((aligned(16))) UINT8 EP8_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* 端点86数据发送缓冲区 */


#define	pSetupReqPak		((PUSB_SETUP_REQ)EP0_Databuf)
const UINT8 *pDescr;
volatile UINT8  USBHS_Dev_SetupReqCode = 0xFF;									/* USB2.0高速设备Setup包命令码 */
volatile UINT16 USBHS_Dev_SetupReqLen = 0x00;									/* USB2.0高速设备Setup包长度 */
volatile UINT8  USBHS_Dev_SetupReqValueH = 0x00;								/* USB2.0高速设备Setup包Value高字节 */	
volatile UINT8  USBHS_Dev_Config = 0x00;										/* USB2.0高速设备配置值 */
volatile UINT8  USBHS_Dev_Address = 0x00;										/* USB2.0高速设备地址值 */
volatile UINT8  USBHS_Dev_SleepStatus = 0x00;                                   /* USB2.0高速设备睡眠状态 */
volatile UINT8  USBHS_Dev_EnumStatus = 0x00;                                    /* USB2.0高速设备枚举状态 */
volatile UINT8  USBHS_Dev_Endp0_Tog = 0x01;                                     /* USB2.0高速设备端点0同步标志 */
volatile UINT8  USBHS_Dev_Speed = 0x01;                                    		/* USB2.0高速设备速度 */
volatile UINT16 USBHS_Up_PackLenMax = 512;                                      /* USB2.0高速设备当前允许上传的包长度最大值(全速64高速512) */

UINT8  USB_Temp_Buf[ 64 ];                                                      /* USB临时缓冲区 */

volatile UINT8 FLAG_Send = 1;

/*******************************************************************************
* Function Name  : USBHS_RCC_Init
* Description    : USB2.0高速设备RCC初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_RCC_Init( void )
{
    RCC->CFGR2 = USBHS_PLL_SRC_HSE | USBHS_PLL_SRC_PRE_DIV2 | USBHS_PLL_CKREF_4M; /* PLL REF = HSE/2 = 4MHz */
    RCC->CFGR2 |= USB_48M_CLK_SRC_PHY | USBHS_PLL_ALIVE;
    RCC->AHBPCENR |= ( (uint32_t)( 1 << 11 ) );
    Delay_Us( 200 );
    USBHSH->HOST_CTRL |= PHY_SUSPENDM;
    Delay_Us( 5 );
}

/*******************************************************************************
* Function Name  : USBHS_Device_Endp_Init
* Description    : USB2.0高速设备端点初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Device_Endp_Init ( void )
{
    /* 使能端点1、端点2发送和接收  */
	USBHSD->ENDP_CONFIG = USBHS_EP0_T_EN | USBHS_EP0_R_EN | USBHS_EP1_T_EN | USBHS_EP1_R_EN |
	                      USBHS_EP2_T_EN | USBHS_EP2_R_EN | USBHS_EP3_T_EN | USBHS_EP3_R_EN |
	                      USBHS_EP4_T_EN | USBHS_EP4_R_EN | USBHS_EP5_T_EN | USBHS_EP5_R_EN |
	                      USBHS_EP6_T_EN | USBHS_EP6_R_EN | USBHS_EP7_T_EN | USBHS_EP7_R_EN |
	                      USBHS_EP8_T_EN | USBHS_EP8_R_EN;

    /* 端点非同步端点 */
	USBHSD->ENDP_TYPE = 0x00;

    /* 端点缓冲区模式，非双缓冲区，ISO传输BUF模式需要指定0  */
	USBHSD->BUF_MODE = 0x00;

    /* 端点最大长度包配置 */
    USBHSD->UEP0_MAX_LEN = 64;
    USBHSD->UEP1_MAX_LEN = 512;
    USBHSD->UEP2_MAX_LEN = 512;
    USBHSD->UEP3_MAX_LEN = 512;
    USBHSD->UEP4_MAX_LEN = 512;
    USBHSD->UEP5_MAX_LEN = 512;
    USBHSD->UEP6_MAX_LEN = 512;
    USBHSD->UEP7_MAX_LEN = 512;
    USBHSD->UEP8_MAX_LEN = 512;
    USBHSD->UEP9_MAX_LEN = 512;
    USBHSD->UEP10_MAX_LEN = 512;
    USBHSD->UEP11_MAX_LEN = 512;
    USBHSD->UEP12_MAX_LEN = 512;
    USBHSD->UEP13_MAX_LEN = 512;
    USBHSD->UEP14_MAX_LEN = 512;
    USBHSD->UEP15_MAX_LEN = 512;

    /* 端点DMA地址配置 */
    USBHSD->UEP0_DMA   = (UINT32)(UINT8 *)EP0_Databuf;
    USBHSD->UEP1_TX_DMA = (UINT32)(UINT8 *)EP1_Tx_Databuf;
    USBHSD->UEP1_RX_DMA = (UINT32)(UINT8 *)EP1_Rx_Databuf;
    USBHSD->UEP2_TX_DMA = (UINT32)(UINT8 *)EP2_Tx_Databuf;
    USBHSD->UEP2_RX_DMA = (UINT32)(UINT8 *)&Comm_Tx_Buf[ 0 ];
    USBHSD->UEP3_TX_DMA = (UINT32)(UINT8 *)EP3_Tx_Databuf;
    USBHSD->UEP3_RX_DMA = (UINT32)(UINT8 *)EP3_Tx_Databuf;
    USBHSD->UEP4_TX_DMA = (UINT32)(UINT8 *)EP4_Tx_Databuf;
    USBHSD->UEP4_RX_DMA = (UINT32)(UINT8 *)EP4_Rx_Databuf;
    USBHSD->UEP6_TX_DMA = (UINT32)(UINT8 *)EP6_Tx_Databuf;
    USBHSD->UEP6_RX_DMA = (UINT32)(UINT8 *)EP6_Rx_Databuf;
    USBHSD->UEP8_TX_DMA = (UINT32)(UINT8 *)EP8_Tx_Databuf;
    USBHSD->UEP8_RX_DMA = (UINT32)(UINT8 *)EP8_Rx_Databuf;

    /* 端点控制寄存器配置(所使用的端点配置成手动翻转) */
    USBHS_Dev_Endp0_Tog = 0x01;
    USBHSD->UEP0_TX_LEN  = 0;
    USBHSD->UEP0_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP0_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP1_TX_LEN  = 0;
    USBHSD->UEP1_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP1_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP2_TX_LEN  = 0;
    USBHSD->UEP2_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP2_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP3_TX_LEN  = 0;
    USBHSD->UEP3_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP3_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP4_TX_LEN  = 0;
    USBHSD->UEP4_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP4_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP5_TX_LEN  = 0;
    USBHSD->UEP5_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP5_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP6_TX_LEN  = 0;
    USBHSD->UEP6_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP6_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP7_TX_LEN  = 0;
    USBHSD->UEP7_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP7_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP8_TX_LEN  = 0;
    USBHSD->UEP8_TX_CTRL = USBHS_EP_T_RES_NAK;
    USBHSD->UEP8_RX_CTRL = USBHS_EP_R_RES_ACK;

    USBHSD->UEP9_TX_LEN  = 0;
    USBHSD->UEP9_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP9_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP10_TX_LEN  = 0;
    USBHSD->UEP10_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP10_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP11_TX_LEN  = 0;
    USBHSD->UEP11_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP11_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP12_TX_LEN  = 0;
    USBHSD->UEP12_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP12_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP13_TX_LEN  = 0;
    USBHSD->UEP13_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP13_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP14_TX_LEN  = 0;
    USBHSD->UEP14_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP14_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    USBHSD->UEP15_TX_LEN  = 0;
    USBHSD->UEP15_TX_CTRL = USBHS_EP_T_AUTOTOG | USBHS_EP_T_RES_NAK;
    USBHSD->UEP15_RX_CTRL = USBHS_EP_R_AUTOTOG | USBHS_EP_R_RES_ACK;

    /* 相关变量初始化 */
    USBHS_Dev_Speed = 0x01;
    USBHS_Up_PackLenMax = 512;
}

/*******************************************************************************
* Function Name  : USBHS_Device_Init
* Description    : USB2.0高速设备初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Device_Init ( FunctionalState sta )
{
    /* USB2.0高速设备RCC初始化 */
    USBHS_RCC_Init( );

    /* 配置DMA、速度、端点使能等 */
    USBHSD->HOST_CTRL = 0x00;
    USBHSD->HOST_CTRL = USBHS_SUSPENDM;
    USBHSD->CONTROL = 0x00;
    USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_HIGH_SPEED;
//  USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_FULL_SPEED;
//  USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_LOW_SPEED;
    USBHSD->INT_EN = 0;
    USBHSD->INT_EN = USBHS_SETUP_ACT_EN | USBHS_TRANSFER_EN | USBHS_DETECT_EN | USBHS_SUSPEND_EN;
    USBHSD->ENDP_CONFIG = 0xffffffff;

    /* USB2.0高速设备端点初始化 */
    USBHS_Device_Endp_Init( );
    Delay_Us( 10 );
    
    /* 使能USB连接 */
    if( sta == ENABLE )
    {
        USBHSD->CONTROL |= USBHS_DEV_PU_EN;
    }
    else
    {
        USBHSD->CONTROL &= ~USBHS_DEV_PU_EN;
        USBHSD->CONTROL |= USBHS_ALL_CLR | USBHS_FORCE_RST;
    }
}

/*******************************************************************************
* Function Name  : USBHS_Device_SetAddress
* Description    : USB2.0高速设备设置设备地址
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Device_SetAddress( UINT32 address )
{
    USBHSD->DEV_AD = 0;
    USBHSD->DEV_AD = address & 0xff;
}

/*******************************************************************************
* Function Name  : USBHS_IRQHandler
* Description    : USB2.0高速设备中断服务程序
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_IRQHandler( void )
{
    UINT8  usb_intstatus;
    UINT32 end_num;
    UINT32 rx_token;
    UINT16 len = 0x00;
    UINT8  errflag = 0x00;
    UINT8  chtype;
    UINT16 temp8;

    usb_intstatus = USBHSD->INT_FG;
    if( usb_intstatus & USBHS_TRANSFER_FLAG )
    {
        /* 端点传输处理 */
        end_num  = (USBHSD->INT_ST) & MASK_UIS_ENDP;
        rx_token = ( ( (USBHSD->INT_ST) & MASK_UIS_TOKEN ) >> 4 ) & 0x03;       /* 00: OUT, 01:SOF, 10:IN, 11:SETUP */
#if 0
        if( !( USBHS->USB_STATUS & TOG_MATCH ) )
        {
            if( end_num )
            {
                DUG_PRINTF(" TOG MATCH FAIL : ENDP %x token %x \n", end_num, rx_token);
            }
        }
#endif
        if( end_num == 2 )
        {
            if( rx_token == PID_IN ) 
            {
                /* USB端点2上传 */
                USBHSD->UEP2_TX_CTRL ^= USBHS_EP_T_TOG_1;
                USBHSD->UEP2_TX_CTRL |= USBHS_EP_T_RES_NAK;
            }
            else if( rx_token == PID_OUT ) 
            {
                /* USB端点2下传 */
                USBHSD->UEP2_RX_CTRL ^= USBHS_EP_R_TOG_1;

                /* 记录相关信息,并切换DMA地址 */
                COMM.CMDP_PackLen[ COMM.CMDP_LoadNum ] = USBHSD->RX_LEN;
                COMM.CMDP_LoadNum++;
                USBHSD->UEP2_RX_DMA = (UINT32)(UINT8 *)&Comm_Tx_Buf[ ( COMM.CMDP_LoadNum * DEF_USB_HS_PACK_LEN ) ];
                if( COMM.CMDP_LoadNum >= DEF_COMM_BUF_PACKNUM_MAX )
                {
                    COMM.CMDP_LoadNum = 0x00;
                    USBHSD->UEP2_RX_DMA = (UINT32)(UINT8 *)&Comm_Tx_Buf[ 0 ];
                }
                COMM.CMDP_RemainNum++;

                /* 判断是否需要暂停下传 */
                if( COMM.CMDP_RemainNum >= ( DEF_COMM_BUF_PACKNUM_MAX - 2 ) )
                {
                    USBHSD->UEP2_RX_CTRL &= ~USBHS_EP_R_RES_MASK;
                    USBHSD->UEP2_RX_CTRL |= USBHS_EP_R_RES_NAK;
                    COMM.USB_Down_StopFlag = 0x01;
                }
            }
        }
        else if( end_num == 1 )
        {
            if( rx_token == PID_IN ) 
            {
                /* USB 端点1上传 */
                USBHSD->UEP1_TX_CTRL ^= USBHS_EP_T_TOG_1;
                USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_NAK;

                /* 处理JTAG数据上传 */
                if( COMM.USB_Up_IngFlag )
                {
                    COMM.USB_Up_IngFlag = 0x00;

                    COMM.Rx_DealPtr += COMM.USB_Up_PackLen;
                    if( COMM.Rx_DealPtr >= DEF_COMM_RX_BUF_LEN )
                    {
                        COMM.Rx_DealPtr = 0x00;
                    }
                    COMM.Rx_RemainLen -= COMM.USB_Up_PackLen;
                    COMM.USB_Up_PackLen = 0x00;
                }
            }
            else if( rx_token == PID_OUT ) 
            {
                /* USB 端点1下传 */
                USBHSD->UEP1_RX_CTRL ^= USBHS_EP_R_TOG_1;
                USBHSD->UEP1_RX_CTRL &= ~ USBHS_EP_R_RES_MASK;
                USBHSD->UEP1_RX_CTRL |= USBHS_EP_R_RES_ACK;
            }
        }
        else if( end_num == 0 )
        {
        	/* 端点0处理 */
            if( rx_token == PID_IN ) 
            {
				/* 端点0上传成功中断 */
				switch( USBHS_Dev_SetupReqCode )
				{
					case USB_GET_DESCRIPTOR:
						len = USBHS_Dev_SetupReqLen >= USBHS_UEP0_SIZE ? USBHS_UEP0_SIZE : USBHS_Dev_SetupReqLen;
						memcpy( EP0_Databuf, pDescr, len );
						USBHS_Dev_SetupReqLen -= len;
						pDescr += len;
						USBHS_Dev_Endp0_Tog ^= 1;
                        USBHSD->UEP0_TX_LEN  = len;
                        USBHSD->UEP0_TX_CTRL =  USBHS_EP_T_RES_ACK | ( USBHS_Dev_Endp0_Tog ? USBHS_EP_T_TOG_0 : USBHS_EP_T_TOG_1 ); // DATA stage (IN -DATA1-ACK)
						break;

					case USB_SET_ADDRESS:
	                    USBHS_Device_SetAddress( USBHS_Dev_Address );
	                    USBHS_Dev_Endp0_Tog = 0x01;
                        USBHSD->UEP0_TX_LEN = 0;
                        USBHSD->UEP0_TX_CTRL = 0;
                        USBHSD->UEP0_RX_CTRL = 0;
						break;

					default:
						/* 状态阶段完成中断或者是强制上传0长度数据包结束控制传输 */
					    USBHSD->UEP0_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_1;
	                    pDescr = NULL;
						break;
				}
            }
            else if( rx_token == PID_OUT )
            {
                USBHSD->UEP0_TX_LEN  = 0;
                USBHSD->UEP0_TX_CTRL = USBHS_EP_T_RES_ACK | USBHS_EP_T_TOG_1;
            }
        }
        USBHSD->INT_FG = USBHS_TRANSFER_FLAG;
    }
    else if( usb_intstatus & USBHS_SETUP_FLAG )
    {
        /* SETUP包处理 */
        USBHSD->UEP0_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_1;
        if( USBHSD->UEP0_RX_CTRL == USBHS_EP_R_RES_STALL )
        {
            USBHSD->UEP0_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_1;
        }
        USBHS_Dev_SetupReqLen = pSetupReqPak->wLength;
        USBHS_Dev_SetupReqCode = pSetupReqPak->bRequest;
        USBHS_Dev_SetupReqValueH = (UINT8)( pSetupReqPak->wValue >> 8 );
        chtype = pSetupReqPak->bRequestType;
        len = 0x00;
        errflag = 0x00;

        /* 判断当前是标准请求还是其他请求 */
        if( ( pSetupReqPak->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )
        {
            /* 其它请求,如类请求,产商请求等 */
            if( pSetupReqPak->bRequestType & 0x40 )                 /* 厂商请求 */
            {

            }
            else if( pSetupReqPak->bRequestType & 0x20 )            /* 类请求 */
            {

            }

            /* 判断是否可以正常处理 */
            if( errflag != 0xFF )
            {
                if( USBHS_Dev_SetupReqLen > len )
                {
                    USBHS_Dev_SetupReqLen = len;
                }
                len = ( USBHS_Dev_SetupReqLen >= USBHS_UEP0_SIZE ) ? USBHS_UEP0_SIZE : USBHS_Dev_SetupReqLen;
                memcpy( EP0_Databuf, pDescr, len );
                pDescr += len;
            }
        }
        else
        {
            /* 处理标准USB请求包 */
            switch( USBHS_Dev_SetupReqCode )
            {
                case USB_GET_DESCRIPTOR:
                {
                    switch( ( ( pSetupReqPak->wValue ) >> 8 ) )
                    {
                        case USB_DESCR_TYP_DEVICE:
                            /* 获取设备描述符 */
                            pDescr = MyDevDescr;
                            len = MyDevDescr[ 0 ];
                            break;

                        case USB_DESCR_TYP_CONFIG:

                            /* 判断USB速度：00：全速; 01：高速; 10:低速 */
                            if( ( USBHSD->SPEED_TYPE & 0x03 ) == 0x01 )
                            {
                                /* 高速模式 */
                                USBHS_Dev_Speed = 0x01;
                                USBHS_Up_PackLenMax = DEF_USB_HS_PACK_LEN;
                            }
                            else
                            {
                                /* 全速模式 */
                                USBHS_Dev_Speed = 0x00;
                                USBHS_Up_PackLenMax = DEF_USB_FS_PACK_LEN;
                            }

                            /* 获取配置描述符 */
                            if( USBHS_Dev_Speed == 0x01 )
                            {
                                /* 高速模式 */
                                pDescr = MyCfgDescr_HS;
                                len = MyCfgDescr_HS[ 2 ] | ( (UINT16)MyCfgDescr_HS[ 3 ] << 8 );
                            }
                            else
                            {
                                /* 全速模式 */
                                pDescr = MyCfgDescr_FS;
                                len = MyCfgDescr_FS[ 2 ] | ( (UINT16)MyCfgDescr_FS[ 3 ] << 8 );
                            }
                            break;

                        case USB_DESCR_TYP_STRING:
                            /* 获取字符串描述符 */
                            switch( ( pSetupReqPak->wValue ) & 0xff )
                            {
                                case 0:
                                    /* 语言字符串描述符 */
                                    pDescr = MyLangDescr;
                                    len = MyLangDescr[ 0 ];
                                    break;

                                case 1:
                                    /* USB产商字符串描述符 */
                                    pDescr = MyManuInfo;
                                    len = sizeof( MyManuInfo );
                                    break;

                                case 2:
                                    /* USB产品字符串描述符 */
                                    pDescr = MyProdInfo;
                                    len = sizeof( MyProdInfo );
                                    break;

                                case 3:
                                    /* USB序列号字符串描述符 */
                                    pDescr = MySerNumInfo;
                                    len = sizeof( MySerNumInfo );
                                    break;

                                default:
                                    errflag = 0xFF;
                                    break;
                            }
                            break;

                        case 6:
                            /* 设备限定描述符 */
                            pDescr = ( PUINT8 )&MyUSBQUADesc[ 0 ];
                            len = sizeof( MyUSBQUADesc );
                            break;

                        case 7:
                            /* 其他速度配置描述符 */
                            if( USBHS_Dev_Speed == 0x01 )
                            {
                                /* 高速模式 */
                                memcpy( &TAB_USB_HS_OSC_DESC[ 2 ], &MyCfgDescr_FS[ 2 ], sizeof( MyCfgDescr_FS ) - 2 );
                                pDescr = ( PUINT8 )&TAB_USB_HS_OSC_DESC[ 0 ];
                                len = sizeof( TAB_USB_HS_OSC_DESC );
                            }
                            else if( USBHS_Dev_Speed == 0x00 )
                            {
                                /* 全速模式 */
                                memcpy( &TAB_USB_FS_OSC_DESC[ 2 ], &MyCfgDescr_HS[ 2 ], sizeof( MyCfgDescr_HS ) - 2 );
                                pDescr = ( PUINT8 )&TAB_USB_FS_OSC_DESC[ 0 ];
                                len = sizeof( TAB_USB_FS_OSC_DESC );
                            }
                            else
                            {
                                errflag = 0xFF;
                            }
                            break;

                        default :
                            errflag = 0xFF;
                            break;
                    }

                    /* 判断是否可以正常处理 */
                    if( errflag != 0xFF )
                    {
                        if( USBHS_Dev_SetupReqLen > len )
                        {
                            USBHS_Dev_SetupReqLen = len;
                        }
                        len = ( USBHS_Dev_SetupReqLen >= USBHS_UEP0_SIZE ) ? USBHS_UEP0_SIZE : USBHS_Dev_SetupReqLen;
                        memcpy( EP0_Databuf, pDescr, len );
                        pDescr += len;
                    }
                }
                break;

                case USB_SET_ADDRESS:
                    /* 设置地址 */
                    USBHS_Dev_Address = ( pSetupReqPak->wValue )& 0xff;
                    break;

                case USB_GET_CONFIGURATION:
                    /* 获取配置值 */
                    EP0_Databuf[ 0 ] = USBHS_Dev_Config;
                    if( USBHS_Dev_SetupReqLen > 1 )
                    {
                        USBHS_Dev_SetupReqLen = 1;
                    }
                    break;

                case USB_SET_CONFIGURATION:
                    /* 设置配置值 */
                    USBHS_Dev_Config = ( pSetupReqPak->wValue ) & 0xff;
                    USBHS_Dev_EnumStatus = 0x01;
                    break;

                case USB_CLEAR_FEATURE:
                    /* 清除特性 */
                    if( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )
                    {
                        /* 清除端点 */
                        switch( ( pSetupReqPak->wIndex ) & 0xff )
                        {
                            case 0x88:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP8_TX_LEN = 0;
                                USBHSD->UEP8_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x08:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP8_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x87:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP7_TX_LEN = 0;
                                USBHSD->UEP7_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x07:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP7_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x86:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP6_TX_LEN = 0;
                                USBHSD->UEP6_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x06:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP6_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x85:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP5_TX_LEN = 0;
                                USBHSD->UEP5_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x05:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP5_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x84:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP4_TX_LEN = 0;
                                USBHSD->UEP4_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x04:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP4_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x83:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP3_TX_LEN = 0;
                                USBHSD->UEP3_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x03:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP3_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x82:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP2_TX_LEN = 0;
                                USBHSD->UEP2_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x02:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP2_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            case 0x81:
                                /* SET Endpx Tx to USBHS_EP_T_RES_NAK;USBHS_EP_T_TOG_0;len = 0 */
                                USBHSD->UEP1_TX_LEN = 0;
                                USBHSD->UEP1_TX_CTRL = USBHS_EP_T_RES_NAK | USBHS_EP_T_TOG_0;
                                break;

                            case 0x01:
                                /* SET Endpx Rx to USBHS_EP_R_RES_NAK;USBHS_EP_R_TOG_0 */
                                USBHSD->UEP1_RX_CTRL = USBHS_EP_R_RES_ACK | USBHS_EP_R_TOG_0;
                                break;

                            default:
                                errflag = 0xFF;
                                break;
                        }
                    }
                    else if( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
                    {
                        if( ( ( pSetupReqPak->wValue ) & 0xff ) == 1 )
                        {
                            USBHS_Dev_SleepStatus &= ~0x01;
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                    break;

                case USB_SET_FEATURE:
                    /* 设置特性 */
                    if( ( pSetupReqPak->bRequestType & 0x1F ) == 0x00 )
                    {
                        /* 设置设备 */
                        if( pSetupReqPak->wValue == 0x01 )
                        {
                            if( MyCfgDescr_HS[ 7 ] & 0x20 )
                            {
                                /* 设置唤醒使能标志 */
                                USBHS_Dev_SleepStatus = 0x01;
                            }
                            else
                            {
                                errflag = 0xFF;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    else if( ( pSetupReqPak->bRequestType & 0x1F ) == 0x02 )
                    {
                        /* 设置端点 */
                        if( pSetupReqPak->wValue == 0x00 )
                        {
                            /* 设置指定端点STALL */
                            switch( ( pSetupReqPak->wIndex ) & 0xff )
                            {
                                case 0x88:
                                    /* 设置端点8 IN STALL */
                                    USBHSD->UEP8_TX_CTRL = ( USBHSD->UEP8_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x08:
                                    /* 设置端点1 OUT STALL */
                                    USBHSD->UEP8_RX_CTRL = ( USBHSD->UEP8_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x87:
                                    /* 设置端点7 IN STALL */
                                    USBHSD->UEP7_TX_CTRL = ( USBHSD->UEP7_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x07:
                                    /* 设置端点7 OUT STALL */
                                    USBHSD->UEP7_RX_CTRL = ( USBHSD->UEP7_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x86:
                                    /* 设置端点6 IN STALL */
                                    USBHSD->UEP6_TX_CTRL = ( USBHSD->UEP6_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x06:
                                    /* 设置端点6 OUT STALL */
                                    USBHSD->UEP6_RX_CTRL = ( USBHSD->UEP6_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x85:
                                    /* 设置端点5 IN STALL */
                                    USBHSD->UEP5_TX_CTRL = ( USBHSD->UEP5_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x05:
                                    /* 设置端点5 OUT STALL */
                                    USBHSD->UEP5_RX_CTRL = ( USBHSD->UEP5_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x84:
                                    /* 设置端点4 IN STALL */
                                    USBHSD->UEP4_TX_CTRL = ( USBHSD->UEP4_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x04:
                                    /* 设置端点4 OUT STALL */
                                    USBHSD->UEP4_RX_CTRL = ( USBHSD->UEP4_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x83:
                                    /* 设置端点3 IN STALL */
                                    USBHSD->UEP3_TX_CTRL = ( USBHSD->UEP3_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x03:
                                    /* 设置端点3 OUT STALL */
                                    USBHSD->UEP3_RX_CTRL = ( USBHSD->UEP3_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x82:
                                    /* 设置端点2 IN STALL */
                                    USBHSD->UEP2_TX_CTRL = ( USBHSD->UEP2_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x02:
                                    /* 设置端点2 OUT STALL */
                                    USBHSD->UEP2_RX_CTRL = ( USBHSD->UEP2_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x81:
                                    /* 设置端点1 IN STALL */
                                    USBHSD->UEP1_TX_CTRL = ( USBHSD->UEP1_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x01:
                                    /* 设置端点1 OUT STALL */
                                    USBHSD->UEP1_RX_CTRL = ( USBHSD->UEP1_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                default:
                                    errflag = 0xFF;
                                    break;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                    break;

                case USB_GET_INTERFACE:
                    EP0_Databuf[ 0 ] = 0x00;
                    if( USBHS_Dev_SetupReqLen > 1 )
                    {
                        USBHS_Dev_SetupReqLen = 1;
                    }
                    break;

                case USB_SET_INTERFACE:
                    EP0_Databuf[ 0 ] = 0x00;
                    if( USBHS_Dev_SetupReqLen > 1 )
                    {
                        USBHS_Dev_SetupReqLen = 1;
                    }
                    break;

                case USB_GET_STATUS:
                    /* 根据当前端点实际状态进行应答 */
                    EP0_Databuf[ 0 ] = 0x00;
                    EP0_Databuf[ 1 ] = 0x00;
                    if( pSetupReqPak->wIndex == 0x81 )
                    {
                        if( ( USBHSD->UEP1_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x01 )
                    {
                        if( ( USBHSD->UEP1_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x82 )
                    {
                        if( ( USBHSD->UEP2_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x02 )
                    {
                        if( ( USBHSD->UEP2_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x83 )
                    {
                        if( ( USBHSD->UEP3_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x03 )
                    {
                        if( ( USBHSD->UEP3_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x84 )
                    {
                        if( ( USBHSD->UEP4_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x04 )
                    {
                        if( ( USBHSD->UEP4_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x85 )
                    {
                        if( ( USBHSD->UEP5_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x05 )
                    {
                        if( ( USBHSD->UEP5_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x86 )
                    {
                        if( ( USBHSD->UEP6_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x06 )
                    {
                        if( ( USBHSD->UEP6_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x87 )
                    {
                        if( ( USBHSD->UEP7_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x07 )
                    {
                        if( ( USBHSD->UEP7_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x88 )
                    {
                        if( ( USBHSD->UEP8_TX_CTRL & USBHS_EP_T_RES_MASK ) == USBHS_EP_T_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    else if( pSetupReqPak->wIndex == 0x08 )
                    {
                        if( ( USBHSD->UEP8_RX_CTRL & USBHS_EP_R_RES_MASK ) == USBHS_EP_R_RES_STALL )
                        {
                            EP0_Databuf[ 0 ] = 0x01;
                        }
                    }
                    if( USBHS_Dev_SetupReqLen > 2 )
                    {
                        USBHS_Dev_SetupReqLen = 2;
                    }
                    break;

                default:
                    errflag = 0xff;
                    break;
            }
        }

        /* 端点0处理 */
        if( errflag == 0xFF )
        {
            /* IN - STALL / OUT - DATA - STALL */
            USBHS_Dev_SetupReqCode = 0xFF;
            USBHS_Dev_Endp0_Tog = 0x01;
            USBHSD->UEP0_TX_LEN  = 0;
            USBHSD->UEP0_TX_CTRL = USBHS_EP_T_RES_STALL;
            USBHSD->UEP0_RX_CTRL = USBHS_EP_R_RES_STALL;
        }
        else
        {
            /* DATA stage (IN -DATA1-ACK) */
            if( chtype & 0x80 )
            {
                len = ( USBHS_Dev_SetupReqLen> USBHS_UEP0_SIZE ) ? USBHS_UEP0_SIZE : USBHS_Dev_SetupReqLen;
                USBHS_Dev_SetupReqLen -= len;
            }
            else
            {
                len = 0;
            }
            USBHS_Dev_Endp0_Tog = 0x00;
            USBHSD->UEP0_TX_LEN  = len;
            USBHSD->UEP0_TX_CTRL = USBHS_EP_T_RES_ACK | USBHS_EP_T_TOG_1;
        }
        USBHSD->INT_FG = USBHS_SETUP_FLAG;
    }
    else if( usb_intstatus & USBHS_DETECT_FLAG )
    {
		/* USB总线复位中断 */  
		DUG_PRINTF("USB Reset\n");   	
		USBHS_Dev_Address = 0x00;
        USBHS_Device_SetAddress( USBHS_Dev_Address );							/* USB2.0高速设备设置设备地址 */
        USBHS_Device_Endp_Init( );                                              /* USB2.0高速设备端点初始化 */

		/* 清USB总线复位 */
		USBHSD->INT_FG = USBHS_DETECT_FLAG;
    }
    else if( usb_intstatus & USBHS_SUSPEND_FLAG )
    {
    	/* USB总线挂起/唤醒完成中断 */
        USBHSD->INT_FG = USBHS_SUSPEND_FLAG;                                    /* 为了USB可以唤醒,必须先清中断 */

        if( USBHSD->MIS_ST & ( 1 << 2 ) )
		{
			/* 挂起 */
//			DUG_PRINTF("USB SUSPEND1!!!\n");
			USBHS_Dev_SleepStatus |= 0x02;
//			if( USBHS_Dev_SleepStatus != 0x03 )
//			{
//				USBHD_EnumStatus = 0x00;
//			}

#if( DEF_USBSLEEP_FUN_EN == 0x01 )
			/* USB主从接口睡眠唤醒配置  */
			USBHS_Sleep_WakeUp_Cfg( );

			/* 停机模式,睡眠后,时钟源变为HSI,需要从新配置USB时钟才生效 */
			SystemInit( );  													/* HSI作PLL源 */

		    /* USB2.0高速设备RCC初始化 */
			Delay_Us( 200 );
		    USBHS_RCC_Init( );

			/* 打开各个外设时钟  */
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB2Periph_USART1, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART3, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );
		    Delay_mS( 1 );

		    /* 重新初始化定时器 */
			TIM2_Init( );
#endif
		}
		else
		{
			/* 唤醒 */
 			DUG_PRINTF("USB SUSPEND2!!!\n");
		    USBHS_Dev_SleepStatus &= ~0x02;
			USBHS_Dev_EnumStatus = 0x01;
		}        
        USBHSD->INT_FG = USBHS_SUSPEND_FLAG;
    }
    else
    {
        temp8 = USBHSD->INT_FG;
        USBHSD->INT_FG = temp8;
    }
}

/*******************************************************************************
* Function Name  : USBHS_Sleep_WakeUp_Cfg
* Description    : USB2.0主从接口睡眠唤醒配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Sleep_WakeUp_Cfg( void )
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};

    /* 关闭中断 */
    __disable_irq( );

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );                       /* 开启电源时钟 */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );                     /* 使能PA端口时钟 */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );

    /* 关闭各个外设时钟  */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB2Periph_USART1, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART3, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, DISABLE );

    /* 配置USB睡眠唤醒 */
    EXTI_InitStructure.EXTI_Line = EXTI_Line20;                                 /* 事件线选择 */
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;                             /* 事件请求 */
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;                      /* 上升沿触发 */
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;                     /* 下降沿触发 */
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;              /* 边沿触发(上升沿或下降沿) */
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /* 进入睡眠模式 */
    USBHSH->HOST_CTRL &= ~PHY_SUSPENDM;
    Delay_Us( 10 );
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFE );
    Delay_Us( 200 );

    /* 打开中断 */
    __enable_irq( );
}

