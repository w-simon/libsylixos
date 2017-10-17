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
;** 文   件   名: sparcContextAsm.h
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef __ARCH_SPARC_CTX_ASM_H
#define __ARCH_SPARC_CTX_ASM_H

;/*********************************************************************************************************
;  保存 CPU 上下文
;*********************************************************************************************************/

MACRO_DEF(SAVE_REGS)
    SUB     %sp  , ASM_REG_STACK_FRAME_SIZE * 2 , %g1                   ;/*  开辟栈空间                  */

    ST      %g1  , [%g1 + REG_GLOBAL(1)]                                ;/*  保存全局寄存器(除了 %g0)    */
    STD     %g2  , [%g1 + REG_GLOBAL(2)]
    STD     %g4  , [%g1 + REG_GLOBAL(4)]
    STD     %g6  , [%g1 + REG_GLOBAL(6)]

    STD     %l0  , [%g1 + REG_LOCAL(0)]                                 ;/*  保存本地寄存器              */
    STD     %l2  , [%g1 + REG_LOCAL(2)]
    STD     %l4  , [%g1 + REG_LOCAL(4)]
    STD     %l6  , [%g1 + REG_LOCAL(6)]

    STD     %i0  , [%g1 + REG_INPUT(0)]                                 ;/*  保存输入寄存器              */
    STD     %i2  , [%g1 + REG_INPUT(2)]
    STD     %i4  , [%g1 + REG_INPUT(4)]
    STD     %i6  , [%g1 + REG_INPUT(6)]

    STD     %o0  , [%g1 + REG_OUTPUT(0)]                                ;/*  保存输出寄存器              */
    STD     %o2  , [%g1 + REG_OUTPUT(2)]
    STD     %o4  , [%g1 + REG_OUTPUT(4)]
    STD     %o6  , [%g1 + REG_OUTPUT(6)]

    RD      %y   , %l0
    ST      %l0  , [%g1 + REG_Y]                                        ;/*  保存 Y 寄存器               */

    ADD     %o7  , 0x8  , %o7                                           ;/*  %o7 = CALL 指令的地址       */
    ST      %o7  , [%g1 + REG_PC]                                       ;/*  保存 PC (延时槽的下一条指令)*/

    ADD     %o7  , 0x4  , %o7                                           ;/*  Next PC = PC + 4            */
    ST      %o7  , [%g1 + REG_NPC]                                      ;/*  保存 Next PC                */

    RD      %psr , %l0
    ST      %l0  , [%g1 + REG_PSR]                                      ;/*  保存 PSR 状态寄存器         */
    MACRO_END()

;/*********************************************************************************************************
;  加载 CPU 上下文
;  %g1 为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

MACRO_DEF(RESTORE_REGS)
    RD      %psr , %g3
    AND     %g3  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    LD      [%g1 + REG_PSR] , %l0                                       ;/*  %l0 = 待恢复的 %psr         */
    ANDN    %l0  , PSR_ET   , %l0                                       ;/*  禁能异常                    */
    ANDN    %l0  , SPARC_PSR_CWP_MASK , %l0                             ;/*  清除 CWP 部分               */
    OR      %l0  , %g3 , %l0                                            ;/*  使用当前的 CWP              */
    WR      %l0  , %psr                                                 ;/*  恢复 %psr                   */
    NOP
    NOP
    NOP

    ADD     %g3  , 1 , %g3                                              ;/*  %g3 = (CWP + 1) % NWIN      */
    AND     %g3  , LW_CFG_CPU_REG_WIN_NR - 1 , %g3

    MOV     1    , %g4
    SLL     %g4  , %g3 , %g4                                            ;/*  %wim = %g4 = 1 << %g3       */
    WR      %g4  , %wim
    NOP
    NOP
    NOP

    LD      [%g1 + REG_Y] , %g4
    WR      %g4  , %y                                                   ;/*  恢复 Y 寄存器               */

    LDD     [%g1 + REG_GLOBAL(2)] , %g2                                 ;/*  恢复全局寄存器              */
    LDD     [%g1 + REG_GLOBAL(4)] , %g4
    LDD     [%g1 + REG_GLOBAL(6)] , %g6

    LDD     [%g1 + REG_OUTPUT(0)] , %o0                                 ;/*  恢复输出寄存器              */
    LDD     [%g1 + REG_OUTPUT(2)] , %o2
    LDD     [%g1 + REG_OUTPUT(4)] , %o4
    LDD     [%g1 + REG_OUTPUT(6)] , %o6

    LDD     [%g1 + REG_LOCAL(0)]  , %l0                                 ;/*  恢复本地寄存器              */
    LDD     [%g1 + REG_LOCAL(2)]  , %l2
    LDD     [%g1 + REG_LOCAL(4)]  , %l4
    LDD     [%g1 + REG_LOCAL(6)]  , %l6

    LDD     [%g1 + REG_INPUT(0)]  , %i0                                 ;/*  恢复输入寄存器              */
    LDD     [%g1 + REG_INPUT(2)]  , %i2
    LDD     [%g1 + REG_INPUT(4)]  , %i4
    LDD     [%g1 + REG_INPUT(6)]  , %i6

    SAVE                                                                ;/*  进入到一个 dummy 窗口       */

    LD      [%g1 + REG_PC]        , %l1                                 ;/*  %l1 在 dummy 窗口!          */
    LD      [%g1 + REG_NPC]       , %l2                                 ;/*  %l2 在 dummy 窗口!          */
    LD      [%g1 + REG_GLOBAL(1)] , %g1                                 ;/*  最后恢复 %g1                */

    JMP     %l1
    RETT    %l2
    MACRO_END()

;/*********************************************************************************************************
;  FLUSH 窗口
;  %g1 %g6 %g7 不会被破坏
;*********************************************************************************************************/

MACRO_DEF(FLUSH_WINDOWS)
    RD      %psr , %l0
    ANDN    %l0  , PSR_ET , %l0
    WR      %l0  , %psr                                                 ;/*  关闭异常                    */
    NOP
    NOP
    NOP

    AND     %l0  , SPARC_PSR_CWP_MASK , %g3                             ;/*  %g3 = CWP                   */

    MOV     1    , %g4                                                  ;/*  %g4 = 1                     */
    SLL     %g4  , %g3 , %g4                                            ;/*  %g4 = WIM mask for CW invalid*/

    RD      %wim , %g2                                                  ;/*  %g2 = wim                   */

123:
    SLL     %g4  , 1   , %g5                                            ;/*  rotate the "wim" left 1     */
    SRL     %g4  , LW_CFG_CPU_REG_WIN_NR - 1 , %g4
    OR      %g4  , %g5 , %g4                                            ;/*  %g4 = wim if we do one restore*/

    ;/*
    ; * If a restore would not underflow, then continue.
    ; */
    ANDCC   %g4  , %g2 , %g0                                            ;/*  Any windows to flush?       */
    BNZ     456f                                                        ;/*  No, then continue           */
    NOP

    RESTORE                                                             ;/*  back one window             */

    ;/*
    ; * Now save the window just as if we overflowed to it.
    ; */
    STD     %l0  , [%sp + SF_LOCAL(0)]
    STD     %l2  , [%sp + SF_LOCAL(2)]
    STD     %l4  , [%sp + SF_LOCAL(4)]
    STD     %l6  , [%sp + SF_LOCAL(6)]

    STD     %i0  , [%sp + SF_INPUT(0)]
    STD     %i2  , [%sp + SF_INPUT(2)]
    STD     %i4  , [%sp + SF_INPUT(4)]
    STD     %i6  , [%sp + SF_INPUT(6)]

    BA      123b
    NOP

456:
    RD      %psr , %l0
    OR      %l0  , PSR_ET  , %l0                                        ;/*  使能异常                    */
    OR      %l0  , PSR_PIL , %l0                                        ;/*  加入 PSR_PIL 去关中断       */
    WR      %l0  , %psr
    NOP
    NOP
    NOP
    MACRO_END()

#endif                                                                  /*  __ARCH_SPARC_CTX_ASM_H       */
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
