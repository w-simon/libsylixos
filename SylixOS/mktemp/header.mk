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
# 文   件   名: header.mk
#
# 创   建   人: Jiao.JinXing(焦进星)
#
# 文件创建日期: 2016 年 08 月 24 日
#
# 描        述: makefile 模板首部
#*********************************************************************************************************

#*********************************************************************************************************
# Check configure
#*********************************************************************************************************
check_defined = \
    $(foreach 1,$1,$(__check_defined))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $(value 2), ($(strip $2)))))

$(call check_defined, CONFIG_MK_EXIST, Please configure this project in RealEvo-IDE or create a config.mk file!)
$(call check_defined, SYLIXOS_BASE_PATH, SylixOS base project path)
$(call check_defined, TOOLCHAIN_PREFIX, the prefix name of toolchain)
$(call check_defined, DEBUG_LEVEL, debug level(debug or release))

#*********************************************************************************************************
# All *.mk files
#*********************************************************************************************************
APPLICATION_MK    = $(MKTEMP)/application.mk
LIBRARY_MK        = $(MKTEMP)/library.mk
STATIC_LIBRARY_MK = $(MKTEMP)/static-library.mk
KERNEL_MODULE_MK  = $(MKTEMP)/kernel-module.mk
KERNEL_LIBRARY_MK = $(MKTEMP)/kernel-library.mk
UNIT_TEST_MK      = $(MKTEMP)/unit-test.mk
LIBSYLIXOS_MK     = $(MKTEMP)/libsylixos.mk
DUMMY_MK          = $(MKTEMP)/dummy.mk
BSP_MK            = $(MKTEMP)/bsp.mk
END_MK            = $(MKTEMP)/end.mk
CLEAR_VARS_MK     = $(MKTEMP)/clear-vars.mk

#*********************************************************************************************************
# Toolchain select
#*********************************************************************************************************
CC      = $(TOOLCHAIN_PREFIX)gcc
CXX     = $(TOOLCHAIN_PREFIX)g++
AS      = $(TOOLCHAIN_PREFIX)gcc
AR      = $(TOOLCHAIN_PREFIX)ar
LD      = $(TOOLCHAIN_PREFIX)g++
OC      = $(TOOLCHAIN_PREFIX)objcopy
SZ      = $(TOOLCHAIN_PREFIX)size
CPP     = $(TOOLCHAIN_PREFIX)cpp
LZOCOM  = $(TOOLCHAIN_PREFIX)lzocom
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
STRIP   = $(TOOLCHAIN_PREFIX)strip

#*********************************************************************************************************
# Commercial toolchain check
#*********************************************************************************************************
COMMERCIAL = $(shell $(LZOCOM) 1>null 2>null && \
			(rm -rf null; echo 1) || \
			(rm -rf null; echo 0))

#*********************************************************************************************************
# Toolchain version
#*********************************************************************************************************
ifeq ($(COMMERCIAL), 1)
GCC_VERSION_STR   := $(shell $(CC) -dumpversion)
GCC_VERSION_MAJOR := $(shell echo $(GCC_VERSION_STR) | cut -f1 -d.)
GCC_VERSION_MINOR := $(shell echo $(GCC_VERSION_STR) | cut -f2 -d.)
GCC_VERSION_PATCH := $(shell echo $(GCC_VERSION_STR) | cut -f3 -d.)
GCC_VERSION       := $(GCC_VERSION_MAJOR)$(GCC_VERSION_MINOR)$(GCC_VERSION_PATCH)
endif

#*********************************************************************************************************
# Build paths
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OUTDIR = Debug
else
OUTDIR = Release
endif

OUTPATH = ./$(OUTDIR)
OBJPATH = $(OUTPATH)/obj
DEPPATH = $(OUTPATH)/dep

#*********************************************************************************************************
# Compiler optimize flag
# Do NOT use -O3 and -Os, -Os is not align for function loop and jump.
#                         -O3 default use inline function.
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OPTIMIZE = -O0 -g3 -gdwarf-2
else
OPTIMIZE = -O2 -g1 -gdwarf-2
endif

#*********************************************************************************************************
# Define some useful variables
#*********************************************************************************************************
GCC_CXX_EXCEPT_CFLAGS    = -fexceptions -frtti
GCC_NO_CXX_EXCEPT_CFLAGS = -fno-exceptions -fno-rtti
GCC_GCOV_CFLAGS          = -fprofile-arcs -ftest-coverage

BIAS  = /
EMPTY =
SPACE = $(EMPTY) $(EMPTY)

__TARGET    = $(word 3,$(subst $(BIAS),$(SPACE),$(@)))
__DEP       = $(addprefix $(DEPPATH)/$(__TARGET)/, $(addsuffix .d, $(basename $(<))))
__LIBRARIES = $($(@F)_DEPEND_LIB_PATH) $($(@F)_DEPEND_LIB)
__OBJS      = $($(@F)_OBJS)
__CPUFLAGS  = $($(@F)_CPUFLAGS)
__DSYMBOL   = $($(@F)_DSYMBOL)

__PRE_LINK_CMD   = $($(@F)_PRE_LINK_CMD)
__POST_LINK_CMD  = $($(@F)_POST_LINK_CMD)

__PRE_STRIP_CMD  = $($(@F)_PRE_STRIP_CMD)
__POST_STRIP_CMD = $($(@F)_POST_STRIP_CMD)

#*********************************************************************************************************
# Include arch.mk
#*********************************************************************************************************
include $(MKTEMP)/arch.mk

#*********************************************************************************************************
# End
#*********************************************************************************************************
