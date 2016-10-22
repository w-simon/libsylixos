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
# 文   件   名: library.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: 动态库类目标 makefile 模板
#*********************************************************************************************************

#*********************************************************************************************************
# Include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# Depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
ifneq (,$(findstring yes,$($(target)_USE_CXX_EXCEPT)))
$(target)_CXX_EXCEPT  := $(GCC_CXX_EXCEPT_CFLAGS)
else
$(target)_CXX_EXCEPT  := $(GCC_NO_CXX_EXCEPT_CFLAGS)
endif

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
$(target)_GCOV_FLAGS  := $(GCC_GCOV_CFLAGS)
else
$(target)_GCOV_FLAGS  :=
endif

$(target)_DSYMBOL     += -DSYLIXOS_LIB

$(target)_CPUFLAGS    := $(CPUFLAGS)
$(target)_COMMONFLAGS := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_SO       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_SO := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)
$(target)_A        := $(OUTPATH)/$(addsuffix .a, $(basename $(LOCAL_TARGET_NAME)))

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

ifneq (,$(findstring libvpmpdm.so,$(LOCAL_TARGET_NAME)))
$(target)_DEPEND_LIB += -lm -lgcc
else
$(target)_DEPEND_LIB += -ldsohandle -lm -lgcc
endif

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
$($(target)_SO): $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(LD) $(__CPUFLAGS) $(ARCH_PIC_LDFLAGS) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Strip target
#*********************************************************************************************************
$($(target)_STRIP_SO): $($(target)_SO)
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# Make archive object files
#*********************************************************************************************************
$($(target)_A): $($(target)_OBJS)
		@rm -f $@
		$(AR) -r $@ $(__OBJS)

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_SO) $($(target)_STRIP_SO) $($(target)_A)

#*********************************************************************************************************
# End
#*********************************************************************************************************
