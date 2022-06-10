/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_it.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main Interrupt Service Routines.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "ch32v30x_it.h"
#include "main.h"

/******************************************************************************/
/* 函数声明 */
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void Ecall_M_Mode_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void Ecall_U_Mode_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void Break_Point_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SW_handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void WWDG_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TAMPER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void RTC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void FLASH_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void RCC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA1_Channel7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void ADC1_2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USB_HP_CAN1_TX_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USB_LP_CAN1_RX0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN1_RX1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN1_SCE_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI9_5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM1_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM1_TRG_COM_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM1_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void I2C2_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void I2C2_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void SPI1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void SPI2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USART3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void EXTI15_10_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void RTCAlarm_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USBWakeUp_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM8_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM8_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM8_TRG_COM_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM8_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void RNG_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void FSMC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void SDIO_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void SPI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void UART4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void UART5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void ETH_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void ETH_WKUP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN2_TX_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN2_RX0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN2_RX1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void CAN2_SCE_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void OTG_FS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USBHSWakeup_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USBHS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DVP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void UART6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void UART7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void UART8_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM9_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM9_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM9_TRG_COM_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM9_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM10_BRK_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM10_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM10_TRG_COM_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void TIM10_CC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel8_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel9_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel10_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void DMA2_Channel11_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/******************************************************************************/
/* 常、变量定义 */
volatile UINT16 T2_100uS_Cout = 0;

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
    DUG_PRINTF("NMI_Handler\n");

}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
    DUG_PRINTF("HardFault_Handler\n");
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM2_IRQHandler( void )
{
	/* 指示灯控制计时 */
	TIM2_100mS_Count++;

    /* JTAG超时计时 */
	T2_100uS_Cout++;
	if( T2_100uS_Cout >= 10 )
	{
	    T2_100uS_Cout = 0;
	    JTAG_Time_Count++;
	}
	COMM.Rx_IdleCount++;
	COMM.USB_Up_TimeOut++;

	/* 清中断状态 */
//	TIM_ClearITPendingBit( TIM2, TIM_IT_Update );
	TIM2->INTFR = (uint16_t)~TIM_IT_Update;
}

/*******************************************************************************
* Function Name  : TIM4_IRQHandler
* Description    : This function handles TIM4 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM4_IRQHandler( void )
{
	/* 清中断状态 */
//	TIM_ClearITPendingBit( TIM4, TIM_IT_Update );
	TIM4->INTFR = (uint16_t)~TIM_IT_Update;
}

/*******************************************************************************
* Function Name  : SW_handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Return         : None
*******************************************************************************/
void SW_handler(void)
{
    //jump  UserCode
    __asm("li  a6, 0x00000 ");
    __asm("jr  a6");

    while(1);

}
