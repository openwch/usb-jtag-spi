/******************** (C) COPYRIGHT 2011 WCH ***********************************
* File Name          : JTAG.c
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USB转JTAG操作相关部分程序
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include <MAIN.h>                                                               /* 头文件包含 */

/******************************************************************************/
/* 常、变量定义 */
volatile COMM_CTL COMM;                                                         /* 接口通信控制相关结构体 */
__attribute__ ((aligned(4))) UINT8  Comm_Tx_Buf[ DEF_COMM_TX_BUF_LEN ];         /* 接口通信发送缓冲区(USB下传缓冲区) */
__attribute__ ((aligned(4))) UINT8  Comm_Rx_Buf[ DEF_COMM_RX_BUF_LEN ];         /* 接口通信接收缓冲区(USB上传缓冲区) */

volatile UINT8  CMDPack_Op_Status = 0x00;                                       /* 命令包的命令执行状态 */
volatile UINT8  CMDPack_Op_Code = 0x00;                                         /* 命令包的命令码 */
volatile UINT16 CMDPack_Op_Len = 0x00;                                          /* 命令包的长度 */
volatile UINT16 CMDPack_Op_Len_Save = 0x00;                                     /* 命令包的长度暂存 */

volatile UINT8  JTAG_Mode = 0x00;                                               /* JTAG模式：0：协议模式；1：bit-bang模式 */
volatile UINT8  JTAG_Speed = 0x00;                                              /* JTAG速度：0-4，4速度最快*/
volatile UINT8  JTAG_Read_Mode = 0;                                             /* JTAG读取模式 */
volatile UINT8  JTAG_Shift_Mode = 0;                                            /* JTAG移位模式 */
volatile UINT32 JTAG_Shift_Cnt = 0;                                             /* JTAG移位计数 */
volatile UINT32 JTAG_Time_Count = 0;                                            /* JTAG计时 */

SPI_InitTypeDef SPI_Cfg;                                                        /* SPI接口配置 */
__attribute__ ((aligned(4))) UINT8  SPI_TxDMA_Buf[ 1024 ];                      /* SPI_I2C发送DMA缓冲区 */
__attribute__ ((aligned(4))) UINT8  SPI_Com_Buf[ 1024 ];                        /* SPI公共数据缓冲区 */

/*******************************************************************************
* Function Name  : JTAG_Init
* Description    : JTAG初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Init( void )
{
	CMDPack_Op_Status = 0x00;                                      
	CMDPack_Op_Code = 0x00;                                         
	CMDPack_Op_Len = 0x00;                                         
	
    JTAG_Mode = 0x00;
    JTAG_Speed = 3;
    JTAG_Shift_Cnt = 0;
    JTAG_Shift_Mode = 0;
    JTAG_Read_Mode = 0;
    
    JTAG_Port_Init( );
    memset( (void *)&COMM.CMDP_LoadNum, 0x00, sizeof( COMM_CTL ) );
}

/*******************************************************************************
* Function Name  : JTAG_SPI_Init
* Description    : JTAG模式下SPIx接口初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_SPI_Init( UINT16 speed )
{
    SPI_InitTypeDef  SPI_InitStructure = {0};

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = speed;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    SPI_Init( SPI2, &SPI_InitStructure );

    /* 判断是否接收标志为1,如果是则读取数据寄存器清0(CH32V307的S[I2\SPI3,该位默认值为1) */
    if( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == SET )
    {
        SPI2->DATAR;
    }

    SPI_Cmd( SPI2, DISABLE );
}

/*******************************************************************************
* Function Name  : JTAG_Port_SwTo_SPIMode
* Description    : JTAG接口设置为SPI模式
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_SwTo_SPIMode( void )
{
    UINT32 temp;

    /* PB13,PB15: 推挽输出 ,50MHz; PB14: 浮空输入 */
    temp = GPIOB->CFGHR;
    temp &= 0x000FFFFF;
    temp |= 0xB4B00000;
    GPIOB->CFGHR = temp;

    /* 使能SPI */
    SPI2->CTLR1 |= CTLR1_SPE_Set;
}

/*******************************************************************************
* Function Name  : JTAG_Port_SwTo_GPIOMode
* Description    : JTAG接口设置为GPIO模式
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_SwTo_GPIOMode( void )
{
    UINT32 temp;

    /* PB13,PB15: 推挽输出  ,50MHz; PB14: 输入上拉 */
    temp = GPIOB->CFGHR;
    temp &= 0x000FFFFF;
    temp |= 0x38300000;
    GPIOB->CFGHR = temp;;
    GPIOB->BSHR = (((uint32_t)0x01 ) << 14 );

    SPI2->CTLR1 &= CTLR1_SPE_Reset;
}

/*******************************************************************************
* Function Name  : COMM_CMDPack_Switch
* Description    : 接口通信命令包切换
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void COMM_CMDPack_Switch( UINT8 mode )
{
    /* 计算当前命令包剩余长度及偏移，并判断是否需要切换缓冲区 */
    if( mode == 0 )
    {
        if( COMM.CMDP_Cur_RemainLen >= COMM.CMDP_Cur_CMDLen )
        {
            COMM.CMDP_Cur_RemainLen -= COMM.CMDP_Cur_CMDLen;
            COMM.CMDP_Cur_DealPtr += COMM.CMDP_Cur_CMDLen;
        }
        else
        {
            COMM.CMDP_Cur_RemainLen = 0;
        }
    }
    if( COMM.CMDP_Cur_RemainLen == 0x00 )
    {
        /* 切换命令包缓冲区 */

        /* 关闭USB中断 */
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );

        /* 计算相关变量 */
        COMM.CMDP_PackLen[ COMM.CMDP_DealNum ] = 0x0000;
        COMM.CMDP_DealNum++;
        if( COMM.CMDP_DealNum >= DEF_COMM_BUF_PACKNUM_MAX )
        {
            COMM.CMDP_DealNum = 0x00;
        }
        COMM.CMDP_RemainNum--;

        /* 如果当前SPI已暂停下传则重启驱动下传 */
        if( ( COMM.USB_Down_StopFlag == 0x01 ) &&
            ( COMM.CMDP_RemainNum < ( DEF_COMM_BUF_PACKNUM_MAX - 2 ) ) )
        {
            USBHSD->UEP2_RX_CTRL &= ~USBHS_EP_R_RES_MASK;
            USBHSD->UEP2_RX_CTRL |= USBHS_EP_R_RES_ACK;

            COMM.USB_Down_StopFlag = 0x00;
        }

        /* 重新打开USB中断 */
        NVIC_EnableIRQ( USBHS_IRQn );
    }
}

/*******************************************************************************
* Function Name  : COMM_Load_PackHead
* Description    : 接口通信装载协议包头
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void COMM_Load_PackHead( UINT8 cmd, UINT16 len )
{
    Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = cmd;
    COMM.Rx_LoadPtr++;
    if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
    {
        COMM.Rx_LoadPtr = 0;
    }
    COMM.Rx_RemainLen++;

    Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = (UINT8)len;
    COMM.Rx_LoadPtr++;
    if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
    {
        COMM.Rx_LoadPtr = 0;
    }
    COMM.Rx_RemainLen++;

    Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = (UINT8)( len >> 8 );
    COMM.Rx_LoadPtr++;
    if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
    {
        COMM.Rx_LoadPtr = 0;
    }
    COMM.Rx_RemainLen++;
}

/*******************************************************************************
* Function Name  : COMM_CMDPack_Deal
* Description    : 接口通信命令包处理
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 COMM_CMDPack_Deal( void )
{
    UINT8  *pTxbuf;
    UINT16 i;
    UINT16 count;
    UINT16 temp16;
    UINT16 pack_len;
    UINT8  dat;
    UINT8  op_flag;
    UINT8  sp_flag;
    UINT16 tx_len;
    UINT16 recv_len;
    UINT8  buf[ 32 ];

    /* 如果有SPI接口数据正在发送，则查询执行状态 */
    if( COMM.Tx_Rx_Status == DEF_SPI_STATUS_TX )
    {
        SPIx_Tx_DMA_Deal( );
        return( 0x01 );
    }

    /* 如果有SPI接口数据正在接收，则查询执行状态 */
    if( COMM.Tx_Rx_Status == DEF_SPI_STATUS_RX )
    {
        SPIx_Rx_DMA_Deal( );
        return( 0x02 );
    }

    /* 如果未接收到命令包则直接返回 */
    if( COMM.CMDP_RemainNum == 0x00 )
    {
        /* 判断是否有JTAG数据需要上传(执行完完整的1包才启动数据上传) */
        if( CMDPack_Op_Status == 0 )
        {
            COMM_RxData_Up_Deal( );
        }
        return( 0x03 );
    }

    /* 判断是否当前命令包中继续装载执行，还是重新装载下一命令包 */
    if( COMM.CMDP_Cur_RemainLen == 0x00 )
    {
        COMM.CMDP_Cur_RemainLen = COMM.CMDP_PackLen[ COMM.CMDP_DealNum ];
        COMM.CMDP_Cur_DealPtr = ( COMM.CMDP_DealNum * DEF_USB_HS_PACK_LEN );
    }

//	DUG_PRINTF("L:%x\n",COMM.CMDP_Cur_RemainLen);
//	DUG_PRINTF("S:%x\n",CMDPack_Op_Status);

    /* 分析命令包并处理 */
    sp_flag = 0x00;
    op_flag = 0x00;
    pTxbuf = &Comm_Tx_Buf[ COMM.CMDP_Cur_DealPtr ];
    while( COMM.CMDP_Cur_RemainLen && ( sp_flag == 0 ) )
    {
        if( CMDPack_Op_Status == 0x00 )
        {
            /* 取1个字节命令码 */
            CMDPack_Op_Code = *pTxbuf++;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

//			DUG_PRINTF("CMD:%x\n",CMDPack_Op_Code);

            /* 判断命令码是否正确,如果是旧版本命令则直接按原先格式处理 */
            if( ( CMDPack_Op_Code >= DEF_CMD_SPI_INIT ) &&
                ( CMDPack_Op_Code <= DEF_CMD_JTAG_DATA_SHIFT_RD ) )
            {
                /* 切换命令执行状态 */
                CMDPack_Op_Status = 0x01;
            }
            else
            {
                /* 切换命令执行状态 */
                CMDPack_Op_Status = 0x00;
            }

            /* 判断是否需要进行缓冲区切换 */
            COMM_CMDPack_Switch( 0x01 );
        }
        else if( CMDPack_Op_Status == 0x01 )
        {
            /* 取2个字节命令长度(第1字节) */
            CMDPack_Op_Len = *pTxbuf++;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

            /* 切换命令执行状态 */
            CMDPack_Op_Status = 0x02;

            /* 判断是否需要进行缓冲区切换 */
            COMM_CMDPack_Switch( 0x01 );
        }
        else if( CMDPack_Op_Status == 0x02 )
        {
            /* 取2个字节命令长度(第2字节) */
            CMDPack_Op_Len |= (((UINT16)*pTxbuf++) << 8 );
            CMDPack_Op_Len_Save = CMDPack_Op_Len;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

//			DUG_PRINTF("C_L:%x\n",CMDPack_Op_Len);

            /* 切换命令执行状态 */
            CMDPack_Op_Status = 0x03;

            /* 判断是否需要进行缓冲区切换 */
            COMM_CMDPack_Switch( 0x01 );
        }
        else
        {
            /* 执行后续N个字节数据 */

            /* 计算后续数据执行长度 */
            pack_len = CMDPack_Op_Len;
            if( pack_len >= COMM.CMDP_Cur_RemainLen )
            {
                pack_len = COMM.CMDP_Cur_RemainLen;
            }

            /* 根据当前命令批量执行后续数据 */
            switch( CMDPack_Op_Code )
            {
                case DEF_CMD_SPI_INIT:
                    /* 0xC0---SPI初始化命令 */
                    /* (1)、18个字节的SPI初始化参数，为SPI具体见“SPI.h”文件中的结构体SPI_InitTypeDef；
                       (2)、2个字节数据读写之间的延时值(仅针对SPI接口常规读取写入数据命令(DEF_CMD_SPI_RD_WR))，单位为uS；
                       (3)、1个字节SPI发送默认字节；
                       (4)、1个字节杂项控制；
                                                                          位7：片选CS1极性控制：0：低电平有效；1：有电平有效；
                                                                          位6：片选CS2极性控制：0：低电平有效；1：有电平有效；
                                                                          位3-0：保留；
                       (5)、4个字节保留；*/

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = DEF_CMD_C0_INFOLEN;

                    /* 执行该命令 */
                    if( pack_len >= COMM.CMDP_Cur_CMDLen )
                    {
                        /* (1)、18个字节的SPI初始化参数 */
                        memcpy( &SPI_Cfg.SPI_Direction, pTxbuf, sizeof( SPI_InitTypeDef ) );
                        SPIx_Port_Init( );
                        SPIx_Tx_DMA_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)&SPI_TxDMA_Buf[ 0 ], 0x00 );
                        SPIx_Rx_DMA_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)&SPI_Com_Buf[ 0 ], 0x00 );
                        pTxbuf += 18;

                        /* (2)、2个字节数据读写之间的延时值 */
                        COMM.SPI_ByteDelay = *pTxbuf++;
                        COMM.SPI_ByteDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                        /* (3)、1个字节SPI发送默认字节 */
                        COMM.SPI_FillData = *pTxbuf++;

                        /* (4)、1个字节杂项控制 */
                        COMM.Bit_Control = *pTxbuf++;

                        /* (5)、4个字节保留；*/
                        pTxbuf += 4;
                    }
                    else
                    {
                        op_flag = 0x01;

                        SPIx_Port_Init( );
                        SPIx_Tx_DMA_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)&SPI_TxDMA_Buf[ 0 ], 0x00 );
                        SPIx_Rx_DMA_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)&SPI_Com_Buf[ 0 ], 0x00 );
                    }

                    /* 设置返回应答包 */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_INIT;
                    SPI_Com_Buf[ 1 ] = 0x01;
                    SPI_Com_Buf[ 2 ] = 0x00;
                    SPI_Com_Buf[ 3 ] = 0x00;
                    if( op_flag )
                    {
                        SPI_Com_Buf[ 3 ] = 0x01;
                    }
                    recv_len = 4;
                    break;

                case DEF_CMD_SPI_CONTROL:
                    /* 0xC1---SPI接口控制命令 */
                    /* (1)、1个字节片选(CS1)引脚控制及后续控制：
                       (2)、2个字节片选(CS1)引脚当前控制延时时间，单位为uS，低字节在前；0x0000表示不进行延时；
                       (3)、2个字节片选(CS1)引脚后续控制延时时间，单位为uS，低字节在前；0x0000表示不进行延时；
                       (4)、1个字节片选(CS2)引脚控制及后续控制：
                       (5)、2个字节片选(CS2)引脚当前控制延时时间，单位为uS，低字节在前；0x0000表示不进行延时；
                       (6)、2个字节片选(CS2)引脚后续控制延时时间，单位为uS，低字节在前；0x0000表示不进行延时；*/

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = DEF_CMD_C1_INFOLEN;

                    /* (1)、1个字节片选(CS1)引脚控制及后续控制 */
                    if( *pTxbuf & 0x80 )
                    {
                        if( *pTxbuf & 0x40 )
                        {
                            PIN_SPI_CS0_HIGH( );
                        }
                        else
                        {
                            PIN_SPI_CS0_LOW( );
                        }
                    }
                    COMM.CS0_LaterOpStatus = 0x00;
                    if( *pTxbuf & 0x20 )
                    {
                        COMM.CS0_LaterOpStatus = 0x01;
                    }
                    pTxbuf++;

                    /* (2)、2个字节片选(CS1)引脚当前控制延时时间 */
                    temp16 = *pTxbuf++;
                    temp16 += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    if( temp16 )
                    {
                        Delay_uS( temp16 );
                    }

                    /* (3)、2个字节片选(CS1)引脚后续控制延时时间 */
                    COMM.CS0_LaterOpDelay = *pTxbuf++;
                    COMM.CS0_LaterOpDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                    /* (4)、1个字节片选(CS2)引脚控制及后续控制 */
                    if( *pTxbuf & 0x80 )
                    {
                        if( *pTxbuf & 0x40 )
                        {
                            PIN_SPI_CS1_HIGH( );
                        }
                        else
                        {
                            PIN_SPI_CS1_LOW( );
                        }
                    }
                    COMM.CS1_LaterOpStatus = 0x00;
                    if( *pTxbuf & 0x20 )
                    {
                        COMM.CS1_LaterOpStatus = 0x01;
                    }
                    pTxbuf++;

                    /* (5)、2个字节片选(CS2)引脚当前控制延时时间 */
                    temp16 = *pTxbuf++;
                    temp16 += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    if( temp16 )
                    {
                        Delay_uS( temp16 );
                    }

                    /* (6)、2个字节片选(CS2)引脚后续控制延时时间 */
                    COMM.CS1_LaterOpDelay = *pTxbuf++;
                    COMM.CS1_LaterOpDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                    /* 注：该命令无应答 */
                    break;

                case DEF_CMD_SPI_RD_WR:
                    /* 0xC2---SPI接口常规读取写入数据命令 */
                    count = pack_len;
                    if( count >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        count = DEF_SPI_TXRX_LEN_MAX;
                    }
                    tx_len = count;

                    /* 看门狗喂狗 */
#if( DEF_WWDG_FUN_EN == 0x01 )
                    WWDG_Tr = WWDG->CTLR & 0x7F;
                    if( WWDG_Tr < WWDG_Wr )
                    {
                        WWDG->CTLR = WWDG_CNT;
                    }
#endif

                    /* 发送数据并回读数据 */
                    recv_len = 3;
                    if( SPI_Cfg.SPI_DataSize == SPI_DataSize_8b )
                    {
                        while( count )
                        {
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_RXNE ) == RESET );
                            SPI_Com_Buf[ recv_len++ ] = SPI2->DATAR;
                            if( COMM.SPI_ByteDelay )
                            {
                                Delay_uS( COMM.SPI_ByteDelay );
                            }
                            count--;
                        }
                    }
                    else
                    {
                        while( count )
                        {
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            temp16 = *pTxbuf++;
                            temp16 |= (UINT16)( *pTxbuf++ << 8 );
                            SPI2->DATAR = temp16;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_RXNE ) == RESET );
                            temp16 = SPI2->DATAR;
                            SPI_Com_Buf[ recv_len++ ] = (UINT8)temp16;
                            SPI_Com_Buf[ recv_len++ ] = (UINT8)( temp16 >> 8 );
                            if( COMM.SPI_ByteDelay )
                            {
                                Delay_uS( COMM.SPI_ByteDelay );
                            }
                            count--;
                            count--;
                        }
                    }

                    /* 判断是否需要自动撤销片选 */
                    if( COMM.CS0_LaterOpStatus )
                    {
                        if( COMM.CS0_LaterOpDelay )
                        {
                            Delay_uS( COMM.CS0_LaterOpDelay );
                        }
                        PIN_SPI_CS0_HIGH( );
                    }
                    if( COMM.CS1_LaterOpStatus )
                    {
                        if( COMM.CS1_LaterOpDelay )
                        {
                            Delay_uS( COMM.CS1_LaterOpDelay );
                        }
                        PIN_SPI_CS1_HIGH( );
                    }

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = tx_len;

                    /* 设置返回应答包 */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_RD_WR;
                    SPI_Com_Buf[ 1 ] = (UINT8)( tx_len );
                    SPI_Com_Buf[ 2 ] = (UINT8)( tx_len >> 8 );
                    break;

                case DEF_CMD_SPI_BLCK_RD:
                    /* 0xC3---SPI接口批量读取数据命令 */

                    /* 计算本次收发长度 */
                    COMM.Rx_TotalLen = *pTxbuf++;
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 16 );
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 24 );
                    COMM.Tx_CurLen = COMM.Rx_TotalLen;
                    if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
                    }

                    /* 配置DMA并进行数据发送 */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_RD;
                    SPI_Com_Buf[ 1 ] = (UINT8)COMM.Tx_CurLen;
                    SPI_Com_Buf[ 2 ] = (UINT8)( COMM.Tx_CurLen >> 8 );
                    SPIx_TxRx_DMA_Init( SPI_TxDMA_Buf, COMM.Tx_CurLen, &SPI_Com_Buf[ 3 ], COMM.Tx_CurLen );

                    COMM.Tx_Rx_Status = DEF_SPI_STATUS_RX;

                    /* 设置当前命令包长度(执行成功才能进行计算) */
                    COMM.CMDP_Cur_CMDLen = 0x00;

                    /* 设置跳出while标志 */
                    sp_flag = 0x01;
                    break;

                case DEF_CMD_SPI_BLCK_WR:
                    /* 0xC4---SPI接口批量写入数据命令 */

                    /* 计算本次收发长度 */
                    COMM.Rx_TotalLen = pack_len;
                    COMM.Tx_CurLen = COMM.Rx_TotalLen;
                    if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
                    }

                    /* 配置DMA并进行数据发送 */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_WR;
                    SPI_Com_Buf[ 1 ] = 0x01;
                    SPI_Com_Buf[ 2 ] = 0x00;
                    SPI_Com_Buf[ 3 ] = 0x00;
                    SPIx_TxRx_DMA_Init( pTxbuf, COMM.Tx_CurLen, &SPI_Com_Buf[ 4 ], COMM.Tx_CurLen );

                    COMM.Tx_Rx_Status = DEF_SPI_STATUS_TX;

                    /* 设置当前命令包长度(执行成功才能进行计算) */
                    COMM.CMDP_Cur_CMDLen = 0x00;

                    /* 设置跳出while标志 */
                    sp_flag = 0x01;
                    break;

                case DEF_CMD_INFO_RD:
                     /* 0xCA---参数获取命令 */
                     /* 0x00：表示获取芯片相关信息；
                        0x01：表示获取SPI接口相关信息；
                        0x02：表示获取JTAG接口相关信息； */
                     dat = *pTxbuf++;
                     if( dat == 0 )
                     {
                         /* 0x00：表示获取芯片相关信息； */
                         buf[ 0 ] = DEF_IC_PRG_VER;
                         buf[ 1 ] = 0x00;
                         buf[ 2 ] = 0;
                         buf[ 3 ] = 0;
                         count = 4;
                     }
                     else if( dat == 1 )
                     {
                         /* 0x01：表示获取SPI接口、IIC接口相关信息 */
                         memcpy( buf, &SPI_Cfg.SPI_Direction, 18 );
                         buf[ 18 ] = (UINT8)COMM.SPI_ByteDelay;
                         buf[ 19 ] = (UINT8)( COMM.SPI_ByteDelay >> 8 );
                         buf[ 20 ] = COMM.SPI_FillData;
                         buf[ 21 ] = COMM.Bit_Control;
                         buf[ 22 ] = 0x00;
                         buf[ 23 ] = 0x00;
                         buf[ 24 ] = 0x00;
                         buf[ 25 ] = 0x00;
                         count = 26;
                     }
                     else if( dat == 2 )
                     {
                         /* 0x02：表示获取JTAG接口相关信息 */
                         buf[ 0 ] = JTAG_Mode;
                         buf[ 1 ] = JTAG_Speed;
                         buf[ 2 ] = 0x00;
                         buf[ 3 ] = 0x00;
                         buf[ 4 ] = 0x00;
                         buf[ 5 ] = 0x00;
                         count = 6;
                     }
                     else
                     {
                         count = 0x00;
                     }

                     /* 设置返回应答包 */
                     SPI_Com_Buf[ 0 ] = DEF_CMD_INFO_RD;
                     SPI_Com_Buf[ 1 ] = (UINT8)count;
                     SPI_Com_Buf[ 2 ] = 0x00;
                     memcpy( &SPI_Com_Buf[ 3 ], buf, (UINT8)count );
                     recv_len = count + 3;

                     /* 设置当前命令包长度 */
                     COMM.CMDP_Cur_CMDLen = 1;
                     break;

                case DEF_CMD_JTAG_INIT:
                    /* 0xD0---JTAG接口初始化命令 */
                    /* (1)、1个字节：工作模式；
                            0x00：bit-bang模式；
                            0x01：自定义协议的快速模式；
                        (2)、1个字节：通信速度；有效值为0-5，值越大通信速度越快；
                        (3)、4个字节：保留；
                    */
                    JTAG_Mode = *pTxbuf++;
                    JTAG_Speed = *pTxbuf++;
                    JTAG_Port_Init( );

                    /* 初始化SPI接口 */
                    if( JTAG_Speed == 5 )
                    {
                        temp16 = SPI_BaudRatePrescaler_2;
                    }
                    else if( JTAG_Speed == 4 )
                    {
                        temp16 = SPI_BaudRatePrescaler_4;
                    }
                    else if( JTAG_Speed == 3 )
                    {
                        temp16 = SPI_BaudRatePrescaler_8;
                    }
                    else if( JTAG_Speed == 2 )
                    {
                        temp16 = SPI_BaudRatePrescaler_16;
                    }
                    else if( JTAG_Speed == 1 )
                    {
                        temp16 = SPI_BaudRatePrescaler_32;
                    }
                    else if( JTAG_Speed == 0 )
                    {
                        temp16 = SPI_BaudRatePrescaler_64;
                    }
                    else
                    {
                        temp16 = SPI_BaudRatePrescaler_4;
                    }
                    JTAG_SPI_Init( temp16 );

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = 6;

                    /* 清空USB上传缓冲区相关变量 */
                    COMM.Rx_LoadPtr = 0;
                    COMM.Rx_DealPtr = 0;
                    COMM.Rx_RemainLen = 0;

                    /* 设置返回应答包 */
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = DEF_CMD_JTAG_INIT;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x01;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x00;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x00;
                    COMM.Rx_RemainLen = 4;
                    break;

                case DEF_CMD_JTAG_BIT_OP:
                    /* 0xD1---JTAG接口引脚位控制命令 */

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* 执行命令 */
                    while( count-- )
                    {
                        JTAG_Port_BitShift( *pTxbuf++ );
                    }
                    break;

                case DEF_CMD_JTAG_BIT_OP_RD:
                    /* 0xD2---JTAG接口引脚位控制并读取命令 */

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* 判断是否装载应答包的包头数据 */
                    if( CMDPack_Op_Len_Save == CMDPack_Op_Len )
                    {
                    	CMDPack_Op_Len_Save = 0x00;
                        COMM_Load_PackHead( DEF_CMD_JTAG_BIT_OP_RD, ( CMDPack_Op_Len / 2 ) );
                    }

                    /* 执行命令 */
                    while( count-- )
                    {
                        /* 发送数据 */
                    	dat = *pTxbuf++;
                    	JTAG_Port_BitShift( dat );

                        /* 将读取数据装置到接收缓冲区 */
                        if( dat & DEF_TCK_BIT_OUT )
                        {
                            Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = JTAG_Port_BitRead( );
                            COMM.Rx_LoadPtr++;
                            if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
                            {
                                COMM.Rx_LoadPtr = 0;
                            }
                            COMM.Rx_RemainLen++;
                        }
                    }
                    break;

                case DEF_CMD_JTAG_DATA_SHIFT:
                    /* 0xD3---JTAG接口数据移位命令 */

#if( DEF_WWDG_FUN_EN == 0x01 )
                    /* 临时关闭WWDG看门狗时钟 */
                    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, DISABLE );
#endif

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* JTAG接口设置为SPI模式 */
                    JTAG_Port_SwTo_SPIMode( );

                    /* 执行命令 */
                    if( count == DEF_HS_PACK_MAX_LEN )
                    {
                        /* 采用SPI模式发送数据 */
                        for( i = 0; i < 10; i++ )
                        {
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );

                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );

                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );

                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );

                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        }
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        SPI2->DATAR = *pTxbuf++;
                        while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                    }
                    else
                    {
                        while( count-- )
                        {
                            SPI2->DATAR = *pTxbuf++;
                            while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET );
                        }
                    }

                    /* 等待SPI传输结束  */
                    while( SPI2->STATR & SPI_I2S_FLAG_BSY );

                    /* JTAG接口设置为GPIO模式 */
                    JTAG_Port_SwTo_GPIOMode( );

#if( DEF_WWDG_FUN_EN == 0x01 )
                    /* 喂狗 */
                    WWDG_Tr = WWDG->CTLR & 0x7F;
                    if( WWDG_Tr < WWDG_Wr )
                    {
                        WWDG->CTLR = WWDG_CNT;
                    }

                    /* 重新打开WWDG看门狗时钟 */
                    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );
#endif
                    break;

                case DEF_CMD_JTAG_DATA_SHIFT_RD:
                    /* 0xD4---JTAG接口数据移位并读取命令 */

                    /* 设置当前命令包长度 */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* 判断是否装载应答包的包头数据 */
                    if( CMDPack_Op_Len_Save == CMDPack_Op_Len )
                    {
                        CMDPack_Op_Len_Save = 0x00;
                        COMM_Load_PackHead( DEF_CMD_JTAG_DATA_SHIFT_RD, CMDPack_Op_Len );
                    }

                    /* 执行命令 */
                    while( count-- )
                    {
                        /* 将读取数据装置到接收缓冲区 */
                        Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = JTAG_Port_DataShift_RD( *pTxbuf++ );
                        COMM.Rx_LoadPtr++;
                        if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
                        {
                            COMM.Rx_LoadPtr = 0;
                        }
                        COMM.Rx_RemainLen++;
                    }
                    break;

                default:
                    /* 设置当前命令包长度(一次性执行完毕) */
                    pack_len = CMDPack_Op_Len;
                    COMM.CMDP_Cur_CMDLen = COMM.CMDP_Cur_RemainLen;

                    DUG_PRINTF("ERROR\n");
                    break;
            }

            /* 切换命令执行状态 */
            CMDPack_Op_Len -= pack_len;
            if( CMDPack_Op_Len == 0 )
            {
                CMDPack_Op_Status = 0x00;
            }

            /* 判断是否是SPI命令处理 */
            if( ( CMDPack_Op_Code < DEF_CMD_JTAG_INIT ) )
            {
                /* 判断是否需要上传数据 */
                if( recv_len )
                {
                    USB_SPI_DataUp( SPI_Com_Buf, recv_len );
                }
            }

            /* 计算当前命令包剩余长度及偏移，并判断是否需要切换缓冲区 */
            COMM_CMDPack_Switch( 0x00 );
        }
    }

    /* 判断是否有JTAG数据需要上传(执行完完整的1包才启动数据上传) */
    if( CMDPack_Op_Status == 0 )
    {
        COMM_RxData_Up_Deal( );
    }
    return( 0x00 );
}

/*******************************************************************************
* Function Name  : COMM_RxData_Up_Deal
* Description    : 接口通信接收数据上传处理
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void COMM_RxData_Up_Deal( void )
{
    UINT16 pack_len;
    UINT16 send_len;
    UINT32 offset;

    /* 判断前一包数据是否上传完毕 */
    if( COMM.USB_Up_IngFlag )
    {
        /* 超时100mS未取走数据则取消本次数据上传 */
        if( COMM.USB_Up_TimeOut >= 1000 )
        {
            COMM.USB_Up_TimeOut = 0;
            COMM.USB_Up_IngFlag = 0;
        }
        return;
    }

    /* 判断是否有数据需要通过USB接口进行上传 */
    if( COMM.Rx_RemainLen == 0x00 )
    {
        /* 无数据需要上传则直接返回 */
        return;
    }

    /* 如果数据包已满则最多上传整包数据，否则超时上传剩余数据 */
    send_len = COMM.Rx_RemainLen;
    pack_len = 0x00;
    if( USBHS_Up_PackLenMax == DEF_USB_HS_PACK_LEN )
    {
        /* 计算本次上传长度 */
        if( send_len >= DEF_USB_HS_PACK_LEN )
        {
            pack_len = DEF_USB_HS_PACK_LEN;
        }
        else
        {
            if( COMM.Rx_IdleCount >= 100 )
            {
                COMM.Rx_IdleCount = 0;
                pack_len = send_len;
            }
        }
    }
    else
    {
        /* 计算本次上传长度 */
        if( send_len >= DEF_USB_FS_PACK_LEN )
        {
            pack_len = DEF_USB_FS_PACK_LEN;
        }
        else
        {
            if( COMM.Rx_IdleCount >= 100 )
            {
                COMM.Rx_IdleCount = 0;
                pack_len = send_len;
            }
        }
    }

    /* 判断到达缓冲区默认是否足够 */
    offset = DEF_COMM_RX_BUF_LEN - COMM.Rx_DealPtr;
    if( pack_len > offset )
    {
        pack_len = offset;
    }

//   DUG_PRINTF("Up:%x\n",pack_len);

    /* 如果有数据需要上传，则启动数据上传 */
    if( pack_len )
    {
        memcpy( &EP1_Tx_Databuf[ 0 ], &Comm_Rx_Buf[ COMM.Rx_DealPtr ], pack_len );

        /* 设置DMA地址并启动上传 */
        COMM.USB_Up_PackLen = pack_len;
        COMM.USB_Up_IngFlag = 0x01;
        COMM.USB_Up_TimeOut = 0x00;
        USBHSD->UEP1_TX_DMA = (UINT32)(UINT8 *)EP1_Tx_Databuf;
        USBHSD->UEP1_TX_LEN  = pack_len;
        USBHSD->UEP1_TX_CTRL &= ~USBHS_EP_T_RES_MASK;
        USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_ACK;
    }
}

/*******************************************************************************
* Function Name  : JTAG_Mode1_Deal
* Description    : JTAG模式1处理
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Mode1_Deal( void )
{
    UINT8  *pbuf;
    UINT8  dat;
    UINT16 send_len;
    UINT16 i;

    /**********************************************************************/
    /* 判断是否有数据需要通过JTAG接口进行发送 */
    if( COMM.CMDP_RemainNum )
    {
        /* 判断是从上次未发送完毕的缓冲区装载还是从新缓冲区中装载 */
        if( COMM.CMDP_Cur_RemainLen == 0x00 )
        {
            COMM.CMDP_Cur_RemainLen = COMM.CMDP_PackLen[ COMM.CMDP_DealNum ];
            COMM.CMDP_Cur_DealPtr = ( COMM.CMDP_DealNum * DEF_USB_HS_PACK_LEN );
        }

        /* 从缓冲区中取出数据分析处理 */
        FLAG_Send = 1;
        pbuf = &Comm_Tx_Buf[ COMM.CMDP_Cur_DealPtr ];
        while( COMM.CMDP_Cur_RemainLen && ( COMM.Rx_RemainLen < 62 ) )
        {
            dat = *pbuf++;
            if( 0 == JTAG_Shift_Cnt )
            {
                JTAG_Shift_Mode = ( 0 != ( dat & DEF_SHIFT_MODE ) );
                JTAG_Read_Mode = ( 0 != ( dat & DEF_READ_MODE ) );
                if( JTAG_Shift_Mode )
                {
                    JTAG_Shift_Cnt = ( dat & DEF_CNT_MASK );
                }
                else if( JTAG_Read_Mode )
                {
                    JTAG_Port_BitShift( dat );

                    /* 将读取数据装置到接收缓冲区 */
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = JTAG_Port_BitRead( );
                    COMM.Rx_LoadPtr++;
                    if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
                    {
                        COMM.Rx_LoadPtr = 0;
                    }
                    COMM.Rx_RemainLen++;
                }
                else
                {
                    JTAG_Port_BitShift( dat );
                }
            }
            else
            {
                if( JTAG_Read_Mode )
                {
                    dat = JTAG_Port_DataShift_RD( dat );

                    /* 将读取数据装置到接收缓冲区 */
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr ] = dat;
                    COMM.Rx_LoadPtr++;
                    if( COMM.Rx_LoadPtr >= DEF_COMM_RX_BUF_LEN )
                    {
                        COMM.Rx_LoadPtr = 0;
                    }
                    COMM.Rx_RemainLen++;
                }
                else
                {
                    JTAG_Port_DataShift( dat );
                }
                JTAG_Shift_Cnt--;
            }
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;
        }

        /* 关闭USB中断 */
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );

        /* 计算相关变量 */
        if( COMM.CMDP_Cur_RemainLen == 0x00 )
        {
            COMM.CMDP_PackLen[ COMM.CMDP_DealNum ] = 0x0000;
            COMM.CMDP_DealNum++;
            if( COMM.CMDP_DealNum >= DEF_COMM_BUF_PACKNUM_MAX )
            {
                COMM.CMDP_DealNum = 0x00;
            }
            COMM.CMDP_RemainNum--;
        }

        /* 如果当前串口已暂停下传则重启驱动下传 */
        if( ( COMM.USB_Down_StopFlag == 0x01 ) &&
            ( COMM.CMDP_RemainNum < ( DEF_COMM_BUF_PACKNUM_MAX - 2 ) ) )
        {
            USBHSD->UEP2_RX_CTRL &= ~USBHS_EP_R_RES_MASK;
            USBHSD->UEP2_RX_CTRL |= USBHS_EP_R_RES_ACK;
            COMM.USB_Down_StopFlag = 0x00;
        }

        /* 重新打开USB中断 */
        NVIC_EnableIRQ( USBHS_IRQn );
    }

    /**********************************************************************/
    /* 判断是否有数据需要通过USB接口进行上传 */
    if( !FLAG_Send )
    {
        return;
    }

    send_len = COMM.Rx_RemainLen;
    if( send_len >= ( DEF_USB_FS_PACK_LEN - 2 ) )
    {
        if( send_len >= DEF_USB_FS_PACK_LEN - 2 )
        {
            send_len = DEF_USB_FS_PACK_LEN - 2;
        }
        EP1_Tx_Databuf[ 0 ] = 0x01;
        EP1_Tx_Databuf[ 1 ] = 0x60;
        memcpy( &EP1_Tx_Databuf[ 2 ], &Comm_Rx_Buf[ COMM.Rx_DealPtr ], send_len );
        COMM.Rx_DealPtr += send_len;
        if( COMM.Rx_DealPtr >= DEF_COMM_RX_BUF_LEN )
        {
            COMM.Rx_DealPtr = 0x00;
        }
        COMM.Rx_RemainLen -= send_len;

        FLAG_Send = 0;
        USBHSD->UEP1_TX_LEN  = send_len + 2;
        USBHSD->UEP1_TX_CTRL &= ~USBHS_EP_T_RES_MASK;
        USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_ACK;
    }
    else if( JTAG_Time_Count >= 10 )
    {
        JTAG_Time_Count = 0;

        EP1_Tx_Databuf[ 0 ] = 0x01;
        EP1_Tx_Databuf[ 1 ] = 0x60;
        memcpy( &EP1_Tx_Databuf[ 2 ], &Comm_Rx_Buf[ COMM.Rx_DealPtr ], send_len );
        COMM.Rx_DealPtr += send_len;
        if( COMM.Rx_DealPtr >= DEF_COMM_RX_BUF_LEN )
        {
            COMM.Rx_DealPtr = 0x00;
        }
        COMM.Rx_RemainLen -= send_len;

        FLAG_Send = 0;
        USBHSD->UEP1_TX_LEN  = send_len + 2;
        USBHSD->UEP1_TX_CTRL &= ~USBHS_EP_T_RES_MASK;
        USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_ACK;
    }

    /* 等待端点1上传数据完成  */
    for(  i = 0; i < 100; i++ )
    {
        if( ( USBHSD->UEP1_TX_CTRL & USBHS_EP_T_RES_NAK ) == USBHS_EP_T_RES_NAK )
        {
            /* 端点1上传完成,从中断出来 */
            break;
        }
        Delay_Us( 1 );
    }
}


/*******************************************************************************
* Function Name  : SPIx_Cfg_DefInit
* Description    : SPIx接口配置默认初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Cfg_DefInit( void )
{
    SPI_Cfg.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_Cfg.SPI_Mode = SPI_Mode_Master;
    SPI_Cfg.SPI_DataSize = SPI_DataSize_8b;
    SPI_Cfg.SPI_CPOL = SPI_CPOL_High;
    SPI_Cfg.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_Cfg.SPI_NSS = SPI_NSS_Soft;
    SPI_Cfg.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_Cfg.SPI_FirstBit = SPI_FirstBit_MSB;// SPI_FirstBit_LSB;
    SPI_Cfg.SPI_CRCPolynomial = 7;

    COMM.Tx_TimeOutMax = 5000;
    COMM.USB_Up_TimeOutMax = 5000;
}

/*******************************************************************************
* Function Name  : SPIx_Port_Init
* Description    : SPIx接口初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Port_Init( void )
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    SPI_InitTypeDef  SPI_InitStructure={0};

    /* 打开SPIx时钟 */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );

    /* 复位SPI,以便清除SPI_I2S_FLAG_RXNE位 */
    RCC_APB1PeriphResetCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3, ENABLE );
    RCC_APB1PeriphResetCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3, DISABLE );
    SPI2->HSCR |= ( 1 << 0 );

    /* 配置相关引脚 */
    GPIO_SetBits( GPIOB, GPIO_Pin_12 );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    SPI_SSOutputCmd( SPI2, ENABLE );

    /* 配置SPIx */
    SPI_InitStructure.SPI_Direction = SPI_Cfg.SPI_Direction;
    SPI_InitStructure.SPI_Mode = SPI_Cfg.SPI_Mode;
    SPI_InitStructure.SPI_DataSize = SPI_Cfg.SPI_DataSize;
    SPI_InitStructure.SPI_CPOL = SPI_Cfg.SPI_CPOL;
    SPI_InitStructure.SPI_CPHA = SPI_Cfg.SPI_CPHA;
    SPI_InitStructure.SPI_NSS = SPI_Cfg.SPI_NSS;//SPI_NSS_Hard;//SPI_NSS_Soft;  // SPI_NSS_Hard;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_Cfg.SPI_BaudRatePrescaler;
    SPI_InitStructure.SPI_FirstBit = SPI_Cfg.SPI_FirstBit; //SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = SPI_Cfg.SPI_CRCPolynomial;
    SPI_Init( SPI2, &SPI_InitStructure );

    /* 判断是否接收标志为1,如果是则读取数据寄存器清0(CH32V307的S[I2\SPI3,该位默认值为1) */
    if( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == SET )
    {
        SPI2->DATAR;
    }

    /* 使能SPIx */
    SPI_Cmd( SPI2, ENABLE );
}

/*******************************************************************************
* Function Name  : SPIx_Tx_DMA_Init
* Description    : SPIx接口发送DMA初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Tx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize )
{
    DMA_InitTypeDef DMA_InitStructure = {0};

//    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

    DMA_DeInit( DMA_CHx );

    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init( DMA_CHx, &DMA_InitStructure );
}

/*******************************************************************************
* Function Name  : SPIx_Rx_DMA_Init
* Description    : SPIx接口接收DMA初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Rx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize )
{
    DMA_InitTypeDef DMA_InitStructure={0};

//    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

    DMA_DeInit( DMA_CHx );

    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init( DMA_CHx, &DMA_InitStructure );
}

/*******************************************************************************
* Function Name  : SPIx_RD_WR_Byte
* Description    : SPIx发送并读写1个字节
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 SPIx_RD_WR_Byte( UINT8 dat )
{
    UINT16 i = 0;

    /* Wait for SPI1 Tx buffer empty */
    i = 0;
//  while( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_TXE ) == RESET )
    while( ( SPI2->STATR & SPI_I2S_FLAG_TXE ) == RESET )
    {
        i++;
        if( i > 2000 )
        {
            return( 0x00 );
        }
    }
    SPI2->DATAR = dat;

    i = 0;
//  while( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == RESET )
    while( ( SPI2->STATR & SPI_I2S_FLAG_RXNE ) == RESET )
    {
        i++;
        if( i > 2000 )
        {
            return( 0x00 );
        }
    }
    return( SPI2->DATAR );
}

/*******************************************************************************
* Function Name  : SPIx_TxRx_DMA_Init
* Description    : SPIx发送接收数据DMA初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_TxRx_DMA_Init( UINT8 *pTxBuf, UINT16 Txlen, UINT8 *pRxBuf, UINT16 Rxlen )
{
    /* 配置DMA并进行发送和接收 */
    SPI_Cmd( SPI2, DISABLE );

    SPI2->CTLR2 &= ~SPI_I2S_DMAReq_Tx;
    SPI2->CTLR2 &= ~SPI_I2S_DMAReq_Rx;
    DMA1->INTFCR = DMA1_FLAG_TC4 | DMA1_FLAG_TC5;
    DMA1_Channel5->CFGR &= (uint16_t)(~DMA_CFGR1_EN);                           // DMA_Cmd( DMA1_Channel5, DISABLE );
    DMA1_Channel5->MADDR = (UINT32)&pTxBuf[ 0 ];
    DMA1_Channel5->PADDR = (UINT32)&SPI2->DATAR;
    DMA1_Channel5->CFGR = 0x3090;
    DMA1_Channel5->CNTR = Txlen;
    DMA1_Channel4->CFGR &= (uint16_t)(~DMA_CFGR1_EN);                           // DMA_Cmd( DMA1_Channel4, DISABLE );
    DMA1_Channel4->MADDR = (UINT32)&pRxBuf[ 0 ];
    DMA1_Channel4->PADDR = (UINT32)&SPI2->DATAR;
    DMA1_Channel4->CFGR = 0x20a0;
    DMA1_Channel4->CNTR = Rxlen;
    SPI2->CTLR2 |= SPI_I2S_DMAReq_Tx;                                           // SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx, ENABLE );
    SPI2->CTLR2 |= SPI_I2S_DMAReq_Rx;                                           // SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Rx, ENABLE );
    DMA1_Channel5->CFGR |= DMA_CFGR1_EN;                                        // DMA_Cmd( DMA1_Channel5, ENABLE );
    DMA1_Channel4->CFGR |= DMA_CFGR1_EN;                                        // DMA_Cmd( DMA1_Channel4, ENABLE );

    SPI_Cmd( SPI2, ENABLE );
}

/*******************************************************************************
* Function Name  : SPIx_Tx_DMA_Deal
* Description    : SPIx发送数据DMA处理
*                                                 仅进行单次发送，查询是否发送完成，发送完成则上传执行状态包
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Tx_DMA_Deal( void )
{
    /* 查询SPI的DMA发送是否完成 */
    if( ( ( DMA1->INTFR & DMA1_FLAG_TC4 ) && ( DMA1->INTFR & DMA1_FLAG_TC5 ) ) ||
        ( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax ) )
    {
        /* 清标志并关闭DMA */
        SPI2->STATR &= ~SPI_I2S_FLAG_TXE;
        SPI2->STATR &= ~SPI_I2S_FLAG_RXNE;

        DMA1->INTFCR = DMA1_FLAG_TC4;
        DMA1->INTFCR = DMA1_FLAG_TC5;
        DMA1_Channel5->CFGR &= (uint16_t)(~DMA_CFGR1_EN);
        DMA1_Channel4->CFGR &= (uint16_t)(~DMA_CFGR1_EN);

        /* 计算当前命令包剩余长度及偏移，并判断是否需要切换缓冲区 */
        COMM.CMDP_Cur_CMDLen = ( COMM.Tx_CurLen + 4 );
        COMM_CMDPack_Switch( 0x00 );

        /* 切换SPI收发状态 */
        COMM.Tx_Rx_Status = DEF_SPI_STATUS_IDLE;

        /* USB上传执行状态 */
        if( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax )
        {
            SPI_Com_Buf[ 3 ] = 0x01;
        }
        USB_SPI_DataUp( SPI_Com_Buf, 4 );
    }
}

/*******************************************************************************
* Function Name  : SPIx_Rx_DMA_Deal
* Description    : SPIx接收数据DMA处理
*                  启动发送，查询本次是否发送完成，本次发送完成则上传接收数据，判断是否还需要接收，
*                  如果需要则继续启动发送，否则完成整个接收
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Rx_DMA_Deal( void )
{
    /* 查询SPI的DMA发送是否完成 */
    if( ( ( DMA1->INTFR & DMA1_FLAG_TC4 ) && ( DMA1->INTFR & DMA1_FLAG_TC5 ) ) ||
        ( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax ) )
    {
        /* 清标志并关闭DMA */
        SPI2->STATR &= ~SPI_I2S_FLAG_TXE;
        SPI2->STATR &= ~SPI_I2S_FLAG_RXNE;

        DMA1->INTFCR = DMA1_FLAG_TC4;
        DMA1->INTFCR = DMA1_FLAG_TC5;
        DMA1_Channel5->CFGR &= (uint16_t)(~DMA_CFGR1_EN);
        DMA1_Channel4->CFGR &= (uint16_t)(~DMA_CFGR1_EN);

        /* 启动USB上传接收数据 */
        USB_SPI_DataUp( SPI_Com_Buf, ( COMM.Tx_CurLen + 3 ) );

        /* 计算是否还有剩余数据未读取 */
        COMM.Rx_TotalLen -= COMM.Tx_CurLen;
        if( COMM.Rx_TotalLen == 0x00 )
        {
            /* 全部数据接收完毕 */

            /* 计算当前命令包剩余长度及偏移，并判断是否需要切换缓冲区 */
            COMM.CMDP_Cur_CMDLen = DEF_CMD_C3_INFOLEN;
            COMM_CMDPack_Switch( 0x00 );

            /* 切换SPI收发状态 */
            COMM.Tx_Rx_Status = DEF_SPI_STATUS_IDLE;
        }
        else
        {
            /* 继续启动DMA发送并接收数据 */

            /* 计算本次收发长度 */
            COMM.Tx_CurLen = COMM.Rx_TotalLen;
            if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
            {
                COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
            }

            /* 配置DMA并进行数据发送 */
            SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_RD;
            SPI_Com_Buf[ 1 ] = (UINT8)COMM.Tx_CurLen;
            SPI_Com_Buf[ 2 ] = (UINT8)( COMM.Tx_CurLen >> 8 );
            SPIx_TxRx_DMA_Init( SPI_TxDMA_Buf, COMM.Tx_CurLen, &SPI_Com_Buf[ 3 ], COMM.Tx_CurLen );

            /* 设置当前命令包长度(执行成功才能进行计算) */
            COMM.CMDP_Cur_CMDLen = 0x00;
        }
    }
}

/*******************************************************************************
* Function Name  : USB_SPI_DataUp
* Description    : USB转SPI数据上传
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_SPI_DataUp( UINT8 *pbuf, UINT16 len )
{
    /* 判断前一次数据是否正常取走,如果未正常取走则超时等待 */
    while( COMM.USB_Up_IngFlag )
    {
        if( COMM.USB_Up_TimeOut >= COMM.USB_Up_TimeOutMax )
        {
            COMM.USB_Up_IngFlag = 0x00;
            break;
        }
        else
        {
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

    /* 关闭USB中断 */
    NVIC_DisableIRQ( USBHS_IRQn );
    NVIC_DisableIRQ( USBHS_IRQn );
    NVIC_DisableIRQ( USBHS_IRQn );

//  DUG_PRINTF("SPI_UP:%x\n",len);

    /* 设置DMA地址并启动上传 */
    COMM.USB_Up_IngFlag = 0x01;
    COMM.USB_Up_TimeOut = 0x00;
    USBHSD->UEP1_TX_DMA = (UINT32)(UINT8 *)pbuf;
    USBHSD->UEP1_TX_LEN  = len;
    USBHSD->UEP1_TX_CTRL &= ~USBHS_EP_T_RES_MASK;
    USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_ACK;

    /* 重新打开USB中断 */
    NVIC_EnableIRQ( USBHS_IRQn );

    /* 判断前一次数据是否正常取走,如果未正常取走则超时等待 */
    while( COMM.USB_Up_IngFlag )
    {
        if( COMM.USB_Up_TimeOut >= COMM.USB_Up_TimeOutMax )
        {
            COMM.USB_Up_IngFlag = 0x00;
            break;
        }
        else
        {
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
//    DUG_PRINTF("SPI_UP_END\n");
}

/*******************************************************************************
* Function Name  : SPIx_ReadID_Test
* Description    : SPIx读取FLASH芯片ID测试
* Input          : None
* Output         : None
* Return         : 返回4个字节,最高字节为0x00,
*                  次高字节为Manufacturer ID(0xEF),
*                  次低字节为Memory Type ID
*                  最低字节为Capacity ID
*                  W25X40BL返回: 0xEF、0x30、0x13
*                  W25X10BL返回: 0xEF、0x30、0x11
*******************************************************************************/
void SPIx_ReadID_Test( void )
{
    UINT32 dat;

    /* Select the FLASH: Chip Select low */
   PIN_SPI_CS0_LOW( );

    /* Send "RDID " instruction */
   SPIx_RD_WR_Byte( 0x9F );                                                     /* 发送读取JEDEC_ID命令 */

    /* Read a byte from the FLASH */
    dat = ( UINT32 )SPIx_RD_WR_Byte( 0xFF ) << 16;

    /* Read a byte from the FLASH */
    dat |= ( UINT32 )SPIx_RD_WR_Byte( 0xFF ) << 8;

    /* Read a byte from the FLASH */
    dat |= SPIx_RD_WR_Byte( 0xFF );

    /* Deselect the FLASH: Chip Select high */
    PIN_SPI_CS0_HIGH( );

    DUG_PRINTF("%08x\n",dat);
}

