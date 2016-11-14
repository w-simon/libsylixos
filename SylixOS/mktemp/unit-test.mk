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
# 文   件   名: unit-test.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 10 月 01 日
#
# 描        述: 单元测试应用程序类目标 makefile 模板
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

$(target)_CPUFLAGS    := $(CPUFLAGS)
$(target)_COMMONFLAGS := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $(ARCH_PIC_CFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_EXE       := $(addprefix $(OUTPATH)/$(target)/, $(basename $(LOCAL_SRCS)))
$(target)_STRIP_EXE := $(addprefix $(OUTPATH)/strip/$(target)/, $(basename $(LOCAL_SRCS)))

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB_PATH += -L"$(SYLIXOS_BASE_PATH)/libcextern/$(OUTDIR)"
endif

$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += -lvpmpdm

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
$(target)_DEPEND_LIB += -lgcov
endif

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB += -lcextern -lstdc++ -ldsohandle
endif

$(target)_DEPEND_LIB += -lm -lgcc

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
__UNIT_TEST_TARGET         = $(word 3,$(subst $(BIAS),$(SPACE),$(1)))
__UNIT_TEST_STRIP_TARGET   = $(word 4,$(subst $(BIAS),$(SPACE),$(1)))

__UNIT_TEST_LIBRARIES      = $($(__UNIT_TEST_TARGET)_DEPEND_LIB_PATH) $($(__UNIT_TEST_TARGET)_DEPEND_LIB)

__UNIT_TEST_PRE_LINK_CMD   = $($(__UNIT_TEST_TARGET)_PRE_LINK_CMD)
__UNIT_TEST_POST_LINK_CMD  = $($(__UNIT_TEST_TARGET)_POST_LINK_CMD)

__UNIT_TEST_PRE_STRIP_CMD  = $($(__UNIT_TEST_STRIP_TARGET)_PRE_STRIP_CMD)
__UNIT_TEST_POST_STRIP_CMD = $($(__UNIT_TEST_STRIP_TARGET)_POST_STRIP_CMD)

__UNIT_TEST_CPUFLAGS       = $($(__UNIT_TEST_TARGET)_CPUFLAGS)

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
define CREATE_TARGET_EXE
$1: $2 $3
		@if [ ! -d "$(dir $1)" ]; then mkdir -p "$(dir $1)"; fi
		@rm -f $1
		$(__UNIT_TEST_PRE_LINK_CMD)
		$(LD) $(__UNIT_TEST_CPUFLAGS) $(ARCH_PIC_LDFLAGS) -o $1 $2 $(__UNIT_TEST_LIBRARIES)
		$(__UNIT_TEST_POST_LINK_CMD)
endef

$(foreach src,$(LOCAL_SRCS),$(eval $(call CREATE_TARGET_EXE,\
$(addprefix $(OUTPATH)/$(target)/, $(basename $(src))),\
$(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(src)))),\
$($(target)_DEPEND_TARGET),\
)))

#*********************************************************************************************************
# Strip target
#*********************************************************************************************************
define CREATE_TARGET_STRIP_EXE
$1: $2
		@if [ ! -d "$(dir $1)" ]; then mkdir -p "$(dir $1)"; fi
		@rm -f $1
		$(__UNIT_TEST_PRE_STRIP_CMD)
		$(STRIP) $2 -o $1
		$(__UNIT_TEST_POST_STRIP_CMD)
endef

$(foreach src,$(LOCAL_SRCS),$(eval $(call CREATE_TARGET_STRIP_EXE,\
$(addprefix $(OUTPATH)/strip/$(target)/, $(basename $(src))),\
$(addprefix $(OUTPATH)/$(target)/, $(basename $(src))),\
)))

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_EXE) $($(target)_STRIP_EXE)

#*********************************************************************************************************
# End
#*********************************************************************************************************
