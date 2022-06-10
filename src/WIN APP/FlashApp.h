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
//FLASH���ԣ���д�ٶ�
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter);

BOOL InitFlashSPI();
//FLASH�����
BOOL FlashBlockErase();
//Flash������д
BOOL FlashBlockWrite();
//FLASH�ֽڶ�
BOOL FlashBlockRead();
//FLASH�ͺ�ʶ��
BOOL FlashIdentify();
//�ر��豸
BOOL CloseDevice();
//���豸
BOOL OpenDevice();