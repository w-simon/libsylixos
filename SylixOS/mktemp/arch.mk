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

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = 
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# x86-64 (Need frame pointer code to debug)
#*********************************************************************************************************

ifneq (,$(findstring x86_64,$(TOOLCHAIN_PREFIX)))
ARCH             = x86
ARCH_COMMONFLAGS = -mno-red-zone -fno-omit-frame-pointer

ARCH_PIC_CFLAGS  = -fPIC -mpreferred-stack-boundary=5
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   = -mcmodel=large

ARCH_KERNEL_CFLAGS  = -mcmodel=kernel
ARCH_KERNEL_LDFLAGS = -z max-page-size=4096

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = -m64
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
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

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ifneq (,$(findstring disable,$(FPU_TYPE)))
FPUFLAGS = 
else
ifneq ($(FLOAT_ABI),)
ifneq (,$(findstring default,$(FLOAT_ABI)))
FLOAT_ABI = softfp
endif
FPUFLAGS = -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU_TYPE)
else
FPUFLAGS = -mfloat-abi=softfp -mfpu=$(FPU_TYPE)
endif
endif

CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
ifneq (,$(findstring cortex-m,$(CPU_TYPE)))
CPUFLAGS_WITHOUT_FPUFLAGS += -mthumb
endif
ifneq (,$(findstring be,$(TOOLCHAIN_PREFIX)))
CPUFLAGS_WITHOUT_FPUFLAGS += -mbig-endian
endif
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS)
endif

#*********************************************************************************************************
# MIPS (SylixOS toolchain 4.9.3 has loogson3x '-mhard-float' patch)
#*********************************************************************************************************
ifneq (,$(findstring mips,$(TOOLCHAIN_PREFIX)))
ARCH             = mips
ARCH_COMMONFLAGS =

ARCH_PIC_CFLAGS  = -fPIC -mabicalls
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -mabicalls -shared

ARCH_KO_CFLAGS   = -mlong-calls

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ifneq (,$(findstring ls3x-float,$(FPU_TYPE)))
LS3X_NEED_NO_ODD_SPREG := $(shell expr `echo $(GCC_VERSION_MAJOR)` \>= 5)
ifeq "$(LS3X_NEED_NO_ODD_SPREG)" "1"
FPUFLAGS = -mhard-float -mno-odd-spreg
else
FPUFLAGS = -mhard-float
endif
else
FPUFLAGS = -m$(FPU_TYPE)
endif

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

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

FPUFLAGS = -m$(FPU_TYPE)

CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
CPUFLAGS                  = $(CPUFLAGS_WITHOUT_FPUFLAGS) $(FPUFLAGS)
CPUFLAGS_NOFPU            = $(CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
