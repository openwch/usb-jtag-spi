/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    Application form  
    The USB2.0 to JTAG scheme based on CH32V305 MCU can be used to build 
	customized USB high-speed JTAG debugger and other products.  
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) 
	device universal driver (CH372DRV) and upper computer routines.  
    The current routine is a custom communication protocol,TDI transmission speed up to 4M bytes 
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (C) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

#ifndef __MAIN_H
#define __MAIN_H

//To disable deprecation, use _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>

#include "DbgFunc.h"
#include "resource.h"
#include "External\\CH375DLL.H"
#pragma comment(lib,"External\\CH375DLL")
#include "JtagDebug.H"

#endif