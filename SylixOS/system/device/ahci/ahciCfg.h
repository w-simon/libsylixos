/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ahciCfg.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 27 日
**
** 描        述: AHCI 驱动配置.
*********************************************************************************************************/

#ifndef __AHCI_CFG_H
#define __AHCI_CFG_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
/*********************************************************************************************************
  调试级别
*********************************************************************************************************/
#define AHCI_LEVEL_DEBUG                    0x01
#define AHCI_LEVEL_RELEASE                  0x02
#define AHCI_LEVEL                          AHCI_LEVEL_RELEASE
/*********************************************************************************************************
  调试模式 (AHCI_LOG_RUN  AHCI_LOG_ERR  AHCI_LOG_BUG  AHCI_LOG_PRT  AHCI_LOG_ALL  AHCI_LOG_NONE)
*********************************************************************************************************/
#if (AHCI_LEVEL == AHCI_LEVEL_DEBUG)                                    /* AHCI_LEVEL                   */
#define AHCI_LOG_EN                         1                           /* 是否使能调试信息             */
#define AHCI_LOG_CMD_EN                     1                           /* 是否使能命令调试信息         */
#define AHCI_INT_LOG_EN                     1                           /* 是否使能中断调试信息         */
#define AHCI_LOG_LEVEL                      AHCI_LOG_ALL                /* 调试信息等级                 */
#else
#define AHCI_LOG_EN                         1                           /* 是否使能调试信息             */
#define AHCI_LOG_CMD_EN                     0                           /* 是否使能命令调试信息         */
#define AHCI_INT_LOG_EN                     0                           /* 是否使能中断调试信息         */
#define AHCI_LOG_LEVEL                      AHCI_LOG_ERR                /* 调试信息等级                 */
#endif                                                                  /* AHCI_LEVEL                   */
/*********************************************************************************************************
  ATAPI 操作
*********************************************************************************************************/
#define AHCI_ATAPI_EN                       0                           /* 是否使能 ATAPI 操作          */
/*********************************************************************************************************
  TRIM 操作
*********************************************************************************************************/
#define AHCI_TRIM_EN                        1                           /* 是否使能 TRIM 操作           */
#define AHCI_TRIM_TIMEOUT_MS                50                          /* TRIM 单次超时时间            */
#define AHCI_TRIM_TIMEOUT_NUM               100                         /* TRIM 单次超时时间次数        */
/*********************************************************************************************************
  磁盘 CACHE 回写 (KINGSTON SUV400S37120G 必须执行回写操作, 否则小文件会错误)
*********************************************************************************************************/
#define AHCI_CACHE_EN                       1                           /* 是否使能磁盘 CACHE           */
/*********************************************************************************************************
  驱动版本 (0x01000101 为 v1.0.1-rc1)
*********************************************************************************************************/
#define AHCI_CTRL_DRV_VER_NUM               0x01000600                  /* 驱动版本数值                 */
/*********************************************************************************************************
  驱动参数
*********************************************************************************************************/
#define AHCI_CTRL_NAME_MAX                  (32 + 1)                    /* 类型名称最大值               */
#define AHCI_DRV_NAME_MAX                   (32 + 1)                    /* 驱动名称最大值               */
#define AHCI_DEV_NAME_MAX                   (32 + 1)                    /* 设备名称最大值               */
#define AHCI_CTRL_IRQ_NAME_MAX              (32 + 1)                    /* 中断名称最大值               */
#define AHCI_NAME                           "ahci"
#define AHCI_MEDIA_NAME                     "/media/hdd"
/*********************************************************************************************************
  大小端
*********************************************************************************************************/
#define AHCI_SWAP(x)                        htole32(x)
#define AHCI_SWAP64(x)                      htole64(x)
/*********************************************************************************************************
  容量参数
*********************************************************************************************************/
#define AHCI_KB                             LW_CFG_KB_SIZE
#define AHCI_MB                             (AHCI_KB * AHCI_KB)
/*********************************************************************************************************
  磁盘 CACHE 参数
*********************************************************************************************************/
#define AHCI_CACHE_BURST_RD                 64							/* 读猝发参数					*/
#define AHCI_CACHE_BURST_WR                 64							/* 写猝发参数					*/
#define AHCI_CACHE_SIZE                     (1 * AHCI_MB)				/* 磁盘缓存大小		            */
#define AHCI_CACHE_PL                       4                           /* 并发线程数量                 */
/*********************************************************************************************************
  扇区操作参数 (扇区缓冲区大小依赖磁盘 CACHE)
*********************************************************************************************************/
#define AHCI_SECTOR_SIZE                    512							/* 扇区字节大小					*/
/*********************************************************************************************************
  驱动器探测时间 UNIT * COUNT (当控制器优先于驱动器上电时, 机械硬盘需要等待)
*********************************************************************************************************/
#define AHCI_DRIVE_PROB_TIME_UNIT           100                         /* 单次等待时间为 100 ms        */
#define AHCI_DRIVE_PROB_TIME_COUNT          100                         /* 单次等待时间数量             */
/*********************************************************************************************************
  RSTON 与 RSTOFF 命令超时时间参数
*********************************************************************************************************/
#define AHCI_DRIVE_RSTON_INTER_TIME_UNIT   	5                         	/* RST ON 单次等待时间为 5 ms   */
#define AHCI_DRIVE_RSTON_INTER_TIME_COUNT  	200                  		/* RST ON 单次等待时间的数量    */

#define AHCI_DRIVE_RSTOFF_INTER_TIME_UNIT  	5                         	/* RST OFF 单次等待时间为 5 ms 	*/
#define AHCI_DRIVE_RSTOFF_INTER_TIME_COUNT 	200                  		/* RST OFF 单次等待时间的数量	*/
/*********************************************************************************************************
  磁盘 CACHE 参数
*********************************************************************************************************/
#define AHCI_DRIVE_DISKCACHE_MSG_COUNT      16                          /* 磁盘缓冲消息数量             */
#define AHCI_DRIVE_DISKCACHE_PARALLEL_EN    LW_TRUE                     /* 磁盘缓冲是否使能并行操作     */
/*********************************************************************************************************
  控制器参数 (PRDT Physical Region Descriptor Table)
*********************************************************************************************************/
#define AHCI_CTRL_MAX                       6                           /* 控制器数量最大值             */
#define AHCI_DRIVE_MAX                      32                          /* 驱动器数量最大值             */
#define AHCI_CMD_SLOT_MAX                   32                          /* 每个端口命令槽的最大值       */
#define AHCI_RCV_FIS_MAX                    1                           /* 每个端口 FIS 的最大值        */
#define AHCI_PRDT_MAX                       16                          /* PRDT 最大值                  */
#define AHCI_PRDT_I                         0x80000000                  /* Interrupt on Completion      */
#define AHCI_PRDT_BYTE_MAX                  0x400000                    /* Data Byte Count              */
#define AHCI_ATAPI_CMD_LEN_MAX              16                          /* ATAPI 命令最大字节长度       */

#define AHCI_CTRL_INDEX_MASK                0x3F                        /* 控制器索引掩码               */
#define AHCI_DRIVE_BASE(drive)              ((drive * 0x80) + 0x100)
/*********************************************************************************************************
  错误处理参数
*********************************************************************************************************/
#define AHCI_RETRY_NUM                       3                          /* 重试次数                     */
/*********************************************************************************************************
  线程参数
*********************************************************************************************************/
#define AHCI_MONITOR_PRIORITY               LW_PRIO_T_SYSMSG
#define AHCI_MONITOR_STK_SIZE               LW_CFG_THREAD_DEFAULT_STK_SIZE
/*********************************************************************************************************
  消息参数
*********************************************************************************************************/
#define AHCI_MSG_QUEUE_SIZE                 (AHCI_DRIVE_MAX * 2)
#define AHCI_MSG_SIZE                       sizeof(AHCI_MSG_CB)
/*********************************************************************************************************
  超时时间参数
*********************************************************************************************************/
#define AHCI_SEM_TIMEOUT_DEF                (5 * API_TimeGetFrequency())/* 同步信号超时时间             */
/*********************************************************************************************************
  热插拔配置
*********************************************************************************************************/
#define AHCI_HOTPLUG_EN                     (1)                         /* 是否使能对磁盘热插拔的支持   */

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_CFG_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
