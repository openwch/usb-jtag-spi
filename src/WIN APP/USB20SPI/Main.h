/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    Form program, providing SPI FLASH model identification, block read, block write, block erase, FLASH content read to file, file
    Write FLASH, speed test and other operations.
    Usb2.0 (480M high speed) to SPI based on CH32V305 MCU, can be used to construct
    Build custom USB high speed FASH programmer and other products.
    The source code of the scheme includes MCU firmware, USB2.0 high-speed (480M) device universal driver (CH372DRV) and upper computer routines.
    The current routine is a custom communication protocol, and the SPI transmission speed can reach 2MB/S
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
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
#include <memory.h>
#pragma comment(lib,"winmm")
#include <time.h>
#include <stdio.h>
#include "setupapi.h"
#pragma comment(lib,"setupapi")
#pragma comment(lib,"comctl32.lib")

#include "DbgFunc.h"
#include "resource.h"

#include "External\\CH375DLL.H"
#pragma comment(lib,"External\\CH375DLL")

#include "USB20SPI.H"
#include "SPI_FLASH.h"
#include "FlashApp.h"

#endif