/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  FLASH application function, FLASH model identification, block read, block write, block erase, FLASH content read to file, file
Write FLASH, speed test and other operating functions.

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
--*/

DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter);
DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter);
DWORD WINAPI ReadFlashToFile(LPVOID lpParameter);
//FLASH测试，先写再读
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter);

BOOL InitFlashSPI();
//FLASH块擦除
BOOL FlashBlockErase();
//Flash块数据写
BOOL FlashBlockWrite();
//FLASH字节读
BOOL FlashBlockRead();
//FLASH型号识别
BOOL FlashIdentify();
//关闭设备
BOOL CloseDevice();
//打开设备
BOOL OpenDevice();