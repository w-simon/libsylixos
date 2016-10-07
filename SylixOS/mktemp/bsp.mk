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
# ��   ��   ��: bsp.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: bsp ��Ŀ�� makefile ģ��
#*********************************************************************************************************

#*********************************************************************************************************
# copy symbol.c symbol.h
#*********************************************************************************************************
SYMBOL_PATH = SylixOS/bsp

$(SYMBOL_PATH)/symbol.c: $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/$(OUTDIR)/symbol.c $(SYMBOL_PATH)/symbol.h
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.c" $(SYMBOL_PATH)/symbol.c

$(SYMBOL_PATH)/symbol.h: $(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/$(OUTDIR)/symbol.h
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.h" $(SYMBOL_PATH)/symbol.h

#*********************************************************************************************************
# add symbol.c to LOCAL_SRCS
#*********************************************************************************************************
LOCAL_SRCS := $(SYMBOL_PATH)/symbol.c $(LOCAL_SRCS)

#*********************************************************************************************************
# include common.mk
#*********************************************************************************************************
include $(MKTEMP)/common.mk

#*********************************************************************************************************
# depend and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
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

$(target)_COMMONFLAGS := $(CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) 

#*********************************************************************************************************
# depend library search paths
#*********************************************************************************************************
$(target)_DEPEND_LIB_PATH := -L"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"
$(target)_DEPEND_LIB_PATH += $(LOCAL_DEPEND_LIB_PATH)

#*********************************************************************************************************
# depend libraries
#*********************************************************************************************************
$(target)_DEPEND_LIB := $(LOCAL_DEPEND_LIB)
$(target)_DEPEND_LIB += -lsylixos

ifneq (,$(findstring yes,$($(target)_USE_CXX)))
$(target)_DEPEND_LIB += -lstdc++
endif

ifneq (,$(findstring yes,$($(target)_USE_GCOV)))
endif

$(target)_DEPEND_LIB += -lm -lgcc

#*********************************************************************************************************
# targets
#*********************************************************************************************************
$(target)_IMG  := $(OUTPATH)/$(LOCAL_TARGET_NAME)
$(target)_BIN  := $(OUTPATH)/$(addsuffix .bin, $(basename $(LOCAL_TARGET_NAME)))
$(target)_SIZ  := $(OUTPATH)/$(addsuffix .siz, $(basename $(LOCAL_TARGET_NAME)))
$(target)_LZO  := $(OUTPATH)/$(addsuffix .lzo, $(basename $(LOCAL_TARGET_NAME)))
$(target)_OS   := $(OUTPATH)/$(addsuffix .os,  $(basename $(LOCAL_TARGET_NAME)))

#*********************************************************************************************************
# link script files
#*********************************************************************************************************
LOCAL_LD_SCRIPT_NT := $(LOCAL_LD_SCRIPT) config.ld

#*********************************************************************************************************
# link object files
#*********************************************************************************************************
$($(target)_IMG): $(LOCAL_LD_SCRIPT_NT) $($(target)_OBJS) $($(target)_DEPEND_TARGET)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(CPP) -E -P config.ld -o config.lds
		$(LD) $(CPUFLAGS) -nostdlib $(addprefix -T, $<) -o $@ $(__OBJS) $(__LIBRARIES)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# create bin
#*********************************************************************************************************
$($(target)_BIN): $($(target)_IMG)
		@rm -f $@
		$(OC) -O binary $< $@

#*********************************************************************************************************
# create siz
#*********************************************************************************************************
$($(target)_SIZ): $($(target)_IMG)
		@rm -f $@
		$(SZ) --format=berkeley $< > $@

#*********************************************************************************************************
# create lzo
#*********************************************************************************************************
$($(target)_LZO): $($(target)_BIN)
		@rm -f $@
		$(LZOCOM) -c $< $@

#*********************************************************************************************************
# create os
#*********************************************************************************************************
$($(target)_OS): $($(target)_IMG)
		@rm -f $@
		$(__PRE_STRIP_CMD)
		$(STRIP) $< -o $@
		$(__POST_STRIP_CMD)

#*********************************************************************************************************
# add targets
#*********************************************************************************************************
ifeq ($(COMMERCIAL), 1)
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_OS) $($(target)_LZO) $(SYMBOL_PATH)/symbol.c $(SYMBOL_PATH)/symbol.h
else
TARGETS := $(TARGETS) $($(target)_IMG) $($(target)_BIN) $($(target)_SIZ) $($(target)_OS) $(SYMBOL_PATH)/symbol.c $(SYMBOL_PATH)/symbol.h
endif

#*********************************************************************************************************
# end
#*********************************************************************************************************
