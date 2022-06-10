/******************** (C) COPYRIGHT 2011 WCH ***********************************
* File Name          : JTAG.c
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USBתJTAG������ز��ֳ���
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* ͷ�ļ����� */
#include <MAIN.h>                                                               /* ͷ�ļ����� */

/******************************************************************************/
/* ������������ */
volatile COMM_CTL COMM;                                                         /* �ӿ�ͨ�ſ�����ؽṹ�� */
__attribute__ ((aligned(4))) UINT8  Comm_Tx_Buf[ DEF_COMM_TX_BUF_LEN ];         /* �ӿ�ͨ�ŷ��ͻ�����(USB�´�������) */
__attribute__ ((aligned(4))) UINT8  Comm_Rx_Buf[ DEF_COMM_RX_BUF_LEN ];         /* �ӿ�ͨ�Ž��ջ�����(USB�ϴ�������) */

volatile UINT8  CMDPack_Op_Status = 0x00;                                       /* �����������ִ��״̬ */
volatile UINT8  CMDPack_Op_Code = 0x00;                                         /* ������������� */
volatile UINT16 CMDPack_Op_Len = 0x00;                                          /* ������ĳ��� */
volatile UINT16 CMDPack_Op_Len_Save = 0x00;                                     /* ������ĳ����ݴ� */

volatile UINT8  JTAG_Mode = 0x00;                                               /* JTAGģʽ��0��Э��ģʽ��1��bit-bangģʽ */
volatile UINT8  JTAG_Speed = 0x00;                                              /* JTAG�ٶȣ�0-4��4�ٶ����*/
volatile UINT8  JTAG_Read_Mode = 0;                                             /* JTAG��ȡģʽ */
volatile UINT8  JTAG_Shift_Mode = 0;                                            /* JTAG��λģʽ */
volatile UINT32 JTAG_Shift_Cnt = 0;                                             /* JTAG��λ���� */
volatile UINT32 JTAG_Time_Count = 0;                                            /* JTAG��ʱ */

SPI_InitTypeDef SPI_Cfg;                                                        /* SPI�ӿ����� */
__attribute__ ((aligned(4))) UINT8  SPI_TxDMA_Buf[ 1024 ];                      /* SPI_I2C����DMA������ */
__attribute__ ((aligned(4))) UINT8  SPI_Com_Buf[ 1024 ];                        /* SPI�������ݻ����� */

/*******************************************************************************
* Function Name  : JTAG_Init
* Description    : JTAG��ʼ��
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
* Description    : JTAGģʽ��SPIx�ӿڳ�ʼ��
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

    /* �ж��Ƿ���ձ�־Ϊ1,��������ȡ���ݼĴ�����0(CH32V307��S[I2\SPI3,��λĬ��ֵΪ1) */
    if( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == SET )
    {
        SPI2->DATAR;
    }

    SPI_Cmd( SPI2, DISABLE );
}

/*******************************************************************************
* Function Name  : JTAG_Port_SwTo_SPIMode
* Description    : JTAG�ӿ�����ΪSPIģʽ
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_SwTo_SPIMode( void )
{
    UINT32 temp;

    /* PB13,PB15: ������� ,50MHz; PB14: �������� */
    temp = GPIOB->CFGHR;
    temp &= 0x000FFFFF;
    temp |= 0xB4B00000;
    GPIOB->CFGHR = temp;

    /* ʹ��SPI */
    SPI2->CTLR1 |= CTLR1_SPE_Set;
}

/*******************************************************************************
* Function Name  : JTAG_Port_SwTo_GPIOMode
* Description    : JTAG�ӿ�����ΪGPIOģʽ
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void JTAG_Port_SwTo_GPIOMode( void )
{
    UINT32 temp;

    /* PB13,PB15: �������  ,50MHz; PB14: �������� */
    temp = GPIOB->CFGHR;
    temp &= 0x000FFFFF;
    temp |= 0x38300000;
    GPIOB->CFGHR = temp;;
    GPIOB->BSHR = (((uint32_t)0x01 ) << 14 );

    SPI2->CTLR1 &= CTLR1_SPE_Reset;
}

/*******************************************************************************
* Function Name  : COMM_CMDPack_Switch
* Description    : �ӿ�ͨ��������л�
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void COMM_CMDPack_Switch( UINT8 mode )
{
    /* ���㵱ǰ�����ʣ�೤�ȼ�ƫ�ƣ����ж��Ƿ���Ҫ�л������� */
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
        /* �л������������ */

        /* �ر�USB�ж� */
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );

        /* ������ر��� */
        COMM.CMDP_PackLen[ COMM.CMDP_DealNum ] = 0x0000;
        COMM.CMDP_DealNum++;
        if( COMM.CMDP_DealNum >= DEF_COMM_BUF_PACKNUM_MAX )
        {
            COMM.CMDP_DealNum = 0x00;
        }
        COMM.CMDP_RemainNum--;

        /* �����ǰSPI����ͣ�´������������´� */
        if( ( COMM.USB_Down_StopFlag == 0x01 ) &&
            ( COMM.CMDP_RemainNum < ( DEF_COMM_BUF_PACKNUM_MAX - 2 ) ) )
        {
            USBHSD->UEP2_RX_CTRL &= ~USBHS_EP_R_RES_MASK;
            USBHSD->UEP2_RX_CTRL |= USBHS_EP_R_RES_ACK;

            COMM.USB_Down_StopFlag = 0x00;
        }

        /* ���´�USB�ж� */
        NVIC_EnableIRQ( USBHS_IRQn );
    }
}

/*******************************************************************************
* Function Name  : COMM_Load_PackHead
* Description    : �ӿ�ͨ��װ��Э���ͷ
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
* Description    : �ӿ�ͨ�����������
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

    /* �����SPI�ӿ��������ڷ��ͣ����ѯִ��״̬ */
    if( COMM.Tx_Rx_Status == DEF_SPI_STATUS_TX )
    {
        SPIx_Tx_DMA_Deal( );
        return( 0x01 );
    }

    /* �����SPI�ӿ��������ڽ��գ����ѯִ��״̬ */
    if( COMM.Tx_Rx_Status == DEF_SPI_STATUS_RX )
    {
        SPIx_Rx_DMA_Deal( );
        return( 0x02 );
    }

    /* ���δ���յ��������ֱ�ӷ��� */
    if( COMM.CMDP_RemainNum == 0x00 )
    {
        /* �ж��Ƿ���JTAG������Ҫ�ϴ�(ִ����������1�������������ϴ�) */
        if( CMDPack_Op_Status == 0 )
        {
            COMM_RxData_Up_Deal( );
        }
        return( 0x03 );
    }

    /* �ж��Ƿ�ǰ������м���װ��ִ�У���������װ����һ����� */
    if( COMM.CMDP_Cur_RemainLen == 0x00 )
    {
        COMM.CMDP_Cur_RemainLen = COMM.CMDP_PackLen[ COMM.CMDP_DealNum ];
        COMM.CMDP_Cur_DealPtr = ( COMM.CMDP_DealNum * DEF_USB_HS_PACK_LEN );
    }

//	DUG_PRINTF("L:%x\n",COMM.CMDP_Cur_RemainLen);
//	DUG_PRINTF("S:%x\n",CMDPack_Op_Status);

    /* ��������������� */
    sp_flag = 0x00;
    op_flag = 0x00;
    pTxbuf = &Comm_Tx_Buf[ COMM.CMDP_Cur_DealPtr ];
    while( COMM.CMDP_Cur_RemainLen && ( sp_flag == 0 ) )
    {
        if( CMDPack_Op_Status == 0x00 )
        {
            /* ȡ1���ֽ������� */
            CMDPack_Op_Code = *pTxbuf++;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

//			DUG_PRINTF("CMD:%x\n",CMDPack_Op_Code);

            /* �ж��������Ƿ���ȷ,����Ǿɰ汾������ֱ�Ӱ�ԭ�ȸ�ʽ���� */
            if( ( CMDPack_Op_Code >= DEF_CMD_SPI_INIT ) &&
                ( CMDPack_Op_Code <= DEF_CMD_JTAG_DATA_SHIFT_RD ) )
            {
                /* �л�����ִ��״̬ */
                CMDPack_Op_Status = 0x01;
            }
            else
            {
                /* �л�����ִ��״̬ */
                CMDPack_Op_Status = 0x00;
            }

            /* �ж��Ƿ���Ҫ���л������л� */
            COMM_CMDPack_Switch( 0x01 );
        }
        else if( CMDPack_Op_Status == 0x01 )
        {
            /* ȡ2���ֽ������(��1�ֽ�) */
            CMDPack_Op_Len = *pTxbuf++;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

            /* �л�����ִ��״̬ */
            CMDPack_Op_Status = 0x02;

            /* �ж��Ƿ���Ҫ���л������л� */
            COMM_CMDPack_Switch( 0x01 );
        }
        else if( CMDPack_Op_Status == 0x02 )
        {
            /* ȡ2���ֽ������(��2�ֽ�) */
            CMDPack_Op_Len |= (((UINT16)*pTxbuf++) << 8 );
            CMDPack_Op_Len_Save = CMDPack_Op_Len;
            COMM.CMDP_Cur_DealPtr++;
            COMM.CMDP_Cur_RemainLen--;

//			DUG_PRINTF("C_L:%x\n",CMDPack_Op_Len);

            /* �л�����ִ��״̬ */
            CMDPack_Op_Status = 0x03;

            /* �ж��Ƿ���Ҫ���л������л� */
            COMM_CMDPack_Switch( 0x01 );
        }
        else
        {
            /* ִ�к���N���ֽ����� */

            /* �����������ִ�г��� */
            pack_len = CMDPack_Op_Len;
            if( pack_len >= COMM.CMDP_Cur_RemainLen )
            {
                pack_len = COMM.CMDP_Cur_RemainLen;
            }

            /* ���ݵ�ǰ��������ִ�к������� */
            switch( CMDPack_Op_Code )
            {
                case DEF_CMD_SPI_INIT:
                    /* 0xC0---SPI��ʼ������ */
                    /* (1)��18���ֽڵ�SPI��ʼ��������ΪSPI�������SPI.h���ļ��еĽṹ��SPI_InitTypeDef��
                       (2)��2���ֽ����ݶ�д֮�����ʱֵ(�����SPI�ӿڳ����ȡд����������(DEF_CMD_SPI_RD_WR))����λΪuS��
                       (3)��1���ֽ�SPI����Ĭ���ֽڣ�
                       (4)��1���ֽ�������ƣ�
                                                                          λ7��ƬѡCS1���Կ��ƣ�0���͵�ƽ��Ч��1���е�ƽ��Ч��
                                                                          λ6��ƬѡCS2���Կ��ƣ�0���͵�ƽ��Ч��1���е�ƽ��Ч��
                                                                          λ3-0��������
                       (5)��4���ֽڱ�����*/

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = DEF_CMD_C0_INFOLEN;

                    /* ִ�и����� */
                    if( pack_len >= COMM.CMDP_Cur_CMDLen )
                    {
                        /* (1)��18���ֽڵ�SPI��ʼ������ */
                        memcpy( &SPI_Cfg.SPI_Direction, pTxbuf, sizeof( SPI_InitTypeDef ) );
                        SPIx_Port_Init( );
                        SPIx_Tx_DMA_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)&SPI_TxDMA_Buf[ 0 ], 0x00 );
                        SPIx_Rx_DMA_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)&SPI_Com_Buf[ 0 ], 0x00 );
                        pTxbuf += 18;

                        /* (2)��2���ֽ����ݶ�д֮�����ʱֵ */
                        COMM.SPI_ByteDelay = *pTxbuf++;
                        COMM.SPI_ByteDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                        /* (3)��1���ֽ�SPI����Ĭ���ֽ� */
                        COMM.SPI_FillData = *pTxbuf++;

                        /* (4)��1���ֽ�������� */
                        COMM.Bit_Control = *pTxbuf++;

                        /* (5)��4���ֽڱ�����*/
                        pTxbuf += 4;
                    }
                    else
                    {
                        op_flag = 0x01;

                        SPIx_Port_Init( );
                        SPIx_Tx_DMA_Init( DMA1_Channel5, (u32)&SPI2->DATAR, (u32)&SPI_TxDMA_Buf[ 0 ], 0x00 );
                        SPIx_Rx_DMA_Init( DMA1_Channel4, (u32)&SPI2->DATAR, (u32)&SPI_Com_Buf[ 0 ], 0x00 );
                    }

                    /* ���÷���Ӧ��� */
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
                    /* 0xC1---SPI�ӿڿ������� */
                    /* (1)��1���ֽ�Ƭѡ(CS1)���ſ��Ƽ��������ƣ�
                       (2)��2���ֽ�Ƭѡ(CS1)���ŵ�ǰ������ʱʱ�䣬��λΪuS�����ֽ���ǰ��0x0000��ʾ��������ʱ��
                       (3)��2���ֽ�Ƭѡ(CS1)���ź���������ʱʱ�䣬��λΪuS�����ֽ���ǰ��0x0000��ʾ��������ʱ��
                       (4)��1���ֽ�Ƭѡ(CS2)���ſ��Ƽ��������ƣ�
                       (5)��2���ֽ�Ƭѡ(CS2)���ŵ�ǰ������ʱʱ�䣬��λΪuS�����ֽ���ǰ��0x0000��ʾ��������ʱ��
                       (6)��2���ֽ�Ƭѡ(CS2)���ź���������ʱʱ�䣬��λΪuS�����ֽ���ǰ��0x0000��ʾ��������ʱ��*/

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = DEF_CMD_C1_INFOLEN;

                    /* (1)��1���ֽ�Ƭѡ(CS1)���ſ��Ƽ��������� */
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

                    /* (2)��2���ֽ�Ƭѡ(CS1)���ŵ�ǰ������ʱʱ�� */
                    temp16 = *pTxbuf++;
                    temp16 += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    if( temp16 )
                    {
                        Delay_uS( temp16 );
                    }

                    /* (3)��2���ֽ�Ƭѡ(CS1)���ź���������ʱʱ�� */
                    COMM.CS0_LaterOpDelay = *pTxbuf++;
                    COMM.CS0_LaterOpDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                    /* (4)��1���ֽ�Ƭѡ(CS2)���ſ��Ƽ��������� */
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

                    /* (5)��2���ֽ�Ƭѡ(CS2)���ŵ�ǰ������ʱʱ�� */
                    temp16 = *pTxbuf++;
                    temp16 += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    if( temp16 )
                    {
                        Delay_uS( temp16 );
                    }

                    /* (6)��2���ֽ�Ƭѡ(CS2)���ź���������ʱʱ�� */
                    COMM.CS1_LaterOpDelay = *pTxbuf++;
                    COMM.CS1_LaterOpDelay += ( ( (UINT16)*pTxbuf++ ) << 8 );

                    /* ע����������Ӧ�� */
                    break;

                case DEF_CMD_SPI_RD_WR:
                    /* 0xC2---SPI�ӿڳ����ȡд���������� */
                    count = pack_len;
                    if( count >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        count = DEF_SPI_TXRX_LEN_MAX;
                    }
                    tx_len = count;

                    /* ���Ź�ι�� */
#if( DEF_WWDG_FUN_EN == 0x01 )
                    WWDG_Tr = WWDG->CTLR & 0x7F;
                    if( WWDG_Tr < WWDG_Wr )
                    {
                        WWDG->CTLR = WWDG_CNT;
                    }
#endif

                    /* �������ݲ��ض����� */
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

                    /* �ж��Ƿ���Ҫ�Զ�����Ƭѡ */
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

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = tx_len;

                    /* ���÷���Ӧ��� */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_RD_WR;
                    SPI_Com_Buf[ 1 ] = (UINT8)( tx_len );
                    SPI_Com_Buf[ 2 ] = (UINT8)( tx_len >> 8 );
                    break;

                case DEF_CMD_SPI_BLCK_RD:
                    /* 0xC3---SPI�ӿ�������ȡ�������� */

                    /* ���㱾���շ����� */
                    COMM.Rx_TotalLen = *pTxbuf++;
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 8 );
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 16 );
                    COMM.Rx_TotalLen += ( ( (UINT16)*pTxbuf++ ) << 24 );
                    COMM.Tx_CurLen = COMM.Rx_TotalLen;
                    if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
                    }

                    /* ����DMA���������ݷ��� */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_RD;
                    SPI_Com_Buf[ 1 ] = (UINT8)COMM.Tx_CurLen;
                    SPI_Com_Buf[ 2 ] = (UINT8)( COMM.Tx_CurLen >> 8 );
                    SPIx_TxRx_DMA_Init( SPI_TxDMA_Buf, COMM.Tx_CurLen, &SPI_Com_Buf[ 3 ], COMM.Tx_CurLen );

                    COMM.Tx_Rx_Status = DEF_SPI_STATUS_RX;

                    /* ���õ�ǰ���������(ִ�гɹ����ܽ��м���) */
                    COMM.CMDP_Cur_CMDLen = 0x00;

                    /* ��������while��־ */
                    sp_flag = 0x01;
                    break;

                case DEF_CMD_SPI_BLCK_WR:
                    /* 0xC4---SPI�ӿ�����д���������� */

                    /* ���㱾���շ����� */
                    COMM.Rx_TotalLen = pack_len;
                    COMM.Tx_CurLen = COMM.Rx_TotalLen;
                    if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
                    {
                        COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
                    }

                    /* ����DMA���������ݷ��� */
                    SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_WR;
                    SPI_Com_Buf[ 1 ] = 0x01;
                    SPI_Com_Buf[ 2 ] = 0x00;
                    SPI_Com_Buf[ 3 ] = 0x00;
                    SPIx_TxRx_DMA_Init( pTxbuf, COMM.Tx_CurLen, &SPI_Com_Buf[ 4 ], COMM.Tx_CurLen );

                    COMM.Tx_Rx_Status = DEF_SPI_STATUS_TX;

                    /* ���õ�ǰ���������(ִ�гɹ����ܽ��м���) */
                    COMM.CMDP_Cur_CMDLen = 0x00;

                    /* ��������while��־ */
                    sp_flag = 0x01;
                    break;

                case DEF_CMD_INFO_RD:
                     /* 0xCA---������ȡ���� */
                     /* 0x00����ʾ��ȡоƬ�����Ϣ��
                        0x01����ʾ��ȡSPI�ӿ������Ϣ��
                        0x02����ʾ��ȡJTAG�ӿ������Ϣ�� */
                     dat = *pTxbuf++;
                     if( dat == 0 )
                     {
                         /* 0x00����ʾ��ȡоƬ�����Ϣ�� */
                         buf[ 0 ] = DEF_IC_PRG_VER;
                         buf[ 1 ] = 0x00;
                         buf[ 2 ] = 0;
                         buf[ 3 ] = 0;
                         count = 4;
                     }
                     else if( dat == 1 )
                     {
                         /* 0x01����ʾ��ȡSPI�ӿڡ�IIC�ӿ������Ϣ */
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
                         /* 0x02����ʾ��ȡJTAG�ӿ������Ϣ */
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

                     /* ���÷���Ӧ��� */
                     SPI_Com_Buf[ 0 ] = DEF_CMD_INFO_RD;
                     SPI_Com_Buf[ 1 ] = (UINT8)count;
                     SPI_Com_Buf[ 2 ] = 0x00;
                     memcpy( &SPI_Com_Buf[ 3 ], buf, (UINT8)count );
                     recv_len = count + 3;

                     /* ���õ�ǰ��������� */
                     COMM.CMDP_Cur_CMDLen = 1;
                     break;

                case DEF_CMD_JTAG_INIT:
                    /* 0xD0---JTAG�ӿڳ�ʼ������ */
                    /* (1)��1���ֽڣ�����ģʽ��
                            0x00��bit-bangģʽ��
                            0x01���Զ���Э��Ŀ���ģʽ��
                        (2)��1���ֽڣ�ͨ���ٶȣ���ЧֵΪ0-5��ֵԽ��ͨ���ٶ�Խ�죻
                        (3)��4���ֽڣ�������
                    */
                    JTAG_Mode = *pTxbuf++;
                    JTAG_Speed = *pTxbuf++;
                    JTAG_Port_Init( );

                    /* ��ʼ��SPI�ӿ� */
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

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = 6;

                    /* ���USB�ϴ���������ر��� */
                    COMM.Rx_LoadPtr = 0;
                    COMM.Rx_DealPtr = 0;
                    COMM.Rx_RemainLen = 0;

                    /* ���÷���Ӧ��� */
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = DEF_CMD_JTAG_INIT;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x01;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x00;
                    Comm_Rx_Buf[ COMM.Rx_LoadPtr++ ] = 0x00;
                    COMM.Rx_RemainLen = 4;
                    break;

                case DEF_CMD_JTAG_BIT_OP:
                    /* 0xD1---JTAG�ӿ�����λ�������� */

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* ִ������ */
                    while( count-- )
                    {
                        JTAG_Port_BitShift( *pTxbuf++ );
                    }
                    break;

                case DEF_CMD_JTAG_BIT_OP_RD:
                    /* 0xD2---JTAG�ӿ�����λ���Ʋ���ȡ���� */

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* �ж��Ƿ�װ��Ӧ����İ�ͷ���� */
                    if( CMDPack_Op_Len_Save == CMDPack_Op_Len )
                    {
                    	CMDPack_Op_Len_Save = 0x00;
                        COMM_Load_PackHead( DEF_CMD_JTAG_BIT_OP_RD, ( CMDPack_Op_Len / 2 ) );
                    }

                    /* ִ������ */
                    while( count-- )
                    {
                        /* �������� */
                    	dat = *pTxbuf++;
                    	JTAG_Port_BitShift( dat );

                        /* ����ȡ����װ�õ����ջ����� */
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
                    /* 0xD3---JTAG�ӿ�������λ���� */

#if( DEF_WWDG_FUN_EN == 0x01 )
                    /* ��ʱ�ر�WWDG���Ź�ʱ�� */
                    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, DISABLE );
#endif

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* JTAG�ӿ�����ΪSPIģʽ */
                    JTAG_Port_SwTo_SPIMode( );

                    /* ִ������ */
                    if( count == DEF_HS_PACK_MAX_LEN )
                    {
                        /* ����SPIģʽ�������� */
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

                    /* �ȴ�SPI�������  */
                    while( SPI2->STATR & SPI_I2S_FLAG_BSY );

                    /* JTAG�ӿ�����ΪGPIOģʽ */
                    JTAG_Port_SwTo_GPIOMode( );

#if( DEF_WWDG_FUN_EN == 0x01 )
                    /* ι�� */
                    WWDG_Tr = WWDG->CTLR & 0x7F;
                    if( WWDG_Tr < WWDG_Wr )
                    {
                        WWDG->CTLR = WWDG_CNT;
                    }

                    /* ���´�WWDG���Ź�ʱ�� */
                    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );
#endif
                    break;

                case DEF_CMD_JTAG_DATA_SHIFT_RD:
                    /* 0xD4---JTAG�ӿ�������λ����ȡ���� */

                    /* ���õ�ǰ��������� */
                    COMM.CMDP_Cur_CMDLen = pack_len;
                    count = pack_len;

                    /* �ж��Ƿ�װ��Ӧ����İ�ͷ���� */
                    if( CMDPack_Op_Len_Save == CMDPack_Op_Len )
                    {
                        CMDPack_Op_Len_Save = 0x00;
                        COMM_Load_PackHead( DEF_CMD_JTAG_DATA_SHIFT_RD, CMDPack_Op_Len );
                    }

                    /* ִ������ */
                    while( count-- )
                    {
                        /* ����ȡ����װ�õ����ջ����� */
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
                    /* ���õ�ǰ���������(һ����ִ�����) */
                    pack_len = CMDPack_Op_Len;
                    COMM.CMDP_Cur_CMDLen = COMM.CMDP_Cur_RemainLen;

                    DUG_PRINTF("ERROR\n");
                    break;
            }

            /* �л�����ִ��״̬ */
            CMDPack_Op_Len -= pack_len;
            if( CMDPack_Op_Len == 0 )
            {
                CMDPack_Op_Status = 0x00;
            }

            /* �ж��Ƿ���SPI����� */
            if( ( CMDPack_Op_Code < DEF_CMD_JTAG_INIT ) )
            {
                /* �ж��Ƿ���Ҫ�ϴ����� */
                if( recv_len )
                {
                    USB_SPI_DataUp( SPI_Com_Buf, recv_len );
                }
            }

            /* ���㵱ǰ�����ʣ�೤�ȼ�ƫ�ƣ����ж��Ƿ���Ҫ�л������� */
            COMM_CMDPack_Switch( 0x00 );
        }
    }

    /* �ж��Ƿ���JTAG������Ҫ�ϴ�(ִ����������1�������������ϴ�) */
    if( CMDPack_Op_Status == 0 )
    {
        COMM_RxData_Up_Deal( );
    }
    return( 0x00 );
}

/*******************************************************************************
* Function Name  : COMM_RxData_Up_Deal
* Description    : �ӿ�ͨ�Ž��������ϴ�����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void COMM_RxData_Up_Deal( void )
{
    UINT16 pack_len;
    UINT16 send_len;
    UINT32 offset;

    /* �ж�ǰһ�������Ƿ��ϴ���� */
    if( COMM.USB_Up_IngFlag )
    {
        /* ��ʱ100mSδȡ��������ȡ�����������ϴ� */
        if( COMM.USB_Up_TimeOut >= 1000 )
        {
            COMM.USB_Up_TimeOut = 0;
            COMM.USB_Up_IngFlag = 0;
        }
        return;
    }

    /* �ж��Ƿ���������Ҫͨ��USB�ӿڽ����ϴ� */
    if( COMM.Rx_RemainLen == 0x00 )
    {
        /* ��������Ҫ�ϴ���ֱ�ӷ��� */
        return;
    }

    /* ������ݰ�����������ϴ��������ݣ�����ʱ�ϴ�ʣ������ */
    send_len = COMM.Rx_RemainLen;
    pack_len = 0x00;
    if( USBHS_Up_PackLenMax == DEF_USB_HS_PACK_LEN )
    {
        /* ���㱾���ϴ����� */
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
        /* ���㱾���ϴ����� */
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

    /* �жϵ��ﻺ����Ĭ���Ƿ��㹻 */
    offset = DEF_COMM_RX_BUF_LEN - COMM.Rx_DealPtr;
    if( pack_len > offset )
    {
        pack_len = offset;
    }

//   DUG_PRINTF("Up:%x\n",pack_len);

    /* �����������Ҫ�ϴ��������������ϴ� */
    if( pack_len )
    {
        memcpy( &EP1_Tx_Databuf[ 0 ], &Comm_Rx_Buf[ COMM.Rx_DealPtr ], pack_len );

        /* ����DMA��ַ�������ϴ� */
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
* Description    : JTAGģʽ1����
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
    /* �ж��Ƿ���������Ҫͨ��JTAG�ӿڽ��з��� */
    if( COMM.CMDP_RemainNum )
    {
        /* �ж��Ǵ��ϴ�δ������ϵĻ�����װ�ػ��Ǵ��»�������װ�� */
        if( COMM.CMDP_Cur_RemainLen == 0x00 )
        {
            COMM.CMDP_Cur_RemainLen = COMM.CMDP_PackLen[ COMM.CMDP_DealNum ];
            COMM.CMDP_Cur_DealPtr = ( COMM.CMDP_DealNum * DEF_USB_HS_PACK_LEN );
        }

        /* �ӻ�������ȡ�����ݷ������� */
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

                    /* ����ȡ����װ�õ����ջ����� */
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

                    /* ����ȡ����װ�õ����ջ����� */
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

        /* �ر�USB�ж� */
        NVIC_DisableIRQ( USBHS_IRQn );
        NVIC_DisableIRQ( USBHS_IRQn );

        /* ������ر��� */
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

        /* �����ǰ��������ͣ�´������������´� */
        if( ( COMM.USB_Down_StopFlag == 0x01 ) &&
            ( COMM.CMDP_RemainNum < ( DEF_COMM_BUF_PACKNUM_MAX - 2 ) ) )
        {
            USBHSD->UEP2_RX_CTRL &= ~USBHS_EP_R_RES_MASK;
            USBHSD->UEP2_RX_CTRL |= USBHS_EP_R_RES_ACK;
            COMM.USB_Down_StopFlag = 0x00;
        }

        /* ���´�USB�ж� */
        NVIC_EnableIRQ( USBHS_IRQn );
    }

    /**********************************************************************/
    /* �ж��Ƿ���������Ҫͨ��USB�ӿڽ����ϴ� */
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

    /* �ȴ��˵�1�ϴ��������  */
    for(  i = 0; i < 100; i++ )
    {
        if( ( USBHSD->UEP1_TX_CTRL & USBHS_EP_T_RES_NAK ) == USBHS_EP_T_RES_NAK )
        {
            /* �˵�1�ϴ����,���жϳ��� */
            break;
        }
        Delay_Us( 1 );
    }
}


/*******************************************************************************
* Function Name  : SPIx_Cfg_DefInit
* Description    : SPIx�ӿ�����Ĭ�ϳ�ʼ��
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
* Description    : SPIx�ӿڳ�ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Port_Init( void )
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    SPI_InitTypeDef  SPI_InitStructure={0};

    /* ��SPIxʱ�� */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );

    /* ��λSPI,�Ա����SPI_I2S_FLAG_RXNEλ */
    RCC_APB1PeriphResetCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3, ENABLE );
    RCC_APB1PeriphResetCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3, DISABLE );
    SPI2->HSCR |= ( 1 << 0 );

    /* ����������� */
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

    /* ����SPIx */
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

    /* �ж��Ƿ���ձ�־Ϊ1,��������ȡ���ݼĴ�����0(CH32V307��S[I2\SPI3,��λĬ��ֵΪ1) */
    if( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == SET )
    {
        SPI2->DATAR;
    }

    /* ʹ��SPIx */
    SPI_Cmd( SPI2, ENABLE );
}

/*******************************************************************************
* Function Name  : SPIx_Tx_DMA_Init
* Description    : SPIx�ӿڷ���DMA��ʼ��
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
* Description    : SPIx�ӿڽ���DMA��ʼ��
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
* Description    : SPIx���Ͳ���д1���ֽ�
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
* Description    : SPIx���ͽ�������DMA��ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_TxRx_DMA_Init( UINT8 *pTxBuf, UINT16 Txlen, UINT8 *pRxBuf, UINT16 Rxlen )
{
    /* ����DMA�����з��ͺͽ��� */
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
* Description    : SPIx��������DMA����
*                                                 �����е��η��ͣ���ѯ�Ƿ�����ɣ�����������ϴ�ִ��״̬��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Tx_DMA_Deal( void )
{
    /* ��ѯSPI��DMA�����Ƿ���� */
    if( ( ( DMA1->INTFR & DMA1_FLAG_TC4 ) && ( DMA1->INTFR & DMA1_FLAG_TC5 ) ) ||
        ( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax ) )
    {
        /* ���־���ر�DMA */
        SPI2->STATR &= ~SPI_I2S_FLAG_TXE;
        SPI2->STATR &= ~SPI_I2S_FLAG_RXNE;

        DMA1->INTFCR = DMA1_FLAG_TC4;
        DMA1->INTFCR = DMA1_FLAG_TC5;
        DMA1_Channel5->CFGR &= (uint16_t)(~DMA_CFGR1_EN);
        DMA1_Channel4->CFGR &= (uint16_t)(~DMA_CFGR1_EN);

        /* ���㵱ǰ�����ʣ�೤�ȼ�ƫ�ƣ����ж��Ƿ���Ҫ�л������� */
        COMM.CMDP_Cur_CMDLen = ( COMM.Tx_CurLen + 4 );
        COMM_CMDPack_Switch( 0x00 );

        /* �л�SPI�շ�״̬ */
        COMM.Tx_Rx_Status = DEF_SPI_STATUS_IDLE;

        /* USB�ϴ�ִ��״̬ */
        if( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax )
        {
            SPI_Com_Buf[ 3 ] = 0x01;
        }
        USB_SPI_DataUp( SPI_Com_Buf, 4 );
    }
}

/*******************************************************************************
* Function Name  : SPIx_Rx_DMA_Deal
* Description    : SPIx��������DMA����
*                  �������ͣ���ѯ�����Ƿ�����ɣ����η���������ϴ��������ݣ��ж��Ƿ���Ҫ���գ�
*                  �����Ҫ������������ͣ����������������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPIx_Rx_DMA_Deal( void )
{
    /* ��ѯSPI��DMA�����Ƿ���� */
    if( ( ( DMA1->INTFR & DMA1_FLAG_TC4 ) && ( DMA1->INTFR & DMA1_FLAG_TC5 ) ) ||
        ( COMM.Tx_TimeOut >= COMM.Tx_TimeOutMax ) )
    {
        /* ���־���ر�DMA */
        SPI2->STATR &= ~SPI_I2S_FLAG_TXE;
        SPI2->STATR &= ~SPI_I2S_FLAG_RXNE;

        DMA1->INTFCR = DMA1_FLAG_TC4;
        DMA1->INTFCR = DMA1_FLAG_TC5;
        DMA1_Channel5->CFGR &= (uint16_t)(~DMA_CFGR1_EN);
        DMA1_Channel4->CFGR &= (uint16_t)(~DMA_CFGR1_EN);

        /* ����USB�ϴ��������� */
        USB_SPI_DataUp( SPI_Com_Buf, ( COMM.Tx_CurLen + 3 ) );

        /* �����Ƿ���ʣ������δ��ȡ */
        COMM.Rx_TotalLen -= COMM.Tx_CurLen;
        if( COMM.Rx_TotalLen == 0x00 )
        {
            /* ȫ�����ݽ������ */

            /* ���㵱ǰ�����ʣ�೤�ȼ�ƫ�ƣ����ж��Ƿ���Ҫ�л������� */
            COMM.CMDP_Cur_CMDLen = DEF_CMD_C3_INFOLEN;
            COMM_CMDPack_Switch( 0x00 );

            /* �л�SPI�շ�״̬ */
            COMM.Tx_Rx_Status = DEF_SPI_STATUS_IDLE;
        }
        else
        {
            /* ��������DMA���Ͳ��������� */

            /* ���㱾���շ����� */
            COMM.Tx_CurLen = COMM.Rx_TotalLen;
            if( COMM.Tx_CurLen >= DEF_SPI_TXRX_LEN_MAX )
            {
                COMM.Tx_CurLen = DEF_SPI_TXRX_LEN_MAX;
            }

            /* ����DMA���������ݷ��� */
            SPI_Com_Buf[ 0 ] = DEF_CMD_SPI_BLCK_RD;
            SPI_Com_Buf[ 1 ] = (UINT8)COMM.Tx_CurLen;
            SPI_Com_Buf[ 2 ] = (UINT8)( COMM.Tx_CurLen >> 8 );
            SPIx_TxRx_DMA_Init( SPI_TxDMA_Buf, COMM.Tx_CurLen, &SPI_Com_Buf[ 3 ], COMM.Tx_CurLen );

            /* ���õ�ǰ���������(ִ�гɹ����ܽ��м���) */
            COMM.CMDP_Cur_CMDLen = 0x00;
        }
    }
}

/*******************************************************************************
* Function Name  : USB_SPI_DataUp
* Description    : USBתSPI�����ϴ�
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_SPI_DataUp( UINT8 *pbuf, UINT16 len )
{
    /* �ж�ǰһ�������Ƿ�����ȡ��,���δ����ȡ����ʱ�ȴ� */
    while( COMM.USB_Up_IngFlag )
    {
        if( COMM.USB_Up_TimeOut >= COMM.USB_Up_TimeOutMax )
        {
            COMM.USB_Up_IngFlag = 0x00;
            break;
        }
        else
        {
            /* ���Ź�ι�� */
#if( DEF_WWDG_FUN_EN == 0x01 )
            WWDG_Tr = WWDG->CTLR & 0x7F;
            if( WWDG_Tr < WWDG_Wr )
            {
                WWDG->CTLR = WWDG_CNT;
            }
#endif
        }
    }

    /* �ر�USB�ж� */
    NVIC_DisableIRQ( USBHS_IRQn );
    NVIC_DisableIRQ( USBHS_IRQn );
    NVIC_DisableIRQ( USBHS_IRQn );

//  DUG_PRINTF("SPI_UP:%x\n",len);

    /* ����DMA��ַ�������ϴ� */
    COMM.USB_Up_IngFlag = 0x01;
    COMM.USB_Up_TimeOut = 0x00;
    USBHSD->UEP1_TX_DMA = (UINT32)(UINT8 *)pbuf;
    USBHSD->UEP1_TX_LEN  = len;
    USBHSD->UEP1_TX_CTRL &= ~USBHS_EP_T_RES_MASK;
    USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_ACK;

    /* ���´�USB�ж� */
    NVIC_EnableIRQ( USBHS_IRQn );

    /* �ж�ǰһ�������Ƿ�����ȡ��,���δ����ȡ����ʱ�ȴ� */
    while( COMM.USB_Up_IngFlag )
    {
        if( COMM.USB_Up_TimeOut >= COMM.USB_Up_TimeOutMax )
        {
            COMM.USB_Up_IngFlag = 0x00;
            break;
        }
        else
        {
            /* ���Ź�ι�� */
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
* Description    : SPIx��ȡFLASHоƬID����
* Input          : None
* Output         : None
* Return         : ����4���ֽ�,����ֽ�Ϊ0x00,
*                  �θ��ֽ�ΪManufacturer ID(0xEF),
*                  �ε��ֽ�ΪMemory Type ID
*                  ����ֽ�ΪCapacity ID
*                  W25X40BL����: 0xEF��0x30��0x13
*                  W25X10BL����: 0xEF��0x30��0x11
*******************************************************************************/
void SPIx_ReadID_Test( void )
{
    UINT32 dat;

    /* Select the FLASH: Chip Select low */
   PIN_SPI_CS0_LOW( );

    /* Send "RDID " instruction */
   SPIx_RD_WR_Byte( 0x9F );                                                     /* ���Ͷ�ȡJEDEC_ID���� */

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

