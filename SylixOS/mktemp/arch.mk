#*********************************************************************************************************
#
#                                    �й������Դ��֯
#
#                                   Ƕ��ʽʵʱ����ϵͳ
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------�ļ���Ϣ--------------------------------------------------------------------------------
#
# ��   ��   ��: arch.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: ����� arch ��صı���
#*********************************************************************************************************

#*********************************************************************************************************
# x86 (Need frame pointer code to debug)
#*********************************************************************************************************
ifneq (,$(findstring i386,$(TOOLCHAIN_PREFIX)))
ARCH             = x86
ARCH_COMMONFLAGS = -mlong-double-64 -fno-omit-frame-pointer

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ARCH_FPUFLAGS = -m$(FPU_TYPE)

ifneq ($(CPU_TYPE),)
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -march=$(CPU_TYPE)
else
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = 
endif
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# x86-64 (Need frame pointer code to debug)
#*********************************************************************************************************
ifneq (,$(findstring x86_64,$(TOOLCHAIN_PREFIX)))
ARCH             = x64
ARCH_COMMONFLAGS = -mlong-double-64 -mno-red-zone -fno-omit-frame-pointer

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   = -mcmodel=large
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  = -mcmodel=kernel
ARCH_KERNEL_LDFLAGS = -z max-page-size=4096

ARCH_FPUFLAGS = -m$(FPU_TYPE)

ifneq ($(CPU_TYPE),)
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -march=$(CPU_TYPE) -m64
else
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -m64
endif
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
endif

#*********************************************************************************************************
# ARM
#*********************************************************************************************************
ifneq (,$(findstring arm,$(TOOLCHAIN_PREFIX)))
ARCH             = arm
ARCH_COMMONFLAGS = -mno-unaligned-access

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -nostdlib -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ifneq (,$(findstring disable,$(FPU_TYPE)))
ARCH_FPUFLAGS = 
else
ifneq ($(FLOAT_ABI),)
ifneq (,$(findstring default,$(FLOAT_ABI)))
FLOAT_ABI = softfp
endif
ARCH_FPUFLAGS = -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU_TYPE)
else
ARCH_FPUFLAGS = -mfloat-abi=softfp -mfpu=$(FPU_TYPE)
endif
endif

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
ifneq (,$(findstring cortex-m,$(CPU_TYPE)))
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS += -mthumb
endif
ifneq (,$(findstring be,$(TOOLCHAIN_PREFIX)))
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS += -mbig-endian
endif
ARCH_CPUFLAGS                   = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU             = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS)
endif

#*********************************************************************************************************
# MIPS (SylixOS toolchain 4.9.3 has loongson3x '-mhard-float' patch)
#*********************************************************************************************************
ifneq (,$(findstring mips-sylixos,$(TOOLCHAIN_PREFIX)))
ARCH             = mips
ARCH_COMMONFLAGS = 

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC -mabicalls
ARCH_PIC_LDFLAGS = -mabicalls -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   = -mlong-calls
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS = -mlong-calls

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ifneq (,$(findstring ls3x-float,$(FPU_TYPE)))
LS3X_NEED_NO_ODD_SPREG := $(shell expr `echo $(TOOLCHAIN_VERSION_MAJOR)` \>= 5)
ifeq "$(LS3X_NEED_NO_ODD_SPREG)" "1"
ARCH_FPUFLAGS = -mhard-float -mno-odd-spreg
else
ARCH_FPUFLAGS = -mhard-float
endif
ifneq (,$(findstring -mdsp,$(FPU_TYPE)))
ARCH_FPUFLAGS += -mdsp
endif
else
ARCH_FPUFLAGS = -m$(FPU_TYPE)
endif

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -march=$(CPU_TYPE) -EL -G 0 -mno-branch-likely
ARCH_CPUFLAGS       = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# MIPS64 (SylixOS toolchain 4.9.3 has loongson3x '-mhard-float' patch)
#*********************************************************************************************************
ifneq (,$(findstring mips64,$(TOOLCHAIN_PREFIX)))
ARCH             = mips64
ARCH_COMMONFLAGS = 

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC -mabicalls
ARCH_PIC_LDFLAGS = -mabicalls -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   = -mlong-calls
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS = -mlong-calls

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ifneq (,$(findstring ls3x-float,$(FPU_TYPE)))
LS3X_NEED_NO_ODD_SPREG := $(shell expr `echo $(TOOLCHAIN_VERSION_MAJOR)` \>= 5)
ifeq "$(LS3X_NEED_NO_ODD_SPREG)" "1"
ARCH_FPUFLAGS = -mhard-float -mno-odd-spreg
else
ARCH_FPUFLAGS = -mhard-float
endif
ifneq (,$(findstring -mdsp,$(FPU_TYPE)))
ARCH_FPUFLAGS += -mdsp
endif
else
ARCH_FPUFLAGS = -m$(FPU_TYPE)
endif

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -march=$(CPU_TYPE) -EL -G 0 -mno-branch-likely -mabi=64
ifneq (,$(findstring mips64-hrsylixos,$(TOOLCHAIN_PREFIX)))
ARCH_CPUFLAGS_WITHOUT_FPUFLAGS += -fno-delayed-branch
endif
ARCH_CPUFLAGS       = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# PowerPC
#*********************************************************************************************************
ifneq (,$(findstring ppc,$(TOOLCHAIN_PREFIX)))
ARCH             = ppc
ARCH_COMMONFLAGS = -G 0 -mstrict-align

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ARCH_FPUFLAGS = -m$(FPU_TYPE)

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float -mno-spe -mno-altivec 
endif

#*********************************************************************************************************
# TI C6X DSP
#*********************************************************************************************************
ifneq (,$(findstring cl6x,$(TOOLCHAIN_PREFIX)))

ifneq (,$(findstring _be,$(CPU_TYPE)))
ARCH_CPU_TYPE   = $(subst _be,,$(CPU_TYPE))
ARCH_CPU_ENDIAN = --big_endian
else
ARCH_CPU_TYPE   = $(CPU_TYPE)
ARCH_CPU_ENDIAN =
endif

ARCH             = c6x
ARCH_COMMONFLAGS = \
	--abi=eabi --c99 --gcc --linux -D__GNUC__=4 -D__GNUC_MINOR__=5 -D__STDC__ \
    -D__extension__= -D__nothrow__= -D__SIZE_TYPE__="unsigned int" \
    -D__WINT_TYPE__="unsigned int" -D__WCHAR_TYPE__="unsigned int" \
    -D__gnuc_va_list=va_list -D__EXCLIB_STDARG --mem_model:data=far \
    --multithread -D__TI_SHARED_DATA_SYNCHRONIZATION -D__TI_RECURSIVE_RESOURCE_MUTEXES

ARCH_PIC_ASFLAGS = --pic
ARCH_PIC_CFLAGS  = --pic
ARCH_PIC_LDFLAGS =  -z --sysv --shared --trampolines=off --dsbt_size=64

ARCH_KO_CFLAGS   =
ARCH_KO_LDFLAGS  = --abi=eabi -z -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ARCH_FPUFLAGS =

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -mv$(ARCH_CPU_TYPE) $(ARCH_CPU_ENDIAN)
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS)
endif

#*********************************************************************************************************
# SPARC (Need frame pointer code to debug)
#*********************************************************************************************************
ifneq (,$(findstring sparc,$(TOOLCHAIN_PREFIX)))
ARCH             = sparc
ARCH_COMMONFLAGS = 

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =
ARCH_KO_LDFLAGS  = -nostdlib -r

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ARCH_FPUFLAGS = -m$(FPU_TYPE)

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = -mcpu=$(CPU_TYPE)
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_FPUFLAGS)
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -msoft-float
endif

#*********************************************************************************************************
# RISC-V (Need frame pointer code to debug)
#*********************************************************************************************************
ifneq (,$(findstring riscv,$(TOOLCHAIN_PREFIX)))
ARCH             = riscv
ARCH_COMMONFLAGS = -mstrict-align -mcmodel=medany -mno-save-restore

ARCH_PIC_ASFLAGS = 
ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   = -fPIC
ARCH_KO_LDFLAGS  = -r -fPIC

ARCH_KLIB_CFLAGS =

ARCH_KERNEL_CFLAGS  =
ARCH_KERNEL_LDFLAGS =

ARCH_FPUFLAGS = -mabi=$(FPU_TYPE)

ARCH_CPUFLAGS_WITHOUT_FPUFLAGS = 
ARCH_CPUFLAGS                  = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) -march=$(CPU_TYPE) $(ARCH_FPUFLAGS)
ARCH_CPU_TYPE_NOFDQ            = -march=$(subst q,,$(subst d,,$(subst f,,$(CPU_TYPE))))
ARCH_FPU_TYPE_NOFDQ            = -mabi=$(subst q,,$(subst d,,$(subst f,,$(FPU_TYPE))))
ARCH_CPUFLAGS_NOFPU            = $(ARCH_CPUFLAGS_WITHOUT_FPUFLAGS) $(ARCH_CPU_TYPE_NOFDQ) $(ARCH_FPU_TYPE_NOFDQ)
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
