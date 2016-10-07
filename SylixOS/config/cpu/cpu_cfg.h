/*
 * include cpu config file.
 */
#ifndef __CPU_CFG_H
#define __CPU_CFG_H

#ifdef __GNUC__
#if defined(__arm__)
#include "cpu_cfg_arm.h"

#elif defined(__mips__)
#include "cpu_cfg_mips.h"

#elif defined(__PPC__)
#include "cpu_cfg_ppc.h"

#elif defined(__i386__)
#include "cpu_cfg_x86.h"
#endif

#else
#include "cpu_cfg_arm.h"
#endif

#endif /* __CPU_CFG_H */
/*
 * end
 */
