/******************** (C) COPYRIGHT ********************************************
* File Name          : PRINTF.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : ���ڴ�ӡ�������ͷ�ļ�
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/



#ifndef	__PRINTF_H__
#define __PRINTF_H__

/*******************************************************************************/
/* ���崮�ڴ�ӡ������� */
#define MY_DEBUG_PRINTF            1				 				 			/* �����ӡ���ӿ��� */

/* �����������ĺ���ͨ�����ڴ�ӡ���,���򲻴��� */
#if( MY_DEBUG_PRINTF == 1 )
#define DUG_PRINTF( format, arg... )    printf( format, ##arg )		 			/* ���ڴ�ӡ��� */
#else
#define DUG_PRINTF( format, arg... )    do{ if( 0 )printf( format, ##arg ); }while( 0 );
#endif

#endif

