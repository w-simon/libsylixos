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
** 文   件   名: net_perf_cfg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 08 日
**
** 描        述: 网络性能参数配置.
*********************************************************************************************************/

#ifndef __NET_PERF_CFG_H
#define __NET_PERF_CFG_H

/*********************************************************************************************************
  缓存配置
*********************************************************************************************************/

#define LW_CFG_LWIP_MEM_SIZE            (512 * LW_CFG_KB_SIZE)          /*  lwip 内存大小               */
#define LW_CFG_LWIP_MSG_SIZE            512                             /*  lwip 内部消息队列缓冲长度   */
#define LW_CFG_LWIP_POOL_SIZE           1560                            /*  lwip POOL 内存块大小        */
                                                                        /*  注意: 必须是字对齐的        */

#define LW_CFG_LWIP_NUM_PBUFS           256                             /*  系统总 pbuf 数量            */
#define LW_CFG_LWIP_NUM_NETBUF          256                             /*  缓冲网络分组 netbuf 数量    */
#define LW_CFG_LWIP_NUM_POOLS           1024                            /*  pool 总数                   */
                                                                        /*  驱动程序与 AF_PACKET 使用   */

/*********************************************************************************************************
  队列配置
  注意: LW_CFG_LWIP_JOBQUEUE_NUM 必须为 1 或者 2 的指数次方, (小于等于 CPU 核心数量).
*********************************************************************************************************/

#define LW_CFG_LWIP_JOBQUEUE_NUM        2                               /*  可使能多个 netjob 并行工作  */
#define LW_CFG_LWIP_JOBQUEUE_SIZE       1024                            /*  sylixos job queue size      */
#define LW_CFG_LWIP_STK_SIZE            4096                            /*  lwip thread default stksize */

/*********************************************************************************************************
  TCP 设置 
  关于 LW_CFG_LWIP_TCP_WND 不能大于网卡的接收缓冲区大小, 否则可能出现网卡接收溢出的错误, 造成网络速度颠簸.
  
  LW_CFG_LWIP_TCP_SCALE 为窗口扩大因子, 
  如果 LW_CFG_LWIP_TCP_SCALE 为 0 表示不使用窗口扩大则
  LW_CFG_LWIP_TCP_WND, LW_CFG_LWIP_TCP_SND 最大值为 65535
  
  如果 LW_CFG_LWIP_TCP_SCALE 为 1 表示不使用窗口扩大则
  LW_CFG_LWIP_TCP_WND, LW_CFG_LWIP_TCP_SND 最大值为 65535 * 2^1 = 131070
  
  如果 LW_CFG_LWIP_TCP_SCALE 为 2 表示不使用窗口扩大则
  LW_CFG_LWIP_TCP_WND, LW_CFG_LWIP_TCP_SND 最大值为 65535 * 2^2 = 262140
  
  ... 
  
  LW_CFG_LWIP_TCP_SCALE 最大值为 14, 建议最大设置不超过 3 ~ 4
*********************************************************************************************************/

#define LW_CFG_LWIP_TCP_WND             8192                            /*  接收缓冲大小, 0 为自动      */
#define LW_CFG_LWIP_TCP_SND             65535                           /*  发送缓冲大小, 0 为自动      */
#define LW_CFG_LWIP_TCP_SCALE           0                               /*  接收窗口扩大指数 0 ~ 14     */

#define LW_CFG_LWIP_TCP_MAXRTX          8                               /*  TCP 最大重传数, 1 ~ 12      */
#define LW_CFG_LWIP_TCP_SYNMAXRTX       6                               /*  最大 SYN 重传数, 1 ~ 12     */

/*********************************************************************************************************
  AF_UNIX
*********************************************************************************************************/

#define LW_CFG_AF_UNIX_256_POOLS        256                             /*  256 字节大小内存池个数      */
#define LW_CFG_AF_UNIX_512_POOLS        128                             /*  512 字节大小内存池个数      */

#endif                                                                  /*  __NET_PERF_CFG_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
