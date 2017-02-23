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
# 文   件   名: extension.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2017 年 02 月 20 日
#
# 描        述: lite 版本 bsp 扩展类目标 makefile 模板
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
$(target)_GCOV_FLAGS  := 
else
$(target)_GCOV_FLAGS  :=
endif

$(target)_CPUFLAGS    := $(CPUFLAGS)
$(target)_COMMONFLAGS := $($(target)_CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# Depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB += -lstdc++
endif

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
endif

$(target)_DEPEND_LIB += -lm -lgcc

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_IMG       := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_STRIP_IMG := $(OUTPATH)/strip/$(LOCAL_TARGET_NAME)
$(target)_BIN       := $(OUTPATH)/$(addsuffix .bin, $(basename $(LOCAL_TARGET_NAME)))
$(target)_SIZ       := $(OUTPATH)/$(addsuffix .siz, $(basename $(LOCAL_TARGET_NAME)))
$(target)_LZO       := $(OUTPATH)/$(addsuffix .lzo, $(basename $(LOCAL_TARGET_NAME)))

#*********************************************************************************************************
# Link script files
#*********************************************************************************************************
LOCAL_LD_SCRIPT_NT := $(LOCAL_LD_SCRIPT) config.ld

#*********************************************************************************************************
# Link object files
#*********************************************************************************************************
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(CPP) -E -P $(__DSYMBOL) config.ld -o config.lds
		$(LD) $(__CPUFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Create bin
#*********************************************************************************************************
$($(target)_BIN): $($(target)_IMG)
		@rm -f $@
		$(OC) -O binary $< $@

#*********************************************************************************************************
# Create siz
#*********************************************************************************************************
$($(target)_SIZ): $($(target)_IMG)
		@rm -f $@
		$(SZ) --format=berkeley $< > $@

#*********************************************************************************************************
# Create lzo
#*********************************************************************************************************
$($(target)_LZO): $($(target)_BIN)
		@rm -f $@
		$(LZOCOM) -c $< $@

#*********************************************************************************************************
# Strip image
#*********************************************************************************************************
$($(target)_STRIP_IMG): $($(target)_IMG)
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
ifeq ($(COMMERCIAL), 1)
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) $($(target)_LZO)
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_STRIP_IMG) 
endif

#*********************************************************************************************************
# End
#*********************************************************************************************************
