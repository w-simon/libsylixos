#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: arch.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: 存放与 arch 相关的变量
#*********************************************************************************************************

#*********************************************************************************************************
# x86 (Need frame pointer code to debug)
#*********************************************************************************************************
ifneq (,$(findstring i386,$(TOOLCHAIN_PREFIX)))
ARCH             = x86
ARCH_COMMONFLAGS = -mlong-double-64 -fno-omit-frame-pointer

ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = 
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# ARM
#*********************************************************************************************************
ifneq (,$(findstring arm,$(TOOLCHAIN_PREFIX)))
ARCH             = arm
ARCH_COMMONFLAGS = -mno-unaligned-access

ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -nostdlib -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =

ifneq (,$(findstring disable,$(FPU_TYPE)))
FPUFLAGS = 
else
FPUFLAGS = -mfloat-abi=softfp -mfpu=$(FPU_TYPE)
endif

CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS)
endif

#*********************************************************************************************************
# MIPS
#*********************************************************************************************************
ifneq (,$(findstring mips,$(TOOLCHAIN_PREFIX)))
ARCH             = mips
ARCH_COMMONFLAGS =

ARCH_PIC_CFLAGS  = -fPIC -mabicalls
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -mabicalls -shared

ARCH_KO_CFLAGS   = -mlong-calls

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = -march=$(CPU_TYPE) -EL -G 0
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# PowerPC
#*********************************************************************************************************
ifneq (,$(findstring ppc,$(TOOLCHAIN_PREFIX)))
ARCH             = ppc
ARCH_COMMONFLAGS =

ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
