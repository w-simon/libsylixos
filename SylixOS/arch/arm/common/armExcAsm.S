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
;** 文   件   名: armExcAsm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    FILE_BEGIN()

;/*********************************************************************************************************
;  ARMv7M 体系构架相关异常处理函数
;*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)

    IMPORT_LABEL(armv7mIntHandle)
    IMPORT_LABEL(armv7mSvcHandle)
    EXPORT_LABEL(archIntEntry)
    EXPORT_LABEL(archSwiEntry)
    EXPORT_LABEL(armv7mIntCtxLoad)

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

FUNC_DEF(archIntEntry)
    ;/*
    ; *  R0 用于记录异常号
    ; */

    ;/*
    ; *  R3 记录 BASEPRI, 并关中断
    ; *  利用优先级屏蔽中断, 才能使用 SVC
    ; */
    MRS     R3 , BASEPRI                                        ;/*  Set prio int mask to mask all       */
    MOV     R2 , #2                                             ;/*  mask prio >= 2                      */
    MSR     BASEPRI,  R2

    ;/*
    ; *  R2 等于异常进入前使用的 SP
    ; */
    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R2 , MSP
    MRSNE   R2 , PSP

    ;/*
    ; *  保存余下的 R3-11, R14 寄存器到异常进入前使用的 SP
    ; */
    SUBS    R2 , R2, #0x28
    STM     R2 , {R3-R11, LR}

    ;/*
    ; *  异常进入前使用的 SP 等于 R2
    ; */
    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    IT      NE                                                  ;/*  change by hx , should use if then   */
    MSRNE   PSP, R2

    ;/*
    ; *  现在使用的是 MSP, 所以无论如何都设置 MSP
    ; */
    MSR     MSP, R2

    ;/*
    ; *  R0, R2 放置到 R4, R5
    ; */
    MOV     R4 , R0
    MOV     R5 , R2

    ;/*
    ; *  调用 API_InterEnter
    ; */
    LDR     R1 , =API_InterEnter
    BLX     R1

    ;/*
    ; *  如果发生中断嵌套, 跳转到 2
    ; */
    CMP     R0 , #1
    BNE     1f

    ;/*
    ; *  调用 API_InterStackBaseGet, 获得当前 CPU 中断堆栈栈顶并设置 MSP
    ; */
    LDR     R1 , =API_InterStackBaseGet
    BLX     R1
    MSR     MSP, R0

    ;/*
    ; *  R4, R5 恢复到 R0, R1
    ; */
1:
    MOV     R0 , R4
    MOV     R1 , R5

    ;/*
    ; *  调用 armv7mIntHandle(UINT32  uiVector, ARCH_REG_CTX  *pregctx)
    ; */
    LDR     R2 , =armv7mIntHandle
    BLX     R2

FUNC_DEF(armv7mIntCtxLoad)
    ;/*
    ; *  恢复 R3-11, R14 寄存器
    ; */
    LDM     R0 , {R3-R11, LR}
    ADDS    R0 , R0, #0x28

    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MSREQ   MSP, R0
    MSRNE   PSP, R0

    MSR     BASEPRI, R3

    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  swi 入口
;*********************************************************************************************************/

FUNC_DEF(archSwiEntry)
    ;/*
    ; *  R0 用于记录异常号
    ; */

    ;/*
    ; *  R3 记录 BASEPRI, 并关中断
    ; *  利用优先级屏蔽中断, 才能使用 SVC
    ; */
    MRS     R3 , BASEPRI                                        ;/*  Set prio int mask to mask all       */
    MOV     R2 , #2                                             ;/*  mask prio >= 2                      */
    MSR     BASEPRI,  R2

    ;/*
    ; *  R2 等于异常进入前使用的 SP
    ; */
    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MRSEQ   R2 , MSP
    MRSNE   R2 , PSP

    ;/*
    ; *  保存余下的 R3-11, R14 寄存器到异常进入前使用的 SP
    ; */
    SUBS    R2 , R2, #0x28
    STM     R2 , {R3-R11, LR}

    ;/*
    ; *  异常进入前使用的 SP 等于 R2
    ; */
    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    IT      NE
    MSRNE   PSP, R2

    ;/*
    ; *  现在使用的是 MSP, 所以无论如何都设置 MSP
    ; */
#if LW_CFG_SMP_EN > 0
    MOV     R9 , R0
    LDR     R2 , =_SchedSafeStack                               ;/*  _SchedSafeStack();                  */
    BLX     R2
    MSR     MSP, R0
    MOV     R0 , R9
#else
    MSR     MSP, R2
#endif

    ;/*
    ; *  R2 放置到 R1
    ; */
    MOV     R1 , R2

    ;/*
    ; *  调用 armv7mSvcHandle(UINT32  uiVector, ARCH_REG_CTX  *pregctx)
    ; */
    LDR     R2 , =armv7mSvcHandle
    BLX     R2

    ;/*
    ; *  恢复 R3-11, R14 寄存器
    ; */
    LDM     R0 , {R3-R11, LR}
    ADDS    R0 , R0, #0x28

    TST     LR , #0x04                                          ;/*  nonzero if context on process stack */
    ITE     EQ                                                  ;/*  next two instructions conditional   */
    MSREQ   MSP, R0
    MSRNE   PSP, R0

    MSR     BASEPRI, R3

    BX      LR
    FUNC_END()

#else
;/*********************************************************************************************************
;  ARM 体系构架相关异常处理函数
;*********************************************************************************************************/

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    
    IMPORT_LABEL(archAbtHandle)
    IMPORT_LABEL(archUndHandle)
    IMPORT_LABEL(archSwiHandle)
    IMPORT_LABEL(bspIntHandle)
    
    EXPORT_LABEL(archIntEntry)
    EXPORT_LABEL(archAbtEntry)
    EXPORT_LABEL(archPreEntry)
    EXPORT_LABEL(archUndEntry)
    EXPORT_LABEL(archSwiEntry)
    
;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 IRQ 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 IRQ 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 IRQ 堆栈指针           */
    SUB     R2 , LR, #4                                                 ;/*  调整用于中断返回的 PC 值    */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR_irq 拷贝到 R3       */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
        
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    LDR     R5 , =bspIntHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archIntEntry)
    IRQ_ENTRY
    FUNC_END()
    
;/*********************************************************************************************************
;  abort 入口 
;  data_abort     $retoff == 8
;  prefetch_abort $retoff == 4
;*********************************************************************************************************/

MACRO_DEF(ABT_ENTRY retoff)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 abt 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 abt 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 abt 堆栈指针           */
    SUB     R2 , LR, #\retoff                                           ;/*  调整用于中断返回的 PC 值    */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR 拷贝到 R3           */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    LDR     R8 , [SP , #(15 * 4)]                                       ;/*  R8 -> LR 地址               */
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
    
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    MOV     R0 , R8                                                     ;/*  R8 -> LR 地址               */
    MOV     R1 , #\retoff                                               ;/*  $retoff                     */
    LDR     R5 , =archAbtHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archAbtEntry)
    ABT_ENTRY 8
    FUNC_END()

FUNC_DEF(archPreEntry)
    ABT_ENTRY 4
    FUNC_END()

;/*********************************************************************************************************
;  undefine 入口
;*********************************************************************************************************/

MACRO_DEF(UND_ENTRY)
    STMFD   SP!, {R1-R3}                                                ;/*  寄存器压入 und 模式堆栈     */
    MOV     R1 , SP                                                     ;/*  保存 und 堆栈指针           */
    ADD     SP , SP, #12                                                ;/*  调整 und 堆栈指针           */
    MRS     R3 , SPSR                                                   ;/*  将 SPSR 拷贝到 R3           */
    TST     R3 , #0x20                                                  ;/*  是否为 Thumb 模式           */
    SUBNE   R2 , LR, #2                                                 ;/*  计算 Thumb 模式返回位置     */
    SUBEQ   R2 , LR, #4                                                 ;/*  计算 ARM 模式返回位置       */
    MSR     CPSR_c, #(DIS_INT | SVC32_MODE)                             ;/*  回到 SVC 模式               */
    
    STMFD   SP!, {R2}                                                   ;/*  任务返回地址压栈            */
    STMFD   SP!, {LR}                                                   ;/*  LR 压栈                     */
    STMFD   SP!, {R4-R12}                                               ;/*  R12 - R4 压栈               */
    
    LDMFD   R1!, {R4-R6}                                                ;/*  将任务寄存器 R1 - R3 从     */
                                                                        ;/*  IRQ 堆栈拷贝到 SVC 堆栈     */
    STMFD   SP!, {R4-R6}
    STMFD   SP!, {R0}                                                   ;/*  R0 压栈                     */
    STMFD   SP!, {R3}                                                   ;/*  CPSR 压栈                   */
    
    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    LDR     R0 , =API_InterEnter
    MOV     LR , PC
    BX      R0
    LDR     R8 , [SP, #(15 * 4)]                                        ;/*  参数为产生未定义指令中断地址*/
    LDR     R9 , [SP]                                                   ;/*  产生异常的 CPSR             */
    CMP     R0 , #1
    BNE     1f
    
    LDR     R0 , =API_ThreadTcbInter                                    ;/*  get current tcb             */
    MOV     LR , PC
    BX      R0
    STR     SP , [R0]                                                   ;/*  current stack = SP          */
    
    LDR     R0 , =API_InterStackBaseGet                                 ;/*  获得当前 CPU 中断堆栈栈顶   */
    MOV     LR , PC
    BX      R0
    MOV     SP , R0
    
1:
    MOV     R0 , R8
    MOV     R1 , R9
    LDR     R5 , =archUndHandle
    MOV     LR , PC
    BX      R5
    
    LDR     R0 , =API_InterExit
    MOV     LR , PC
    BX      R0
    
    LDMFD   SP!, {R4}
    MSR     SPSR_cxsf, R4
    LDMFD   SP!, {R0-R12, LR, PC}^
    MACRO_END()
    
FUNC_DEF(archUndEntry)
    UND_ENTRY
    FUNC_END()
    
;/*********************************************************************************************************
;  swi 入口
;*********************************************************************************************************/

MACRO_DEF(SWI_ENTRY)
    STMFD   SP!, {R0-R12, LR}

    LDR     R0 , [LR, #-4]                                              ;/*  读取 SWI 指令               */
    BIC     R0 , R0, #0xff000000                                        ;/*  获得 SWI 指令号             */
    MOV     R1 , SP                                                     ;/*  获得参数表位置              */

    LDR     R5 , =archSwiHandle
    MOV     LR , PC
    BX      R5

    LDMFD   SP!, {R0-R12, PC}^                                          ;/*  SWI 返回                    */
    MACRO_END()

FUNC_DEF(archSwiEntry)
    SWI_ENTRY
    FUNC_END()
#endif

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
