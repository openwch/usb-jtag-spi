/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_usb_device.c
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USB���ٲ�������ļ�
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* ͷ�ļ����� */
#include "main.h"
#include "USB_Desc.h"
#include "JTAG.h"

/******************************************************************************/
/* ������������ */

/* �������� */
void USBHS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/* USB���������� */
__attribute__ ((aligned(16))) UINT8 EP0_Databuf[ USBHS_UEP0_SIZE ]          __attribute__((section(".DMADATA"))); /*�˵�0�����շ�������*/
__attribute__ ((aligned(16))) UINT8 EP1_Rx_Databuf[ DEF_USB_EP1_FS_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�1���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP1_Tx_Databuf[ DEF_USB_EP1_FS_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�1���ݷ��ͻ����� */
__attribute__ ((aligned(16))) UINT8 EP2_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�2���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP2_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�2���ݷ��ͻ����� */
__attribute__ ((aligned(16))) UINT8 EP3_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�3���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP3_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�3���ݷ��ͻ����� */
__attribute__ ((aligned(16))) UINT8 EP4_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�4���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP4_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�4���ݷ��ͻ����� */
__attribute__ ((aligned(16))) UINT8 EP6_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�6���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP6_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�6���ݷ��ͻ����� */
__attribute__ ((aligned(16))) UINT8 EP8_Rx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�8���ݽ��ջ����� */
__attribute__ ((aligned(16))) UINT8 EP8_Tx_Databuf[ USBHS_MAX_PACK_SIZE ]   __attribute__((section(".DMADATA"))); /* �˵�86���ݷ��ͻ����� */


#define	pSetupReqPak		((PUSB_SETUP_REQ)EP0_Databuf)
const UINT8 *pDescr;
volatile UINT8  USBHS_Dev_SetupReqCode = 0xFF;									/* USB2.0�����豸Setup�������� */
volatile UINT16 USBHS_Dev_SetupReqLen = 0x00;									/* USB2.0�����豸Setup������ */
volatile UINT8  USBHS_Dev_SetupReqValueH = 0x00;								/* USB2.0�����豸Setup��Value���ֽ� */	
volatile UINT8  USBHS_Dev_Config = 0x00;										/* USB2.0�����豸����ֵ */
volatile UINT8  USBHS_Dev_Address = 0x00;										/* USB2.0�����豸��ֵַ */
volatile UINT8  USBHS_Dev_SleepStatus = 0x00;                                   /* USB2.0�����豸˯��״̬ */
volatile UINT8  USBHS_Dev_EnumStatus = 0x00;                                    /* USB2.0�����豸ö��״̬ */
volatile UINT8  USBHS_Dev_Endp0_Tog = 0x01;                                     /* USB2.0�����豸�˵�0ͬ����־ */
volatile UINT8  USBHS_Dev_Speed = 0x01;                                    		/* USB2.0�����豸�ٶ� */
volatile UINT16 USBHS_Up_PackLenMax = 512;                                      /* USB2.0�����豸��ǰ�����ϴ��İ��������ֵ(ȫ��64����512) */

UINT8  USB_Temp_Buf[ 64 ];                                                      /* USB��ʱ������ */

volatile UINT8 FLAG_Send = 1;

/*******************************************************************************
* Function Name  : USBHS_RCC_Init
* Description    : USB2.0�����豸RCC��ʼ��
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
* Description    : USB2.0�����豸�˵��ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Device_Endp_Init ( void )
{
    /* ʹ�ܶ˵�1���˵�2���ͺͽ���  */
	USBHSD->ENDP_CONFIG = USBHS_EP0_T_EN | USBHS_EP0_R_EN | USBHS_EP1_T_EN | USBHS_EP1_R_EN |
	                      USBHS_EP2_T_EN | USBHS_EP2_R_EN | USBHS_EP3_T_EN | USBHS_EP3_R_EN |
	                      USBHS_EP4_T_EN | USBHS_EP4_R_EN | USBHS_EP5_T_EN | USBHS_EP5_R_EN |
	                      USBHS_EP6_T_EN | USBHS_EP6_R_EN | USBHS_EP7_T_EN | USBHS_EP7_R_EN |
	                      USBHS_EP8_T_EN | USBHS_EP8_R_EN;

    /* �˵��ͬ���˵� */
	USBHSD->ENDP_TYPE = 0x00;

    /* �˵㻺����ģʽ����˫��������ISO����BUFģʽ��Ҫָ��0  */
	USBHSD->BUF_MODE = 0x00;

    /* �˵���󳤶Ȱ����� */
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

    /* �˵�DMA��ַ���� */
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

    /* �˵���ƼĴ�������(��ʹ�õĶ˵����ó��ֶ���ת) */
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

    /* ��ر�����ʼ�� */
    USBHS_Dev_Speed = 0x01;
    USBHS_Up_PackLenMax = 512;
}

/*******************************************************************************
* Function Name  : USBHS_Device_Init
* Description    : USB2.0�����豸��ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Device_Init ( FunctionalState sta )
{
    /* USB2.0�����豸RCC��ʼ�� */
    USBHS_RCC_Init( );

    /* ����DMA���ٶȡ��˵�ʹ�ܵ� */
    USBHSD->HOST_CTRL = 0x00;
    USBHSD->HOST_CTRL = USBHS_SUSPENDM;
    USBHSD->CONTROL = 0x00;
    USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_HIGH_SPEED;
//  USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_FULL_SPEED;
//  USBHSD->CONTROL = USBHS_DMA_EN | USBHS_INT_BUSY_EN | USBHS_LOW_SPEED;
    USBHSD->INT_EN = 0;
    USBHSD->INT_EN = USBHS_SETUP_ACT_EN | USBHS_TRANSFER_EN | USBHS_DETECT_EN | USBHS_SUSPEND_EN;
    USBHSD->ENDP_CONFIG = 0xffffffff;

    /* USB2.0�����豸�˵��ʼ�� */
    USBHS_Device_Endp_Init( );
    Delay_Us( 10 );
    
    /* ʹ��USB���� */
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
* Description    : USB2.0�����豸�����豸��ַ
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
* Description    : USB2.0�����豸�жϷ������
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
        /* �˵㴫�䴦�� */
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
                /* USB�˵�2�ϴ� */
                USBHSD->UEP2_TX_CTRL ^= USBHS_EP_T_TOG_1;
                USBHSD->UEP2_TX_CTRL |= USBHS_EP_T_RES_NAK;
            }
            else if( rx_token == PID_OUT ) 
            {
                /* USB�˵�2�´� */
                USBHSD->UEP2_RX_CTRL ^= USBHS_EP_R_TOG_1;

                /* ��¼�����Ϣ,���л�DMA��ַ */
                COMM.CMDP_PackLen[ COMM.CMDP_LoadNum ] = USBHSD->RX_LEN;
                COMM.CMDP_LoadNum++;
                USBHSD->UEP2_RX_DMA = (UINT32)(UINT8 *)&Comm_Tx_Buf[ ( COMM.CMDP_LoadNum * DEF_USB_HS_PACK_LEN ) ];
                if( COMM.CMDP_LoadNum >= DEF_COMM_BUF_PACKNUM_MAX )
                {
                    COMM.CMDP_LoadNum = 0x00;
                    USBHSD->UEP2_RX_DMA = (UINT32)(UINT8 *)&Comm_Tx_Buf[ 0 ];
                }
                COMM.CMDP_RemainNum++;

                /* �ж��Ƿ���Ҫ��ͣ�´� */
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
                /* USB �˵�1�ϴ� */
                USBHSD->UEP1_TX_CTRL ^= USBHS_EP_T_TOG_1;
                USBHSD->UEP1_TX_CTRL |= USBHS_EP_T_RES_NAK;

                /* ����JTAG�����ϴ� */
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
                /* USB �˵�1�´� */
                USBHSD->UEP1_RX_CTRL ^= USBHS_EP_R_TOG_1;
                USBHSD->UEP1_RX_CTRL &= ~ USBHS_EP_R_RES_MASK;
                USBHSD->UEP1_RX_CTRL |= USBHS_EP_R_RES_ACK;
            }
        }
        else if( end_num == 0 )
        {
        	/* �˵�0���� */
            if( rx_token == PID_IN ) 
            {
				/* �˵�0�ϴ��ɹ��ж� */
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
						/* ״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ��� */
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
        /* SETUP������ */
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

        /* �жϵ�ǰ�Ǳ�׼�������������� */
        if( ( pSetupReqPak->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )
        {
            /* ��������,��������,��������� */
            if( pSetupReqPak->bRequestType & 0x40 )                 /* �������� */
            {

            }
            else if( pSetupReqPak->bRequestType & 0x20 )            /* ������ */
            {

            }

            /* �ж��Ƿ������������ */
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
            /* �����׼USB����� */
            switch( USBHS_Dev_SetupReqCode )
            {
                case USB_GET_DESCRIPTOR:
                {
                    switch( ( ( pSetupReqPak->wValue ) >> 8 ) )
                    {
                        case USB_DESCR_TYP_DEVICE:
                            /* ��ȡ�豸������ */
                            pDescr = MyDevDescr;
                            len = MyDevDescr[ 0 ];
                            break;

                        case USB_DESCR_TYP_CONFIG:

                            /* �ж�USB�ٶȣ�00��ȫ��; 01������; 10:���� */
                            if( ( USBHSD->SPEED_TYPE & 0x03 ) == 0x01 )
                            {
                                /* ����ģʽ */
                                USBHS_Dev_Speed = 0x01;
                                USBHS_Up_PackLenMax = DEF_USB_HS_PACK_LEN;
                            }
                            else
                            {
                                /* ȫ��ģʽ */
                                USBHS_Dev_Speed = 0x00;
                                USBHS_Up_PackLenMax = DEF_USB_FS_PACK_LEN;
                            }

                            /* ��ȡ���������� */
                            if( USBHS_Dev_Speed == 0x01 )
                            {
                                /* ����ģʽ */
                                pDescr = MyCfgDescr_HS;
                                len = MyCfgDescr_HS[ 2 ] | ( (UINT16)MyCfgDescr_HS[ 3 ] << 8 );
                            }
                            else
                            {
                                /* ȫ��ģʽ */
                                pDescr = MyCfgDescr_FS;
                                len = MyCfgDescr_FS[ 2 ] | ( (UINT16)MyCfgDescr_FS[ 3 ] << 8 );
                            }
                            break;

                        case USB_DESCR_TYP_STRING:
                            /* ��ȡ�ַ��������� */
                            switch( ( pSetupReqPak->wValue ) & 0xff )
                            {
                                case 0:
                                    /* �����ַ��������� */
                                    pDescr = MyLangDescr;
                                    len = MyLangDescr[ 0 ];
                                    break;

                                case 1:
                                    /* USB�����ַ��������� */
                                    pDescr = MyManuInfo;
                                    len = sizeof( MyManuInfo );
                                    break;

                                case 2:
                                    /* USB��Ʒ�ַ��������� */
                                    pDescr = MyProdInfo;
                                    len = sizeof( MyProdInfo );
                                    break;

                                case 3:
                                    /* USB���к��ַ��������� */
                                    pDescr = MySerNumInfo;
                                    len = sizeof( MySerNumInfo );
                                    break;

                                default:
                                    errflag = 0xFF;
                                    break;
                            }
                            break;

                        case 6:
                            /* �豸�޶������� */
                            pDescr = ( PUINT8 )&MyUSBQUADesc[ 0 ];
                            len = sizeof( MyUSBQUADesc );
                            break;

                        case 7:
                            /* �����ٶ����������� */
                            if( USBHS_Dev_Speed == 0x01 )
                            {
                                /* ����ģʽ */
                                memcpy( &TAB_USB_HS_OSC_DESC[ 2 ], &MyCfgDescr_FS[ 2 ], sizeof( MyCfgDescr_FS ) - 2 );
                                pDescr = ( PUINT8 )&TAB_USB_HS_OSC_DESC[ 0 ];
                                len = sizeof( TAB_USB_HS_OSC_DESC );
                            }
                            else if( USBHS_Dev_Speed == 0x00 )
                            {
                                /* ȫ��ģʽ */
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

                    /* �ж��Ƿ������������ */
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
                    /* ���õ�ַ */
                    USBHS_Dev_Address = ( pSetupReqPak->wValue )& 0xff;
                    break;

                case USB_GET_CONFIGURATION:
                    /* ��ȡ����ֵ */
                    EP0_Databuf[ 0 ] = USBHS_Dev_Config;
                    if( USBHS_Dev_SetupReqLen > 1 )
                    {
                        USBHS_Dev_SetupReqLen = 1;
                    }
                    break;

                case USB_SET_CONFIGURATION:
                    /* ��������ֵ */
                    USBHS_Dev_Config = ( pSetupReqPak->wValue ) & 0xff;
                    USBHS_Dev_EnumStatus = 0x01;
                    break;

                case USB_CLEAR_FEATURE:
                    /* ������� */
                    if( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )
                    {
                        /* ����˵� */
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
                    /* �������� */
                    if( ( pSetupReqPak->bRequestType & 0x1F ) == 0x00 )
                    {
                        /* �����豸 */
                        if( pSetupReqPak->wValue == 0x01 )
                        {
                            if( MyCfgDescr_HS[ 7 ] & 0x20 )
                            {
                                /* ���û���ʹ�ܱ�־ */
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
                        /* ���ö˵� */
                        if( pSetupReqPak->wValue == 0x00 )
                        {
                            /* ����ָ���˵�STALL */
                            switch( ( pSetupReqPak->wIndex ) & 0xff )
                            {
                                case 0x88:
                                    /* ���ö˵�8 IN STALL */
                                    USBHSD->UEP8_TX_CTRL = ( USBHSD->UEP8_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x08:
                                    /* ���ö˵�1 OUT STALL */
                                    USBHSD->UEP8_RX_CTRL = ( USBHSD->UEP8_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x87:
                                    /* ���ö˵�7 IN STALL */
                                    USBHSD->UEP7_TX_CTRL = ( USBHSD->UEP7_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x07:
                                    /* ���ö˵�7 OUT STALL */
                                    USBHSD->UEP7_RX_CTRL = ( USBHSD->UEP7_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x86:
                                    /* ���ö˵�6 IN STALL */
                                    USBHSD->UEP6_TX_CTRL = ( USBHSD->UEP6_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x06:
                                    /* ���ö˵�6 OUT STALL */
                                    USBHSD->UEP6_RX_CTRL = ( USBHSD->UEP6_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x85:
                                    /* ���ö˵�5 IN STALL */
                                    USBHSD->UEP5_TX_CTRL = ( USBHSD->UEP5_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x05:
                                    /* ���ö˵�5 OUT STALL */
                                    USBHSD->UEP5_RX_CTRL = ( USBHSD->UEP5_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x84:
                                    /* ���ö˵�4 IN STALL */
                                    USBHSD->UEP4_TX_CTRL = ( USBHSD->UEP4_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x04:
                                    /* ���ö˵�4 OUT STALL */
                                    USBHSD->UEP4_RX_CTRL = ( USBHSD->UEP4_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x83:
                                    /* ���ö˵�3 IN STALL */
                                    USBHSD->UEP3_TX_CTRL = ( USBHSD->UEP3_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x03:
                                    /* ���ö˵�3 OUT STALL */
                                    USBHSD->UEP3_RX_CTRL = ( USBHSD->UEP3_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x82:
                                    /* ���ö˵�2 IN STALL */
                                    USBHSD->UEP2_TX_CTRL = ( USBHSD->UEP2_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x02:
                                    /* ���ö˵�2 OUT STALL */
                                    USBHSD->UEP2_RX_CTRL = ( USBHSD->UEP2_RX_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                    break;

                                case 0x81:
                                    /* ���ö˵�1 IN STALL */
                                    USBHSD->UEP1_TX_CTRL = ( USBHSD->UEP1_TX_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                    break;

                                case 0x01:
                                    /* ���ö˵�1 OUT STALL */
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
                    /* ���ݵ�ǰ�˵�ʵ��״̬����Ӧ�� */
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

        /* �˵�0���� */
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
		/* USB���߸�λ�ж� */  
		DUG_PRINTF("USB Reset\n");   	
		USBHS_Dev_Address = 0x00;
        USBHS_Device_SetAddress( USBHS_Dev_Address );							/* USB2.0�����豸�����豸��ַ */
        USBHS_Device_Endp_Init( );                                              /* USB2.0�����豸�˵��ʼ�� */

		/* ��USB���߸�λ */
		USBHSD->INT_FG = USBHS_DETECT_FLAG;
    }
    else if( usb_intstatus & USBHS_SUSPEND_FLAG )
    {
    	/* USB���߹���/��������ж� */
        USBHSD->INT_FG = USBHS_SUSPEND_FLAG;                                    /* Ϊ��USB���Ի���,���������ж� */

        if( USBHSD->MIS_ST & ( 1 << 2 ) )
		{
			/* ���� */
//			DUG_PRINTF("USB SUSPEND1!!!\n");
			USBHS_Dev_SleepStatus |= 0x02;
//			if( USBHS_Dev_SleepStatus != 0x03 )
//			{
//				USBHD_EnumStatus = 0x00;
//			}

#if( DEF_USBSLEEP_FUN_EN == 0x01 )
			/* USB���ӽӿ�˯�߻�������  */
			USBHS_Sleep_WakeUp_Cfg( );

			/* ͣ��ģʽ,˯�ߺ�,ʱ��Դ��ΪHSI,��Ҫ��������USBʱ�Ӳ���Ч */
			SystemInit( );  													/* HSI��PLLԴ */

		    /* USB2.0�����豸RCC��ʼ�� */
			Delay_Us( 200 );
		    USBHS_RCC_Init( );

			/* �򿪸�������ʱ��  */
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB2Periph_USART1, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART3, ENABLE );
		    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );
		    Delay_mS( 1 );

		    /* ���³�ʼ����ʱ�� */
			TIM2_Init( );
#endif
		}
		else
		{
			/* ���� */
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
* Description    : USB2.0���ӽӿ�˯�߻�������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBHS_Sleep_WakeUp_Cfg( void )
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};

    /* �ر��ж� */
    __disable_irq( );

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );                       /* ������Դʱ�� */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );                     /* ʹ��PA�˿�ʱ�� */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );

    /* �رո�������ʱ��  */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB2Periph_USART1, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART3, DISABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, DISABLE );

    /* ����USB˯�߻��� */
    EXTI_InitStructure.EXTI_Line = EXTI_Line20;                                 /* �¼���ѡ�� */
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;                             /* �¼����� */
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;                      /* �����ش��� */
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;                     /* �½��ش��� */
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;              /* ���ش���(�����ػ��½���) */
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /* ����˯��ģʽ */
    USBHSH->HOST_CTRL &= ~PHY_SUSPENDM;
    Delay_Us( 10 );
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFE );
    Delay_Us( 200 );

    /* ���ж� */
    __enable_irq( );
}

