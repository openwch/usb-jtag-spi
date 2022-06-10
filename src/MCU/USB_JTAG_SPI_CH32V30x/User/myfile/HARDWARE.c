/******************** (C) COPYRIGHT 2011 WCH ***********************************
* File Name          : HAL.C
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : 硬件相关部分程序
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include <MAIN.h>																/* 头文件包含 */

/******************************************************************************/
/* 常、变量定义 */
volatile UINT16 TIM2_100mS_Count = 0x00;										/* 定时器2 100mS定时计数 */
volatile UINT8  TIM2_1S_Count = 0x00;											/* 定时器2 1S定时计数 */

/*******************************************************************************
* Function Name  : Delay_uS
* Description    : 微秒级延时函数
* Input          : delay---延时值
* Output         : None
* Return         : None
*******************************************************************************/
void Delay_uS( UINT16 delay )
{
	UINT16 i, j;

	for( i = delay; i != 0; i -- ) 
	{
        /* 看门狗喂狗 */
#if( DEF_WWDG_FUN_EN == 0x01 )
        WWDG_Tr = WWDG->CTLR & 0x7F;
        if( WWDG_Tr < WWDG_Wr )
        {
            WWDG->CTLR = WWDG_CNT;
        }
#endif

		for( j = 6; j != 0; j -- )
		{
			asm volatile ("nop");
			asm volatile ("nop");
			asm volatile ("nop");
			asm volatile ("nop");
			asm volatile ("nop");
		}
	}
}

/*******************************************************************************
* Function Name  : Delay_mS
* Description    : 毫秒级延时函数
* Input          : delay---延时值
* Output         : None
* Return         : None
*******************************************************************************/
void Delay_mS( UINT16 delay )
{
	UINT16 i, j;

	for( i = delay; i != 0; i -- ) 
	{
	    /* 看门狗喂狗 */
#if( DEF_WWDG_FUN_EN == 0x01 )
	    WWDG_Tr = WWDG->CTLR & 0x7F;
	    if( WWDG_Tr < WWDG_Wr )
	    {
	        WWDG->CTLR = WWDG_CNT;
	    }
#endif

		for( j = 200; j != 0; j -- )
		{
			Delay_uS( 5 );
		}		
	}
}

/*******************************************************************************
* Function Name  : 时钟配置
* Description    : Configures the different system clocks.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 RCC_Configuration( void )
{
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
	                        RCC_APB2Periph_USART1, ENABLE );

	/* AFIO clock enable */
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );

	/* TIM2 clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );						/* 使能TIM2 */

	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );

    /* 由于其中的PB3、PB4对应与单片机的JTAG功能,所以必须先禁用JTAG功能 */
#if( DEF_DEBUG_FUN_EN == 1 )
    GPIO_PinRemapConfig( GPIO_Remap_SWJ_Disable, ENABLE );
#endif
	return( 0x00 );
}

/*******************************************************************************
* Function Name  : 中断配置
* Description    : Configure the nested vectored interrupt controller.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration( void )
{
	NVIC_InitTypeDef NVIC_InitStructure = {0};

#ifdef  VECT_TAB_RAM
  	/* Set the Vector Table base location at 0x20000000 */
  	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else
	/* VECT_TAB_FLASH  */
  	/* Set the Vector Table base location at 0x08000000 */

	/* 判断是否使能IAP功能 */
#if( DEF_IAP_EN == 1 )
//	NVIC_SetVectorTable( NVIC_VectTab_FLASH, DEF_USER_PROG_ADDR - DEF_IAP_PROG_ADDR );
#else
//	NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0000 );
#endif

#endif

	/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_1 );

	/* Enable the TIM2 gloabal Interrupt */
	TIM_ClearFlag( TIM2, TIM_FLAG_Update );
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;		  						/* TIM2中断的开启 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init( &NVIC_InitStructure );
}

/*******************************************************************************
* Function Name  : TIM2_Init
* Description    : TIM2初始化
				   TIM2定时器主要用于定时100us
				   144 * 100 * 13.8888 -----> 100uS
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void TIM2_Init( void )
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure = {0};

	TIM_DeInit( TIM2 );

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 144;
	TIM_TimeBaseStructure.TIM_Prescaler = 100;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit( TIM2, &TIM_TimeBaseStructure );

	/* Clear TIM2 update pending flag[清除TIM2溢出中断标志] */
	TIM_ClearFlag( TIM2, TIM_FLAG_Update );

	/* Prescaler configuration */
//	TIM_PrescalerConfig( TIM2, 4, TIM_PSCReloadMode_Immediate );	 			/* 如果使能的话,一开始就会先进入中断服务程序 */

	/* TIM IT enable */
	TIM_ITConfig( TIM2, TIM_IT_Update, ENABLE );

    /* TIM2 enable counter [允许tim2计数]*/
	TIM_Cmd( TIM2, ENABLE );
}

/*******************************************************************************
* Function Name  : IWDG_Init
* Description    : Initializes IWDG.
* Input          : IWDG_Prescaler: specifies the IWDG Prescaler value.
*                    IWDG_Prescaler_4: IWDG prescaler set to 4.
*                    IWDG_Prescaler_8: IWDG prescaler set to 8.
*                    IWDG_Prescaler_16: IWDG prescaler set to 16.
*                    IWDG_Prescaler_32: IWDG prescaler set to 32.
*                    IWDG_Prescaler_64: IWDG prescaler set to 64.
*                    IWDG_Prescaler_128: IWDG prescaler set to 128.
*                    IWDG_Prescaler_256: IWDG prescaler set to 256.
*										Reload: specifies the IWDG Reload value.
*                    This parameter must be a number between 0 and 0x0FFF.
* Return         : None
*******************************************************************************/
void IWDG_Feed_Init( UINT16 prer, UINT16 rlr )
{
	IWDG_WriteAccessCmd( IWDG_WriteAccess_Enable );
	IWDG_SetPrescaler( prer );
	IWDG_SetReload( rlr );
	IWDG_ReloadCounter( );
	IWDG_Enable( );
}

/*******************************************************************************
* Function Name  : WWDG_Config
* Description    : Configure WWDG.
* Input          : tr : The value of the decrement counter(0x7f~0x40)
*                  wr : Window value(0x7f~0x40)
*                  prv: Prescaler value
*                       WWDG_Prescaler_1
*                       WWDG_Prescaler_2
*                       WWDG_Prescaler_4
*                       WWDG_Prescaler_8
* Return         : None
*******************************************************************************/
void WWDG_Config( UINT8 tr, UINT8 wr, UINT32 prv )
{
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );

	WWDG_SetCounter( tr );
	WWDG_SetPrescaler( prv );
	WWDG_SetWindowValue( wr );
	WWDG_Enable( WWDG_CNT );
	WWDG_ClearFlag( );
	WWDG_EnableIT( );
}

/*******************************************************************************
* Function Name  : WWDG_Feed
* Description    : Feed WWDG.
* Input          : None
* Return         : None
*******************************************************************************/
void WWDG_Feed( void )
{
	WWDG_SetCounter( WWDG_CNT );
}


