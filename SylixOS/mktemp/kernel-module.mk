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
# ��   ��   ��: kernel-module.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: �ں�ģ�����Ŀ�� makefile ģ��
#*********************************************************************************************************

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
ifneq (,$(findstring yes,$($(target)_USE_CXX_EXCEPT)))
$(target)_CXX_EXCEPT  := $(GCC_NO_CXX_EXCEPT_CFLAGS)
else
$(target)_CXX_EXCEPT  := $(GCC_NO_CXX_EXCEPT_CFLAGS)
endif

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
$(target)_GCOV_FLAGS  := $(GCC_GCOV_CFLAGS)
else
$(target)_GCOV_FLAGS  :=
endif

$(target)_DSYMBOL     += -DSYLIXOS_LIB
$(target)_COMMONFLAGS := $(CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) $(ARCH_KO_CFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $(ARCH_KO_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $(ARCH_KO_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) 

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
$(target)_DEPEND_LIB += -lgcov
endif

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB += -lstdc++
endif

$(target)_DEPEND_LIB += -lm -lgcc

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_KO       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_KO := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# Make archive object files
#*********************************************************************************************************
$($(target)_KO): $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(LD) $(CPUFLAGS) -nostdlib -r -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Strip target
#*********************************************************************************************************
$($(target)_STRIP_KO): $($(target)_KO)
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) --strip-unneeded $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_KO) $($(target)_STRIP_KO)

#*********************************************************************************************************
# End
#*********************************************************************************************************
