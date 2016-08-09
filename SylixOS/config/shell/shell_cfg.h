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
** 文   件   名: shell_cfg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 调试与简单交互使用的超小型 shell 配置文件。
*********************************************************************************************************/

#ifndef __SHELL_CFG_H
#define __SHELL_CFG_H

/*********************************************************************************************************
*                                 SHELL 全局配置
*  依存关系: 1: 互斥信号量
             2: I/O 系统
             3: TTY/PTY 设备 (SIO)
             4: FIO 服务
             5: 信号服务
             6: RTC 服务
             7: POSIX 线程高级操作功能管理
*********************************************************************************************************/

#define LW_CFG_SHELL_EN                         1                       /*  是否允许系统提供 tshell     */
#define LW_CFG_SHELL_THREAD_OPTION              LW_OPTION_THREAD_STK_CHK/*  tshell 线程 option          */
#define LW_CFG_SHELL_THREAD_STK_SIZE            (32 * LW_CFG_KB_SIZE)   /*  tshell 线程堆栈大小         */

/*********************************************************************************************************
*                                 SHELL 系统配置
*********************************************************************************************************/

#define LW_CFG_SHELL_MAX_COMMANDLEN             512                     /*  最长的 shell 命令长度       */
#define LW_CFG_SHELL_MAX_KEYWORDLEN             64                      /*  最长的 shell 关键字长度     */
#define LW_CFG_SHELL_MAX_PARAMNUM               64                      /*  最多的 shell 参数个数       */
#define LW_CFG_SHELL_KEY_HASH_SIZE              27                      /*  命令哈希表的大小, (素数)    */

/*********************************************************************************************************
*                                 SHELL Tar 工具
*********************************************************************************************************/

#define LW_CFG_SHELL_TAR_EN                     1                       /*  是否使能 shell tar 工具     */

/*********************************************************************************************************
*                                 SHELL 用户管理工具
*********************************************************************************************************/

#define LW_CFG_SHELL_USER_EN                    1                       /*  是否使能 shell 用户管理工具 */

/*********************************************************************************************************
*                                 SHELL 内存跟踪工具 (用于检查内存泄露)
*  依存关系: 1: shell
             2: 定长内存管理
*********************************************************************************************************/

#define LW_CFG_SHELL_HEAP_TRACE_EN              1                       /*  是否使能 shell heap 跟踪工具*/

/*********************************************************************************************************
*                                 SHELL 系统性能分析工具
*  依存关系: 1: shell
             2: loader
             3: sysperf
*********************************************************************************************************/

#define LW_CFG_SHELL_PERF_TRACE_EN              1                       /*  是否使能系统性能分析工具    */

/*********************************************************************************************************
*                                 变量系统配置
*********************************************************************************************************/

#define LW_CFG_SHELL_MAX_VARNAMELEN             64                      /*  最长的变量名                */
#define LW_CFG_SHELL_VAR_HASH_SIZE              43                      /*  变量哈希表的大小, (素数)    */
                                                                        /*  本应该使用自适应的算法, 自动*/
                                                                        /*  调整哈希表的大小, 但是这个  */
                                                                        /*  主要用于调试, 故使用固定值  */
                                                                        
/*********************************************************************************************************
*                                 SHELL 内部指令配置
*********************************************************************************************************/

#define LW_CFG_SHELL_HELP_PERLINE               2                       /*  每一行显示的帮助信息个数    */
#define LW_CFG_SHELL_MAX_HELPLINE               20                      /*  help 显示关键字列表时       */
                                                                        /*  一次可以显示的最大行数      */

/*********************************************************************************************************
*                                 SHELL 用户密码否使用加密管理
*********************************************************************************************************/

#define LW_CFG_SHELL_PASS_CRYPT_EN              1                       /*  用户密码相关支持            */

#endif                                                                  /*  __SHELL_CFG_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
