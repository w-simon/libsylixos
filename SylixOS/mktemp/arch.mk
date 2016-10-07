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
# x86
#*********************************************************************************************************
ifneq (,$(findstring i386,$(TOOLCHAIN_PREFIX)))
ARCH             = x86
ARCH_COMMONFLAGS = -mlong-double-64

ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =

FPUFLAGS = -m$(FPU_TYPE)
CPUFLAGS = $(FPUFLAGS)
endif

#*********************************************************************************************************
# arm
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

CPUFLAGS = -mcpu=$(CPU_TYPE) $(FPUFLAGS)
endif

#*********************************************************************************************************
# mips
#*********************************************************************************************************
ifneq (,$(findstring mips,$(TOOLCHAIN_PREFIX)))
ARCH             = mips
ARCH_COMMONFLAGS =

ARCH_PIC_CFLAGS  = -fPIC -mabicalls
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -mabicalls -shared

ARCH_KO_CFLAGS   = -mlong-calls

FPUFLAGS = -m$(FPU_TYPE)
CPUFLAGS = -march=$(CPU_TYPE) -EL -G 0 $(FPUFLAGS)
endif

#*********************************************************************************************************
# powerpc
#*********************************************************************************************************
ifneq (,$(findstring ppc,$(TOOLCHAIN_PREFIX)))
ARCH             = ppc
ARCH_COMMONFLAGS =

ARCH_PIC_CFLAGS  = -fPIC
ARCH_PIC_LDFLAGS = -Wl,-shared -fPIC -shared

ARCH_KO_CFLAGS   =

FPUFLAGS = -m$(FPU_TYPE)
CPUFLAGS = -mcpu=$(CPU_TYPE) $(FPUFLAGS)
endif

#*********************************************************************************************************
# end
#*********************************************************************************************************
