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
# ��   ��   ��: common.mk
#
# ��   ��   ��: Jiao.JinXing(������)
#
# �ļ���������: 2016 �� 08 �� 24 ��
#
# ��        ��: makefile ģ�幫������
#*********************************************************************************************************

#*********************************************************************************************************
# Target name
#*********************************************************************************************************
target := $(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# Objects
#*********************************************************************************************************
ifneq (,$(findstring arm,$(ARCH)))
LOCAL_ARCH_SRCS := $(LOCAL_ARM_SRCS)
endif

ifneq (,$(findstring mips,$(ARCH)))
LOCAL_ARCH_SRCS := $(LOCAL_MIPS_SRCS)
endif

ifneq (,$(findstring ppc,$(ARCH)))
LOCAL_ARCH_SRCS := $(LOCAL_PPC_SRCS)
endif

ifneq (,$(findstring x86,$(ARCH)))
LOCAL_ARCH_SRCS := $(LOCAL_X86_SRCS)
endif

LOCAL_SRCS := $(LOCAL_SRCS) $(LOCAL_ARCH_SRCS)
LOCAL_SRCS := $(filter-out $(LOCAL_EXCLUDE_SRCS),$(LOCAL_SRCS))

$(target)_OBJS := $(addprefix $(OBJPATH)/$(target)/, $(addsuffix .o, $(basename $(LOCAL_SRCS))))
$(target)_DEPS := $(addprefix $(DEPPATH)/$(target)/, $(addsuffix .d, $(basename $(LOCAL_SRCS))))

#*********************************************************************************************************
# Include depend files
#*********************************************************************************************************
ifneq ($(MAKECMDGOALS), clean)
sinclude $($(target)_DEPS)
endif

#*********************************************************************************************************
# Include paths
#*********************************************************************************************************
$(target)_INC_PATH := -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS"
$(target)_INC_PATH += -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include"
$(target)_INC_PATH += -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include/network"
$(target)_INC_PATH += $(LOCAL_INC_PATH)

#*********************************************************************************************************
# Compiler preprocess
#*********************************************************************************************************
$(target)_DSYMBOL := -DSYLIXOS
$(target)_DSYMBOL += $(LOCAL_DSYMBOL)

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
$(target)_CFLAGS   := $(LOCAL_CFLAGS)
$(target)_CXXFLAGS := $(LOCAL_CXXFLAGS)

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
$(target)_USE_CXX        := $(LOCAL_USE_CXX)
$(target)_USE_CXX_EXCEPT := $(LOCAL_USE_CXX_EXCEPT)
$(target)_USE_GCOV       := $(LOCAL_USE_GCOV)

$(target)_PRE_LINK_CMD   := $(LOCAL_PRE_LINK_CMD)
$(target)_POST_LINK_CMD  := $(LOCAL_POST_LINK_CMD)

$(target)_PRE_STRIP_CMD  := $(LOCAL_PRE_STRIP_CMD)
$(target)_POST_STRIP_CMD := $(LOCAL_POST_STRIP_CMD)

$(target)_DEPEND_TARGET  := $(LOCAL_DEPEND_TARGET)

#*********************************************************************************************************
# Compile source files
#*********************************************************************************************************
$(OBJPATH)/$(target)/%.o: %.S
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(AS) $($(__TARGET)_ASFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.c
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CC) $($(__TARGET)_CFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cpp
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cxx
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

$(OBJPATH)/$(target)/%.o: %.cc
		@if [ ! -d "$(dir $@)" ]; then \
			mkdir -p "$(dir $@)"; fi
		@if [ ! -d "$(dir $(__DEP))" ]; then \
			mkdir -p "$(dir $(__DEP))"; fi
		$(CXX) $($(__TARGET)_CXXFLAGS) -MMD -MP -MF $(__DEP) -c $< -o $@

#*********************************************************************************************************
# End
#*********************************************************************************************************
