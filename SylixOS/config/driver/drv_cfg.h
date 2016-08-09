/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: drv_cfg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 08 月 27 日
**
** 描        述: sylixos 标准设备驱动程序配置。
*********************************************************************************************************/

#ifndef __DRV_CFG_H
#define __DRV_CFG_H

/*********************************************************************************************************
*                                       SIO 设备驱动
*
* 依存关系: 1: I/O 系统
            2: TTY 设备
*********************************************************************************************************/

#define LW_CFG_DRV_SIO_16C550               1                           /*  16C550 串口驱动使能         */

/*********************************************************************************************************
*                                       CAN 设备驱动
*
* 依存关系: 1: I/O 系统
            2: CAN 设备
*********************************************************************************************************/

#define LW_CFG_DRV_CAN_SJA1000              1                           /*  sja1000 CAN 驱动使能        */

/*********************************************************************************************************
*                                       中断控制器驱动
*
* 依存关系: 无
*********************************************************************************************************/

#define LW_CFG_DRV_INT_I8259A               1                           /*  Intel 8259A 驱动使能        */

/*********************************************************************************************************
*                                       定时器驱动
*
* 依存关系: 无
*********************************************************************************************************/

#define LW_CFG_DRV_TIMER_I8254              1                           /*  Intel 8254 驱动使能         */

/*********************************************************************************************************
*                                       存储器接口驱动
*
* 依存关系: 1: I/O 系统
            2: PCI 接口
            3: 文件系统
*********************************************************************************************************/

#define LW_CFG_DRV_ATA_IDE                  1                           /*  ATA / IDE 驱动使能          */
#define LW_CFG_DRV_SATA_AHCI                1                           /*  AHCI PCIe SATA 驱动使能     */

/*********************************************************************************************************
*                                       网络接口控制器驱动
*
* 依存关系: 无
*********************************************************************************************************/

#define LW_CFG_DRV_NIC_INTEL                1                           /*  Intel e1000 igb 等系列网卡  */

#endif                                                                  /*  __DRV_CFG_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
