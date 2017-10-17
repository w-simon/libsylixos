;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: sparExcAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "sparcContextAsm.h"

    FILE_BEGIN()
    
    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    
    IMPORT_LABEL(archMpCur)
    IMPORT_LABEL(archCtxLoad)
    IMPORT_LABEL(_G_ulCpu)
    IMPORT_LABEL(_G_ulIntNesting)
    IMPORT_LABEL(_G_ulIntSafeStack)

    IMPORT_LABEL(archDataStoreErrHandle)
    IMPORT_LABEL(archInstAccessMmuMissHandle)
    IMPORT_LABEL(archInstAccessErrHandle)
    IMPORT_LABEL(archRRegAccessErrHandle)
    IMPORT_LABEL(archInstAccessExcHandle)
    IMPORT_LABEL(archPrivInstHandle)
    IMPORT_LABEL(archIllegalInstHandle)
    IMPORT_LABEL(archFpDisableHandle)
    IMPORT_LABEL(archCpDisableHandle)
    IMPORT_LABEL(archUnimplFlushHandle)
    IMPORT_LABEL(archWatchPointDectectHandle)
    IMPORT_LABEL(archMemAddrNoAlignHandle)
    IMPORT_LABEL(archFpExcHandle)
    IMPORT_LABEL(archCpExcHandle)
    IMPORT_LABEL(archDataAccessErrHandle)
    IMPORT_LABEL(archDataAccessMmuMissHandle)
    IMPORT_LABEL(archDataAccessExcHandle)
    IMPORT_LABEL(archTagOverFlowHandle)
    IMPORT_LABEL(archDivZeroHandle)
    IMPORT_LABEL(archTrapInstHandle)
    IMPORT_LABEL(bspIntHandle)

    EXPORT_LABEL(archDataStoreErrEntry)
    EXPORT_LABEL(archInstAccessMmuMissEntry)
    EXPORT_LABEL(archInstAccessErrEntry)
    EXPORT_LABEL(archRRegAccessErrEntry)
    EXPORT_LABEL(archInstAccessExcEntry)
    EXPORT_LABEL(archPrivInstEntry)
    EXPORT_LABEL(archIllegalInstEntry)
    EXPORT_LABEL(archFpDisableEntry)
    EXPORT_LABEL(archCpDisableEntry)
    EXPORT_LABEL(archUnimplFlushEntry)
    EXPORT_LABEL(archWatchPointDectectEntry)
    EXPORT_LABEL(archMemAddrNoAlignEntry)
    EXPORT_LABEL(archFpExcEntry)
    EXPORT_LABEL(archCpExcEntry)
    EXPORT_LABEL(archDataAccessErrEntry)
    EXPORT_LABEL(archDataAccessMmuMissEntry)
    EXPORT_LABEL(archDataAccessExcEntry)
    EXPORT_LABEL(archTagOverFlowEntry)
    EXPORT_LABEL(archDivZeroEntry)
    EXPORT_LABEL(archTrapInstEntry)
    EXPORT_LABEL(archIntEntry)

;/*********************************************************************************************************
;  参数寄存器
;*********************************************************************************************************/

#define PSR_ARG_REG         l0
#define PC_ARG_REG          l1
#define NPC_ARG_REG         l2
#define TRAP_TYPE_ARG_REG   l3

;/*********************************************************************************************************
;  SPARC 异常入口宏
;   l0 = PSR
;   l1 = PC
;   l2 = nPC
;   l3 = trap type
;*********************************************************************************************************/

MACRO_DEF(SPARC_TRAP_ENTRY handle arg)
    RESTORE                                                             ;/*  回到异常前的寄存器窗口      */

    ;/*
    ; * 保存 CPU 上下文
    ; */
    ST      %sp  , [%sp - ASM_REG_STACK_FRAME_SIZE*2 + REG_OUTPUT(6)]   ;/*  保存异常前的 %sp            */

    SUB     %sp  , ASM_REG_STACK_FRAME_SIZE*2 , %sp                     ;/*  开辟栈空间                  */

    ST      %g1  , [%sp + REG_GLOBAL(1)]                                ;/*  保存全局寄存器(除了 %g0)    */
    STD     %g2  , [%sp + REG_GLOBAL(2)]
    STD     %g4  , [%sp + REG_GLOBAL(4)]
    STD     %g6  , [%sp + REG_GLOBAL(6)]

    STD     %l0  , [%sp + REG_LOCAL(0)]                                 ;/*  保存本地寄存器              */
    STD     %l2  , [%sp + REG_LOCAL(2)]
    STD     %l4  , [%sp + REG_LOCAL(4)]
    STD     %l6  , [%sp + REG_LOCAL(6)]

    STD     %i0  , [%sp + REG_INPUT(0)]                                 ;/*  保存输入寄存器              */
    STD     %i2  , [%sp + REG_INPUT(2)]
    STD     %i4  , [%sp + REG_INPUT(4)]
    STD     %i6  , [%sp + REG_INPUT(6)]

    STD     %o0  , [%sp + REG_OUTPUT(0)]                                ;/*  保存输出寄存器              */
    STD     %o2  , [%sp + REG_OUTPUT(2)]
    STD     %o4  , [%sp + REG_OUTPUT(4)]
    ST      %o7  , [%sp + REG_OUTPUT(7)]                                ;/*  %o6(%sp) 已经保存           */

    RD      %y   , %l0
    ST      %l0  , [%sp + REG_Y]                                        ;/*  保存 Y 寄存器               */

    RD      %wim , %g1                                                  ;/*  %g1 记录 %wim !!!           */
    WR      %g0  , %wim                                                 ;/*  清除 %wim, 避免异常         */
    NOP
    NOP
    NOP

    MOV     %sp  , %g6                                                  ;/*  %g6 = %sp !!!               */

    SAVE                                                                ;/*  回到异常后的寄存器窗口      */

    WR      %g1  , %wim                                                 ;/*  恢复 %wim                   */
    NOP
    NOP
    NOP

    ST      %l0  , [%g6 + REG_PSR]                                      ;/*  保存 PSR 状态寄存器         */
    ST      %l1  , [%g6 + REG_PC]                                       ;/*  保存 PC                     */
    ST      %l2  , [%g6 + REG_NPC]                                      ;/*  保存 Next PC                */
    MOV     %\arg , %g7

    RESTORE                                                             ;/*  回到异常前的寄存器窗口      */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    ;/*
    ; * 中断嵌套计数加一
    ; */
    SET     _G_ulIntNesting , %l0                                       ;/*  %l0 = _G_ulIntNesting       */

#if LW_CFG_SMP_EN > 0
    READ_CPUID(%l2)                                                     ;/*  %l2 = CPU_ID                */
    SLL     %l2   , 2   , %l2                                           ;/*  %l2 = CPU_ID * 4            */
    ADD     %l2   , %l0 , %l0                                           ;/*  %l0 = &nesting[CPU_ID]      */
#endif

    LD      [%l0] , %l1                                                 ;/*  %l1 = 中断嵌套计数地址      */
    LD      [%l1] , %l0                                                 ;/*  %l0 = 中断嵌套计数          */

    ADD     %l0 , 1 , %g1                                               ;/*  %g1 = 中断嵌套计数加一!!!   */

    ST      %g1 , [%l1]                                                 ;/*  存储回去                    */
    STBAR

    CMP     %g1 , 1
    BNE     1f
    NOP

    SET     _G_ulCpu , %l0                                              ;/*  %l0 = _G_ulCpu              */

#if LW_CFG_SMP_EN > 0
    ADD     %l2 , %l0 , %l0                                             ;/*  %l0 = &_G_ulCpu[CPU_ID]     */
#endif

    LD      [%l0] , %l1                                                 ;/*  %l1 = 当前 CPU 结构指针     */
    LD      [%l1] , %l0                                                 ;/*  %l0 = 当前 TCB 结构指针     */

    ST      %g6  , [%l0]                                                ;/*  保存当前的栈指针到当前 TCB  */

1:
    ;/*
    ; * %g1 = 当前中断嵌套计数, 保证 flush 窗口后, 还能通过 %g1 获取当前中断嵌套计数
    ; * %g6 = 当前 %sp, 保证 flush 窗口后, 还能通过 %g6 获取当前栈指针
    ; * %g7 = trap type 或 PC
    ; */

    FLUSH_WINDOWS                                                       ;/*  flush 窗口                  */

    CMP     %g1 , 1
    BE      2f
    NOP

    SUB     %g6   , ASM_STACK_FRAME_SIZE , %fp                          ;/*  开辟栈帧                    */
    B       3f
    SUB     %fp   , ASM_STACK_FRAME_SIZE , %sp                          ;/*  开辟栈空间                  */

2:
    ;/*
    ; * 切换到中断安全栈
    ; */
    SET     _G_ulIntSafeStack , %l0                                     ;/*  %l0 = _G_ulIntSafeStack     */

#if LW_CFG_SMP_EN > 0
    READ_CPUID(%l1)                                                     ;/*  %l1 = CPU_ID                */
    SLL     %l1   , 2   , %l1                                           ;/*  %l1 = CPU_ID * 4            */
    ADD     %l1   , %l0 , %l0                                           ;/*  %l0 = &safestack[CPU_ID]    */
#endif

    LD      [%l0] , %l1                                                 ;/*  %l1 = 中断安全栈            */

    SUB     %l1   , ASM_STACK_FRAME_SIZE , %fp                          ;/*  开辟栈帧                    */
    SUB     %fp   , ASM_STACK_FRAME_SIZE , %sp                          ;/*  开辟栈空间                  */

3:
    CALL    \handle
    MOV     %g7   , %o0

    CALL    API_InterExit
    NOP

    MOV     %g6   , %g1
    RESTORE_REGS
    MACRO_END()

;/*********************************************************************************************************
;  data_store_error 入口宏
;*********************************************************************************************************/

FUNC_DEF(archDataStoreErrEntry)
    SPARC_TRAP_ENTRY        archDataStoreErrHandle  PC_ARG_REG
    FUNC_END(archDataStoreErrEntry)

;/*********************************************************************************************************
;  instruction_access_MMU_miss 入口宏
;*********************************************************************************************************/

FUNC_DEF(archInstAccessMmuMissEntry)
    SPARC_TRAP_ENTRY        archInstAccessMmuMissHandle  PC_ARG_REG
    FUNC_END(archInstAccessMmuMissEntry)

;/*********************************************************************************************************
;  instruction_access_error 入口宏
;*********************************************************************************************************/

FUNC_DEF(archInstAccessErrEntry)
    SPARC_TRAP_ENTRY        archInstAccessErrHandle  PC_ARG_REG
    FUNC_END(archInstAccessErrEntry)

;/*********************************************************************************************************
;  r_register_access_error 入口宏
;*********************************************************************************************************/

FUNC_DEF(archRRegAccessErrEntry)
    SPARC_TRAP_ENTRY        archRRegAccessErrHandle  PC_ARG_REG
    FUNC_END(archRRegAccessErrEntry)

;/*********************************************************************************************************
;  instruction_access_exception 入口宏
;*********************************************************************************************************/

FUNC_DEF(archInstAccessExcEntry)
    SPARC_TRAP_ENTRY        archInstAccessExcHandle  PC_ARG_REG
    FUNC_END(archInstAccessExcEntry)

;/*********************************************************************************************************
;  privileged_instruction 入口宏
;*********************************************************************************************************/

FUNC_DEF(archPrivInstEntry)
    SPARC_TRAP_ENTRY        archPrivInstHandle  PC_ARG_REG
    FUNC_END(archPrivInstEntry)

;/*********************************************************************************************************
;  illegal_instruction 入口宏
;*********************************************************************************************************/

FUNC_DEF(archIllegalInstEntry)
    SPARC_TRAP_ENTRY        archIllegalInstHandle  PC_ARG_REG
    FUNC_END(archIllegalInstEntry)

;/*********************************************************************************************************
;  fp_disabled 入口宏
;*********************************************************************************************************/

FUNC_DEF(archFpDisableEntry)
    SPARC_TRAP_ENTRY        archFpDisableHandle  PC_ARG_REG
    FUNC_END(archFpDisableEntry)

;/*********************************************************************************************************
;  cp_disabled 入口宏
;*********************************************************************************************************/

FUNC_DEF(archCpDisableEntry)
    SPARC_TRAP_ENTRY        archCpDisableHandle  PC_ARG_REG
    FUNC_END(archCpDisableEntry)

;/*********************************************************************************************************
;  unimplemented_FLUSH 入口宏
;*********************************************************************************************************/

FUNC_DEF(archUnimplFlushEntry)
    SPARC_TRAP_ENTRY        archUnimplFlushHandle  PC_ARG_REG
    FUNC_END(archUnimplFlushEntry)

;/*********************************************************************************************************
;  watchpoint_detected 入口宏
;*********************************************************************************************************/

FUNC_DEF(archWatchPointDectectEntry)
    SPARC_TRAP_ENTRY        archWatchPointDectectHandle  PC_ARG_REG
    FUNC_END(archWatchPointDectectEntry)

;/*********************************************************************************************************
;  mem_address_not_aligned 入口宏
;*********************************************************************************************************/

FUNC_DEF(archMemAddrNoAlignEntry)
    SPARC_TRAP_ENTRY        archMemAddrNoAlignHandle  PC_ARG_REG
    FUNC_END(archMemAddrNoAlignEntry)

;/*********************************************************************************************************
;  fp_exception 入口宏
;*********************************************************************************************************/

FUNC_DEF(archFpExcEntry)
    SPARC_TRAP_ENTRY        archFpExcHandle  PC_ARG_REG
    FUNC_END(archFpExcEntry)

;/*********************************************************************************************************
;  cp_exception 入口宏
;*********************************************************************************************************/

FUNC_DEF(archCpExcEntry)
    SPARC_TRAP_ENTRY        archCpExcHandle  PC_ARG_REG
    FUNC_END(archCpExcEntry)

;/*********************************************************************************************************
;  data_access_error 入口宏
;*********************************************************************************************************/

FUNC_DEF(archDataAccessErrEntry)
    SPARC_TRAP_ENTRY        archDataAccessErrHandle  PC_ARG_REG
    FUNC_END(archDataAccessErrEntry)

;/*********************************************************************************************************
;  data_access_MMU_miss 入口宏
;*********************************************************************************************************/

FUNC_DEF(archDataAccessMmuMissEntry)
    SPARC_TRAP_ENTRY        archDataAccessMmuMissHandle  PC_ARG_REG
    FUNC_END(archDataAccessMmuMissEntry)

;/*********************************************************************************************************
;  data_access_exception 入口宏
;*********************************************************************************************************/

FUNC_DEF(archDataAccessExcEntry)
    SPARC_TRAP_ENTRY        archDataAccessExcHandle  PC_ARG_REG
    FUNC_END(archDataAccessExcEntry)

;/*********************************************************************************************************
;  tag_overflow 入口宏
;*********************************************************************************************************/

FUNC_DEF(archTagOverFlowEntry)
    SPARC_TRAP_ENTRY        archTagOverFlowHandle  PC_ARG_REG
    FUNC_END(archTagOverFlowEntry)

;/*********************************************************************************************************
;  division_by_zero 入口宏
;*********************************************************************************************************/

FUNC_DEF(archDivZeroEntry)
    SPARC_TRAP_ENTRY        archDivZeroHandle  PC_ARG_REG
    FUNC_END(archDivZeroEntry)

;/*********************************************************************************************************
;  trap_instruction 入口宏
;*********************************************************************************************************/

FUNC_DEF(archTrapInstEntry)
    SPARC_TRAP_ENTRY        archTrapInstHandle  PC_ARG_REG
    FUNC_END(archIntEntry)

;/*********************************************************************************************************
;  interrupt_level_n 入口宏
;*********************************************************************************************************/

FUNC_DEF(archIntEntry)
    SPARC_TRAP_ENTRY        bspIntHandle  TRAP_TYPE_ARG_REG
    FUNC_END(archIntEntry)

    FILE_END()
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
