/********************************** (C) COPYRIGHT *******************************
* File Name          : HAL.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : 底层硬件相关定义头文件
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef	__HAL_H__
#define __HAL_H__


/******************************************************************************/
/* 硬件引脚部分 */
/******************************************************************************/
#define PIN_SPI_CS0_LOW( )          ( GPIOB->BCR = GPIO_Pin_12 )                /* SPI_CS0引脚输出低电平 */
#define PIN_SPI_CS0_HIGH( )         ( GPIOB->BSHR = GPIO_Pin_12 )               /* SPI_CS0引脚输出高电平 */

#define PIN_SPI_CS1_LOW( )          ( GPIOC->BCR = GPIO_Pin_6 )                 /* SPI2_CS1引脚输出低电平 */
#define PIN_SPI_CS1_HIGH( )         ( GPIOC->BSHR = GPIO_Pin_6 )                /* SPI2_CS1引脚输出高电平 */

/******************************************************************************/
/* WWDG看门狗喂狗值  */
#define WWDG_CNT				    0X7F
#define NOP( )                      asm volatile ("nop");

/******************************************************************************/
/* 变量外扩 */
extern volatile UINT16 TIM2_100mS_Count;										/* 定时器2 100mS定时计数 */
extern volatile UINT8  TIM2_1S_Count;											/* 定时器2 1S定时计数 */

/******************************************************************************/
/* 函数外扩 */
extern void Delay_uS( UINT16 delay );											/* 软件微秒级延时 */
extern void Delay_mS( UINT16 delay );											/* 软件豪秒级延时 */
extern UINT8 RCC_Configuration( void );											/* 时钟配置 */
extern void NVIC_Configuration( void );											/* 中断配置 */
extern void TIM2_Init( void );													/* TIM2初始化 */
extern void IWDG_Feed_Init( UINT16 prer, UINT16 rlr );							/* IWDG看门狗初始化 */
extern void WWDG_Config( UINT8 tr, UINT8 wr, UINT32 prv );						/* WWDG看门狗初始化 */
extern void WWDG_Feed( void );													/* WWDG看门狗喂狗 */

#endif

/*********************************END OF FILE**********************************/






