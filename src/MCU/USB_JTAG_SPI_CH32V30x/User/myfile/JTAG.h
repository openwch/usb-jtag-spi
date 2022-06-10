/********************************** (C) COPYRIGHT *******************************
* File Name          : JTAG.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : USBתJTAG���ͷ�ļ�
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef __JTAG_H__
#define __JTAG_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* ͷ�ļ����� */
#include "JTAG_Port.h"
#include "ch32vf30x_usb_device.h"

/******************************************************************************/
/* JTAG�����붨�� */

/******************************************************************************/
/* JTAG��غ궨�� */

/* SPI������غ궨�� */
#define CTLR1_SPE_Set          ((uint16_t)0x0040)
#define CTLR1_SPE_Reset        ((uint16_t)0xFFBF)

/* USB����С��غ궨�� */
#define DEF_USB_FS_PACK_LEN        64                                           /* USBȫ��ģʽ����С */
#define DEF_USB_HS_PACK_LEN        512                                          /* USB����ģʽ����С */

/* �ӿ�ͨ�Ż�������ض��� */
#define DEF_COMM_TX_BUF_LEN        4096                                         /* �ӿ�ͨ�ŷ��ͻ�����(USB�´�������)��С */
#define DEF_COMM_RX_BUF_LEN        4096                                         /* �ӿ�ͨ�Ž��ջ�����(USB�ϴ�������)��С */
#define DEF_COMM_BUF_PACKNUM_MAX   ( DEF_COMM_TX_BUF_LEN / DEF_USB_HS_PACK_LEN )/* �ӿ�ͨ�ŷ��ͻ������������ֵ */

/* ��������غ궨�� */
#define DEF_CMD_SPI_INIT            0xC0                                  /* SPI��ʼ������ */
#define DEF_CMD_SPI_CONTROL         0xC1                                  /* SPI�ӿڿ������� */
#define DEF_CMD_SPI_RD_WR           0xC2                                  /* SPI�ӿڳ����ȡд���������� */
#define DEF_CMD_SPI_BLCK_RD         0xC3                                  /* SPI�ӿ�������ȡ�������� */
#define DEF_CMD_SPI_BLCK_WR         0xC4                                  /* SPI�ӿ�����д���������� */
#define DEF_CMD_INFO_RD             0xCA                                  /* ��Ϣ��ȡ */

#define DEF_CMD_JTAG_INIT           0xD0                                  /* JTAG�ӿڳ�ʼ������ */
#define DEF_CMD_JTAG_BIT_OP         0xD1                                  /* JTAG�ӿ�����λ�������� */
#define DEF_CMD_JTAG_BIT_OP_RD      0xD2                                  /* JTAG�ӿ�����λ���Ʋ���ȡ���� */
#define DEF_CMD_JTAG_DATA_SHIFT     0xD3                                  /* JTAG�ӿ�������λ���� */
#define DEF_CMD_JTAG_DATA_SHIFT_RD  0xD4                                  /* JTAG�ӿ�������λ����ȡ���� */

/* �����������غ궨�� */
#define DEF_FS_PACK_MAX_LEN         59                                    /* ȫ��ģʽ���������󳤶� */
#define DEF_HS_PACK_MAX_LEN         507                                   /* ����ģʽ���������󳤶� */
#define DEF_SPI_TXRX_LEN_MAX        507                                   /* SPI_I2C�շ����ֵ */

/* SPI������غ궨�� */
#define DEF_SPI_OP_IDLE             0x00                                  /* SPI�ӿڲ���״̬������ */
#define DEF_SPI_OP_DMA_RD           0x01                                  /* SPI�ӿڲ���״̬��DMA��ȡ */
#define DEF_SPI_OP_DMA_WR           0x02                                  /* SPI�ӿڲ���״̬��DMAд�� */

#define DEF_CMD_C0_INFOLEN          26                                    /* 0xC0��������� */
#define DEF_CMD_C1_INFOLEN          10                                    /* 0xC1��������� */
#define DEF_CMD_C3_INFOLEN          4                                     /* 0xC3��������� */
#define DEF_CMD_D0_INFOLEN          6                                     /* 0xD0��������� */

/************************************************************/
/* �ӿ�ͨ����ؽṹ�嶨�� */
#define DEF_SPI_STATUS_IDLE         0x00                                        /* SPI�շ�״̬Ϊ����  */
#define DEF_SPI_STATUS_TX           0x01                                        /* SPI�շ�״̬Ϊ���ڷ���  */
#define DEF_SPI_STATUS_RX           0x02                                        /* SPI�շ�״̬Ϊ���ڷ���  */

typedef struct  _COMM_CTL
{
    UINT16V CMDP_LoadNum;                                                       /* �ӿ�ͨ�������������װ�ر�� */
    UINT16V CMDP_DealNum;                                                       /* �ӿ�ͨ������������������� */
    UINT16V CMDP_RemainNum;                                                     /* �ӿ�ͨ�������������ʣ��δ������ */
    UINT16V CMDP_PackLen[ DEF_COMM_BUF_PACKNUM_MAX ];                           /* �ӿ�ͨ���������������ǰ������ */
    UINT16  CMDP_Cur_RemainLen;                                                 /* �ӿ�ͨ���������ǰ���ڴ�����������ʣ�ദ���� */
    UINT16  CMDP_Cur_DealPtr;                                                   /* �ӿ�ͨ���������ǰ���ڴ����������Ĵ���ָ�� */
    UINT16  CMDP_Cur_CMDLen;                                                    /* �ӿ�ͨ���������ǰ���ڴ�����������ռ���� */

    UINT16  Rx_LoadPtr;                                                         /* JTAG���ݽ��ջ�����װ��ָ�� */
    UINT16  Rx_DealPtr;                                                         /* JTAG���ݽ��ջ���������ָ�� */
    UINT16V Rx_RemainLen;                                                       /* JTAG���ݽ��ջ�����ʣ��δ������ */
    UINT16V Rx_IdleCount;                                                       /* JTAG���ݽ��ճ�ʱ��ʱ */

    UINT16  USB_Up_PackLen;                                                     /* USB���ݰ��ϴ����� */
    UINT8   USB_Up_IngFlag;                                                     /* USB���ݰ������ϴ���־ */
    UINT16  USB_Up_TimeOut;                                                     /* USB���ݰ��ϴ���ʱ��ʱ */
    UINT16  USB_Up_TimeOutMax;                                                  /* USB���ݰ��ϴ���ʱ���ֵ */
    UINT8   USB_Down_StopFlag;                                                  /* USB���ݰ�ֹͣ�´���־ */

    UINT16  Tx_TimeOut;                                                         /* SPI���ݷ��ͳ�ʱ */
    UINT16  Tx_TimeOutMax;                                                      /* SPI���ݷ��ͳ�ʱ���ֵ */
    UINT8   Tx_Rx_Status;                                                       /* SPI�����շ�״̬(0�����У�1��SPI���ڷ��ͣ� 2��SPI���ڽ��գ�) */
    UINT32  Rx_TotalLen;                                                        /* SPI�����ܳ���  */
    UINT32  Tx_CurLen;                                                          /* SPI���η��ͳ���  */

    UINT8   CS0_LaterOpStatus;                                                  /* ƬѡCS0����������Ч״̬ */
    UINT16  CS0_LaterOpDelay;                                                   /* ƬѡCS0����������ʱʱ�� */
    UINT8   CS1_LaterOpStatus;                                                  /* ƬѡCS1����������Ч״̬ */
    UINT16  CS1_LaterOpDelay;                                                   /* ƬѡCS1����������ʱʱ�� */
    UINT16  SPI_ByteDelay;                                                      /* SPI�����ֽڼ���ʱ */
    UINT8   SPI_FillData;                                                       /* SPI����ֽ� */
    UINT8   Bit_Control;                                                        /* ����λ���� */
}COMM_CTL, *PCOMM_CTL;

/******************************************************************************/
/* �������� */
extern volatile COMM_CTL COMM;                                                 /* �ӿ�ͨ�ſ�����ؽṹ�� */
extern __attribute__ ((aligned(4))) UINT8  Comm_Tx_Buf[ DEF_COMM_TX_BUF_LEN ];  /* �ӿ�ͨ�ŷ��ͻ�����(USB�´�������) */
extern __attribute__ ((aligned(4))) UINT8  Comm_Rx_Buf[ DEF_COMM_RX_BUF_LEN ];  /* �ӿ�ͨ�Ž��ջ�����(USB�ϴ�������) */

extern volatile UINT8  CMDPack_Op_Status;                                       /* �����������ִ��״̬ */
extern volatile UINT8  CMDPack_Op_Code;                                         /* ������������� */
extern volatile UINT16 CMDPack_Op_Len;                                          /* ������ĳ��� */

extern volatile UINT8  JTAG_Mode;                                               /* JTAGģʽ��0��Э��ģʽ��1��bit-bangģʽ */
extern volatile UINT8  JTAG_Speed;                                              /* JTAG�ٶȣ�0-4��4�ٶ����*/
extern volatile UINT8  JTAG_Read_Mode;                                          /* JTAG��ȡģʽ */
extern volatile UINT8  JTAG_Shift_Mode;                                         /* JTAG��λģʽ */
extern volatile UINT32 JTAG_Shift_Cnt;                                          /* JTAG��λ���� */
extern volatile UINT32 JTAG_Time_Count;                                         /* JTAG��ʱ */

extern SPI_InitTypeDef SPI_Cfg;                                                 /* SPI�ӿ����� */
extern __attribute__ ((aligned(4))) UINT8  SPI_TxDMA_Buf[ 1024 ];               /* SPI_I2C����DMA������ */
extern __attribute__ ((aligned(4))) UINT8  SPI_Com_Buf[ 1024 ];                 /* SPI�������ݻ����� */

/********************************************************************************/
/* �������� */
extern void JTAG_Init( void );                                                  /* JTAG��ʼ�� */
extern void JTAG_SPI_Init( UINT16 speed );                                      /* JTAGģʽ��SPIx�ӿڳ�ʼ�� */
extern void JTAG_Port_SwTo_SPIMode( void );                                     /* JTAG�ӿ�����ΪSPIģʽ */
extern void JTAG_Port_SwTo_GPIOMode( void );                                    /* JTAG�ӿ�����ΪGPIOģʽ */
extern void JTAG_Mode1_Deal( void );										    /* JTAGģʽ1���� */	

extern void SPIx_Cfg_DefInit( void );                                           /* SPIx�ӿ�����Ĭ�ϳ�ʼ�� */
extern void SPIx_Port_Init( void );                                             /* SPIx�ӿڳ�ʼ�� */
extern void SPIx_Tx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize );
extern void SPIx_Rx_DMA_Init( DMA_Channel_TypeDef* DMA_CHx, UINT32 ppadr, UINT32 memadr, UINT16 bufsize );
extern UINT8 SPIx_RD_WR_Byte( UINT8 dat );                                      /* SPIx���Ͳ���д1���ֽ� */
extern void SPIx_TxRx_DMA_Init( UINT8 *pTxBuf, UINT16 Txlen, UINT8 *pRxBuf, UINT16 Rxlen ); /* SPIx���ͽ�������DMA��ʼ�� */
extern void SPIx_Tx_DMA_Deal( void );                                           /* SPIx��������DMA���� */
extern void SPIx_Rx_DMA_Deal( void );                                           /* SPIx��������DMA���� */
extern void SPIx_ReadID_Test( void );                                           /* SPIx��ȡFLASHоƬID���� */
extern void USB_SPI_DataUp( UINT8 *pbuf, UINT16 len );                          /* USBתSPI�����ϴ� */

extern void COMM_CMDPack_Switch( UINT8 mode );                                  /* �ӿ�ͨ��������л� */
extern UINT8 COMM_CMDPack_Deal( void );                                         /* �ӿ�ͨ����������� */
extern void COMM_RxData_Up_Deal( void );                                        /* �ӿ�ͨ�Ž��������ϴ����� */

#ifdef __cplusplus
}
#endif

#endif

/*********************************END OF FILE**********************************/
