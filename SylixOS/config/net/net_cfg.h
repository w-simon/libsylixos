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
** 文   件   名: net_cfg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 13 日
**
** 描        述: 网络部分配置参数.
*********************************************************************************************************/

#ifndef __NET_CFG_H
#define __NET_CFG_H

/*********************************************************************************************************
*                                       裁剪配置
*  依存关系: 1: 定长分区内存管理
*            2: 互斥信号量
*            3: 二值信号量
*            4: I/O 系统
*            5: 定时器
*            6: LW_CFG_NET_MROUTER 需要 raw 支持.
*********************************************************************************************************/

#define LW_CFG_NET_EN                   1                               /*  是否允许网络功能            */
#define LW_CFG_NET_ROUTER               1                               /*  配置为路由器模式            */
#define LW_CFG_NET_MROUTER              1                               /*  配置为支持组播路由模式      */
#define LW_CFG_NET_BALANCING            1                               /*  源地址流量均衡路由支持      */
#define LW_CFG_NET_IPV6                 1                               /*  是否使能 IPv6 协议支持      */
#define LW_CFG_NET_SAFE                 1                               /*  1: 全双工安全选项           */
                                                                        /*  2: 全双工 + 删除安全        */
/*********************************************************************************************************
*                                       基本设置
*  依存关系: 1: 计数信号量, 读写锁
*            2: I/O 系统
*            3: FIO 
*            4: shell (网络工具需要)
*            5: thread ext
*
*  注  意  : SylixOS 网络协议栈为 Lwip 深度改造版, 代码量比 Lwip 大 4 倍, 效率更高, 功能更完善.
*            支持 AF_INET, AF_INET6, AF_PACKET, AF_UNIX, AF_ROUTE 协议域.
*            
*            网络驱动程序对内存的操作尽量不要放在中断中, 建议完全交由 netjob 处理.
*********************************************************************************************************/
/*********************************************************************************************************
*                                       多域安全
*********************************************************************************************************/

#define LW_CFG_LWIP_SEC_REGION          0                               /*  是否使能多域信息安全        */

/*********************************************************************************************************
*                                       调试配置
*********************************************************************************************************/

#define LW_CFG_LWIP_DEBUG               0                               /*  运行在调试模式              */
#define LW_CFG_LWIP_DEBUG_LEVEL         1                               /*  调试等级                    */

/*********************************************************************************************************
*                                       IP 设置
*********************************************************************************************************/

#define LW_CFG_LWIP_IPQOS               1                               /*  IP QoS 支持 (数据包优先级)  */
#define LW_CFG_LWIP_IPFRAG              1                               /*  IP 分片支持                 */

/*********************************************************************************************************
*                                       传输层设置
*********************************************************************************************************/

#define LW_CFG_LWIP_TCP_PCB             128                             /*  允许同时的 TCP 连接数       */
#define LW_CFG_LWIP_UDP_PCB             64                              /*  允许同时的 UDP 数量         */
#define LW_CFG_LWIP_RAW_PCB             32                              /*  允许同时的 RAW 数量         */

/*********************************************************************************************************
                                        组件配置
                                        
*  注  意  : 在不修改源码的前提下: armcc v1.1 不能编译 ppp 库.
*********************************************************************************************************/

#define LW_CFG_LWIP_PPP                 1                               /*  PPP 支持                    */
#define LW_CFG_LWIP_PPPOE               1                               /*  PPPoE 支持                  */
#define LW_CFG_LWIP_PPPOL2TP            1                               /*  PPPoL2TP 隧道协议支持       */
#define LW_CFG_LWIP_NUM_PPP             2                               /*  PPP 最大会话数              */

/*********************************************************************************************************
*                                       DNS 配置
*********************************************************************************************************/

#define LW_CFG_LWIP_DNS_SWITCH          1                               /*  多网络通过路由选择合适 DNS  */

/*********************************************************************************************************
*                                       TCP 设置
*********************************************************************************************************/

#define LW_CFG_LWIP_TCP_MULTI_PORTS     0                               /*  TCP 多端口服务器技术 (实验) */
#define LW_CFG_LWIP_TCP_SIG_EN          0                               /*  是否使能 TCP 签名技术       */
                                                                        /*  目前仅用于边界网关协议安全  */
                                                                        /*  依赖于 mbedTLS              */
/*********************************************************************************************************
*                                       TLS 配置
*********************************************************************************************************/

#define LW_CFG_LWIP_TLS_SNI_DEF         0                               /*  没有 SNI 选项时, 也调用回调 */
                                                                        /*  EdgerOS 自动签名系统需要    */
/*********************************************************************************************************
*                                       组播设置
*********************************************************************************************************/

#define LW_CFG_LWIP_DEF_MCAST_LOOP      1                               /*  默认是否为组播回环          */
#define LW_CFG_LWIP_GROUP_MAX           32                              /*  可加入的组播组最大数量      */

/*********************************************************************************************************
                                        以太网设置
*********************************************************************************************************/

#if LW_CFG_NET_ROUTER > 0
#define LW_CFG_LWIP_ARP_TABLE_SIZE      256                             /*  以太网接口 ARP 表大小       */
#else
#define LW_CFG_LWIP_ARP_TABLE_SIZE      127                             /*  以太网接口 ARP 表大小       */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

#define LW_CFG_LWIP_ARP_QUEUE_LEN       64                              /*  网络接口 APR 缓存队列长度   */

/*********************************************************************************************************
                                        AF_UNIX 配置
*********************************************************************************************************/

#define LW_CFG_NET_UNIX_EN              1                               /*  AF_UNIX 使能                */
#define LW_CFG_NET_UNIX_MULTI_EN        1                               /*  是否有应用多个线程发送, 多个*/
                                                                        /*  线程接收同一双向管道, 如果无*/
                                                                        /*  可设为 0 提高效率, 这种应用 */
                                                                        /*  很少见, 为了兼容性, 这里默认*/
                                                                        /*  设置为 1                    */
/*********************************************************************************************************
                                        AF_PACKET 配置
*********************************************************************************************************/

#define LW_CFG_NET_PACKET_POOL          0                               /*  使用 POOL 缓存 (速度快)     */
#define LW_CFG_NET_PACKET_MMAP          1                               /*  AF_PACKET 是否支持 RING 操作*/
                                                                        /*  必须使能 VMM 管理           */
/*********************************************************************************************************
                                        wireless 配置
*********************************************************************************************************/

#define LW_CFG_NET_WIRELESS_EN          1                               /*  是否支持无线网络(wifi802.11)*/

/*********************************************************************************************************
                                        Adhoc 配置
*
* Adhoc 路由器支持单网口 IP 数据报转发功能, 非 Adhoc 应用不能设置此项, 否则可能出现默认路由接口网络风暴.
*********************************************************************************************************/

#define LW_CFG_NET_ADHOC_ROUTER         0                               /*  是否配置为多跳网路由器模式  */
#define LW_CFG_NET_AODV_EN              1                               /*  是否使能 AODV 功能          */

/*********************************************************************************************************
                                        网络设备管理
*********************************************************************************************************/

#define LW_CFG_NET_DEV_MAX              32                              /*  最大网卡数量 (> 2, < 255)   */
#define LW_CFG_NET_DEV_PROTO_ANALYSIS   1                               /*  是否使能网络设备协议分析    */
#define LW_CFG_NET_DEV_TXQ_EN           1                               /*  网卡发送队列支持            */
#define LW_CFG_NET_DEV_ZCBUF_EN         1                               /*  网卡接收 0 拷贝缓冲支持     */
#define LW_CFG_NET_DEV_DESC_HELPER_EN   1                               /*  网卡 Tx/Rx 描述符缓冲管理   */
#define LW_CFG_NET_DEV_BRIDGE_EN        1                               /*  网卡桥接管理                */
#define LW_CFG_NET_DEV_BONDING_EN       1                               /*  网卡 Bonding 支持           */

/*********************************************************************************************************
                                        网络流量控制
                                        
*  使能流量控制需要将 LW_CFG_LWIP_MEM_SIZE 配置的大一些, 需要更多的空间作为数据分组缓存.
*********************************************************************************************************/

#define LW_CFG_NET_FLOWCTL_EN           1                               /*  是否使能流量控制            */
#define LW_CFG_NET_FLOWCTL_HZ           10                              /*  流量控制扫描频率 (2 ~ 10)   */
                                                                        /*  (100 ~ 500 ms) 之间最佳     */

#endif                                                                  /*  __NET_CFG_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
