/****************************** (C) COPYRIGHT **********************************
* File Name          : TYPE.H
* Author             : WCH
* Version            : V1.00
* Date               : 2022/04/14
* Description        : 常用类型和常量预定义
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/

#ifndef 	__TYPE_H__
#define 	__TYPE_H__

#ifdef 		__CX51__
#ifndef 	__C51__
#define 	__C51__     	  1
#endif
#endif

#ifdef 	  	__C51__
typedef 	bit bdata         BOOL1;
#else
#define 	xdata
typedef 	unsigned int   	  BOOL1;
#endif

#ifndef 	TRUE
#define 	TRUE    		  1
#define 	FALSE   		  0
#endif

#ifndef 	NULL
#define 	NULL    		  0
#endif

#ifndef 	UINT8
typedef 	unsigned char     UINT8;							 /* defined for unsigned 8-bits integer variable  无符号8位整型变量  */
#endif

#ifndef 	UINT8C
//typedef 	unsigned char  code   UINT8C;						 
#endif

#ifndef 	UINT16
typedef 	unsigned short    UINT16;						 	 /* defined for unsigned 16-bits integer variable 无符号16位整型变量 */
#endif

#ifndef 	UINT32
typedef 	unsigned long     UINT32;							 /* defined for unsigned 32-bits integer variable 无符号32位整型变量 */
#endif

#ifndef 	UINT8X
typedef 	UINT8   xdata     UINT8X;
#endif

#ifndef 	UINT16X
typedef 	UINT16  xdata     UINT16X;
#endif

#ifndef 	UINT32X
typedef 	UINT32  xdata     UINT32X;
#endif

#ifndef 	PUINT8
typedef 	UINT8   xdata     *PUINT8;
#endif

#ifndef 	PUINT8
typedef 	UINT32  xdata     *PUINT32;
#endif

#endif

/********************************* END OF FILE *********************************/
