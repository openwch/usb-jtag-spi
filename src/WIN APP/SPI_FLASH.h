/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    The FLASH operation function based on the SPI interface provides the SPI FLASH model identification, block read, block write, and block erase functions.
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

#ifndef __SPI_FLAH_H
#define __SPI_FLAH_H

/******************************************************************************/
/* 头文件包含 */
//#include <stdio.h>
//#include <string.h>

#define UINT8V UCHAR
#define UINT16V USHORT
#define UINT32V ULONG
//#define PIN_FLASH_CS_LOW
//#define PIN_FLASH_CS_HIGH

/******************************************************************************/
/* SPI Serial Flash OPERATION INSTRUCTIONS */
#define	CMD_FLASH_READ		       0x03					   						/* Read Memory at 25 MHz */
#define	CMD_FLASH_H_READ		   0x0B					   						/* Read Memory at 50 MHz */
#define	CMD_FLASH_SECTOR_ERASE	   0x20					   						/* Erase 4 KByte of memory array */
#define	CMD_FLASH_BLOCK_ERASE1	   0x52					   						/* Erase 32 KByte of memory array */
#define	CMD_FLASH_BLOCK_ERASE2	   0xD8					   						/* Erase 64 KByte of memory array */
#define	CMD_FLASH_CHIP_ERASE1	   0x60					   						/* Erase Full Memory Array */
#define	CMD_FLASH_CHIP_ERASE2	   0xC7					   						/* Erase Full Memory Array */
#define	CMD_FLASH_BYTE_PROG		   0x02					   						/* To Program One Data Byte */
#define	CMD_FLASH_AAI_WORD_PROG	   0xAD					   						/* Auto Address Increment Programming */
#define	CMD_FLASH_RDSR	  		   0x05					   						/* Read-Status-Register */
#define	CMD_FLASH_EWSR	  		   0x50					   						/* Enable-Write-Status-Register */
#define	CMD_FLASH_WRSR	  		   0x01					   						/* Write-Status-Register */
#define	CMD_FLASH_WREN	  		   0x06					   						/* Write-Enable */
#define	CMD_FLASH_WRDI	  		   0x04					   						/* Write-Disable */
#define	CMD_FLASH_RDID1	  		   0x90					   						/* Read-ID */
#define	CMD_FLASH_RDID2	  		   0xAB					   						/* Read-ID */
#define	CMD_FLASH_JEDEC_ID	  	   0x9F					   						/* JEDEC ID read */
#define	CMD_FLASH_EBSY	  		   0x70					   						/* Enable SO to output RY/BY# status during AAI programming */
#define	CMD_FLASH_DBSY	  		   0x80					   						/* Disable SO to output RY/BY# status during AAI programming */

#define CMD_M25PXX_SECTOR_ERASE    0xD8                                         /* M25PXX芯片 Erase 512Kbit of memory array */
#define CMD_M25PXX_BLOCK_ERASE     0xC7                                         /* M25PXX芯片 Erase 4/8/16Mbit of memory array(Chip Erase) */

/* 以下为MT25QXXX系列芯片相关命令 */
#define	CMD_RD_EX_ADDR_REG	  	   0xC8					   						/* READ EXTENDED ADDRESS REGISTER */
#define	CMD_WR_EX_ADDR_REG	  	   0xC5					   						/* WRITE EXTENDED ADDRESS REGISTER */

/* 注意：与SST25VF512A的区别：  
		 1、
		   对于SST25VF512A： CMD_FLASH_AAI_WORD_PROG----0XAF
		   对于SST25VF016B： CMD_FLASH_AAI_WORD_PROG----0XAD
		   并且操作的时序方法也不一样；不能混用。

		 2、
		   对于SST25VF512A： 最后三条命令不支持		
*/

/******************************************************************************/
/* FLASH操作填充自己定义 */
#define DEF_DUMMY_BYTE             0xFF

/******************************************************************************/
/* FLASH操作相关参数定义 */
#define SPI_FLASH_SectorSize       4096
#define SPI_FLASH_PageSize         256
#define SPI_FLASH_PerWritePageSize 256

/******************************************************************************/
/* SPI FLASH芯片类型定义 */
#define DEF_TYPE_W25XXX            0											/* W25XXX系列 */
#define DEF_TYPE_SST25XXX          1											/* SST25XXX系列 */
#define DEF_TYPE_M25PXXX           2                                            /* M25PXXX系列 */
#define DEF_TYPE_MX25LXXX          3                                            /* MX25LXXX系列 */
#define DEF_TYPE_MT25QXXX          4											/* MT25QXXX系列 */

/******************************************************************************/
/* SPI FLASH芯片型号定义 */

/* W25XXX系列 */
#define W25X10_FLASH_ID            0xEF3011										/* 1M bit */
#define W25X20_FLASH_ID            0xEF3012										/* 2M bit */
#define W25X40_FLASH_ID            0xEF3013										/* 4M bit */
#define W25X80_FLASH_ID            0xEF4014										/* 8M bit */
#define W25Q16_FLASH_ID1           0xEF3015							 			/* 16M bit */
#define W25Q16_FLASH_ID2           0xEF4015							 			/* 16M bit */
#define W25Q32_FLASH_ID1           0xEF4016							 			/* 32M bit */
#define W25Q32_FLASH_ID2           0xEF6016							 			/* 32M bit */
#define W25Q64_FLASH_ID1           0xEF4017							 			/* 64M bit */
#define W25Q64_FLASH_ID2           0xEF6017							 			/* 64M bit */
#define W25Q128_FLASH_ID1          0xEF4018							 			/* 128M bit */
#define W25Q128_FLASH_ID2          0xEF6018							 			/* 128M bit */
#define W25Q256_FLASH_ID1          0xEF4019							 			/* 256M bit */
#define W25Q256_FLASH_ID2          0xEF6019							 			/* 256M bit */

/* SST25XXX系列 */
#define SST25VF040_FLASH_ID        0xBF258D                                     /* 4M bit */
#define SST25VF080_FLASH_ID        0xBF258E                                     /* 8M bit */
#define SST25VF016_FLASH_ID        0xBF2541										/* 16M bit */
#define SST25VF032_FLASH_ID        0xBF254A										/* 32M bit */
#define SST25VF064_FLASH_ID        0xBF254B										/* 64M bit */

/* M25PXXX系列 */
#define M25P40_FLASH_ID            0x202013                                     /* 4M bit */
#define M25P80_FLASH_ID            0x202014                                     /* 8M bit */
#define M25P16_FLASH_ID            0x202015                                     /* 16M bit */
#define M25P32_FLASH_ID            0x202016                                     /* 32M bit */
#define M25P64_FLASH_ID            0x202017                                     /* 64M bit */

/* MX25LXXX系列 */
#define MX25L40_FLASH_ID           0xC22013                                     /* 4M bit */
#define MX25L80_FLASH_ID           0xC22014                                     /* 8M bit */
#define MX25L16_FLASH_ID           0xC22015                                     /* 16M bit */
#define MX25L32_FLASH_ID           0xC22016                                     /* 32M bit */
#define MX25L64_FLASH_ID           0xC22017                                     /* 64M bit */

/* MT25XXX系列 */
#define MT25Q_FLASH_64Mb	       0x20BA17										/* 64M bit */
#define MT25Q_FLASH_128Mb	       0x20BA18										/* 128M bit */
#define MT25Q_FLASH_256Mb	       0x20BA19										/* 256M bit */
#define MT25Q_FLASH_512Mb	       0x20BA20										/* 512M bit */
#define MT25Q_FLASH_1024Mb	       0x20BA21										/* 1G bit */
#define MT25Q_FLASH_2048Mb	       0x20BA22										/* 2G bit */

/******************************************************************************/
/* FLASH操作相关定义 */
#define DEF_FLASH_BUF_LEN          512											/* FLASH操作临时缓冲区长度 */

/******************************************************************************/
/* 常、变量外扩 */
//UINT8V  Flash_Type;                                                      /* FLASH芯片类型: 0: W25XXX系列; 1: SST25XXX系列; 2: MT25QXX系列  */
//UINT32V Flash_ID;                                                        /* FLASH芯片ID号 */
//UINT32V Flash_Sector_Count;                                              /* FLASH芯片扇区数 */
//UINT16V Flash_Sector_Size;                                               /* FLASH芯片扇区大小 */

//UINT32V Flash_Erase_Address;                                      	    /* FLASH擦除起始地址 */
//UINT8   Flash_BackUp_Buf[ DEF_FLASH_BUF_LEN ];                           /* FLASH操作备份缓冲区 */
//UINT8V  Flash_Cur_Hold_SecNum;                                           /* FLASH拷贝当前使用的保留扇区号 */
//UINT8V  Flash_Erase_Flag;                                                /* FLASH扇区擦除标志 */
//UINT8V  Flash_Used_Flag ;                                                /* FLASH扇区拷贝标志(每个位表示一个512字节的小扇区) */
//UINT32V Flash_Erase_Address;                                       	    /* FLASH擦除起始地址 */

#define FLASH_OP_TIMEOUT 1000 //FLASH操作超时

//FLASH芯片检测
BOOL FLASH_IC_Check( void );									 

//FLASH块数据读取
UINT32 FLASH_RD_Block( UINT32 address,UINT8 *pbuf,UINT32 len );  

//FLASH块数据写入
UINT32 FLASH_WR_Block( UINT32 address, UINT8 *pbuf, UINT32 len );

//FLASH扇区擦除
BOOL FLASH_Erase_Sector( UINT32 address );		

/* FLASH全片擦除 */
BOOL FLASH_Erase_Full();								

/*******************************************************************************
* Function Name  : FLASH_WriteEnable
* Description    : FLASH芯片允许写操作
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL FLASH_WriteEnable( void );

/*******************************************************************************
* Function Name  : FLASH_ReadStatusReg
* Description    : FLASH芯片读取状态寄存器 
* Input          : None
* Output         : None
* Return         : 返回寄存器值
*******************************************************************************/
UINT8 FLASH_ReadStatusReg( void );

/******************************************************************************/
/* 函数外扩 */
//extern void FLASH_Port_Init( void );											/* FLASH芯片操作相关引脚初始化 */
//extern UINT8 SPI_FLASH_SendByte( UINT8 byte );									/* SPI发送一个字节数据 */
//extern UINT8 SPI_FLASH_ReadByte( void );										/* SPI接收一个字节数据 */
//extern UINT32 FLASH_ReadID( void );												/* 读取FLASH芯片ID */
//extern void FLASH_WriteEnable( void );											/* FLASH芯片允许写操作 */
//extern void FLASH_WriteDisable( void );											/* FLASH芯片禁止写操作 */
//extern UINT8 FLASH_ReadStatusReg( void );										/* FLASH芯片读取状态寄存器 */
//extern void FLASH_WriteStatusReg( UINT8 dat );									/* FLASH芯片写状态寄存器 */

//void FLASH_IC_Check( void );												/* FLASH芯片检测 */
//extern void FLASH_Erase_Sector( UINT32 address );								/* FLASH扇区擦除 */
//extern void FLASH_RD_Block_Start( UINT32 address );	   							/* FLASH块数据读取开始 */
//extern void FLASH_RD_Block( UINT8 *pbuf, UINT32 len );		 					/* FLASH块数据读取 */
//extern void FLASH_RD_Block_End( void );											/* FLASH块数据读取结束 */
//extern void FLASH_WR_Block( UINT32 address, UINT8 *pbuf, UINT32 len );			/* FLASH块数据写入 */
#ifdef Used

extern UINT8 MT25QXXX_ReadExAddrReg( void );									/* MT25QXXX芯片读取扩展地址寄存器 */
extern void MT25QXXX_WriteExAddrReg( UINT8 dat );								/* MT25QXXX芯片写扩展地址寄存器 */
extern void MT25QXXX_ChangeExAddrReg( UINT8 dat );								/* MT25QXXX芯片改变扩展地址寄存器 */

extern UINT8 FLASH_Read_OneSector( UINT32 address, UINT8 *pbuf );				/* FLASH读取单个扇区数据 */
extern UINT8 FLASH_Read_MulSector( UINT32 address, UINT8 *pbuf, UINT32 len );	/* FLASH读取多个扇区数据 */
extern UINT8 FLASH_Write_OneSector( UINT32 address, UINT8 *pbuf );				/* FLASH写入单个扇区数据 */

#endif
#endif
/******************* (C) COPYRIGHT 2013 MJX*********************END OF FILE****/
