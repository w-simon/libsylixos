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
# 文   件   名: libsylixos.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 10 月 02 日
#
# 描        述: libsylixos makefile 模板
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

$(target)_COMMONFLAGS := $(CPUFLAGS) $(ARCH_COMMONFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums $($(target)_GCOV_FLAGS) 
$(target)_ASFLAGS     := $($(target)_COMMONFLAGS) -x assembler-with-cpp $($(target)_DSYMBOL) $($(target)_INC_PATH) 
$(target)_CFLAGS      := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CFLAGS)
$(target)_CXXFLAGS    := $($(target)_COMMONFLAGS) $($(target)_DSYMBOL) $($(target)_INC_PATH) $($(target)_CXX_EXCEPT) $($(target)_CXXFLAGS)

#*********************************************************************************************************
# Targets
#*********************************************************************************************************
$(target)_A := $(OUTPATH)/$(LOCAL_TARGET_NAME)

#*********************************************************************************************************
# Objects
#*********************************************************************************************************
OBJS_ARCH    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LOCAL_ARCH_SRCS))))
OBJS_APPL    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(APPL_SRCS))))
OBJS_DEBUG   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(DEBUG_SRCS))))
OBJS_DRV     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(DRV_SRCS))))
OBJS_FS      := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(FS_SRCS))))
OBJS_GUI     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(GUI_SRCS))))
OBJS_KERN    := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(KERN_SRCS))))
OBJS_LIB     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LIB_SRCS))))
OBJS_LOADER  := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(LOADER_SRCS))))
OBJS_MONITOR := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(MONITOR_SRCS))))
OBJS_MPI     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(MPI_SRCS))))
OBJS_NET     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(NET_SRCS))))
OBJS_POSIX   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(POSIX_SRCS))))
OBJS_SHELL   := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SHELL_SRCS))))
OBJS_SYMBOL  := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYMBOL_SRCS))))
OBJS_SYS     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYS_SRCS))))
OBJS_SYSPERF := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(SYSPERF_SRCS))))
OBJS_CPP     := $(addprefix $(OBJPATH)/libsylixos.a/, $(addsuffix .o, $(basename $(CPP_SRCS))))

#*********************************************************************************************************
# Make archive object files
#*********************************************************************************************************
$($(target)_A): $($(target)_OBJS)
		@rm -f $@
		$(__PRE_LINK_CMD)
		$(AR) -r $@ $(OBJS_APPL)
		$(AR) -r $@ $(OBJS_ARCH)
		$(AR) -r $@ $(OBJS_DEBUG)
		$(AR) -r $@ $(OBJS_DRV)
		$(AR) -r $@ $(OBJS_FS)
		$(AR) -r $@ $(OBJS_GUI)
		$(AR) -r $@ $(OBJS_KERN)
		$(AR) -r $@ $(OBJS_LIB)
		$(AR) -r $@ $(OBJS_MONITOR)
		$(AR) -r $@ $(OBJS_LOADER)
		$(AR) -r $@ $(OBJS_MPI)
		$(AR) -r $@ $(OBJS_NET)
		$(AR) -r $@ $(OBJS_POSIX)
		$(AR) -r $@ $(OBJS_SHELL)
		$(AR) -r $@ $(OBJS_SYMBOL)
		$(AR) -r $@ $(OBJS_SYS)
		$(AR) -r $@ $(OBJS_SYSPERF)
		$(AR) -r $@ $(OBJS_CPP)
		$(__POST_LINK_CMD)

#*********************************************************************************************************
# Create symbol files
#*********************************************************************************************************
$(OUTPATH)/symbol.c: $($(target)_A)
		@rm -f $@
		cp SylixOS/hosttools/makesymbol/Makefile $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/makesymbol.bat $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/makesymbol.sh $(OUTDIR)
		cp SylixOS/hosttools/makesymbol/nm.exe $(OUTDIR)
		make -C $(OUTDIR)

#*********************************************************************************************************
# Add targets
#*********************************************************************************************************
TARGETS := $(TARGETS) $($(target)_A) $(OUTPATH)/symbol.c

#*********************************************************************************************************
# End
#*********************************************************************************************************
