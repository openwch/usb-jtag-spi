/********************************** (C) COPYRIGHT *******************************
* File Name          : JTAG.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USB转JTAG相关头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef __JTAG_H__
#define __JTAG_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* 头文件包含 */
#include "JTAG_Port.h"
#include "ch32vf30x_usb_device.h"

/******************************************************************************/
/* JTAG命令码定义 */

/******************************************************************************/
/* JTAG相关宏定义 */

/* SPI操作相关宏定义 */
#define CTLR1_SPE_Set          ((uint16_t)0x0040)
#define CTLR1_SPE_Reset        ((uint16_t)0xFFBF)

/* USB包大小相关宏定义 */
#define DEF_USB_FS_PACK_LEN        64                                           /* USB全速模式包大小 */
#define DEF_USB_HS_PACK_LEN        512                                          /* USB高速模式包大小 */

/* 接口通信缓冲区相关定义 */
#define DEF_COMM_TX_BUF_LEN        4096                                         /* 接口通信发送缓冲区(USB下传缓冲区)大小 */
#define DEF_COMM_RX_BUF_LEN        4096                                         /* 接口通信接收缓冲区(USB上传缓冲区)大小 */
#define DEF_COMM_BUF_PACKNUM_MAX   ( DEF_COMM_TX_BUF_LEN / DEF_USB_HS_PACK_LEN )/* 接口通信发送缓冲区个数最大值 */

/* 命令码相关宏定义 */
#define DEF_CMD_SPI_INIT            0xC0                                  /* SPI初始化命令 */
#define DEF_CMD_SPI_CONTROL         0xC1                                  /* SPI接口控制命令 */
#define DEF_CMD_SPI_RD_WR           0xC2                                  /* SPI接口常规读取写入数据命令 */
#define DEF_CMD_SPI_BLCK_RD         0xC3                                  /* SPI接口批量读取数据命令 */
#define DEF_CMD_SPI_BLCK_WR         0xC4                                  /* SPI接口批量写入数据命令 */
#define DEF_CMD_INFO_RD             0xCA                                  /* 信息读取 */

#define DEF_CMD_JTAG_INIT           0xD0                                  /* JTAG接口初始化命令 */
#define DEF_CMD_JTAG_BIT_OP         0xD1                                  /* JTAG接口引脚位控制命令 */
#define DEF_CMD_JTAG_BIT_OP_RD      0xD2                                  /* JTAG接口引脚位控制并读取命令 */
#define DEF_CMD_JTAG_DATA_SHIFT     0xD3                                  /* JTAG接口数据移位命令 */
#define DEF_CMD_JTAG_DATA_SHIFT_RD  0xD4                                  /* JTAG接口数据移位并读取命令 */

/* 命令包长度相关宏定义 */
#define DEF_FS_PACK_MAX_LEN         59                                    /* 全速模式下命令包最大长度 */
#define DEF_HS_PACK_MAX_LEN         507                                   /* 高速模式下命令包最大长度 */
#define DEF_SPI_TXRX_LEN_MAX        507                                   /* SPI_I2C收发最大值 */

/* SPI操作相关宏定义 */
#define DEF_SPI_OP_IDLE             0x00                                  /* SPI接口操作状态：空闲 */
#define DEF_SPI_OP_DMA_RD           0x01                                  /* SPI接口操作状态：DMA读取 */
#define DEF_SPI_OP_DMA_WR           0x02                                  /* SPI接口操作状态：DMA写入 */

#define DEF_CMD_C0_INFOLEN          26                                    /* 0xC0命令包长度 */
#define DEF_CMD_C1_INFOLEN          10                                    /* 0xC1命令包长度 */
#define DEF_CMD_C3_INFOLEN          4                                     /* 0xC3命令包长度 */
#define DEF_CMD_D0_INFOLEN          6                                     /* 0xD0命令包长度 */

/************************************************************/
/* 接口通信相关结构体定义 */
#define DEF_SPI_STATUS_IDLE         0x00                                        /* SPI收发状态为空闲  */
#define DEF_SPI_STATUS_TX           0x01                                        /* SPI收发状态为正在发送  */
#define DEF_SPI_STATUS_RX           0x02                                        /* SPI收发状态为正在发送  */

typedef struct  _COMM_CTL
{
    UINT16V CMDP_LoadNum;                                                       /* 接口通信命令包缓冲区装载编号 */
    UINT16V CMDP_DealNum;                                                       /* 接口通信命令包缓冲区处理编号 */
    UINT16V CMDP_RemainNum;                                                     /* 接口通信命令包缓冲区剩余未处理编号 */
    UINT16V CMDP_PackLen[ DEF_COMM_BUF_PACKNUM_MAX ];                           /* 接口通信命令包缓冲区当前包长度 */
    UINT16  CMDP_Cur_RemainLen;                                                 /* 接口通信命令包当前正在处理的命令包的剩余处理长度 */
    UINT16  CMDP_Cur_DealPtr;                                                   /* 接口通信命令包当前正在处理的命令包的处理指针 */
    UINT16  CMDP_Cur_CMDLen;                                                    /* 接口通信命令包当前正在处理的命令包所占长度 */

    UINT16  Rx_LoadPtr;                                                         /* JTAG数据接收缓冲区装载指针 */
    UINT16  Rx_DealPtr;                                                         /* JTAG数据接收缓冲区处理指针 */
    UINT16V Rx_RemainLen;                                                       /* JTAG数据接收缓冲区剩余未处理长度 */
    UINT16V Rx_IdleCount;                                                       /* JTAG数据接收超时计时 */

    UINT16  USB_Up_PackLen;                                                     /* USB数据包上传长度 */
    UINT8   USB_Up_IngFlag;                                                     /* USB数据包正在上传标志 */
    UINT16  USB_Up_TimeOut;                                                     /* USB数据包上传超时计时 */
    UINT16  USB_Up_TimeOutMax;                                                  /* USB数据包上传超时最大值 */
    UINT8   USB_Down_StopFlag;                                                  /* USB数据包停止下传标志 */

    UINT16  Tx_TimeOut;                                                         /* SPI数据发送超时 */
    UINT16  Tx_TimeOutMax;                                                      /* SPI数据发送超时最大值 */
    UINT8   Tx_Rx_Status;                                                       /* SPI数据收发状态(0：空闲；1：SPI正在发送； 2：SPI正在接收；) */
    UINT32  Rx_TotalLen;                                                        /* SPI接收总长度  */
    UINT32  Tx_CurLen;                                                          /* SPI本次发送长度  */

    UINT8   CS0_LaterOpStatus;                                                  /* 片选CS0后续操作有效状态 */
    UINT16  CS0_LaterOpDelay;                                                   /* 片选CS0后续操作延时时间 */
    UINT8   CS1_LaterOpStatus;                                                  /* 片选CS1后续操作有效状态 */
    UINT16  CS1_LaterOpDelay;                                                   /* 片选CS1后续操作延时时间 */
    UINT16  SPI_ByteDelay;                                                      /* SPI操作字节间延时 */
    UINT8   SPI_FillData;                                                       /* SPI填充字节 */
    UINT8   Bit_Control;                                                        /* 杂项位控制 */
}COMM_CTL, *PCOMM_CTL;

/******************************************************************************/
/* 变量外扩 */
extern volatile COMM_CTL COMM;                                                 /* 接口通信控制相关结构体 */
extern __attribute__ ((aligned(4))) UINT8  Comm_Tx_Buf[ DEF_COMM_TX_BUF_LEN ];  /* 接口通信发送缓冲区(USB下传缓冲区) */
extern __attribute__ ((aligned(4))) UINT8  Comm_Rx_Buf[ DEF_COMM_RX_BUF_LEN ];  /* 接口通信接收缓冲区(USB上传缓冲区) */

extern volatile UINT8  CMDPack_Op_Status;                                       /* 命令包的命令执行状态 */
extern volatile UINT8  CMDPack_Op_Code;                                         /* 命令包的命令码 */
extern volatile UINT16 CMDPack_Op_Len;                                          /* 命令包的长度 */

extern volatile UINT8  JTAG_Mode;                                               /* JTAG模式：0：协议模式；1：bit-bang模式 */
extern volatile UINT8  JTAG_Speed;                                              /* JTAG速度：0-4，4速度最快*/
extern volatile UINT8  JTAG_Read_Mode;                                          /* JTAG读取模式 */
extern volatile UINT8  JTAG_Shift_Mode;                                         /* JTAG移位模式 */
extern volatile UINT32 JTAG_Shift_Cnt;                                          /* JTAG移位计数 */
extern volatile UINT32 JTAG_Time_Count;                                         /* JTAG计时 */

extern SPI_InitTypeDef SPI_Cfg;                                                 /* SPI接口配置 */
extern __attribute__ ((aligned(4))) UINT8  SPI_TxDMA_Buf[ 1024 ];               /* SPI_I2C发送DMA缓冲区 */
extern __attribute__ ((aligned(4))) UINT8  SPI_Com_Buf[ 1024 ];                 /* SPI公共数据缓冲区 */

/********************************************************************************/
/* 函数外扩 */
extern void JTAG_Init( void );                                                  /* JTAG初始化 */
extern void JTAG_SPI_Init( UINT16 speed );                                      /* JTAG模式下SPIx接口初始化 */
extern void JTAG_Port_SwTo_SPIMode( void );                                     /* JTAG接口设置为SPI模式 */
extern void JTAG_Port_SwTo_GPIOMode( void );                                    /* JTAG接口设置为GPIO模式 */
extern void JTAG_Mode1_Deal( void );										    /* JTAG模式1处理 */	

extern void SPIx_Cfg_DefInit( void );                                           /* SPIx接口配置默认初始化 */
extern void SPIx_Port_Init( void );                                             /* SPIx接口初始化 */
extern void SPIx_Tx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize );
extern void SPIx_Rx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize );
extern UINT8 SPIx_RD_WR_Byte( UINT8 dat );                                      /* SPIx发送并读写1个字节 */
extern void SPIx_TxRx_DMA_Init( UINT8 *pTxBuf, UINT16 Txlen, UINT8 *pRxBuf, UINT16 Rxlen ); /* SPIx发送接收数据DMA初始化 */
extern void SPIx_Tx_DMA_Deal( void );                                           /* SPIx发送数据DMA处理 */
extern void SPIx_Rx_DMA_Deal( void );                                           /* SPIx接收数据DMA处理 */
extern void SPIx_ReadID_Test( void );                                           /* SPIx读取FLASH芯片ID测试 */
extern void USB_SPI_DataUp( UINT8 *pbuf, UINT16 len );                          /* USB转SPI数据上传 */

extern void COMM_CMDPack_Switch( UINT8 mode );                                  /* 接口通信命令包切换 */
extern UINT8 COMM_CMDPack_Deal( void );                                         /* 接口通信命令包处理 */
extern void COMM_RxData_Up_Deal( void );                                        /* 接口通信接收数据上传处理 */

#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
