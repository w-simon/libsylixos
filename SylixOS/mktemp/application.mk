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
# 文   件   名: application.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: 应用程序类目标 makefile 模板
#*********************************************************************************************************

#*********************************************************************************************************
# include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
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

$(target)_COMMONFLAGS := $(CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) 

#*********************************************************************************************************
# targets
#*********************************************************************************************************
$(target)_EXE       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_EXE := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += -lvpmpdm

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
$(target)_DEPEND_LIB += -lgcov
endif

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB += -lstdc++
endif

$(target)_DEPEND_LIB += -ldsohandle -lm -lgcc

#*********************************************************************************************************
# link object files
#*********************************************************************************************************
$($(target)_EXE): $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(LD) $(CPUFLAGS) $(ARCH_PIC_LDFLAGS) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# strip target
#*********************************************************************************************************
$($(target)_STRIP_EXE): $($(target)_EXE)
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_EXE) $($(target)_STRIP_EXE)

#*********************************************************************************************************
# end
#*********************************************************************************************************
