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
** 文   件   名: k_object.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 这是系统内部对象定义。

** BUG
2007.04.10  去掉了 _OBJECT_KERNEL  _OBJECT_SCHED  _OBJECT_INTERRUPT  _OBJECT_ERR 几种类型
2014.07.19  去掉老的电源管理节点类型.
*********************************************************************************************************/

#ifndef  __K_OBJECT_H
#define  __K_OBJECT_H

/*********************************************************************************************************
          对象的结构
          
	31             26 25                 16 15                                      0 bit
    +-------------------------------------------------------------------------------+
    |                |                     |                                        |
    |     CLASS      |        NODE         |                INDEX                   |
    |                |                     |                                        |
    +-------------------------------------------------------------------------------+
    
    CLASS   :   Type
    NODE    :   Processor Number (MPI)
    INDEX   :   Buffer Address
    
*********************************************************************************************************/

#define _OBJECT_THREAD          1                                       /*  线程                        */
#define _OBJECT_THREAD_POOL     2                                       /*  线程池                      */
#define _OBJECT_SEM_C           3                                       /*  计数型信号量                */
#define _OBJECT_SEM_B           4                                       /*  二值型信号量                */
#define _OBJECT_SEM_M           5                                       /*  互斥型信号量                */
#define _OBJECT_MSGQUEUE        7                                       /*  消息队列                    */
#define _OBJECT_EVENT_SET       8                                       /*  事件集                      */
#define _OBJECT_SIGNAL          9                                       /*  信号                        */
#define _OBJECT_TIMER          14                                       /*  定时器                      */
#define _OBJECT_PARTITION      16                                       /*  内存块分区                  */
#define _OBJECT_REGION         17                                       /*  内存可变分区                */
#define _OBJECT_RMS            23                                       /*  精度单调器                  */

#endif                                                                  /*  __K_OBJECT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
