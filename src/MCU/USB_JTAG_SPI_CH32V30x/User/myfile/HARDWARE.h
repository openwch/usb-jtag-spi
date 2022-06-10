/********************************** (C) COPYRIGHT *******************************
* File Name          : HAL.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : �ײ�Ӳ����ض���ͷ�ļ�
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef	__HAL_H__
#define __HAL_H__


/******************************************************************************/
/* Ӳ�����Ų��� */
/******************************************************************************/
#define PIN_SPI_CS0_LOW( )          ( GPIOB->BCR = GPIO_Pin_12 )                /* SPI_CS0��������͵�ƽ */
#define PIN_SPI_CS0_HIGH( )         ( GPIOB->BSHR = GPIO_Pin_12 )               /* SPI_CS0��������ߵ�ƽ */

#define PIN_SPI_CS1_LOW( )          ( GPIOC->BCR = GPIO_Pin_6 )                 /* SPI2_CS1��������͵�ƽ */
#define PIN_SPI_CS1_HIGH( )         ( GPIOC->BSHR = GPIO_Pin_6 )                /* SPI2_CS1��������ߵ�ƽ */

/******************************************************************************/
/* WWDG���Ź�ι��ֵ  */
#define WWDG_CNT				    0X7F
#define NOP( )                      asm volatile ("nop");

/******************************************************************************/
/* �������� */
extern volatile UINT16 TIM2_100mS_Count;										/* ��ʱ��2 100mS��ʱ���� */
extern volatile UINT8  TIM2_1S_Count;											/* ��ʱ��2 1S��ʱ���� */

/******************************************************************************/
/* �������� */
extern void Delay_uS( UINT16 delay );											/* ���΢�뼶��ʱ */
extern void Delay_mS( UINT16 delay );											/* ������뼶��ʱ */
extern UINT8 RCC_Configuration( void );											/* ʱ������ */
extern void NVIC_Configuration( void );											/* �ж����� */
extern void TIM2_Init( void );													/* TIM2��ʼ�� */
extern void IWDG_Feed_Init( UINT16 prer, UINT16 rlr );							/* IWDG���Ź���ʼ�� */
extern void WWDG_Config( UINT8 tr, UINT8 wr, UINT32 prv );						/* WWDG���Ź���ʼ�� */
extern void WWDG_Feed( void );													/* WWDG���Ź�ι�� */

#endif

/*********************************END OF FILE**********************************/






